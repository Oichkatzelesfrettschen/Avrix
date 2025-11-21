#!/bin/bash
set -e

# ─── verify_profiles.sh ──────────────────────────────────────────────
#
#  Verifies that the OS compiles correctly under all 3 profiles
#  (Low, Mid, High) and reports binary sizes.
#
# ──────────────────────────────────────────────────────────────────────

GREEN='\033[0;32m'
NC='\033[0m'

function build_profile() {
    PROFILE=$1
    echo -e "\n${GREEN}>>> Building Profile: ${PROFILE} <<<${NC}"

    BUILD_DIR="build_${PROFILE}"
    rm -rf ${BUILD_DIR}

    # Construct arguments manually
    ARGS=""
    # Read file line by line
    while IFS= read -r line; do
        # Trim whitespace
        line=$(echo "$line" | xargs)

        # Skip empty lines
        if [[ -z "$line" ]]; then continue; fi
        # Skip comments
        if [[ "$line" == \#* ]]; then continue; fi
        # Skip section headers [project options]
        if [[ "$line" == \[* ]]; then continue; fi

        # Split by first =
        key=$(echo "$line" | cut -d'=' -f1 | xargs)
        value=$(echo "$line" | cut -d'=' -f2- | xargs)

        # Remove single quotes
        value=$(echo "$value" | tr -d "'")

        ARGS="$ARGS -D${key}=${value}"
    done < config/profiles/${PROFILE}.ini

    echo "Configuring with: $ARGS"

    meson setup ${BUILD_DIR} $ARGS --wipe
    meson compile -C ${BUILD_DIR}

    echo "Build Success!"
}

# Build all 3
build_profile "low"
build_profile "mid"
build_profile "high"

echo -e "\n${GREEN}All profiles verified successfully!${NC}"
