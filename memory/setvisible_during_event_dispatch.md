---
name: setvisible-during-event-dispatch
description: On Sonoma, any setVisible() during mouseDown cancels NSView drag tracking — including setVisible(true). Fix: defer all beginLasso/setVisible calls to the first mouseDrag.
metadata:
  type: feedback
---

# macOS Sonoma: setVisible() During Mouse Event Dispatch Cancels Drag

On macOS Sonoma (14.x), calling `setVisible()` on ANY JUCE component — including `setVisible(true)` — from within `mouseDown` causes the native NSView layer to cancel in-progress drag tracking. `mouseDrag` is never delivered after that point.

**Why:** Debug builds are affected too, but DBG/jassert overhead adds enough latency that testing can appear to pass. Release builds (no overhead) fail consistently.

The bug does not reproduce on macOS Tahoe (26.x).

## Root Cause in bitKlavier2 (selectorLasso)

`MainSection::MainSection()` registers ConstructionSite as a deep mouse listener:

```cpp
addMouseListener(constructionSite_.get(), true);
```

This causes ConstructionSite's `mouseDown` to fire **twice** for clicks on empty canvas — once as the direct target, once as MainSection's listener.

### First fix attempt (cd5ce8ea) — incomplete

Guarded the lasso block with `!lasso_active_` to prevent the second invocation from calling `endLasso()` → `setVisible(false)`. This appeared to work in Debug but still failed in Release: the **first** invocation still called `beginLasso(e, ...)` → `setVisible(true)`. On Sonoma, even `setVisible(true)` during `mouseDown` cancels drag tracking.

### Complete fix (15019820)

Remove ALL `setVisible`-triggering calls from `mouseDown`. Defer `beginLasso` entirely to the first `mouseDrag`:

```cpp
// mouseDown — only set the flag, no setVisible:
if (!cableView.cableBeingDragged() && !lasso_active_)
{
    lasso_active_ = true;
    // beginLasso (setVisible) deferred to first mouseDrag
}

// mouseDrag — safe to call setVisible here:
if (lasso_active_)
{
    juce::MouseEvent eLasso = e.getEventRelativeTo(this);
    if (!lasso_started_)
    {
        selectorLasso.toFront(false);
        selectorLasso.endLasso();
        selectorLasso.beginLasso(eLasso, &preparationSelector);
        lasso_started_ = true;
    }
    selectorLasso.dragLasso(eLasso);
    ...
}

// mouseUp:
lasso_active_ = false;
lasso_started_ = false;
```

By the time `mouseDrag` fires, native drag tracking is already established — `setVisible(true)` there is safe.

## General Rule

**Never call `setVisible()` (true OR false) on any component during `mouseDown` event dispatch on Sonoma.** This includes calls made indirectly through JUCE APIs (e.g. `beginLasso` → `setVisible(true)`).

If lasso setup requires `setVisible`, defer the entire setup block to the first `mouseDrag` call, gated on a `lasso_started_` flag.

**Debug vs Release diagnostic gotcha:** A fix that only suppresses `setVisible(false)` (second invocation) may appear to work in Debug due to timing overhead masking the remaining `setVisible(true)` in the first invocation. Always verify the fix in a Release build on Sonoma.

## Why CI builds failed even after the ConstructionSite fix

The JUCE NSOpenGLView patch (`patches/juce_opengl_mac_mouse_forwarding.patch`) is the **deeper root fix** — without it, `mouseDragged:` events are swallowed by NSOpenGLView on Sonoma and never reach JUCE at all. Our ConstructionSite.cpp changes are also necessary but secondary.

The patch was silently never applied in CI because `git apply --check` inside the JUCE submodule directory failed with `"fatal: unsafe repository"` (GitHub Actions ownership mismatch). `ERROR_QUIET` swallowed the error, the non-zero return was misread as "already applied", and the patch was skipped every CI run. See [[ci-git-apply-safe-directory]] for the fix.

## Key files
- `source/interface/sections/ConstructionSite.cpp` — lasso mouseDown/mouseDrag/mouseUp logic
- `source/interface/sections/ConstructionSite.h` — `lasso_active_`, `lasso_started_` flags
- `source/interface/sections/main_section.cpp` — the `addMouseListener` registration
- `patches/juce_opengl_mac_mouse_forwarding.patch` — the JUCE-level fix (NSOpenGLView event forwarding)
- `CMakeLists.txt` lines 43–80 — patch application logic with safe.directory fix
