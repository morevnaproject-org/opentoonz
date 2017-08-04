#!/bin/bash

set -e

[ -z "$1" ] && echo "usage: \"$0\" <file>" && exit 1

BASE_FILE="$1"
BASE_NAME=$(basename "$BASE_FILE")
BASE_DIR=$(dirname "$BASE_FILE")
BASE_DIR=$(cd "$BASE_DIR"; pwd)
BASE_FILE="$BASE_DIR/$BASE_NAME"

function process_lib() {
    local OWNER="$1"
    local FILE="$2"
    local SUB_SCAN="$3"
    local FILE_ORIG="$FILE"
    [[ "$FILE" = /System/* ]] && return 0
    [[ "$FILE" = /usr/lib/* ]] && return 0
    [[ "$FILE" = @rpath/Qt* ]] && return 0
    if [[ "$FILE" = @rpath/* ]]; then
        FILE=$BASE_DIR/${FILE:7}
    fi
    if [[ "$FILE" = @loader_path/* ]]; then
        FILE=$BASE_DIR/${FILE:13}
    fi

    # copy
    local FILE_DIR=$(dirname "$FILE")
    local FILE_NAME=$(basename "$FILE")
    if [ ! "$FILE_DIR" = "$BASE_DIR" ]; then
        if [ ! -f "$BASE_DIR/$FILE_NAME" ] || [ "$FILE" -nt "$BASE_DIR/$FILE_NAME" ]; then
            echo "copy $FILE_NAME"
            cp "$FILE" "$BASE_DIR"
        fi
        FILE="$BASE_DIR/$FILE_NAME"
        chmod a+rw "$FILE"
    fi

    if [ -z "$SUB_SCAN" ]; then
        # relink
        local FILE_LINK="@rpath/$FILE_NAME"
        if [ ! "$FILE_LINK" = "$FILE_ORIG" ]; then
            local OWNER_NAME=$(basename "$OWNER")
            echo "relink $FILE_NAME in $OWNER_NAME"
            install_name_tool -change "$FILE_ORIG" "$FILE_LINK" "$OWNER"
        fi
    else
        # scan
        scan "$FILE"
        scan "$FILE" subscan
    fi
}

function scan() {
    local FILE="$1"
    local SUB_SCAN="$2"
    local FIRST=true
    local LINE=
    dyldinfo -dylibs "$1" | while read -r LINE; do
        set -e
        if $FIRST; then
            FIRST=false
        else
            process_lib "$FILE" "$LINE" "$SUB_SCAN"
        fi
    done
    [[ "${PIPESTATUS[@]}" = "0 0" ]]
}

echo ""
echo "gather deps for: $BASE_FILE"
echo ""

scan "$BASE_FILE"
scan "$BASE_FILE" subscan

echo ""
echo "success"
echo ""

