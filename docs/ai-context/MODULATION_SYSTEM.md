# Modulation System Architecture

**Status:** ✅ Active System  
**Last Updated:** 2026-05-22  
**Version:** 4.9.10

**Note:** This document is a work-in-progress. I (dt) have not fully reviewed it and can't guarantee it is accurate.

## Overview

bitKlavier's modulation system allows dynamic parameter control through two distinct mechanisms:

1. **Audio-Rate (Continuous) Modulation** — Real-time parameter changes via LFO and Ramp modulators, routed through the `AudioProcessorGraph` modulation bus
2. **State Modulation** — Discrete parameter changes (keymaps, combo box selections, etc.) applied atomically via ValueTree updates

Both systems coexist and operate independently but share common UI components and routing infrastructure.

## Architecture Diagram

```
                ModulationManager
(UI overlay, connection visualization, meter rendering)
                        |
    +-------------------+-------------------+
    |                                       |
    v                                       v
Audio-Rate Mod                          State Mod
(Continuous)                            (Discrete)
    |                                       |
    |-- LFOModulator                        |-- StateModulator
    |-- RampModulator                       +-- StateModulatedComponent
    |-- ModulationBus (AudioGraph)               |
    +-- SynthSlider                              |-- KeyboardState
         +-- liveModulatedValue_                 |-- ComboBox selections
                                                 +-- Complex UI state
```



## Part 1: Audio-Rate (Continuous) Modulation

### Overview

Audio-rate modulation enables real-time, sample-accurate parameter changes. Modulation sources (LFO, Ramp) generate control signals that are routed through the `AudioProcessorGraph`'s modulation bus to destination parameters.

### Key Components

#### **Modulation Sources**

**Location:** `source/modulation/`

- **`LFOModulator`** — Low-frequency oscillator with multiple waveforms
- **`RampModulator`** — Linear ramp generator for parameter sweeps
- **`ModulatorBase`** — Base class for all modulation sources

Each modulator:
- Generates audio-rate control signals
- Outputs to a dedicated channel in the modulation bus
- Can modulate multiple destinations simultaneously

#### **Modulation Bus Architecture**

**Implementation:** `AudioProcessorGraph` with dedicated modulation input buses

Each preparation defines its modulation input channels in its `*BusLayout()` method:

```cpp
// Example from DirectProcessor.h
juce::AudioProcessor::BusesProperties directBusLayout()
{
    return BusesProperties()
        .withOutput("Output",       juce::AudioChannelSet::stereo(), true)
        .withInput ("Input",        juce::AudioChannelSet::stereo(), true)
        .withInput ("Send Pad",     juce::AudioChannelSet::stereo(), true)
        
        // CRITICAL: Channel count must match number of modulatable params!
        // 22 params × 2 channels (Ramp + LFO) = 44 channels
        .withInput ("Modulation",   juce::AudioChannelSet::discreteChannels (22 * 2), true)
        
        .withOutput("Modulation",   juce::AudioChannelSet::mono(), false)
        .withOutput("Send",         juce::AudioChannelSet::stereo(), true);
}
```

**Channel Allocation:**
- Each modulatable parameter gets **2 channels**: one for Ramp, one for LFO
- Modulators write to their assigned channel
- Preparations read from the modulation bus and apply scaling

#### **ModulationConnection: The Routing Glue**

**Location:** `source/modulation/ModulationConnection.h`

Each audio-rate connection tracks:

```cpp
struct ModulationConnection : ModulatorBase::Listener {
    std::string source_name;        // e.g., "lfo_1"
    std::string destination_name;   // parameter UUID

    std::atomic<float> modAmt_;         // User-set modulation amount
    std::atomic<float> scalingValue_;   // Normalized [0,1] scaling factor
    
    bool bipolar_;                  // Symmetric ±modulation
    bool offset_;                   // Offset from base vs. absolute target
    
    juce::NormalisableRange<float> range;  // Destination parameter range
    
    // Audio thread scaling calculation
    void updateScalingAudioThread(float knobValueParamUnits) noexcept;
    float getScalingForDSP() const noexcept;
    
    // Trigger lock mechanism (prevents scaling changes mid-ramp)
    void lockScaling() noexcept;
    void unlockScaling() noexcept;
};
```

