#!/usr/bin/env bash
# Generate a sanitized, dark-theme screenshot for the AppStream metainfo.
#
# Uses a throwaway HOME with a FAKE ~/.ssh/config (generic example hosts) so no
# real/private hosts ever end up in a public store screenshot. Runs the app
# headless under Xvfb with the software renderer, the Breeze Dark color scheme
# and a minimal WM (openbox) that maps the window maximized and undecorated,
# then grabs the virtual screen.
#
# Requires: Xvfb + openbox + ImageMagick + the installed `gitprofiles` binary.
#   Fedora:  sudo dnf install -y xorg-x11-server-Xvfb openbox ImageMagick
#
# Usage:  data/screenshots/capture.sh
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)

for c in gitprofiles xvfb-run openbox import; do
  command -v "$c" >/dev/null || { echo "missing: $c"; exit 1; }
done

fake=$(mktemp -d)
trap 'rm -rf "$fake"' EXIT
mkdir -p "$fake/.ssh" "$fake/.config/openbox"
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

# Breeze Dark (KColorScheme reads these groups from kdeglobals).
cp /usr/share/color-schemes/BreezeDark.colors "$fake/.config/kdeglobals"
printf '\n[General]\nColorScheme=BreezeDark\n' >> "$fake/.config/kdeglobals"

# openbox: map every window maximized and without decorations.
cat > "$fake/.config/openbox/rc.xml" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<openbox_config xmlns="http://openbox.org/3.4/rc">
  <applications>
    <application class="*">
      <decor>no</decor>
      <maximized>true</maximized>
    </application>
  </applications>
</openbox_config>
EOF

export HOME="$fake"
unset WAYLAND_DISPLAY                      # else Qt talks to the real compositor
export QT_QPA_PLATFORM=xcb                 # render into Xvfb, not the host session
export LIBGL_ALWAYS_SOFTWARE=1
export QT_QUICK_BACKEND=software           # no GPU under Xvfb
export QT_QUICK_CONTROLS_STYLE=org.kde.desktop

xvfb-run -a -s "-screen 0 1280x800x24" bash -c '
  openbox >/dev/null 2>&1 & wm=$!
  sleep 1
  gitprofiles >/dev/null 2>&1 & app=$!
  sleep 7
  import -window root "'"$here"'/01-profiles.png"
  kill "$app" "$wm" 2>/dev/null || true
'
echo "Wrote $here/01-profiles.png"
identify "$here/01-profiles.png"
