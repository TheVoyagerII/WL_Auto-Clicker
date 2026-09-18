#!/bin/bash

set -e
APPDIR="build/WL_Auto-Clicker.AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"

gcc running_program.c -o "$APPDIR/prog"
cp AppRun "$APPDIR/"
cp user_prompt.sh "$APPDIR/"
cp WL_Auto-Clicker.desktop "$APPDIR/"
cp WL_Auto-Clicker_Icon.png "$APPDIR/"
chmod +x "$APPDIR/AppRun"

appimagetool "$APPDIR" build/WL_Auto-Clicker-86_64.AppImage