**Scaling Calculation:**
The connection converts the raw modulation amount into a normalized [0,1] scaling factor that respects the destination parameter's range and skew:
- **Bipolar mode:** Modulation applies symmetrically (e.g., LFO sweeps ±20 Hz around center frequency)
- **Unipolar mode:** Modulation applies in one direction (e.g., Ramp sweeps from current value to target)
- **Range clamping:** All values clamped to parameter min/max to prevent out-of-bounds behavior

**Trigger Locking:**
When a Ramp modulator is triggered:
1. `lockScaling()` freezes the scaling factor based on the current knob position
2. Audio thread uses `getScalingForDSP()` which returns the locked value
3. `unlockScaling()` called on reset or modulation end, allowing UI edits to take effect again

This prevents glitches if the user adjusts the modulation amount mid-ramp.

#### **SynthSlider: The Destination**

**Location:** `source/interface/components/synth_slider.h`

Each `SynthSlider` that supports audio-rate modulation includes:

```cpp
class SynthSlider : public OpenGlSlider {
public:
    // Called by audio thread (via VSTParamAttachment) to push live modulated value
    void setLiveModulatedValue(float v) { liveModulatedValue_ = v; }
    
    // Returns current effective value (base + modulation)
    float getLiveModulation() const {
        if (!std::isnan(liveModulatedValue_))
            return liveModulatedValue_;  // Use pushed value if available
        return (float)getValue();        // Fallback to UI slider position
    }
    
private:
    // NaN = no modulation active; otherwise holds last modulated value
    float liveModulatedValue_ = std::numeric_limits<float>::quiet_NaN();
};
```

**How Modulation Flows to UI:**
1. **Audio thread:** Modulation signal arrives via modulation bus
2. **Parameter update:** `chowdsp::FloatParameter` applies modulation via its attachment
3. **UI sync:** `VSTParamAttachment` calls `setLiveModulatedValue(newValue)`
4. **Visual feedback:** Modulation meter arc renders using `getLiveModulation()` to show offset from base value

**Visual Rendering:**
The modulation arc (blue overlay on knobs) shows the current modulation offset:
- Base value = slider position
- Live value = base + modulation
- Arc spans from base → live to visualize modulation depth

#### **ModulationProcessor: The Hub**

**Location:** `source/synthesis/framework/ModulationProcessor.h`

The `ModulationProcessor` sits in the `AudioProcessorGraph` and:
- Hosts LFO and Ramp modulator instances
- Routes modulator outputs to the correct modulation bus channels
- Maintains `ModulationConnectionBank` (all audio-rate connections)
- Handles trigger/reset events from MIDI notes or other sources

**Key Methods:**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
    // 1. Generate modulation signals from all active modulators
    for (auto* modulator : modulators)
        modulator->getNextAudioBlock(buffer, midi);

    // 2. Route signals to correct channels in modulation bus
    // 3. Connections read from bus and apply scaling in their respective prep processBlock()
}
```

#### **Real-Time Safety**

**Critical Constraints:**
1. **No allocations:** All modulation path code is allocation-free
2. **Lock-free atomics:** Connection scaling uses `std::atomic<float>` with relaxed ordering
3. **Pre-allocated buffers:** Modulation bus channels allocated during `prepareToPlay()`
4. **No ValueTree access:** Audio thread never touches ValueTree directly

**ThreadSanitizer Testing:**
Always run ThreadSanitizer on modulation changes:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_THREAD_SANITIZER=ON ..
```

## Part 2: State Modulation (Discrete Changes)

