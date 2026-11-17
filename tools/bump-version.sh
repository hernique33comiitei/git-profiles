#!/usr/bin/env bash
# Single source of truth for the release version: the CMake project() version.
#
# The app itself already derives its version from CMake (GITPROFILES_VERSION).
# The packaging/metadata files (RPM spec, debian/changelog, AppStream metainfo,
# .po header) are read by external tools and can't import the CMake value, so
# this script propagates it to all of them from one place, and fills their
# changelog entries from the git commits since the last tag.
#
# Usage:
#   tools/bump-version.sh 0.0.2   # set a new version, sync every file, build changelogs
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

# --- Change list: commit subjects since the last tag (newest first). ---
lasttag=$(git describe --tags --abbrev=0 2>/dev/null || true)
range=""; [ -n "$lasttag" ] && range="$lasttag..HEAD"
mapfile -t raw < <(git log --no-merges --pretty='%s' $range 2>/dev/null || true)
changes=()
for c in "${raw[@]}"; do
  case "$c" in
    "chore: release "*|"chore: bump version"*) continue ;;  # skip our own bump commits
  esac
  changes+=("$c")
done
[ ${#changes[@]} -eq 0 ] && changes=("Release $new")

xml_escape() { sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g'; }

# 1) CMake project() — the canonical source.
sed -i -E "s/(project\(gitprofiles VERSION )[0-9]+\.[0-9]+\.[0-9]+/\1$new/" "$CMAKE"

# 2) RPM spec: Version field + a new %changelog entry (only when the version is new).
sed -i -E "s/^(Version:[[:space:]]*)[0-9]+\.[0-9]+\.[0-9]+/\1$new/" "$SPEC"
if ! grep -q "\- $new-1\$" "$SPEC"; then
  frag=$(mktemp)
  { echo "* $rpmdate $AUTHOR - $new-1"
    for c in "${changes[@]}"; do echo "- $c"; done
    echo ""; } > "$frag"
  awk -v f="$frag" '/^%changelog/ && !d { print; while ((getline l < f) > 0) print l; d=1; next } { print }' \
    "$SPEC" > "$SPEC.tmp" && mv "$SPEC.tmp" "$SPEC"
  rm -f "$frag"
fi

# 3) AppStream metainfo: prepend a <release> with a <ul> list (only when the version is new).
if ! grep -q "release version=\"$new\"" "$META"; then
  frag=$(mktemp)
  { echo "    <release version=\"$new\" date=\"$today\">"
    echo "      <description>"
    echo "        <ul>"
    for c in "${changes[@]}"; do echo "          <li>$(printf '%s' "$c" | xml_escape)</li>"; done
    echo "        </ul>"
    echo "      </description>"
    echo "    </release>"; } > "$frag"
  awk -v f="$frag" '/<releases>/ && !d { print; while ((getline l < f) > 0) print l; d=1; next } { print }' \
    "$META" > "$META.tmp" && mv "$META.tmp" "$META"
  rm -f "$frag"
fi

# 4) debian/changelog: prepend a new entry (only when the version is new).
if ! head -1 "$DEB" | grep -q "($new-1)"; then
  frag=$(mktemp)
  { echo "git-profiles ($new-1) unstable; urgency=medium"; echo ""
    for c in "${changes[@]}"; do echo "  * $c"; done
    echo ""; echo " -- $AUTHOR  $debdate"; echo ""; } > "$frag"
  cat "$frag" "$DEB" > "$DEB.tmp" && mv "$DEB.tmp" "$DEB"
  rm -f "$frag"
fi

# 5) .po header (informational).
sed -i -E "s/(Project-Id-Version: gitprofiles )[0-9]+\.[0-9]+\.[0-9]+/\1$new/" "$PO"

echo "Version synced to $new (${#changes[@]} changelog line(s) from ${lasttag:-start}..HEAD):"
echo "  CMakeLists.txt       -> $(grep -oP 'project\(gitprofiles VERSION \K[0-9.]+' "$CMAKE")"
echo "  $SPEC  -> $(grep -oP '^Version:[[:space:]]*\K[0-9.]+' "$SPEC")"
echo "  $DEB     -> $(head -1 "$DEB" | grep -oP '\(\K[0-9.]+')"
echo "  $META -> $(grep -oP 'release version=\"\K[0-9.]+' "$META" | head -1)"
