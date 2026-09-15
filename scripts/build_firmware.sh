#!/usr/bin/env bash
#
# Builds the firmware against the pinned SDK and compiler and puts the .uf2
# files in dist/. Run scripts/setup_toolchain.sh first; this script reads the
# paths it wrote and nothing else, so the build is the same on the
# development machine and in continuous integration.
#
# Usage:
#   scripts/build_firmware.sh
set -euo pipefail

repositoryRoot="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pathsFile="${repositoryRoot}/.toolchain/paths.env"
if [[ ! -f "${pathsFile}" ]]; then
	echo "${pathsFile} is missing: run scripts/setup_toolchain.sh first." >&2
	exit 1
fi
# shellcheck source=/dev/null
source "${pathsFile}"
export PICO_TOOLCHAIN_PATH PICO_SDK_PATH

buildDirectory="${repositoryRoot}/build/firmware"
distDirectory="${repositoryRoot}/dist"

cmake -S "${repositoryRoot}/firmware" -B "${buildDirectory}" -G Ninja -DPICO_BOARD=pico2_w
cmake --build "${buildDirectory}"

mkdir -p "${distDirectory}"
cp "${buildDirectory}"/*.uf2 "${distDirectory}/"
echo
echo "Firmware images:"
ls -l "${distDirectory}"/*.uf2
