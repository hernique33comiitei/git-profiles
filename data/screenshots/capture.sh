#!/usr/bin/env bash
# Generate sanitized screenshots for the AppStream metainfo.
#
# Uses a throwaway HOME with a FAKE ~/.ssh/config (generic example hosts) so no
# real/private hosts ever end up in a public store screenshot. Runs the app
# headless under Xvfb and grabs the virtual screen.
#
# Requires: Xvfb + ImageMagick + the installed `gitprofiles` binary.
#   Fedora:  sudo dnf install -y xorg-x11-server-Xvfb ImageMagick
#
# Usage:  data/screenshots/capture.sh
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)

command -v gitprofiles >/dev/null || { echo "gitprofiles not installed (run ./install.sh)"; exit 1; }
command -v xvfb-run    >/dev/null || { echo "xvfb-run missing (dnf install xorg-x11-server-Xvfb)"; exit 1; }
command -v import      >/dev/null || { echo "import missing (dnf install ImageMagick)"; exit 1; }

fake=$(mktemp -d)
trap 'rm -rf "$fake"' EXIT
mkdir -p "$fake/.ssh"
cat > "$fake/.ssh/config" <<'EOF'
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519
    #@gp provider=github

Host gitlab-work
    HostName gitlab.com
    User git
    IdentityFile ~/.ssh/id_gitlab
    #@gp provider=gitlab

Host gitea-selfhosted
    HostName git.example.com
    User git
    IdentityFile ~/.ssh/id_gitea
    #@gp provider=gitea
EOF
touch "$fake/.ssh/id_ed25519" "$fake/.ssh/id_gitlab" "$fake/.ssh/id_gitea"
chmod 600 "$fake/.ssh/config" "$fake/.ssh/id_"*

export HOME="$fake"
xvfb-run -a -s "-screen 0 1100x740x24" bash -c '
  gitprofiles >/dev/null 2>&1 & app=$!
  sleep 5
  import -window root "'"$here"'/01-profiles.png"
  kill "$app" 2>/dev/null || true
'
echo "Wrote $here/01-profiles.png"
