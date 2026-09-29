#!/bin/bash

#
# This is a section of the main build_rive.sh script that downloads, builds, and installs premake
# We need this separate from the main script because on Linux, for unreal, we need to build with
# the Unreal C++ toolchain. But we can't build the premake binary with this chain, because it's lacking
# the correct libraries. So we build this externally, then pass the sysroot parameters to the main
# build_rive.sh script

set -e
set -o pipefail

mkdir -p "$SCRIPT_DIR/dependencies"
pushd "$SCRIPT_DIR/dependencies" > /dev/null

# Add premake5 to the $PATH.
# Install premake5 to a specific directory based on our current tag, to make
# sure we rebuild if this script runs for a different tag.
RIVE_PREMAKE_TAG="${RIVE_PREMAKE_TAG:-v5.0.0-beta7}"
PREMAKE_INSTALL_DIR="$SCRIPT_DIR/dependencies/premake-core/bin/${RIVE_PREMAKE_TAG}_release"
if [ ! -f "$PREMAKE_INSTALL_DIR/premake5" ] && [ ! -f "$PREMAKE_INSTALL_DIR/premake5.exe" ]; then
    if [[ $HOST_MACHINE == "windows" ]]; then
        echo Downloading prebuilt Premake...
        PREMAKE_VERSION="${RIVE_PREMAKE_TAG#v}"
        PREMAKE_ZIP="premake-${PREMAKE_VERSION}-windows.zip"
        mkdir -p "$PREMAKE_INSTALL_DIR"
        curl -fL -o "$PREMAKE_ZIP" \
            "https://github.com/premake/premake-core/releases/download/${RIVE_PREMAKE_TAG}/${PREMAKE_ZIP}"
        unzip -o "$PREMAKE_ZIP" -d "$PREMAKE_INSTALL_DIR"
        rm "$PREMAKE_ZIP"
    else
        echo Building Premake...
        rm -fr premake-core # Wipe out a prior checkout if it exists without a premake5 binary.
        git clone --depth 1 --branch $RIVE_PREMAKE_TAG https://github.com/premake/premake-core.git
        pushd premake-core > /dev/null
        case "$HOST_MACHINE" in
            mac_arm64) make -f Bootstrap.mak osx PLATFORM=ARM ;;
            mac_x64) make -f Bootstrap.mak osx ;;
            *) make -f Bootstrap.mak linux ;;
        esac
        mkdir -p "$PREMAKE_INSTALL_DIR"
        cp -r bin/release/* $PREMAKE_INSTALL_DIR
        popd > /dev/null
    fi
fi
export PATH="$PREMAKE_INSTALL_DIR:$PATH"