### Overview

State modulation enables atomic, instantaneous changes to non-continuous parameters like keymap selections, combo box choices, or complex UI state. Unlike audio-rate modulation, state changes happen discretely (on trigger events) and update the ValueTree directly.

### Key Components

#### **StateModulator**

**Location:** `source/modulation/StateModulator.h`

A minimal modulator that triggers ValueTree state changes:

```cpp
class StateModulatorProcessor : public ModulatorBase {
public:
    static constexpr ModulatorType type = ModulatorType::STATE;

    void getNextAudioBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override {
        // No audio processing — triggers handled via listener callbacks
    }
    
    // Inherited from ModulatorBase:
    void triggerModulation() override {
        for (auto* listener : listeners_)
            listener->modulationTriggered();  // Queues state change
    }
    
    void triggerReset() override {
        for (auto* listener : listeners_)
            listener->resetTriggered();  // Restores default state
    }
};
```

#### **StateConnection: The Discrete Link**

**Location:** `source/modulation/ModulationConnection.h`

Each state connection:

```cpp
struct StateConnection : public ModulatorBase::Listener {
    std::string source_name;        // State modulator name
    std::string destination_name;   // Target component UUID

    ParameterChangeBuffer* changeBuffer;  // Lock-free queue for state changes
    juce::ValueTree change;               // Desired state (e.g., new keymap bits)
    
    void modulationTriggered() override {
        if (!changeBuffer->tryPush(0, change)) {
            DBG("State change dropped: buffer contended");
        }
    }
    
    void resetTriggered() override {
        changeBuffer->tryPush(0, changeBuffer->defaultState);
    }
};
```

**ParameterChangeBuffer:**
A lock-free FIFO queue that safely passes ValueTree snapshots from audio thread → message thread:
- **Audio thread:** Calls `tryPush()` to enqueue state change
- **Message thread:** Polls queue and applies changes to ValueTree (which updates UI)
- **Safety:** Uses JUCE's `AbstractFifo` for wait-free enqueue/dequeue

#### **StateModulatedComponent: The Base Class**

**Location:** `source/interface/components/StateModulatedComponent.h`

All UI components that support state modulation inherit from this:

```cpp
struct StateModulatedComponent : juce::Component {
    bool isModulation_;   // true if this is a modulation editor copy
    bool isModulated_;    // true if this is the main UI component
    
    juce::ValueTree modulationState;  // Holds modulated values
    juce::ValueTree defaultState;     // Holds base/default values
    
    // Subclasses implement to sync UI ↔ ValueTree
    virtual void syncToValueTree() = 0;
    
    // Subclasses implement to create modulation editor instance
    virtual StateModulatedComponent* clone() = 0;
};
```

**Concrete Implementations:**
1. **`OpenGLKeymapKeyboardComponent`** (wraps `BKKeymapKeyboardComponent`)
    - Modulates keymap selection (which MIDI notes are active)
    - bitset stored as `keyStates` in ValueTree `IDs::keymapBits`
    - Clone creates modulation editor with black border + "MODIFIED" label

2. **`OpenGLComboBox`** (wraps `juce::ComboBox`)
    - Modulates combo box selection (e.g., switching between transposition modes)
    - Selected item ID stored in ValueTree
    - Clone creates modulation editor for authoring alternate selections

#### **How State Modulation Works (Step-by-Step)**

**1. Setup (Message Thread):**

```cpp
// User creates state modulation connection in ModulationManager UI
auto* stateConn = stateConnectionBank->createConnection("state_mod_1", "keymap_uuid");

// Assign target component's change buffer
stateConn->changeBuffer = keymapComponent->getChangeBuffer();

// Set desired modulated state (e.g., different keymap)
stateConn->setChange(modulatedValueTree);  // Holds alternate keymap bits
```

**2. Trigger (Audio Thread):**

