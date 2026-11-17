# Git Profiles — project guide

Desktop app (Kirigami/QML + C++20/Qt6, CMake+ECM) that organizes git repos by SSH identity.
`~/.ssh/config` is the single source of truth: each `Host` block is a profile. The app attaches
profiles to directories, bulk-switches repo remotes (editing `.git/config` directly, dry-run first),
and configures the provider CLI (gh/gt/glab) per directory via direnv, keeping tokens in the keyring.

- Repo: [https://github.com/hernique33comiitei/git-profiles](https://github.com/hernique33comiitei/git-profiles) (push over SSH, personal account only)
- App-id: `io.github.hernique33comiitei.GitProfiles` · binary / icon / gettext-domain: `gitprofiles`

## Conventions (must follow)

- **Code, comments and docs are English only.** The UI is localized via `i18n()` (KI18n);
  Portuguese lives **only** in `po/pt_BR.po` (compiled to `gitprofiles.mo` by CMake).
- **GitHub: only the `hernique33comiitei` account.** Never the work account (`hesantos-png`).
  Git pushes go over SSH with the personal key. Author email: henriik.dev@gmail.com.
- **No Claude attribution** in commits or PRs (no `Co-Authored-By`, no "Generated with…").
- **The app installs nothing.** git / gh / gt / glab are warn-only preflight checks (missing git
  blocks the UI; gh/gt/glab warn per provider, on demand). `direnv` (≥ 2.37.1) + libsecret are the
  app's own runtime deps, installed by `install.sh`. CLI version pins live in `src/Requirements.cpp`
  (gh ≥ 2.104.0, gt ≥ 1.2.0, glab ≥ 1.130.0; git = latest) and bump together with the app version.

## Build / run / test

- Build + install to `~/.local`: `./install.sh`
  (dev deps: `extra-cmake-modules qt6-qtdeclarative-devel kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel`)
- Binary: `gitprofiles`. QML module: `org.gitprofiles`.
- Smoke test: `QT_QPA_PLATFORM=offscreen gitprofiles` (loads and exits cleanly).

## Layout

- `src/` — C++ core, exposed to QML via `QML_ELEMENT` / `QML_SINGLETON`:
  `Requirements` (preflight), `SshConfig` (round-trip parser/writer; backup + atomic + `ssh -G`),
  `ProfileModel`, `RepoScanner` (1-level scan + host rewrite), `RemoteSwitcher` (direct `.git/config`
  edit, parallel, dry-run), `Secrets` (secret-tool), `ProviderCli` (.envrc + direnv + CLI login),
  `Config`, `SshTester` (`ssh -T` connection test).
- `qml/` — UI: Main, Profiles, ProfileEditor, Directories, Apply, Settings, Dependencies, About.
- `data/` — `.desktop` + AppStream metainfo + icons.
- `packaging/fedora/` — `.spec`. `debian/` — deb packaging. `po/` — translations.

## ssh/config as database

Extra metadata lives in `#@gp key=value` comment lines inside each `Host` block (ssh ignores them):
`provider`, `apiurl`, `ghconfig`, `owner`, `token-ref`, `dir=<path> [children]`. The app only touches
`#@gp` lines and the blocks it manages; writes are backed up, atomic, and validated with `ssh -G`.

## Releases (GitHub Actions → GitHub Releases)

Distribution is RPM (Fedora) + DEB (Ubuntu/Debian), built by `.github/workflows/release.yml`.

- **The trigger is a tag `v*`, not a commit.** Pushing to `main` — even bumping the version — never
  builds a release. A release happens only when a `vX.Y.Z` tag is pushed.
- The version in `CMakeLists.txt`, `packaging/fedora/git-profiles.spec` and `debian/changelog` only
  names the package — it does **not** trigger anything, but it **must match the tag**.
- A tag name can't be reused for a new release; cut a new number (`v0.0.2`, …). (Re-pushing the same
  tag needs `-f`, which we only used during initial testing.)

### Cut a new version

1. Bump the version in `CMakeLists.txt` (`project(... VERSION x.y.z)`), the `.spec` (`Version:` +
   a new `%changelog` entry) and `debian/changelog`.
2. Commit + push `main` (no release yet).
3. `git tag vX.Y.Z && git push origin vX.Y.Z` → CI builds RPM + DEB and attaches them to the Release.

### Packaging gotchas

- Package names differ Fedora ↔ Debian/Ubuntu. The tricky one: Fedora `kf6-kirigami-devel` →
  Debian/Ubuntu **`libkirigami-dev`** (NOT `libkf6kirigami-dev`; `kirigami2-dev` is the KF5 one).
  i18n/coreaddons do follow `libkf6*-dev`.
- The deb builds directly on the `ubuntu-latest` runner (26.04 LTS). Do **not** pin an old interim
  Ubuntu container — they go EOL and their apt mirrors vanish (that killed the first deb run).
- The RPM job only attaches the main `.rpm` + `.src.rpm`; `-debuginfo`/`-debugsource` are filtered out.

### Install for users

- Fedora: `sudo dnf install <release-url>/git-profiles-<ver>.fc41.x86_64.rpm`
- Ubuntu/Debian: download the `.deb`, then `sudo apt install ./git-profiles_<ver>_amd64.deb`
