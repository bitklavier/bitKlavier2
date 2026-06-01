Merge the current branch to main and push both branches with [skip ci] — no version bump, no CI triggered.

Steps:
1. Note the current branch name.
2. `git checkout main && git merge <branch> --no-edit`
3. Amend the HEAD commit on main to append " [skip ci]" to its message (if not already present), then `git push origin main --force-with-lease`.
4. `git checkout <branch>`
5. Amend the HEAD commit on the branch to append " [skip ci]" to its message (if not already present), then `git push origin <branch> --force-with-lease`.
6. Confirm the final git log on both branches looks correct before finishing.

Important rules:
- Do not bump the VERSION file or touch any files — only amend commit messages.
- Do not use `-o ci.skip` (that is GitLab syntax; GitHub ignores it silently).
- The `[skip ci]` must be in the HEAD commit of each push — not a separate empty commit. Amending is the correct approach.
- If the HEAD commit message already contains "[skip ci]", do not double-append it.