```cpp
// MIDI note triggers state modulator
stateModulator->triggerModulation();
  → StateConnection::modulationTriggered()
    → changeBuffer->tryPush(0, change);  // Enqueue state change
```

**3. Application (Message Thread):**

```cpp
// Timer callback polls change buffer
void timerCallback() override {
    juce::ValueTree newState;
    if (changeBuffer->tryPop(newState)) {
        defaultState.copyPropertiesAndChildrenFrom(newState, nullptr);
        syncToValueTree();  // Updates UI component
    }
}
```

**4. Reset (Audio Thread):**

```cpp
// Reset prep or noteOff triggers reset
stateModulator->triggerReset();
  → StateConnection::resetTriggered()
    → changeBuffer->tryPush(0, changeBuffer->defaultState);
```

#### **Keymap Example: Full Flow**

**Scenario:** Modulate keymap to switch from C major → A minor on trigger

**Setup:**

```cpp
// Base keymap (C major: C D E F G A B)
std::bitset<128> cMajor;
for (int oct = 0; oct < 10; ++oct) {
    int base = oct * 12 + 24;  // Start at C1
    cMajor.set(base + 0);  // C
    cMajor.set(base + 2);  // D
    cMajor.set(base + 4);  // E
    // ... etc
}
defaultState.setProperty(IDs::keymapBits, getOnKeyString(cMajor), nullptr);

// Modulated keymap (A minor: A B C D E F G)
std::bitset<128> aMinor;
// ... populate A natural minor scale
modulationState.setProperty(IDs::keymapBits, getOnKeyString(aMinor), nullptr);
```

**Trigger:**
1. User plays MIDI note mapped to state modulator
2. `StateConnection::modulationTriggered()` enqueues `modulationState`
3. Message thread timer applies: keyboard switches to A minor
4. Subsequent notes use new keymap until reset


## Part 3: Common Infrastructure

### ModulationManager (UI Overlay)

**Location:** `source/modulation/modulation_manager.h`

The `ModulationManager` is the unified UI for creating, visualizing, and editing both audio-rate and state modulations.

**Responsibilities:**

**Connection Visualization**
- Draws curved lines connecting modulation sources → destinations
- Color-codes by modulator type (LFO = blue, Ramp = orange, State = purple)
- Highlights active connections during playback

**Modulation Amount Knobs**
- `ModulationAmountKnob` instances overlay destination sliders
- Shows current modulation depth as a percentage
- Right-click menu: Disconnect, Toggle Bypass, Toggle Bipolar

**Modulation Meters**
- Real-time arc rendering around modulated sliders
- Reads `SynthSlider::getLiveModulation()` to display current offset
- Updates at ~60 fps for smooth visual feedback

**Hover Interactions**
- Hovering over slider highlights all connected modulators
- Hovering over modulator highlights all destinations
- Tooltip shows connection details (source, destination, amount)

**Key Classes:**

```cpp
class ModulationAmountKnob : public SynthSlider {
public:
    void setDestinationSlider(SynthSlider* dest);
    float get0to1value();  // Returns normalized [0,1] modulation depth
    
    bool isBypass();
    bool isBipolar();
    bool isOffsetMod();
    
private:
    SynthSlider* destination;
    float my0to1amt;  // Cached normalized amount for meter rendering
};
```

**`ModulationIndicator`**

For state-modulated components, which don't have continuous knobs:

```cpp
class ModulationIndicator : public OpenGlQuad {
    // Renders a small badge/icon on state components showing mod connections
    // Clicking opens modulation editor (clone of component)
};
```

#### **Connection Rendering**

**Algorithm:**

1. **Gather endpoints:**

```cpp
juce::Point<float> sourcePos = modulatorButton->getScreenPosition();
juce::Point<float> destPos = destinationSlider->getScreenPosition();
```

2. **Draw curve:**

