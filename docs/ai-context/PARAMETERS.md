# PARAMETERS: How Parameters Are Saved, Recalled, and Initialized in bitKlavier4

## Overview

bitKlavier4 uses the `chowdsp_utils` library for parameter management. Parameters are declared in "ParamHolder" structs, serialized to/from XML, and observed via listeners. Understanding the full lifecycle is essential for correctly applying parameter values at all stages.

---

## Parameter Declaration

Parameters are declared as fields in structs that inherit from `chowdsp::ParamHolder`. Each preparation type has its own param holder (e.g., `SpringTuningParams`, `TuningParams`).

**Example** (`SpringTuningParams.h`):
```cpp
struct SpringTuningParams : chowdsp::ParamHolder
{
    SpringTuningParams() : chowdsp::ParamHolder("SpringTuningParams")
    {
        add(scaleId, intervalFundamental, scaleId_tether, tetherFundamental, ...);
    }

    chowdsp::EnumChoiceParameter<ScaleId>::Ptr scaleId;
    chowdsp::EnumChoiceParameter<PitchClass>::Ptr intervalFundamental;
    chowdsp::EnumChoiceParameter<ScaleId>::Ptr scaleId_tether;
    chowdsp::EnumChoiceParameter<PitchClass>::Ptr tetherFundamental;
    ...
};
```

All parameters must be registered via `add(...)` in the constructor. The `chowdsp::ParamHolder` base class handles recursive traversal for serialization and listener registration.

---

## Parameter Types

- **`chowdsp::FloatParameter`** — continuous float value with range and default
- **`chowdsp::BoolParameter`** — boolean toggle
- **`chowdsp::EnumChoiceParameter<EnumType>`** — enum-backed choice; `.get()` returns the enum value directly (via `magic_enum`)
- **`chowdsp::ChoiceParameter`** — integer-indexed choice

All parameter types expose a `.get()` method for reading the current value.

---

## Serialization (Saving)

Serialization is handled by `chowdsp::ParamHolder::serialize(...)`, which recursively visits all registered parameters and writes their current values to an XML element.

In bitKlavier4, each processor's `TuningParams::serialize` (in `TuningProcessor.cpp`) calls the chowdsp serializer and may also write additional non-parameter state (e.g., Scala KBM strings) as XML attributes.

**Key point:** Serialization captures the raw parameter value (a normalized float internally), not the enum or display value.

---

## Deserialization (Restoring / Gallery Load)

Deserialization is handled by `chowdsp::ParamHolder::deserialize(...)`, which calls `chowdsp::ParameterTypeHelpers::deserializeParameter(...)` for each parameter.

### Critical behavior: `setValue` vs. `setValueNotifyingHost`

Inside `deserializeParameter`, the value is restored using the **assignment operator** (equivalent to `setValue`), **not** `setValueNotifyingHost`. This means:

> **Parameter listeners do NOT fire during deserialization.**

This is a fundamental constraint of the chowdsp framework. Any code that relies on listeners to react to parameter changes will be silently bypassed during gallery load.

**Consequence:** If a processor or subsystem needs to update its internal state when a parameter is restored, it must do so **explicitly** after deserialization completes — it cannot rely on listeners alone.

---

## Parameter Listeners

Listeners can be registered via `state.getParameterListeners().addParameterListener(...)`:

```cpp
tuningCallbacks +=
{
    state.getParameterListeners().addParameterListener(
        state.params.tuningState.springTuningParams.tetherFundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]()
        {
            state.params.tuningState.springTuner->tetherFundamentalChanged();
        }
    )
};
```

Listeners fire when a parameter changes **at runtime** (e.g., via UI interaction, automation, or `setValueNotifyingHost`). They are the correct mechanism for keeping subsystems in sync during normal operation.

**Listeners do NOT fire during deserialization** (see above).

---

## The Two-Phase Problem and Solution Pattern

Because listeners don't fire during deserialization, any subsystem that maintains derived state from parameters needs **two separate mechanisms**:

### 1. Listeners (for runtime changes)
Register listeners in the processor constructor (e.g., `TuningProcessor::TuningProcessor`). These handle UI changes, automation, and modulation.

### 2. Explicit post-deserialize calls (for gallery load)
After `chowdsp::ParamHolder::deserialize(...)` completes, explicitly call the update methods that would normally be triggered by listeners.

**Example** (from `TuningProcessor.cpp`, `TuningParams::deserialize`):
```cpp
// chowdsp deserializer restores param values but does NOT notify listeners
chowdsp::ParamHolder::deserialize(*deserial, paramHolder.tuningState.springTuningParams);

// ...other custom deserialization (e.g., KBM string)...

// Explicitly re-apply spring tuning params after deserialization,
// since setValue() does not notify listeners
paramHolder.tuningState.springTuner->intervalScaleChanged();
paramHolder.tuningState.springTuner->intervalFundamentalChanged();
paramHolder.tuningState.springTuner->tetherScaleChanged();
paramHolder.tuningState.springTuner->tetherFundamentalChanged();
```

This pattern ensures correctness at both gallery load time and during runtime interaction.

---

## Where Listeners Should Live

Listeners for parameters that affect audio processing (not just UI display) should be registered in the **Processor** (e.g., `TuningProcessor`), not in the **View** (e.g., `TuningParametersView`).

- **Processor listeners**: Always present, even when the UI is closed. Correct for audio-affecting state.
- **View listeners**: Only present when the UI is open. Appropriate for display-only updates.

If a listener is only in the View, parameter changes made while the UI is closed (including gallery load) will not be applied to the audio engine.

---

## SpringTuning: A Concrete Example

`SpringTuning` is a subsystem inside `TuningProcessor` that maintains a physical spring model for microtonal tuning. It has internal state derived from four parameters:

| Parameter | SpringTuning method |
|---|---|
| `scaleId` | `intervalScaleChanged()` |
| `intervalFundamental` | `intervalFundamentalChanged()` |
| `scaleId_tether` | `tetherScaleChanged()` |
| `tetherFundamental` | `tetherFundamentalChanged()` |

These methods read the current parameter values and update the spring model accordingly. They are idempotent — calling them multiple times with the same values is safe.

**Bug that was fixed:** When a gallery was loaded with non-default values for these parameters, the `SpringTuning` object was never updated because:
1. Listeners didn't fire during deserialization (chowdsp constraint).
2. The listeners that did exist were only in `TuningParametersView` (UI), which may not be open during gallery load.

**Fix:** Added listeners in `TuningProcessor` constructor (for runtime) AND explicit calls at the end of `TuningParams::deserialize` (for gallery load).

---

## Summary of Key Rules

1. **Always register audio-affecting listeners in the Processor, not just the View.**
2. **Never rely solely on listeners for state that must be correct after gallery load.**
3. **After any custom `deserialize` call, explicitly re-apply derived state** that would normally be set by listeners.
4. **`chowdsp::ParameterTypeHelpers::setValue` (used internally during deserialization) does not notify listeners.** Only `setValueNotifyingHost` does.
5. **`EnumChoiceParameter::get()` returns the enum value directly** — no manual index-to-enum conversion needed.
