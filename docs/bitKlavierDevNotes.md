# Notes about how to do stuff in the bK codebase
## Priorities before Davis leaves
- [ ] Piano Switch prep: Davis works on this next
  - all pianos in graph together, bypass notes for inactive pianos
    - getBypassParameter in https://docs.juce.com/master/classAudioProcessorGraph.html
  - will need to be able to gracefully handle preps that need to finish their thing (Nostalgic holdover, noteOns that are still active, etc...)
    - as well as how mods are handled at that moment
- [ ] finish mods for Direct and Tuning:
  - working, saving/loading, Dan mostly understanding the code
- [ ] Blendrónic audio in: Dan will try this after Wisconsin
- [ ] Blurry fonts ;--} look at `OpenGlImageComponent` in open_gl_image_component.cpp
- [ ] opening multiple sample libs, assigning to individual preps!
  - ConstructionSite::moduleAdded, should be doable there.

## Quick Bug/Feature Notes
- [ ] Keymap parameter targeting: 
  - requires a big new solution!
  - a new preparation: the Target Prep, or actually a MidiFilter prep
  - Keymap => Preparation, we get the default behavior, as always
  - Keymap => MidiFilter => Preparation (let's say Synchronic for this example)
  - in Target, we have 
    - all non-default behaviors (Pattern Sync, Beat Sync, etc...) that were in the old Keymap 
    - along with the NoteOn/Off/Both options for each that were in the old Synchronic
    - for every behavior that is checked on, we repack the MIDI messages with a new channel, assigned to that behavior, and filter according to the NoteOn/Off/Both settings
      - so, if Pattern Sync is checked on, with Note-Off as the mode, the MIDI messages will be unpacked, NoteOn messages discarded, and the NoteOn messages reassigned to channel 2 (for Pattern Sync, 3 for Beat Sync, etc..), and the messages are repacked and sent out
      - if there are multiple behaviors checked on, then there will be multiple MIDI messages for each original MIDI message, one on each behavior channel
  - this is essentially a new class of MIDI filter prep, which could be useful going forward
    - all/some of the filtering in the old Keymap might now be here (invert noteOn/Off, for example)
      - but for now, let's just focus on the Target functionality
    - MidiFilter.... Filter (F). We're using V for VST for audio plugin and F is available
- [ ] control-click menu is completely unmanageable right now with all the plugins there...
- [ ] i'm thinking the knobs should show their values at all times, or at least on mouse-over; very hard to track what's going on just by knob position
- [ ] we'll need to be able to override the default ranges of params, when a user types in a value outside the default range. 
  - should be able to create a public function in chowdsp_ParameterTypes that accesses `const juce::NormalisableRange<float>& valueRange`. should able to make that not a `const` where it is privately declared there
  - for changes to chowdsp, need to add/commit to that submodule branch, and then push. supdate first. we have our own fork of chowdsp, also used by electrosynth
- [ ] weird dialog boxes when control-clicking on knobs
- [ ] for the transposition slider, if 0 isn't the first element we don't get it at all. so [0 4] works but [4 0] does not
- [ ] maybe we should allow drag/drop to make audio connections between preps as well
- [ ] also need to be able to see the transposition values when mousing over the sub-sliders in Transposition slider
- [ ] i believe pitchbend is not yet implemented
- [ ] MTS still needs to be implemented for Tuning, as does Scala
- [ ] Spring and Adaptive tuning don't handle Transpositions properly
- [ ] dragging preps around the construction site is a little frustrating; sometimes they move, sometimes they just pop back where they were
- [ ] in general, the preps seem BIG. is scaling them going to be straightforward? might even want a user option to zoom in and out to various percentages for the construction site
- [ ] would like the Audio Buffer Size to default to 128, not 512
---------
## Questions for Davis (or things to check on with him)
and hopefully with answers included here for the record!

- [ ] struggling with the UI drawing
  - in Tuning, trying to setVisible to false and many things still lingering; spent a good bit of time trying to find solutions to no avail
  - i did end up in FullInterface.cpp, and tried `open_gl_context_.setComponentPaintingEnabled (false);` to true and commenting out the paintBackground stuff in SemitoneWidthSection, and MANY things looked different, some things looked better, and the hiding/visibility issue went away, but i gather there are other complications?
  - interesting: when i do this change, the border around Semitone Width is no longer blurry.
- [ ] for the serializer/deserializers in TuningProcessor.cpp, take a look at my todo questions in the .cpp file
  - i think we don't need to serialize the circularTuningOffset arrays, but we do need to serialize the circularTuningOffset_custom array, yes?
- [ ] check on saving/loading galleries and Direct, not working well right now (drawing funny, Direct preps not loading)
  - Myra is working on. but Direct saves now, except Transpositions, so check that
- [ ] check on Mods with Direct; are they working for all params? do they save?
  - need to be tested and fixed
  - let's prioritize this, for Direct and Tuning
- [ ] i’m not clear when we need to create and run processStateChanges for params
  - this for ui sections, where we don't do audio-rate mods, just param-state mods, and want to change the whole thing
    - for instance, the transposition slider: we're going to change all those values at once, and try to change them individually continuously
    - the mod stuff is complicated and needs a full section in this doc!
- [x] what’s up with the `initializer_lists` in TuningProcessor.h, like `chowdsp::EnumChoiceParameter<PitchClass>`? 
  - these looks strange but the `initializer_lists` provide substitution patterns for text
    - `{ '_', ' ' }, { '1', '/' }` means _ will be replaced by a space, and 1 will replaced by a /
    - so, then in the `PitchClass` enum, 
      - 4 => #
      - 1 =>/
      - 5 => b
      - so `C41D5` becomes "C#/Db"
    - this is all because certain characters are not allowed in enums
- [x] how is BinaryData.h autogenerated, as it says in its comments? i'm assuming we can export the svg layers from Illustrator
  - so there is a `juce_binarydata_Assets… BinaryData.h` that is different from some of the other BinaryDat.h files that seem to be around.
  - anyhow, yes that stuff should be autogenerated by the assets/svg files when CMAKE does its thing (CMAKE replaces all the Projucer functions where this sort of thing used to happen)
- [x] for UI constants, do you think we should be working with the Tytel skins.h stuff, or the BKGraphicsConstants from the old bK?
  - see the section below about Default Color and Graphics Constants
- [x] currently getting tuning info into BKSynthezier from TuningState, with getTargetFrequency(), but not sure how to get other param info into it; for instance, the SemitoneWidthParam. or should we be handling this differently, especially since there will be a LOT in Tuning
- [x] I need some help understanding how the tuningCallbacks work in TuningParametersView.h. In particular how the static tuning systems get updated on the back end after the user makes a choice.
- [x] how to think about copy constructors and parameters: `void SpringTuning::copy(SpringTuning* st)` for instance. these require that the parameters be set internally, which feels off.
  - shouldn't need them, using valueTrees instead
- [x] check out `lastSynthState` in DirectProcessor and whether it's thread-safe
- need to extend this with std::map of currently play notes/tunings, for spiralview
- [x] for DP: tell me more about `params.tuningState.setFromAudioThread` and why it might be important. In TuningParametersView.cpp
  - this was a conditional that Davis was working with, concerned that there may be parameters that are updated on the audio thread for some reason that would then trigger this callback
    - however, it's not being set anywhere at the moment, so it may not be necessary in the end
- [x] will creating global preferences, probably under Options menu with new window, be straightforward?
  - need A440, for instance.
  - and will save? will app, or with gallery?
---------
## Default Colors and Graphics Constants
- in `assets/default.bitklavierskin` we have all the values
  - this includes the global values (hex for colors, ints/floats for other things), but also overrides for particular subsections
    - in section "default", for instance, we've overridden the "Rotary Arc" color to match the color of Direct
- the names of these values are then matched in arrays in `skin.cpp`
- the ValueIDs for these are in enums in `skin.h`, which are in the same order the names are held in string arrays in `skin.cpp`
  - so, if you want to add new values, they need to be in the same placement/order in both `skin.cpp` and `skin.h`
- `synth_section.cpp` then has helper functions to get some of these vals (`SynthSection::getKnobSectionHeight()` for instance)
- can add to all these as needed!

---------
## Modulatable Parameters
- we need a section here about how they work!
- when creating a parameter that should be continuously modulatable, the last argument should be `true
`. For instance:
  - `chowdsp::GainDBParameter::Ptr gainParam { juce::ParameterID { "Main", 100 }, "Main", juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false }, 0.0f, true };`
- all of these need to get added to a parameter vector of `ParamPtrVariant` (like this in DirectProcessor.h: `std::unordered_map<std::string, ParamPtrVariant> modulatableParams;`)
- and then these are assigned to modulation audio channels: see DirectProcessor constructor, the `modChan.setProperty` calls

## State Change Parameters
- as opposed to Modulatable parameters; these are not changed continuously at the audio rate, the way Modulatable Parameters are, but rather are changed together, all at once
- see the `parent.getStateBank().addParam` call in the DirectProcessor constructor

---------
## Adding a Parameter to a Preparation (Direct, for example)
- in `DirectProcessor.h` (in Source/framework/synthesis/Processors):
  - define the new chowdsp param in the DirectParams struct 
  - if it's a simple parameter (a single float, for instance) requiring only a basic knob, add it as a chowdsp param
    - `chowdsp::GainDBParameter::Ptr gainParam` for instance 
  - for a more complex set of parameters requiring a more complex UI element, you might need to create a `ParamHolder`
    - see `TransposeParams` for instance 
  - see the section here about Modulatable parameters; last argument for the parameter definition should probably be `true` if this parameter is to be available to mod.
  - Call `add` and include your new param with the others 
    - first line in the DirectParams struct, for instance 
  - Note that in the UI elements that are OpenGL wrappers around legacy bitKlavier components (like the transposition slider, and the velocityMinMax slider) you'll need to create a `void processStateChanges() override` function and call it with every block
    - see `struct VelocityMinMaxParams : chowdsp::ParamHolder` for instance, and also where they are called in `DirectProcessor::processBlock`


- in `DirectParametersView.h` (in Source/interface/ParameterView):
  - this is where we'll connect the parameters from the previous step with specific UI elements
  - so, in Direct, each of the generic output stage knobs is created:
    - `auto slider = std::make_unique<SynthSlider> (param_->paramID);`
  - and gets passed to a SliderAttachment, which automatically keeps track of the slider's value
    - `auto attachment = std::make_unique<chowdsp::SliderAttachment> (*param_.get(), listeners, *slider.get(), nullptr);`
  - the listeners for this prep also get passed here, so they know to listen for changes from the slider
  - Another example, when we create the `levelMeter` we pass it its parameter (`outputGain`) and the listeners so it can attach them both to the actual slider inside the class
    - `levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);`
      - (the `outputLevels` parameter is used to display the audio levels for the meter, so we pass that parameter here as well, but it doesn't require an attachment since it's not an element that the users would interact with)
    - since the `levelMeter` combines a few elements, its a "section" that needs to get added with `addSubSection`
      - one of the nice things about this setup is that we don't have to go into the `levelMeter` to get the slider value; since it's registered with the chowdsp parameter system, that will keep track of the slider value, and we can query it elsehwere
        - in fact, if you look at the bottom of `DirectProcessor::processBlock` you will see these two lines:
          - `auto outputgainmult = bitklavier::utils::dbToMagnitude(state.params.outputGain->getCurrentValue());`
          - `buffer.applyGain(outputgainmult);`
        - which is where we can find the slider setting and use it. 
  - if you need information back from the BKSynthesizer, you can use `lastSynthState = mainSynth->getSynthesizerState();`, which will return a struct with various info, which you might need to extend. We use this with the velocityMinMax slider, getting the lastVelocity back from BKSynthesizer, since that's where all the MIDI msgs get unpacked

  
- then, in `DirectParametersView::resized()` you need to actually place the UI elements in the window

---------
## Creating a New Preparation
Typing as I do MidiFilter...
- create MidiFilterProcessor class, with params
- create the MidiFilterParametersView class
- create the MidiFilterPreparation class
- add to `BKPreparationTypes` in common.h
- create class MidiFilterItem in BKItem.h
- in BKItem.cpp, add path for preparation in `getPathForPreparation (bitklavier::BKPreparationType type)`
- add to menu in synth_gui_interface.cpp `SynthGuiInterface::getPluginPopupItems()`
- `nodeFactory.Register` it in ConstructionSite.cpp.
  - and to `CommandIDs` here
  - also add a switch case in `perform()` here
  - also `ConstructionSite::getAllCommands` and `getCommandInfo`
- register in the constructor for `PreparationList.cpp`
  - `prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeMidiFilter,MidiFilterProcessor::create);`
- might need to add it to `PreparationSection::itemDropped`
  - requires creating `midifilterDropped()` funcs in various places
- icon svg layers in assets/midifilter, with further info in BinaryData.h; BinaryData.h says it is autowritten -- how?
  - need to add a path() call in `paths.h`
  - some drawing stuff happens in `BKItem.h`
- preparation icon size is set in `ConstructionSite::perform`?
  - popup size is set in `FullInterface::resized()`, `prep_popup->setBounds`, as fraction of full window size