```cpp
juce::Path connectionPath;
connectionPath.startNewSubPath(sourcePos);

// Bezier curve with control points offset vertically
float controlOffset = std::abs(destPos.y - sourcePos.y) * 0.5f;
connectionPath.cubicTo(
    sourcePos.x, sourcePos.y + controlOffset,
    destPos.x, destPos.y - controlOffset,
    destPos.x, destPos.y
);

g.strokePath(connectionPath, juce::PathStrokeType(2.0f));
```

3. **Animate:**
    - Active connections pulse with alpha modulation
    - Hovered connections thicken and brighten

#### **Modulation Editor Workflow**

**For Audio-Rate Parameters:**
1. Click modulation button on slider
2. Drag to modulation amount knob
3. Adjust knob to set depth
4. Connection automatically created in `ModulationConnectionBank`

**For State Parameters:**
1. Click "+" icon on state component (e.g., keymap keyboard)
2. Modulation editor opens (clone of component with black border)
3. Edit desired state in clone (e.g., select different keys)
4. Changes stored in ValueTree `modulationState`
5. Drag from state modulator → component to create connection

## Part 4: Integration with Preparations

### How Preparations Consume Modulation

Each preparation's `processBlock()` reads modulation signals and applies them to parameters:

**Example: DirectProcessor**

```cpp
void DirectProcessor::processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midi) {
    // 1. Read modulation bus
    auto* modBus = getBusBuffer(buffer, false, getModulationBusIndex());
    
    // 2. For each parameter with connections:
    for (auto* conn : getConnectionsForParam("transpose")) {
        int channel = conn->modulation_output_bus_index;
        float modSignal = modBus->getSample(channel, 0);  // Read from assigned channel
        
        // 3. Apply scaling
        float scaled = modSignal * conn->getScalingForDSP();
        
        // 4. Convert to parameter units
        float baseValue = transposeParam->getCurrentValue();
        float modulatedValue = baseValue + (scaled * transposeRange);
        
        // 5. Apply to processing
        currentTranspose = modulatedValue;
    }
    
    // 6. Use currentTranspose in audio processing...
}
```

**Parameter Update Flow:**

```
Modulator generates signal (e.g., LFO at 1.0 Hz)
    ↓
Signal written to modulation bus channel N
    ↓
Preparation reads channel N in processBlock()
    ↓
Connection applies scaling (bipolar, range, skew)
    ↓
Scaled value applied to parameter
    ↓
VSTParamAttachment pushes value to UI (setLiveModulatedValue)
    ↓
ModulationManager renders meter arc showing offset
```

## Part 5: Practical Examples

### Example 1: LFO Modulating Filter Cutoff

**Setup:**
- Direct preparation has "Cutoff" parameter (20 Hz – 20 kHz, exponential)
- Create LFO modulator: 2 Hz sine wave
- Connect LFO → Cutoff with amount = ±2 octaves (bipolar)

**Implementation:**

```cpp
// ModulationConnection setup (message thread)
auto* conn = connectionBank->createConnection("lfo_1", "cutoff_uuid");
conn->setPolarity(true);  // Bipolar
conn->setModulationAmount(2400.0f);  // ±2 octaves in cents

conn->setParamTree(cutoffParam->getValueTree());
conn->setScalingValue(2400.0f, cutoffParam->getCurrentValue());

// Audio thread (in DirectProcessor::processBlock)
float cutoffBase = cutoffParam->getCurrentValue();  // e.g., 1000 Hz
float modSignal = modBus->getSample(lfoChannel, 0);  // -1.0 to +1.0
float scaled = modSignal * conn->getScalingForDSP();  // e.g., -0.15 to +0.15 (normalized)

// Convert normalized offset to frequency
float cutoffModulated = cutoffRange.convertFrom0to1(
    cutoffRange.convertTo0to1(cutoffBase) + scaled
);

// Result: cutoff sweeps 500 Hz → 2000 Hz at 2 Hz rate
```

### Example 2: Ramp Modulating Transpose

