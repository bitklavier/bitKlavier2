# Notes about how to do stuff in the bK codebase

---------
## adding a parameter to a preparation (Direct, for example)
1. in DirectProcessor.h (in Source/synthesis/Processors)
   2. add the new param in the DirectParams struct
      3. if it's a simple parameter (a single float, for instance) requiring only a basic knob, add it as a chowdsp param 
         4. `chowdsp::GainDBParameter::Ptr gainParam` for instance
      5. for a more complex set of parameters requiring a more complex UI element, you might need to create a `ParamHolder`
         6. see `TransposeParams` for instance
      7. be sure to `add` your new param!