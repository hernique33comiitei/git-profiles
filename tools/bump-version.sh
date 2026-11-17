#!/usr/bin/env bash
# Single source of truth for the release version: the CMake project() version.
#
# The app itself already derives its version from CMake (GITPROFILES_VERSION).
# The packaging/metadata files (RPM spec, debian/changelog, AppStream metainfo,
# .po header) are read by external tools and can't import the CMake value, so
# this script propagates it to all of them from one place.
#
# Usage:
#   tools/bump-version.sh 0.0.2   # set a new version and sync every file
#   tools/bump-version.sh         # re-sync every file to the current CMake version
#
# After bumping: review the diff, commit, then tag (git tag vX.Y.Z && git push --tags).
set -euo pipefail
cd "$(dirname "$0")/.."

CMAKE=CMakeLists.txt
SPEC=packaging/fedora/git-profiles.spec
META=data/io.github.hernique33comiitei.GitProfiles.metainfo.xml
DEB=debian/changelog
PO=po/pt_BR.po
AUTHOR="Henrique Oliveira <henriik.dev@gmail.com>"

cur=$(grep -oP 'project\(gitprofiles VERSION \K[0-9]+\.[0-9]+\.[0-9]+' "$CMAKE")
new="${1:-$cur}"
if ! [[ "$new" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "usage: $0 [X.Y.Z]" >&2; exit 1
fi

today=$(date +%Y-%m-%d)
rpmdate=$(LC_ALL=C date '+%a %b %d %Y')
debdate=$(LC_ALL=C date -R)

# 1) CMake project() — the canonical source.
sed -i -E "s/(project\(gitprofiles VERSION )[0-9]+\.[0-9]+\.[0-9]+/\1$new/" "$CMAKE"

# 2) RPM spec: Version field, and a new %changelog entry (only when the version changes).
sed -i -E "s/^(Version:[[:space:]]*)[0-9]+\.[0-9]+\.[0-9]+/\1$new/" "$SPEC"
if ! grep -q "\- $new-1\$" "$SPEC"; then
  awk -v v="$new" -v d="$rpmdate" -v a="$AUTHOR" '
    /^%changelog/ && !done { print; print "* " d " " a " - " v "-1"; print "- Release " v; print ""; done=1; next }
    { print }' "$SPEC" > "$SPEC.tmp" && mv "$SPEC.tmp" "$SPEC"
fi

# 3) AppStream metainfo: prepend a <release> (only when the version is new).
if ! grep -q "release version=\"$new\"" "$META"; then
  sed -i -E "s|([[:space:]]*)<releases>|\1<releases>\n\1  <release version=\"$new\" date=\"$today\">\n\1    <description>\n\1      <p>Release $new.</p>\n\1    </description>\n\1  </release>|" "$META"
fi

# 4) debian/changelog: prepend a new entry (only when the version is new).
if ! head -1 "$DEB" | grep -q "($new-1)"; then
  { printf 'git-profiles (%s-1) unstable; urgency=medium\n\n  * Release %s.\n\n -- %s  %s\n\n' \
      "$new" "$new" "$AUTHOR" "$debdate"; cat "$DEB"; } > "$DEB.tmp" && mv "$DEB.tmp" "$DEB"
fi

# 5) .po header (informational).
sed -i -E "s/(Project-Id-Version: gitprofiles )[0-9]+\.[0-9]+\.[0-9]+/\1$new/" "$PO"

echo "Version synced to $new:"
echo "  CMakeLists.txt       -> $(grep -oP 'project\(gitprofiles VERSION \K[0-9.]+' "$CMAKE")"
echo "  $SPEC  -> $(grep -oP '^Version:[[:space:]]*\K[0-9.]+' "$SPEC")"
echo "  $DEB     -> $(head -1 "$DEB" | grep -oP '\(\K[0-9.]+')"
echo "  $META -> $(grep -oP 'release version=\"\K[0-9.]+' "$META" | head -1)"
