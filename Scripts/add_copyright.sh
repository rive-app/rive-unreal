sed#!/bin/bash

# Usage: ./add_copyright.sh [optional-starting-directory]
ROOT_DIR="${1:-.}"

OLD_NOTICE="// Copyright Rive, Inc. All rights reserved."
NEW_NOTICE="// Copyright 2024, 2025 Rive, Inc. All rights reserved."
UNREAL_PLACEHOLDER="// Fill out your copyright notice in the Description page of Project Settings."

EXTENSIONS=("*.cpp" "*.h" "*.hpp" "*.c" "*.cc" "*.cs")

for EXT in "${EXTENSIONS[@]}"; do
    find "$ROOT_DIR" -type f -name "$EXT" \
        -not -path "*/generated/*" \
        -not -path "*/ThirdParty/*" | while read -r FILE; do

        MODIFIED=false

        # --- Remove Unreal placeholder if present ---
        if grep -Fq "$UNREAL_PLACEHOLDER" "$FILE"; then
            echo "Removing Unreal placeholder from $FILE"
            sed -i.bak "\|$UNREAL_PLACEHOLDER|d" "$FILE" && rm "$FILE.bak"
            MODIFIED=true
        fi

        # --- Copyright Notice Logic ---
        if grep -Fq "$NEW_NOTICE" "$FILE"; then
            echo "Notice already up-to-date in $FILE"

            LINE_AFTER_NOTICE=$(grep -nF "$NEW_NOTICE" "$FILE" | cut -d: -f1 | head -n1)
            NEXT_LINE=$((LINE_AFTER_NOTICE + 1))
            if ! sed -n "${NEXT_LINE}p" "$FILE" | grep -q '^[[:space:]]*$'; then
                echo "Inserting blank line after notice in $FILE"
                sed -i.bak "${LINE_AFTER_NOTICE}a\\"$'\n' "$FILE" && rm "$FILE.bak"
                MODIFIED=true
            fi

        elif grep -Fq "$OLD_NOTICE" "$FILE"; then
            echo "Updating notice in $FILE"
            sed -i.bak "s|$OLD_NOTICE|$NEW_NOTICE|" "$FILE" && rm "$FILE.bak"
            MODIFIED=true

            LINE_AFTER_NOTICE=$(grep -nF "$NEW_NOTICE" "$FILE" | cut -d: -f1 | head -n1)
            NEXT_LINE=$((LINE_AFTER_NOTICE + 1))
            if ! sed -n "${NEXT_LINE}p" "$FILE" | grep -q '^[[:space:]]*$'; then
                echo "Inserting blank line after updated notice in $FILE"
                sed -i.bak "${LINE_AFTER_NOTICE}a\\"$'\n' "$FILE" && rm "$FILE.bak"
                MODIFIED=true
            fi

        else
            echo "Adding notice to $FILE"
            TMP_FILE=$(mktemp)
            {
                echo "$NEW_NOTICE"
                echo ""
                sed '1s/^\xEF\xBB\xBF//' "$FILE"
            } > "$TMP_FILE"
            mv "$TMP_FILE" "$FILE"
            MODIFIED=true
        fi

        # --- BOM + Invisible Character Cleanup ---
        if xxd -p -l3 "$FILE" | grep -iq "^efbbbf"; then
            echo "BOM detected in $FILE"
        fi

        TMP_CLEANED=$(mktemp)
        if iconv -f UTF-8 -t UTF-8 "$FILE" > "$TMP_CLEANED"; then
            # Clean garbage before first preprocessor directive
            sed -i '1s/^[^[:print:]]*#/#/' "$TMP_CLEANED"
            if ! cmp -s "$FILE" "$TMP_CLEANED"; then
                echo "Stripping BOM or invisible characters in $FILE"
                mv "$TMP_CLEANED" "$FILE"
            else
                rm "$TMP_CLEANED"
            fi
        else
            echo "Skipped: $FILE contains invalid byte sequences"
            rm -f "$TMP_CLEANED"
        fi

    done
done

echo "Done."
