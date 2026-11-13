#!/bin/bash
# Removes MIRACLE DSP (plugins, app, settings). Samples and DAW projects are kept.
# Entfernt MIRACLE DSP (Plugins, App, Einstellungen). Samples und DAW-Projekte bleiben erhalten.

echo "MIRACLE DSP - Uninstall / Deinstallation"
echo
echo "These items will be removed / Folgendes wird entfernt:"
echo "  /Library/Audio/Plug-Ins/Components/MIRACLE DSP.component"
echo "  /Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3"
echo "  /Applications/MIRACLE DSP.app"
echo "  ~/Library/Audio/Plug-Ins/Components/MIRACLE DSP.component"
echo "  ~/Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3"
echo "  ~/Library/Application Support/MIRACLE DSP"
echo
read -r -p "Continue? / Fortfahren? (y/j/n) " answer
case "$answer" in
    j|J|y|Y) ;;
    *) echo "Cancelled. / Abgebrochen."; exit 0 ;;
esac

echo "Your Mac password is needed for the system-wide files."
echo "Für die systemweiten Dateien wird dein Mac-Passwort benötigt."
sudo rm -rf "/Library/Audio/Plug-Ins/Components/MIRACLE DSP.component" \
            "/Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3" \
            "/Applications/MIRACLE DSP.app"
sudo pkgutil --forget com.ess76.miracledsp.pkg >/dev/null 2>&1

rm -rf "$HOME/Library/Audio/Plug-Ins/Components/MIRACLE DSP.component" \
       "$HOME/Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3" \
       "$HOME/Library/Application Support/MIRACLE DSP"

killall -9 AudioComponentRegistrar 2>/dev/null

echo
echo "Done. MIRACLE DSP has been removed. / Fertig. MIRACLE DSP wurde entfernt."
read -r -p "Press Enter to close. / Zum Schließen Enter drücken."
