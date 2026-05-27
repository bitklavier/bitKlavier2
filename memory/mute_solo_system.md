# Mute/Solo System for Sound-Making Preparations

All 8 sound-making processors (Direct, Nostalgic, Synchronic, Resonance, Blendronic, Compressor, EQ, Reverb) have **M** and **S** buttons side-by-side in their ParametersView popups.

## 4-Atomic State Model (in each `*Params` struct)

```cpp
std::atomic<bool> muted_     { false };  // effective mute — read by audio thread
std::atomic<bool> userMuted_ { false };  // user's explicit mute intent
std::atomic<bool> soloed_    { false };  // this prep is soloed
std::atomic<bool> soloMuted_ { false };  // muted because another prep is soloed
```

Invariant: `muted_ = userMuted_ || soloMuted_` — maintained by `IMuteSolable::updateEffectiveMute()`.

**Solo state is transient only — not saved to gallery files.**

## IMuteSolable Interface

`source/synthesis/framework/Processors/IMuteSolable.h` — abstract base with 4 virtual atomic getters + default `updateEffectiveMute()`. All 8 processors inherit from it.

## SynthBase Coordinator Methods

`SynthBase::coordinateSoloChanged(NodeID changedId, bool isOptionClick)` — call after storing new `soloed_` value on the changed prep. Iterates `getActivePreparationList()->objects`, `dynamic_cast<IMuteSolable*>`, recomputes `soloMuted_` and `muted_` for all.

`SynthBase::coordinateMuteChanged(NodeID changedId, bool newUserMuted, bool isOptionClick)` — call after storing new `userMuted_` value. Handles option-click exclusive mute/unmute-all.

## Option-Click Behavior

- **Option-click Solo ON** → exclusive solo: all other preps' `soloed_` cleared
- **Option-click Solo OFF** (when multiple soloed) → all solos cleared
- **Option-click Mute ON** → all other preps' `userMuted_` cleared
- **Option-click Mute OFF** → all user mutes cleared

## UI — onClick pattern

**M button** reads `userMuted_` (NOT button toggle state, which blinks during solo-mute):
```cpp
bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
bool newMuted = !params.userMuted_.load(std::memory_order_relaxed);
params.userMuted_.store(newMuted, ...);
params.muted_.store(newMuted || params.soloMuted_.load(...), ...);
if (synth_) synth_->coordinateMuteChanged(nodeId_, newMuted, isOptionClick);
```

**S button**:
```cpp
bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
bool newSoloed = !params.soloed_.load(...);
params.soloed_.store(newSoloed, ...);
if (synth_) synth_->coordinateSoloChanged(nodeId_, isOptionClick);
```

## timerCallback — blink logic

Timer is 250ms for Direct, 50ms for all others. M button blinks when `soloMuted_ && !userMuted_`:
```cpp
soloButton_->setToggleState(soloed, juce::dontSendNotification);
if (soloMuted && !userMuted) {
    bool blinkPhase = (juce::Time::getMillisecondCounter() / 300) % 2;
    muteButton_->setToggleState(blinkPhase, juce::dontSendNotification);
} else {
    muteButton_->setToggleState(userMuted, juce::dontSendNotification);
}
```

## Layout — resized()

M+S placed side-by-side with 2px gap spanning Send+Main meter columns:
```cpp
constexpr int kGap = 2;
int halfW = (muteRow.getWidth() - kGap) / 2;
muteButton_->setBounds(muteRow.removeFromLeft(halfW));
muteRow.removeFromLeft(kGap);
soloButton_->setBounds(muteRow);
```
Blendronic uses a different style (direct `setBounds(x, y, w, h)`) — see `BlendronicParametersView.cpp`.

## Preparation getPrepPopup() Pattern

All 8 Preparation classes pass synth + nodeId to their ParametersView:
```cpp
auto nodeId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(state.getProperty(IDs::nodeID));
return std::make_unique<XxxParametersView>(..., parent->getSynth(), nodeId);
```
Bus processors (Compressor, EQ, Reverb) insert `isPrepVersion=true` before synth/nodeId.