**Setup:**
- Synchronic preparation, "Transpose" parameter (-24 to +24 semitones)
- Create Ramp modulator: 5 second duration
- Connect Ramp → Transpose, start at 0, end at +12 (octave up)

**Behavior:**
1. Trigger ramp with MIDI note
2. Transpose smoothly sweeps from 0 → +12 over 5 seconds
3. If retriggered mid-sweep (e.g., at +7), `retriggerFrom(7.0f)` restarts from current position
4. After 5 seconds, holds at +12 until reset

**Curve Parameter:**
- `curve = 1.0` → linear ramp
- `curve > 1.0` → exponential (slow start, fast end)
- `curve < 1.0` → logarithmic (fast start, slow end)

### Example 3: State Modulation of Keymap

**Setup:**
- Keymap preparation with default selection: white keys only
- Create State modulator
- Set modulated state: black keys only
- Connect State → Keymap

**Behavior:**
1. Default: User plays MIDI notes, only white keys trigger samples
2. Trigger state modulator: Keymap instantly switches to black keys
3. User now hears samples only on black keys
4. Reset: Keymap switches back to white keys

**Use Case:**
- Live performance: Switch between chord voicings instantly
- Composition: Trigger scale changes synchronized to musical events


## Part 6: Best Practices & Gotchas

### Audio-Rate Modulation

**DO:**
- ✅ Use atomics for all audio-thread-accessible state
- ✅ Pre-allocate modulation bus channels in `prepareToPlay()`
- ✅ Clamp modulated values to valid parameter ranges
- ✅ Use `lockScaling()` for Ramp modulators to prevent mid-sweep glitches

