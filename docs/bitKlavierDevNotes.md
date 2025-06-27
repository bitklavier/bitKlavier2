# Notes about how to do stuff in the bK codebase

---------
## Adding a Parameter to a Preparation (Direct, for example)
- in DirectProcessor.h (in Source/synthesis/Processors)
  - define the new chowdsp param in the DirectParams struct 
  - if it's a simple parameter (a single float, for instance) requiring only a basic knob, add it as a chowdsp param
    - `chowdsp::GainDBParameter::Ptr gainParam` for instance 
  - for a more complex set of parameters requiring a more complex UI element, you might need to create a `ParamHolder`
    - see `TransposeParams` for instance 
  - Call `add` and include your new param with the others 
    - first line in the DirectParams struct, for instance