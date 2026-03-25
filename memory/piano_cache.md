# Per-Piano Component Cache — Design Notes

## Goal
When switching pianos in ConstructionSite, reuse PreparationSection components instead of destroying and recreating them. First visit to each piano costs the same as now; subsequent visits are near-instant.

## Why it helps
setActivePiano() currently does a full teardown/rebuild per piano switch:
- destroyOpenGlComponents() per preparation (GL texture deallocation via executeOnGLThread)
- nodeFactory.CreateObject() per preparation (full PreparationSection constructor)
- setSkinValues(), setDefaultColor(), setSizeRatio(), setSize()/resized() per preparation
  — resized() triggers BKItem::redrawImage() which allocates a juce::Image and software-renders shadow
- First render frame: lazy GL init (init()) for all new components

The render loop already skips invisible components (isVisible() check in renderOpenGlComponents at synth_section.cpp:429), so hidden cached components have no render cost.

## Proposed implementation
Add to ConstructionSite.h:
  std::map<juce::String, std::vector<std::unique_ptr<PreparationSection>>> piano_component_cache_;
  juce::String current_piano_id_;  // e.g. piano name or uuid

In setActivePiano(), when leaving a piano:
  - setVisible(false) on each component
  - detach listeners (cableView, modulationLineView, lasso, this)
  - move plugin_components → piano_component_cache_[current_piano_id_]
  - prep_list->removeListener(this)
  - do NOT call destroyOpenGlComponents() → GL resources stay valid

When arriving at a piano:
  - if in cache: restore, setVisible(true), re-attach listeners, reposition, skip rebuildAllGui()
  - if not in cache: normal rebuildAllGui() path

CableView and ModulationLineView: still rebuild on every switch (fast, no redrawImage calls)

## Known risks to solve before implementing

1. **Cache invalidation**: prep_list->removeListener(this) is called for the hidden piano.
   If a preparation is added/deleted/undo'd while that piano is cached, the cache goes stale.
   Fix option A: re-validate cache on restore (compare ValueTree child count vs. cached component count; fall back to full rebuild if mismatch)
   Fix option B: keep ALL pianos' prep_lists listened to simultaneously, update cache incrementally

2. **all_sliders_ accumulation**: removeSubSection() only cleans all_modulation_buttons_, not
   all_sliders_, all_synth_buttons_, all_combo_box_, all_state_modulated_components.
   Cached pianos would leave stale entries in those maps on ConstructionSite.
   Could affect modulation system's slider lookup by parameterID.
   Fix: extend removeSubSection() to clean all maps, or accept stale entries are harmless.

3. **Memory**: N pianos × M preparations × GL textures all alive simultaneously.
   Probably fine (a few MB per piano), but worth profiling in large galleries.

4. **sub_sections_ iteration**: Cached components remain in sub_sections_ (if we skip removeSubSection),
   making the render loop longer. Mitigated by isVisible() guard, but still O(all cached preps) per frame.

## When to implement
Best suited for performance-mode scenarios (PianoSwitch processor triggering rapid switches).
More risky for editing workflows where undo/add/delete while on a different piano is common.
Consider Fix option A (stale detection + fallback) as the minimal safe approach.
