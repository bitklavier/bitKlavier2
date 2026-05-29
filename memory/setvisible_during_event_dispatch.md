# macOS Sonoma: setVisible() During Mouse Event Dispatch Cancels Drag

## Bug

On macOS Sonoma (14.x), calling `setVisible()` on a JUCE component from within a mouse event handler causes the native NSView layer to cancel the in-progress drag. The result: `mouseDrag` is never delivered to any component after that point, even though `mouseDown` fired correctly. This was confirmed via `DBG` output showing `mouseDown` firing (twice) with no subsequent `mouseDrag` output at all.

The bug does not reproduce on macOS Tahoe (26.x).

## Root Cause in bitKlavier2

`MainSection::MainSection()` (`source/interface/sections/main_section.cpp`) registers ConstructionSite as a deep mouse listener:

```cpp
addMouseListener(constructionSite_.get(), true);
```

This causes ConstructionSite's `mouseDown` to fire **twice** for any click on empty ConstructionSite canvas space — once as the direct target, and once as MainSection's registered listener. In the second invocation, the old lasso code called `selectorLasso.endLasso()` which called `selectorLasso.setVisible(false)`. On Sonoma, that `setVisible` call during the event dispatch cancelled the NSView drag tracking.

## Fix

Guard the lasso setup block with `!lasso_active_` so the second (listener) invocation is a no-op:

```cpp
if (!cableView.cableBeingDragged() && !lasso_active_)
{
    lasso_active_ = true;
    selectorLasso.toFront(false);
    selectorLasso.endLasso();
    selectorLasso.beginLasso(e, &preparationSelector);
}
```

The first (direct) call sets `lasso_active_ = true` and runs setup. The second (listener) call sees `lasso_active_` already true and skips everything — no `setVisible(false)`, no drag cancellation.

## General Rule

Never call `setVisible()` more than once per mouse event dispatch chain. If a component's mouse handler is invoked both directly (as the hit-test target) and as a listener (via `addMouseListener`), use a state flag to make the second invocation a no-op. This is especially critical on Sonoma where `setVisible` during event dispatch has an observable side effect on native drag tracking.

## Additional changes made alongside this fix

- `selectorLasso` moved from dynamic `addChildComponent`/`removeChildComponent` per-drag to a permanent child added in the ConstructionSite constructor (eliminates dynamic component tree mutation during OpenGL rendering)
- `lasso_active_` bool flag replaces `selectorLasso.isVisible()` as the gate for drag selection logic — decouples selection from visual state
- `mouseDrag` now calls `e.getEventRelativeTo(this)` before passing the event to `dragLasso`, consistent with `mouseDown`

## Key files
- `source/interface/sections/ConstructionSite.cpp` — lasso mouseDown/mouseDrag/mouseUp logic
- `source/interface/sections/main_section.cpp` — the `addMouseListener` registration
