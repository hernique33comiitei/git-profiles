---
name: release
description: Cut a new git-profiles release end to end — bump the version in every file, generate the changelog from the commits since the last tag, commit, tag and push to trigger the RPM/DEB build on GitHub Actions. Use when the user asks to release, cut, publish or ship a version, e.g. "suba a versão 0.0.2", "release 0.0.2", "/release 0.0.2".
---

# Release a new version

Cut and publish version `X.Y.Z` of git-profiles. The version is single-sourced from
`CMakeLists.txt`; `tools/bump-version.sh` syncs it everywhere and builds the changelogs from git
history; pushing the tag `vX.Y.Z` triggers the RPM + DEB build in `.github/workflows/release.yml`.

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

1. **Bump + sync:** run `tools/bump-version.sh X.Y.Z`. It updates `CMakeLists.txt`, the RPM spec
   (+ `%changelog`), the AppStream metainfo (+ `<release>`), `debian/changelog` and the `.po`
   header, filling the changelog entries from the commit subjects since the last tag.
2. **Review the changelog:** show the user the generated entries (`git diff -- debian/changelog`
   is the most readable). The entries are raw commit subjects — offer to trim noise (docs/chore)
   or reword before continuing. Apply any edits they ask for to all three changelog files
   (spec, debian/changelog, metainfo) so they stay consistent. Then get a go-ahead to publish.
3. **Commit:** `git add -A && git commit -m "chore: release X.Y.Z"`.
   Never add Claude attribution (no `Co-Authored-By`, no "Generated with…").
4. **Tag + push:** `git tag vX.Y.Z` then `git push origin main vX.Y.Z`.
   Pushing is over SSH with the personal account (`hernique33comiitei`) — never the work account.
   This push is what publishes the release, so only do it after the user confirmed in step 2.
5. **Report:** the tag push started the `Release` workflow. Watch it
   (`gh run watch "$(gh run list -R hernique33comiitei/git-profiles --limit 1 --json databaseId --jq '.[0].databaseId')"`)
   and confirm both **Build RPM** and **Build DEB** pass and the `.rpm` + `.deb` are attached to the
   GitHub Release. If a job fails, read its log, fix, and re-run (a re-tag needs `-f`).

## Notes

- A tag name can't be reused; each release is a new number. Re-pushing the same tag needs `-f` and
  only during fixes.
- `tools/bump-version.sh` with no argument re-syncs every file to the current CMake version (a safe
  no-op when already consistent) — useful if the packaging files ever drift.
- The gh/gt/glab version pins in `src/Requirements.cpp` are a separate decision; bump them by hand
  when a release should require newer provider CLIs.
