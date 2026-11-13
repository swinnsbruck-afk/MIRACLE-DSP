#!/bin/bash
# Builds "MIRACLE DSP Installer.pkg" and the DMG from the universal release build.
# Usage: ./build_installer.sh [path-to-universal-build-dir]
set -euo pipefail

VERSION="1.3.1"
HERE="$(cd "$(dirname "$0")" && pwd)"
PROJ="$(cd "$HERE/.." && pwd)"
BUILD="${1:-$HOME/Developer/MIRACLE-DSP-build-universal}"
ART="$BUILD/MiracleDSP_artefacts/Release"
OUT="$HERE/MIRACLE DSP $VERSION"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

[ -f "$PROJ/LICENSE" ] || { echo "Missing licence file: $PROJ/LICENSE" >&2; exit 1; }

for f in "$ART/AU/MIRACLE DSP.component" "$ART/VST3/MIRACLE DSP.vst3" "$ART/Standalone/MIRACLE DSP.app"; do
    [ -e "$f" ] || { echo "Missing build output: $f" >&2; exit 1; }
done

# Payload laid out like the target file system
ROOT="$WORK/root"
mkdir -p "$ROOT/Library/Audio/Plug-Ins/Components" "$ROOT/Library/Audio/Plug-Ins/VST3" "$ROOT/Applications"
ditto "$ART/AU/MIRACLE DSP.component" "$ROOT/Library/Audio/Plug-Ins/Components/MIRACLE DSP.component"
ditto "$ART/VST3/MIRACLE DSP.vst3"    "$ROOT/Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3"
ditto "$ART/Standalone/MIRACLE DSP.app" "$ROOT/Applications/MIRACLE DSP.app"

# Keep bundles at their install location even if a copy exists elsewhere
pkgbuild --analyze --root "$ROOT" "$WORK/components.plist" >/dev/null
for i in $(seq 0 10); do
    /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$WORK/components.plist" 2>/dev/null || break
done

mkdir -p "$WORK/scripts"
cat > "$WORK/scripts/postinstall" <<'EOF'
#!/bin/bash
# Remove per-user copies of the same plugin so hosts do not list it twice,
# then refresh the Audio Unit cache.
USER_HOME=$(dscl . -read "/Users/$(stat -f %Su /dev/console)" NFSHomeDirectory 2>/dev/null | awk '{print $2}')
if [ -n "$USER_HOME" ]; then
    rm -rf "$USER_HOME/Library/Audio/Plug-Ins/Components/MIRACLE DSP.component" \
           "$USER_HOME/Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3"
fi
killall -9 AudioComponentRegistrar 2>/dev/null || true
exit 0
EOF
chmod +x "$WORK/scripts/postinstall"

pkgbuild --root "$ROOT" \
         --component-plist "$WORK/components.plist" \
         --scripts "$WORK/scripts" \
         --identifier com.ess76.miracledsp.pkg \
         --version "$VERSION" \
         --install-location / \
         "$WORK/MiracleDSP.pkg"

mkdir -p "$WORK/resources"
cat > "$WORK/resources/welcome.html" <<EOF
<html><body style="font-family:-apple-system,Helvetica;font-size:13px">
<h2>MIRACLE DSP $VERSION</h2>
<p>8-bit sample player. Digital Sample Player, just like 1985.</p>
<p>Installs / Installiert:</p>
<ul>
<li>Audio Unit &rarr; /Library/Audio/Plug-Ins/Components</li>
<li>VST3 &rarr; /Library/Audio/Plug-Ins/VST3</li>
<li>Standalone app &rarr; /Applications</li>
</ul>
<p>Restart your DAW after installing. / Nach der Installation die DAW neu starten.</p>
<p>Copyright &copy; 2026 Stefan Sch&auml;fer (ESS76).<br>
Free software under the GNU Affero General Public License v3 or later (AGPL-3.0-or-later). No warranty.
The source code is included on the disk image (<i>MIRACLE DSP $VERSION Source.zip</i>).<br>
Freie Software unter der GNU AGPL v3 oder sp&auml;ter. Keine Gew&auml;hrleistung. Der Quellcode liegt auf dem Disk-Image bei.</p>
<p style="color:#777;font-size:11px">Not affiliated with Ensoniq or Creative Technology.<br>Nicht mit Ensoniq oder Creative Technology verbunden.</p>
</body></html>
EOF

cat > "$WORK/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>MIRACLE DSP $VERSION</title>
    <welcome file="welcome.html"/>
    <readme file="LICENSE.txt"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <domains enable_localSystem="true"/>
    <os-version min="11.0"/>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default" title="MIRACLE DSP"><pkg-ref id="com.ess76.miracledsp.pkg"/></choice>
    <pkg-ref id="com.ess76.miracledsp.pkg" version="$VERSION">MiracleDSP.pkg</pkg-ref>
</installer-gui-script>
EOF

# Licence text: shown in the installer (read-only page, no "Agree" step) and placed in the release folder
cp "$PROJ/LICENSE" "$WORK/resources/LICENSE.txt"
cp "$PROJ/LICENSE" "$OUT/LICENSE.txt"

# Third-party notices and licence texts for everything compiled into the binaries
cp "$PROJ/THIRD_PARTY_NOTICES.md" "$OUT/THIRD_PARTY_NOTICES.md"
rm -rf "$OUT/LICENSES"
ditto "$PROJ/LICENSES" "$OUT/LICENSES"

# Corresponding source code (AGPL): source, build file, licence and READMEs as a zip next to the installer
SRC="$WORK/src/MIRACLE DSP $VERSION Source"
mkdir -p "$SRC"
ditto "$PROJ/Source" "$SRC/Source"
cp "$PROJ/CMakeLists.txt" "$PROJ/LICENSE" "$PROJ/README.md" "$PROJ/README.en.md" "$PROJ/THIRD_PARTY_NOTICES.md" "$SRC/"
ditto "$PROJ/LICENSES" "$SRC/LICENSES"
mkdir -p "$SRC/Release"
cp "$PROJ/Release/build_installer.sh" "$SRC/Release/"
find "$SRC" -name .DS_Store -delete
rm -f "$OUT/MIRACLE DSP $VERSION Source.zip"
(cd "$WORK/src" && ditto -c -k --norsrc --keepParent "MIRACLE DSP $VERSION Source" "$OUT/MIRACLE DSP $VERSION Source.zip")

productbuild --distribution "$WORK/distribution.xml" \
             --resources "$WORK/resources" \
             --package-path "$WORK" \
             "$OUT/MIRACLE DSP Installer.pkg"

# Disk image with the whole release folder
DMG="$HERE/MIRACLE DSP $VERSION.dmg"
rm -f "$DMG"
find "$OUT" -name .DS_Store -delete
hdiutil create -volname "MIRACLE DSP $VERSION" -srcfolder "$OUT" -ov -format UDZO "$DMG" >/dev/null

echo "Installer: $OUT/MIRACLE DSP Installer.pkg"
echo "Source:    $OUT/MIRACLE DSP $VERSION Source.zip"
echo "DMG:       $DMG"
