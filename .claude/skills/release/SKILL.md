---
name: release
description: Cut a new git-profiles release end to end — write a curated changelog, bump the version in every file, commit, tag and push to trigger the RPM/DEB build on GitHub Actions. Use when the user asks to release, cut, publish or ship a version, e.g. "suba a versão 0.0.2", "release 0.0.2", "/release 0.0.2".
---

# Release a new version

Cut and publish version `X.Y.Z` of git-profiles. The version is single-sourced from
`CMakeLists.txt`; `tools/bump-version.sh` syncs it everywhere and writes the changelogs; pushing
the tag `vX.Y.Z` triggers the RPM + DEB build in `.github/workflows/release.yml`.

## Before anything

- Get the target version `X.Y.Z` from the request (semver `major.minor.patch`). If the user said
  "next patch/minor/major" instead, compute it from the current `project(... VERSION ...)` in
  `CMakeLists.txt`.
- Run `git status` / `git branch --show-current`. You must be on `main` with a clean working tree.
  If it is dirty or on another branch, **stop and ask** — do not bundle unrelated changes into a
  release.
- Confirm there are commits to release: `git log "$(git describe --tags --abbrev=0)"..HEAD`.
  If empty, tell the user there is nothing new to release and stop.

## Steps

1. **Curate the changelog (always).** The release changelog is a short, **user-facing** summary —
   never the raw commit list. Read the commits since the last tag
   (`git log --no-merges --pretty='%s' "$(git describe --tags --abbrev=0)"..HEAD`), condense them
   into a few plain-language lines (drop pure docs/chore/ci noise, group related work, reword for a
   user), and write one line per entry to a notes file (e.g. in a scratch dir).
2. **Bump + sync:** run `tools/bump-version.sh X.Y.Z <notes-file>`. It updates `CMakeLists.txt`,
   the RPM spec (+ `%changelog`), the AppStream metainfo (+ `<release>`), `debian/changelog` and
   the `.po` header, using the curated lines so all three changelogs stay identical.
3. **Review:** show the user the result (`git diff -- debian/changelog` is the most readable) and
   get a go-ahead to publish.
4. **Commit:** `git add -A && git commit -m "chore: release X.Y.Z"`.
   Never add Claude attribution (no `Co-Authored-By`, no "Generated with…").
5. **Tag + push:** `git tag vX.Y.Z` then `git push origin main vX.Y.Z`.
   Pushing is over SSH with the personal account (`hernique33comiitei`) — never the work account.
   This publishes the release, so only do it after the user confirmed in step 3.
6. **Report:** the tag push started the `Release` workflow. Watch it
   (`gh run watch "$(gh run list -R hernique33comiitei/git-profiles --limit 1 --json databaseId --jq '.[0].databaseId')"`)
   and confirm both **Build RPM** and **Build DEB** pass and the `.rpm` + `.deb` are attached to the
   GitHub Release. If a job fails, read its log, fix, and re-run (a re-tag needs `-f`).

## Notes

- **The changelog is always curated** (user-facing, not raw commits) — this is a standing decision.
- A tag name can't be reused; each release is a new number. Re-pushing the same tag needs `-f` and
  only during fixes.
- `tools/bump-version.sh` with no argument re-syncs every file to the current CMake version (a safe
  no-op when already consistent) — useful if the packaging files ever drift.
- The gh/gt/glab version pins in `src/Requirements.cpp` are a separate decision; bump them by hand
  when a release should require newer provider CLIs.
