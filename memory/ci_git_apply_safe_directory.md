---
name: ci-git-apply-safe-directory
description: In GitHub Actions, git apply in a submodule directory silently fails with "unsafe repository" unless safe.directory is configured first. Always add safe.directory before git operations in submodule dirs in CMake.
metadata:
  type: feedback
---

In GitHub Actions, `git apply` (or any git command) inside a submodule directory fails with `"fatal: unsafe repository / dubious ownership"` because the runner's user doesn't own the directory. If this error is suppressed with `ERROR_QUIET`, a non-zero return code looks identical to "patch already applied" — so patches are silently skipped every CI run.

**Why:** GitHub Actions sandboxes runs in containers where the checkout directory's owner may differ from the process user. Git 2.35.2+ refuses operations in such directories without explicit `safe.directory` opt-in.

**How to apply:** In any CMakeLists.txt `execute_process` block that runs git in a submodule directory:
1. Add `git config --global --add safe.directory <submodule-path>` before the git command
2. Never use `ERROR_QUIET` when a non-zero return has multiple interpretations (error vs. "already done")
3. Check `ERROR_VARIABLE` output for `"unsafe repository"` and `FATAL_ERROR` explicitly rather than silently skipping

```cmake
# Before any git apply in a submodule:
execute_process(
    COMMAND git config --global --add safe.directory "${CMAKE_CURRENT_SOURCE_DIR}/JUCE"
    OUTPUT_QUIET ERROR_QUIET
)
execute_process(
    COMMAND git apply --check "${PATCH_FILE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/JUCE"
    RESULT_VARIABLE CHECK_RESULT
    ERROR_VARIABLE CHECK_ERROR   # capture, don't suppress
)
if(CHECK_RESULT EQUAL 0)
    # apply patch
elseif(CHECK_ERROR MATCHES "unsafe repository|dubious ownership")
    message(FATAL_ERROR "safe.directory not set — see ci_git_apply_safe_directory memory")
else()
    # genuinely already applied
endif()
```

### Key file
`CMakeLists.txt` lines 43–80 — JUCE patch application logic (fixed in commit `cea432e4`)
