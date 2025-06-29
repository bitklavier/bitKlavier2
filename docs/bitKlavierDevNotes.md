# Notes about how to do stuff in the bK codebase

---------
## Questions for Davis (or things to check on with him)
and hopefully with answers included here for the record!
-[ ] check on saving/loading galleries and Direct, not working well right now (drawing funny, Direct preps not loading)
-[ ] check on Mods with Direct; are they working for all params? do they save?
-[ ] i’m not clear when we need to create and run processStateChanges for params; is it only when dealing with legacy bK UI elements? like TuningState, or velocityMinMaxSlider?)
-[ ] and what about the serializer/deserializers, like in TuningProcessor.h? do i need to add SemitoneWidthParams to them? 
-[ ] what’s up with the initializer_lists in TuningProcessor.h, like chowdsp::EnumChoiceParameter<Fundamental>? 
-[ ] in DirectParametersView.h, constructor args question for EnvSection, for instance 
-[ ] for UI constants, do you think we should be working with the Tytel skins.h stuff, or the BKGraphicsConstants from the old bK? // 8) currently getting tuning info from TuningState, but not sure how to get other param info into it; for instance, the SemitoneWidthParams
- [ ] currently getting tuning info into BKSynthezier from TuningState, with getTargetFrequency(), but not sure how to get other param info into it; for instance, the SemitoneWidthParam. or should we be handling this differently, especially since there will be a LOT in Tuning


---------
## Adding a Parameter to a Preparation (Direct, for example)
- in `DirectProcessor.h` (in Source/framework/synthesis/Processors):
  - define the new chowdsp param in the DirectParams struct 
  - if it's a simple parameter (a single float, for instance) requiring only a basic knob, add it as a chowdsp param
    - `chowdsp::GainDBParameter::Ptr gainParam` for instance 
  - for a more complex set of parameters requiring a more complex UI element, you might need to create a `ParamHolder`
    - see `TransposeParams` for instance 
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
