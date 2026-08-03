Bump the patch version, merge the current branch to main, push main (which triggers CI), then push the current branch with [skip ci] to avoid a duplicate build.

Steps:
1. Read the VERSION file and increment the patch number (e.g. 4.9.24 → 4.9.25).
2. Write the new version back to VERSION.
3. Commit VERSION with message "v<new_version>" (no Co-Authored-By needed here, keep it clean).
4. Note the current branch name.
5. `git checkout main && git merge <branch> --no-edit`
6. `git push origin main`
7. `git checkout <branch>`
8. Amend the last commit to append " [skip ci]" to its message, then `git push origin <branch> --force-with-lease`.

Important rules:
- Never touch main.cpp or any other file for the version bump — only VERSION changes unless the major version digit changes (which would require updating `getApplicationName()` in source/standalone/main.cpp).
- The `[skip ci]` must be in the HEAD commit of the branch push — not a separate empty commit after the push. Amending is the correct approach.
- Do not use `-o ci.skip` (that is GitLab syntax; GitHub ignores it silently, which causes a spurious CI run).
- Confirm the final git log on both branches looks correct before finishing.
