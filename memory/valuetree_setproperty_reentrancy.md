---
name: valuetree-setproperty-reentrancy
description: JUCE ValueTree setProperty fires valueTreePropertyChanged synchronously — intermediate callbacks can corrupt values not yet saved in a multi-property save loop
metadata:
  type: feedback
---

When saving multiple VT properties in sequence, each `state.setProperty(...)` fires `valueTreePropertyChanged` **synchronously on the same call stack**. If the listener re-reads other VT properties to sync local state, it will read stale (not-yet-saved) values for properties that come later in the loop.

**Why:** Encountered in `CommentPreparation::onFormatChanged`. The callback saved bold, italic, alignment, … in order. The `valueTreePropertyChanged` handler for `commentBold` re-read ALL properties including `commentAlignment` — but at that moment `commentAlignment` wasn't in VT yet (fresh comment), so it read default 0 and overwrote `textEditor.alignment_ = 0`. Then the `commentAlignment` setProperty line read `textEditor.alignment_` (now 0) and saved the wrong value. Second attempt worked because bold/italic were already in VT, so their setProperty calls were no-ops (JUCE skips the callback when value is unchanged).

**How to apply:** Whenever a callback saves multiple VT properties and the `valueTreePropertyChanged` handler reads VT properties back into local state, capture ALL values into locals **before** the first `setProperty` call:

```cpp
// WRONG — intermediate callbacks corrupt later values
state.setProperty(IDs::commentBold,      (int) textEditor.bold_,   &undo);
state.setProperty(IDs::commentAlignment, textEditor.alignment_,    &undo); // alignment_ may have been reset!

// CORRECT — snapshot first, then save
const int bold      = (int) textEditor.bold_;
const int alignment = textEditor.alignment_;
state.setProperty(IDs::commentBold,      bold,      &undo);
state.setProperty(IDs::commentAlignment, alignment, &undo);
```

This applies any time a listener that saves N properties also has a `valueTreePropertyChanged` handler that reads those properties back into local variables.
