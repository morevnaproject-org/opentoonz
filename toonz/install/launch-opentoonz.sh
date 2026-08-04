#!/bin/bash

set -e

SCRIPT_DIR=$(dirname "$0")
SCRIPT_DIR=$(cd "$SCRIPT_DIR"; pwd)
BASE_DIR=`dirname "$SCRIPT_DIR"`

BIN_DIR="$BASE_DIR/bin"
LIB_DIR="$BASE_DIR/lib"
CONFIG_DIR="$HOME/.config/OpenToonz"
STUFF_DIR="$CONFIG_DIR/stuff"
STUFF_DIST_DIR="$BASE_DIR/share/opentoonz/stuff"

if [[ "$(uname -s)" = "Darwin"* ]]; then
    BIN_DIR="$BASE_DIR/MacOS"
    LIB_DIR="$BASE_DIR/MacOS"
    STUFF_DIST_DIR="$BASE_DIR/stuff"
else
    export LD_LIBRARY_PATH="$LIB_DIR:$LIB_DIR/opentoonz:$BASE_DIR/lib64:$LD_LIBRARY_PATH"
    export QT_XKB_CONFIG_ROOT=$QT_XKB_CONFIG_ROOT:/usr/local/share/X11/xkb:/usr/share/X11/xkb
fi

# fix stuff
if [ ! -d "$CONFIG_DIR" ]; then
    echo "fix config: copy stuff".
    mkdir -p "$CONFIG_DIR"
    cp -r "$STUFF_DIST_DIR" "$CONFIG_DIR"

    echo "fix config: create SystemVar.ini" 
    cat << EOF > "$CONFIG_DIR/SystemVar.ini"
[General]
OPENTOONZROOT="$STUFF_DIR"
OpenToonzPROFILES="$STUFF_DIR/profiles"
TOONZCACHEROOT="$STUFF_DIR/cache"
TOONZCONFIG="$STUFF_DIR/config"
TOONZFXPRESETS="$STUFF_DIR/fxs"
TOONZLIBRARY="$STUFF_DIR/library"
TOONZPROFILES="$STUFF_DIR/profiles"
TOONZPROJECTS="$STUFF_DIR/projects"
TOONZROOT="$STUFF_DIR"
TOONZSTUDIOPALETTE="$STUFF_DIR/projects/studiopalette"
EOF

else
    # add mypaint brushes if need
    if [ ! -d "$STUFF_DIR/library/mypaint brushes" ]; then
        echo "fix config: copy mypaint brushes" 
        mkdir -p "$STUFF_DIR/library"
        cp -r "$STUFF_DIST_DIR/library/mypaint brushes" "$STUFF_DIR/library/" 
    fi

    # fix paths
    INI="$CONFIG_DIR/SystemVar.ini"
    if [ -e "$INI" ]; then
        [ -e "$INI.bak" ] || cp "$INI" "$INI.bak"

        # fix path to fxs
        FX_PATH_OLD="$STUFF_DIR/projects/fxs"
        FX_PATH_NEW="$STUFF_DIR/fxs"
        FX_LINE_OLD="TOONZFXPRESETS=\"$FX_PATH_OLD\""
        FX_LINE_NEW="TOONZFXPRESETS=\"$FX_PATH_NEW\""
        if [ ! -z "`grep "$FX_LINE_OLD" "$INI"`" ] \
         && ( [ ! -d "$FX_PATH_OLD" ] || [ -z "`ls -A "$FX_PATH_OLD"`" ] ); then
            echo "fix config: fix path to fxs" 
            cat "$INI" \
              | sed "s|$FX_LINE_OLD|$FX_LINE_NEW|g" \
              > "$INI.out"
            cp "$INI.out" "$INI"
            rm -f "$INI.out"
        fi

        # fix path to library
        LIBRARY_PATH_OLD="$STUFF_DIR/projects/library"
        LIBRARY_PATH_NEW="$STUFF_DIR/library"
        LIBRARY_LINE_OLD="TOONZLIBRARY=\"$LIBRARY_PATH_OLD\""
        LIBRARY_LINE_NEW="TOONZLIBRARY=\"$LIBRARY_PATH_NEW\""
        if [ ! -z "`grep "$LIBRARY_LINE_OLD" "$INI"`" ] \
         && ( [ ! -d "$LIBRARY_PATH_OLD" ] || [ -z "`ls -A "$LIBRARY_PATH_OLD"`" ] ); then
            echo "fix config: fix path to library" 
            cat "$INI" \
              | sed "s|$LIBRARY_LINE_OLD|$LIBRARY_LINE_NEW|g" \
              > "$INI.out"
            cp "$INI.out" "$INI"
            rm -f "$INI.out"
        fi
    fi
fi

cd "$BIN_DIR"
./OpenToonz_* "$@"

