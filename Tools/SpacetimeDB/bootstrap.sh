#!/usr/bin/env bash
set -euo pipefail

readonly SPACETIMEDB_VERSION="2.7.1"
readonly SPACETIMEDB_TAG="v${SPACETIMEDB_VERSION}"
readonly UPSTREAM_REPOSITORY="https://github.com/clockworklabs/SpacetimeDB.git"

SCRIPT_DIRECTORY="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIRECTORY}/../.." && pwd)"
MODULE_DIRECTORY="${PROJECT_ROOT}/spacetimedb"
PLUGIN_DIRECTORY="${PROJECT_ROOT}/Plugins/SpacetimeDbSdk"
VERSION_MARKER="${PLUGIN_DIRECTORY}/.aetherfront-version"

command -v git >/dev/null 2>&1 || {
    echo "git is required to fetch the pinned Unreal SDK." >&2
    exit 1
}
command -v spacetime >/dev/null 2>&1 || {
    echo "SpacetimeDB CLI ${SPACETIMEDB_VERSION} is required: https://spacetimedb.com/install" >&2
    exit 1
}

if [[ -d "${PLUGIN_DIRECTORY}" ]]; then
    if [[ ! -f "${VERSION_MARKER}" ]] || [[ "$(<"${VERSION_MARKER}")" != "${SPACETIMEDB_VERSION}" ]]; then
        echo "${PLUGIN_DIRECTORY} exists but is not the pinned ${SPACETIMEDB_VERSION} SDK." >&2
        echo "Move that directory aside and rerun this script." >&2
        exit 1
    fi
    echo "SpacetimeDB Unreal SDK ${SPACETIMEDB_VERSION} is already installed."
else
    TEMP_DIRECTORY="$(mktemp -d "${TMPDIR:-/tmp}/aetherfront-spacetimedb.XXXXXX")"
    cleanup() {
        if [[ -n "${TEMP_DIRECTORY:-}" && -d "${TEMP_DIRECTORY}" ]]; then
            rm -rf -- "${TEMP_DIRECTORY}"
        fi
    }
    trap cleanup EXIT

    git clone \
        --depth 1 \
        --branch "${SPACETIMEDB_TAG}" \
        --filter=blob:none \
        --sparse \
        "${UPSTREAM_REPOSITORY}" \
        "${TEMP_DIRECTORY}/upstream"
    git -C "${TEMP_DIRECTORY}/upstream" sparse-checkout set sdks/unreal/src/SpacetimeDbSdk

    mkdir -p "${PROJECT_ROOT}/Plugins"
    cp -R \
        "${TEMP_DIRECTORY}/upstream/sdks/unreal/src/SpacetimeDbSdk" \
        "${PLUGIN_DIRECTORY}"
    printf '%s\n' "${SPACETIMEDB_VERSION}" >"${VERSION_MARKER}"
    echo "Installed the pinned SpacetimeDB Unreal SDK ${SPACETIMEDB_VERSION}."
fi

(
    cd "${MODULE_DIRECTORY}"
    spacetime build
)

spacetime generate \
    --lang unrealcpp \
    --uproject-dir "${PROJECT_ROOT}" \
    --module-path "${MODULE_DIRECTORY}" \
    --unreal-module-name Aetherfront

echo "SpacetimeDB module built and Unreal bindings generated."