**DON'T:**
- ❌ Allocate memory in `processBlock()`
- ❌ Touch ValueTree from audio thread
- ❌ Assume modulation signals are normalized (they're not — apply scaling!)
- ❌ Forget to update modulation channel count when adding parameters

### State Modulation

**DO:**
- ✅ Use `ParameterChangeBuffer` for thread-safe state passing
- ✅ Clone components for modulation editors (don't reuse instances)
- ✅ Validate ValueTree before applying changes (check `isValid()`)
- ✅ Handle change buffer contention gracefully (drop changes if queue full)

**DON'T:**
- ❌ Apply state changes directly from audio thread
- ❌ Block audio thread waiting for change buffer
- ❌ Assume state changes are instantaneous (they're queued)
- ❌ Forget to set `defaultState` for reset behavior

### UI/UX

**DO:**
- ✅ Show visual feedback for modulation (meters, arcs, pulsing)
- ✅ Provide right-click menus for connection management
- ✅ Allow users to adjust modulation amounts without disconnecting
- ✅ Display source/destination names in tooltips

**DON'T:**
- ❌ Hide modulation controls (users need to see what's active)
- ❌ Make connection creation overly complex (drag-and-drop is intuitive)
- ❌ Forget to update meters during playback (users expect live feedback)


## Part 7: Debugging & Troubleshooting

### Common Issues

#### 1. Modulation Not Audible

**Symptoms:** Connection exists, but parameter doesn't change

**Check:**
- Is modulator active? (LFO started? Ramp triggered?)
- Is modulation bus channel allocated? (`getBusBuffer()` not null?)
- Is scaling calculated correctly? (Print `conn->getScalingForDSP()`)
- Is parameter clamped too aggressively? (Check min/max)

#### 2. Crackling/Glitches

**Symptoms:** Audio artifacts when modulation is active

**Check:**
- Is modulation signal aliasing? (LFO frequency too high?)
- Are parameter changes sample-accurate? (No block-rate updates)
- Is `lockScaling()` called for Ramp? (Prevents mid-sweep changes)
- Is buffer size mismatched? (Modulation bus vs. audio bus)

#### 3. State Changes Not Applying

**Symptoms:** State modulator triggers, but UI doesn't update

**Check:**
- Is `ParameterChangeBuffer` initialized? (Not null?)
- Is message thread polling? (Timer callback active?)
- Is ValueTree valid? (Not destroyed/detached?)
- Is `syncToValueTree()` implemented correctly?

#### 4. Memory Leaks

**Symptoms:** Memory usage grows over time with modulation active

**Check:**
- Are connections cleaned up on deletion? (`reset()` called?)
- Are modulators properly removed from listeners? (`removeListener()`)
- Are OpenGL components disposed correctly? (Check `open_gl_` cleanup)

### Debug Logging

Enable modulation debug logs:

```cpp
#define MOD_DEBUG 1  // In ModulationConnection.cpp

// Logs connection creation, trigger events, scaling calculations
```

### Profiling

Measure modulation overhead:

```bash
# Instruments (macOS)
instruments -t "Time Profiler" ./bitKlavier.app

# Look for time spent in:
# - ModulationProcessor::processBlock()
# - ModulationConnection::updateScalingAudioThread()
# - SynthSlider::setLiveModulatedValue()
```

**Expected Overhead:**
- Single LFO: ~0.01% CPU (negligible)
- 10 Ramp modulators: ~0.05% CPU
- 50 connections: ~0.1% CPU

If higher, investigate scaling calculation hot spots.

## Part 8: Future Enhancements

### Potential Additions

1. **Envelope Followers**
    - Modulate parameters based on input audio amplitude
    - Use `juce::dsp::BallisticsFilter` for smoothing

2. **MIDI CC Modulators**
    - Map MIDI CC messages → parameter modulation
    - Already partially implemented in `MidiManager`

3. **Multi-Stage Envelopes**
    - ADSR/AHDSR with sustain loops
    - More flexible than simple Ramp

4. **Modulation Matrix View**
    - Spreadsheet-style UI showing all connections
    - Bulk edit modulation amounts

5. **Preset Modulation Templates**
    - Save/load modulation setups independently
    - Quick recall for common routing patterns

### Architectural TODOs
- **Consolidate connection banks:** Merge `ModulationConnectionBank` and `StateConnectionBank`?
- **Simplify scaling calculation:** Precompute more at setup time (message thread)
- **Optimize meter rendering:** Use GPU for modulation arc drawing (currently CPU)
- **Add modulation recording:** Capture modulation automation for DAW playback

## Glossary

| Term | Definition |
| --- | --- |
| **Audio-Rate Modulation** | Continuous parameter changes at sample rate (e.g., LFO sweeping filter) |
| **State Modulation** | Discrete parameter changes (e.g., switching keymap on/off) |
| **Modulation Bus** | Dedicated audio channels in `AudioProcessorGraph` for routing mod signals |
| **Scaling** | Converting raw modulation signal to parameter units (respects range, skew) |
| **Bipolar** | Modulation applies symmetrically (±) around base value |
| **Unipolar** | Modulation applies in one direction (0 → +) from base value |
| **Lock Scaling** | Freeze scaling factor during Ramp to prevent mid-sweep glitches |
| **ParameterChangeBuffer** | Lock-free queue for passing ValueTree snapshots audio → message thread |
| **StateModulatedComponent** | Base class for UI components supporting state modulation |
| **ModulationManager** | UI overlay for visualizing and editing all modulation connections |

---

**Last Updated:** 2026-05-24  
**Version:** 4.9.10  
**Contributors:** Dan Trueman, Davis Polito, Myra Norton

**Related Documentation:**
- `CLAUDE.md` — Main project architecture guide
- `RESONANCE_THREADING.md` — Multi-threaded audio processing patterns
- `bitKlavierDevNotes.md` — Parameter system details

**External References:**
- [JUCE AudioProcessorGraph](https://docs.juce.com/master/classAudioProcessorGraph.html)
- [chowdsp_parameters](https://github.com/Chowdhury-DSP/chowdsp_utils)
- [Lock-Free Programming](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
