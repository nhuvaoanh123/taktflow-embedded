# Submodule Workflow

This repo is used as a submodule in `Taktflow-Systems`.

1. **Always checkout a branch** before making changes — never work on detached HEAD
2. **Commit here first, parent second** — never let parent point to a commit that doesn't exist on remote
3. **Pull routine**: `git pull && git submodule update --init` at session start (from parent repo)
4. **Use feature branches + PRs** to avoid parallel pointer conflicts on main
5. **Pointer conflicts**: take the newer submodule commit (verify with `git log`)
