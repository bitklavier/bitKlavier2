Bump the patch version, merge the current branch to main, push main (which triggers CI), and push the current branch with [skip ci] to avoid a duplicate build.

The version commit carries [skip ci] itself and is pushed to the branch FIRST. Main then gets a plain `--no-ff` merge commit as its HEAD, which is what triggers CI (GitHub only checks the HEAD commit of a push). The version commit therefore exists exactly once, with one hash, in both histories — nothing is amended or force-pushed, so VERSION can never conflict on the next ship.

Steps:
1. Note the current branch name. Make sure any other work is already committed (only VERSION should change in the bump commit).
2. Read the VERSION file and increment the patch number (e.g. 4.9.24 → 4.9.25).
3. Write the new version back to VERSION.
4. Commit VERSION with message "v<new_version> [skip ci]" (no Co-Authored-By needed here, keep it clean).
5. `git push origin <branch>` (plain push, no force). The HEAD commit contains [skip ci], so no CI runs for the branch.
6. `git checkout main && git merge <branch> --no-ff -m "Merge branch '<branch>'"`
   - `--no-ff` is required: a fast-forward would make main's HEAD the "[skip ci]" version commit and skip CI on main.
7. `git push origin main` (triggers CI).
8. `git checkout <branch> && git merge main --ff-only` so the local branch includes main's merge commit. Do NOT push the branch again: its tip would be the merge commit (no [skip ci]) and would start a duplicate build. The remote branch simply stays one commit behind until the next ship.
9. Confirm the final git log on both branches, and that local main matches origin/main.

Important rules:
- Never touch main.cpp or any other file for the version bump — only VERSION changes unless the major version digit changes (which would require updating `getApplicationName()` in source/standalone/main.cpp).
- Never amend the version commit or force-push. Amending after the merge gives main and the branch two different hashes for the same bump, and the next bump then conflicts on VERSION.
- The `[skip ci]` must be in the HEAD commit of the branch push, and must NOT be in the HEAD commit of the main push.
- Do not use `-o ci.skip` (that is GitLab syntax; GitHub ignores it silently, which causes a spurious CI run).
- If the merge into main conflicts on VERSION, resolve it to the new version, but treat it as a sign the branches have diverged and say so.
