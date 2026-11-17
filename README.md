# Git Profiles

A Kirigami desktop app to organize your git repositories by SSH profile. It uses
`~/.ssh/config` as its database: each `Host` block is a profile. Attaching a
profile to directories rewrites the `git remote` of the repositories there to the
profile's identity, and sets up the provider CLI (`gh` / `gt` / `glab`) per
directory via `direnv`.

Same graphical stack as `kde-headphones`: Qt6 + QML + Kirigami, built with
CMake/ECM and installed to `~/.local`.

## Install

```sh
./install.sh
```

`install.sh` installs every dependency the app needs and builds it (Fedora). It
only asks for `sudo` the first time, to install missing packages:

- build: `extra-cmake-modules`, `cmake`, `ninja-build`, `gcc-c++`,
  `qt6-qtbase-devel`, `qt6-qtdeclarative-devel`, `kf6-kirigami-devel`,
  `kf6-ki18n-devel`, `kf6-kcoreaddons-devel`
- runtime: **direnv ≥ 2.37.1** (pinned; the shell hook is added to `~/.bashrc`)
  and **libsecret** (`secret-tool`, for the keyring)

## Checked on demand, never installed (the app only warns)

- **git** — required. <https://git-scm.com/downloads>
- **gh ≥ 2.104.0** — only for GitHub profiles
- **gt ≥ 1.2.0** — only for Gitea profiles
- **glab ≥ 1.130.0** — only for GitLab profiles

Status: v0.0.1 — all phases implemented.
