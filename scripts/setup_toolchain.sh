#!/usr/bin/env bash
#
# Puts the pinned cross compiler and SDK in place (Pico spec KE1, KD6).
#
# WHAT THIS DOES
#
#   1. Downloads the Arm GNU Toolchain named in scripts/toolchain_versions.env
#      for this host (Linux x86_64 or Windows x86_64 under Git Bash) into
#      .toolchain/, verifies Arm's published sha256, and unpacks it. A second
#      run with the toolchain already present does nothing.
#   2. Finds pico-sdk: PICO_SDK_PATH if set, else a checkout beside this
#      repository at ../pico-sdk, else clones it into .toolchain/pico-sdk at
#      the pinned tag with submodules. Whichever is used, its commit must be
#      the pinned one, or the script fails and says so.
#   3. Confirms cmake and ninja are on PATH. It does not install them: they are
#      ordinary packages (scoop on Windows, apt on Linux) and the versions the
#      SDK accepts are wide (cmake 3.13 and later).
#
# WHAT THIS DOES NOT DO
#
#   It never writes outside this repository's .toolchain/ directory, never
#   touches PATH or any profile, and never runs anything it downloaded beyond
#   asking the compiler for its version. scripts/build_firmware.sh points the
#   build at the results; nothing else needs to know where they are.
#
# Usage:
#   scripts/setup_toolchain.sh
set -euo pipefail

repositoryRoot="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
toolchainRoot="${repositoryRoot}/.toolchain"
# shellcheck source=toolchain_versions.env
source "${repositoryRoot}/scripts/toolchain_versions.env"

mkdir -p "${toolchainRoot}"

case "$(uname -s)" in
Linux)
	archiveName="${ARM_TOOLCHAIN_LINUX_X86_64_ARCHIVE}"
	archiveSha256="${ARM_TOOLCHAIN_LINUX_X86_64_SHA256}"
	;;
MINGW* | MSYS* | CYGWIN*)
	archiveName="${ARM_TOOLCHAIN_WINDOWS_X86_64_ARCHIVE}"
	archiveSha256="${ARM_TOOLCHAIN_WINDOWS_X86_64_SHA256}"
	;;
*)
	echo "Unsupported host $(uname -s): only Linux x86_64 and Windows x86_64 archives are pinned." >&2
	exit 1
	;;
esac
if [[ "$(uname -m)" != "x86_64" ]]; then
	echo "Unsupported host architecture $(uname -m): only x86_64 archives are pinned." >&2
	exit 1
fi

# The archive unpacks to a directory named after itself without the extension.
toolchainDirectory="${toolchainRoot}/${archiveName%%.tar.xz}"
toolchainDirectory="${toolchainDirectory%%.zip}"

if [[ -x "${toolchainDirectory}/bin/arm-none-eabi-gcc" || -x "${toolchainDirectory}/bin/arm-none-eabi-gcc.exe" ]]; then
	echo "Toolchain already present: ${toolchainDirectory}"
else
	archivePath="${toolchainRoot}/${archiveName}"
	if [[ ! -f "${archivePath}" ]]; then
		echo "Downloading ${archiveName} from ${ARM_TOOLCHAIN_BASE_URL}"
		curl --fail --location --progress-bar --output "${archivePath}" "${ARM_TOOLCHAIN_BASE_URL}/${archiveName}"
	fi
	echo "Verifying sha256"
	echo "${archiveSha256}  ${archivePath}" | sha256sum --check --strict
	# The two archives are laid out differently: the Windows zip has no top
	# level directory (observed 2026-09-15, it unpacks flat), and whether the
	# Linux tarball has one is not assumed either. Unpack into a staging
	# directory and normalise to one named directory whichever way it comes.
	stagingDirectory="${toolchainDirectory}.unpacking"
	rm -rf "${stagingDirectory}" "${toolchainDirectory}"
	mkdir -p "${stagingDirectory}"
	echo "Unpacking into ${toolchainDirectory}"
	case "${archiveName}" in
	*.tar.xz) tar --extract --xz --file "${archivePath}" --directory "${stagingDirectory}" ;;
	*.zip) unzip -q -o "${archivePath}" -d "${stagingDirectory}" ;;
	esac
	if [[ -d "${stagingDirectory}/bin" ]]; then
		mv "${stagingDirectory}" "${toolchainDirectory}"
	else
		singleEntry="$(find "${stagingDirectory}" -mindepth 1 -maxdepth 1 | head -n 1)"
		if [[ -z "${singleEntry}" || ! -d "${singleEntry}/bin" ]]; then
			echo "The archive did not unpack to a directory with bin/ at its top or one level down; look in ${stagingDirectory}." >&2
			exit 1
		fi
		mv "${singleEntry}" "${toolchainDirectory}"
		rmdir "${stagingDirectory}"
	fi
	rm -f "${archivePath}"
fi
"${toolchainDirectory}/bin/arm-none-eabi-gcc" --version | head -n 1

if [[ -n "${PICO_SDK_PATH:-}" ]]; then
	sdkPath="${PICO_SDK_PATH}"
elif [[ -d "${repositoryRoot}/../pico-sdk/.git" ]]; then
	sdkPath="$(cd "${repositoryRoot}/../pico-sdk" && pwd)"
elif [[ -d "${toolchainRoot}/pico-sdk/.git" ]]; then
	sdkPath="${toolchainRoot}/pico-sdk"
else
	echo "Cloning pico-sdk ${PICO_SDK_TAG} into ${toolchainRoot}/pico-sdk"
	git clone --quiet --depth 1 --branch "${PICO_SDK_TAG}" --recurse-submodules \
		https://github.com/raspberrypi/pico-sdk.git "${toolchainRoot}/pico-sdk"
	sdkPath="${toolchainRoot}/pico-sdk"
fi

sdkCommit="$(git -C "${sdkPath}" rev-parse HEAD)"
if [[ "${sdkCommit}" != "${PICO_SDK_COMMIT}" ]]; then
	echo "pico-sdk at ${sdkPath} is at ${sdkCommit}, not the pinned ${PICO_SDK_COMMIT} (tag ${PICO_SDK_TAG})." >&2
	echo "Check out the pinned tag there, or unset PICO_SDK_PATH and remove ../pico-sdk to let this script clone it." >&2
	exit 1
fi
if [[ ! -f "${sdkPath}/lib/tinyusb/src/tusb.h" ]]; then
	echo "pico-sdk at ${sdkPath} has no tinyusb submodule; run: git -C ${sdkPath} submodule update --init" >&2
	exit 1
fi
echo "pico-sdk ${PICO_SDK_TAG} at ${sdkPath}"

for tool in cmake ninja; do
	if ! command -v "${tool}" >/dev/null 2>&1; then
		echo "${tool} is not on PATH. Install it (scoop install ${tool} on Windows, apt install ${tool}-build or cmake on Linux)." >&2
		exit 1
	fi
done
cmake --version | head -n 1
echo "ninja $(ninja --version)"

# Recorded for scripts/build_firmware.sh, which sources it.
cat >"${toolchainRoot}/paths.env" <<EOF
PICO_TOOLCHAIN_PATH=${toolchainDirectory}
PICO_SDK_PATH=${sdkPath}
EOF
echo "Wrote ${toolchainRoot}/paths.env"
