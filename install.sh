#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

DIRENV_PIN="2.37.1"

# Dependencies installed together with the app — everything it needs EXCEPT
# git / gh / gt / glab, which stay as on-demand checks inside the app.
DEPS=(
  extra-cmake-modules cmake ninja-build gcc-c++
  qt6-qtbase-devel qt6-qtdeclarative-devel
  kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel
  direnv libsecret
)

missing=()
for pkg in "${DEPS[@]}"; do
  rpm -q "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
done
if [ ${#missing[@]} -gt 0 ]; then
  echo "Installing dependencies (sudo): ${missing[*]}"
  sudo dnf install -y "${missing[@]}"
fi

# Enforce the pinned minimum for direnv.
if command -v direnv >/dev/null 2>&1; then
  have=$(direnv version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
  if [ -n "$have" ] && [ "$(printf '%s\n%s\n' "$DIRENV_PIN" "$have" | sort -V | head -1)" != "$DIRENV_PIN" ]; then
    echo "Warning: direnv $have is older than the pinned minimum $DIRENV_PIN."
  fi
fi

# Activate the direnv shell hook for bash (once).
if ! grep -q 'direnv hook bash' "$HOME/.bashrc" 2>/dev/null; then
  echo 'eval "$(direnv hook bash)"' >> "$HOME/.bashrc"
  echo 'Added the direnv hook to ~/.bashrc — open a new terminal to activate it.'
fi

cmake -B build-user -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build-user
cmake --install build-user

echo "Installed. Launch 'Git Profiles' from your app launcher, or run: gitprofiles"
