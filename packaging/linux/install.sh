#!/bin/sh
# Installs Audiosphere for the current user (no root needed).
#   ./install.sh            install
#   ./install.sh --uninstall
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
VST3_DIR="$HOME/.vst3"
BIN_DIR="$HOME/.local/bin"
APP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"

if [ "$1" = "--uninstall" ]; then
    rm -rf "$VST3_DIR/Audiosphere.vst3" "$BIN_DIR/Audiosphere" "$APP_DIR/audiosphere.desktop" "$ICON_DIR/audiosphere.png"
    echo "Audiosphere removed."
    exit 0
fi

mkdir -p "$VST3_DIR" "$BIN_DIR" "$APP_DIR" "$ICON_DIR"
rm -rf "$VST3_DIR/Audiosphere.vst3"
cp -R "$HERE/Audiosphere.vst3" "$VST3_DIR/"
install -m 755 "$HERE/Audiosphere" "$BIN_DIR/Audiosphere"
cp "$HERE/audiosphere.png" "$ICON_DIR/audiosphere.png"

cat > "$APP_DIR/audiosphere.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Audiosphere
Comment=Wavetable synthesizer
Exec=$BIN_DIR/Audiosphere
Icon=audiosphere
Categories=AudioVideo;Audio;Music;
EOF

echo "Audiosphere installed:"
echo "  VST3 plug-in    -> $VST3_DIR/Audiosphere.vst3"
echo "  Standalone app  -> $BIN_DIR/Audiosphere"
