---
name: clone-constructor-null-params
description: "State-mod clone constructors on multi-element widgets must not deref `params` — `params` is null on the clone path. Pass real range literals (or shared constants) to the parent constructor instead."
metadata:
  type: project
---

The state-mod editor "clone" for a parameter widget is built by a private no-arg constructor (`OpenGL_Foo()`) invoked from `Foo::clone()` and rendered with `isModulation_ = true` and the "MODIFIED" border. Because this constructor takes no `_params` argument, the member `params` stays nullptr — but several clone constructors used to dereference `params->...->getNormalisableRange()` to push the real param range into the slider. That crashes on the first user click of the indicator.

Two distinct ways this manifested:

1. **Direct null deref** — `setMinValue(params->...->start, …)` reads through a null pointer. The backtrace shows `this=0x118` (or similar small offset) — that's the offset of the chowdsp param inside the params struct when the struct pointer is null.
2. **Silent range typo unmasked by removing the deref** — `OpenGL_HoldTimeMinMaxSlider`'s clone passed `12000.f` to the parent `BKRangeSlider` constructor (one zero short of the real `holdTimeMinMax_rangeMax = 120000.f`). In the regular constructor, a subsequent `setMaxValue(params->…->end)` extended the slider's range to 120000, so `setSkewFactorFromMidPoint(60000)` landed inside the range. The clone had no such extension, so 60000 was out of range and tripped JUCE's `NormalisableRange::setSkewForCentre` assertion.

## How to apply

When fixing or auditing a clone constructor:

- Never read `params` (or any `_params`-derived member) in the clone constructor body. The clone path passes nothing — it is a UI-only stub.
- Pass the **shared range constants** (e.g. `holdTimeMinMax_rangeMin/Max`, `clusterMinMax_rangeMin/Max`) to the parent constructor, not handwritten literals. Literals drift; the constants are the same ones used in `ParamUtils::createNormalisableRange`, so the slider range matches what `setSkewFactorFromMidPoint(*_rangeMid)` expects.
- For `Slider::setSkewFactorFromMidPoint`, the midpoint **must** be inside the current slider range — JUCE asserts otherwise. The slider's range is whatever the parent constructor set; range-stretching happens via later `setMinValue/setMaxValue` calls on the regular path, not on the clone.

## Fixed instances (commit `6401dab1`)

- `source/interface/components/opengl/OpenGL_ClusterMinMaxSlider.h` — removed `params->clusterMinParam/clusterMaxParam` derefs in the clone constructor. Cluster's literal range `1–12` was already correct, so no parent-ctor change needed.
- `source/interface/components/opengl/OpenGL_HoldTimeMinMaxSlider.h` — removed `params->holdTimeMin/MaxParam` derefs and switched the clone's parent-ctor range from `0.f, 12000.f` to `holdTimeMinMax_rangeMin, holdTimeMinMax_rangeMax`.

## Audit hint

`grep -n 'params->' source/interface/components/opengl/OpenGL_*MinMaxSlider.h` will turn up any other range-slider clone constructors with the same shape. Same risk applies to any `Foo::clone()` that calls a no-arg `Foo()` and whose body reads `params`/`_params` — the pattern is wider than just range sliders.

Related: [[state_mod_indicator_placement]] (the indicator click path that ends up in `clone()`).
