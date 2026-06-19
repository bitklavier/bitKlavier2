---
name: feedback-no-worktrees
description: Don't create git worktrees for this project — they clutter CLion's file search
metadata:
  type: feedback
---

Don't use the EnterWorktree tool or create git worktrees in this project.

**Why:** Worktrees land under `.claude/worktrees/` and CLion indexes them, polluting "Find File" results with duplicate copies of every source file. The user found this very disruptive.

**How to apply:** Work directly on the current branch (e.g. `dandev`) for all tasks. Never pass `isolation: "worktree"` to Agent calls or invoke EnterWorktree. If a task genuinely needs parallel branch work, use a regular `git checkout -b` instead and ask the user.
