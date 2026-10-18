#!/bin/bash
# Builds the macOS installer package.
#
#   packaging/macos/build-pkg.sh <version> <artefacts dir> <output .pkg>
#
# The installer puts the VST3 and AU plug-ins in /Library/Audio/Plug-Ins and the
# standalone app in /Applications. Files installed by a package do not carry the
# download quarantine flag, so the plug-ins load in the DAW straight away.

set -euo pipefail

VERSION="$1"
ARTEFACTS="$2"
OUTPUT="$3"

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

ID_BASE="com.audiosphere.synth"

# Builds one component package. Bundles are marked non-relocatable so the
# installer always writes to the intended folder instead of "upgrading" a copy
# of the bundle it finds somewhere else on disk.
component () {
    local name="$1" source="$2" location="$3" scripts="${4:-}"
    local root="$WORK/root-$name"
    mkdir -p "$root"
    cp -R "$source" "$root/"

    pkgbuild --analyze --root "$root" "$WORK/$name.plist" > /dev/null
    local i=0
    while /usr/libexec/PlistBuddy -c "Print :$i" "$WORK/$name.plist" > /dev/null 2>&1; do
        /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$WORK/$name.plist" > /dev/null 2>&1 \
            || /usr/libexec/PlistBuddy -c "Add :$i:BundleIsRelocatable bool false" "$WORK/$name.plist"
        i=$((i + 1))
    done

    local args=(--root "$root" --component-plist "$WORK/$name.plist" --identifier "$ID_BASE.$name"
                --version "$VERSION" --install-location "$location")
    if [ -n "$scripts" ]; then
        args+=(--scripts "$scripts")
    fi
    pkgbuild "${args[@]}" "$WORK/Audiosphere-$name.pkg"
}

component vst3 "$ARTEFACTS/VST3/Audiosphere.vst3" "/Library/Audio/Plug-Ins/VST3"
component au "$ARTEFACTS/AU/Audiosphere.component" "/Library/Audio/Plug-Ins/Components" "$HERE/scripts"
component app "$ARTEFACTS/Standalone/Audiosphere.app" "/Applications"

# Installer pages
RES="$WORK/resources"
mkdir -p "$RES"
cp "$HERE/resources/"* "$RES/"
cp "$REPO/LICENSE" "$RES/LICENSE.txt"
sed "s/@VERSION@/$VERSION/g" "$HERE/resources/welcome.html" > "$RES/welcome.html"
sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"

productbuild --distribution "$WORK/distribution.xml" --resources "$RES" --package-path "$WORK" "$OUTPUT"
echo "Built $OUTPUT"
