#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="${repo_dir}/build/release"
tools_dir="${repo_dir}/build/appimage-tools"
appdir="${repo_dir}/build/ZealRN.AppDir"
dist_dir="${repo_dir}/dist"

linuxdeploy_url="https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage"
appimage_plugin_url="https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-appimage-x86_64.AppImage"
qt_plugin_url="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/1-alpha-20250213-1/linuxdeploy-plugin-qt-x86_64.AppImage"

download() {
    local url="$1"
    local output="$2"
    if [[ ! -x "${output}" ]]; then
        curl -fL "${url}" -o "${output}"
        chmod +x "${output}"
    fi
}

cmake --preset release -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja -D CMAKE_INSTALL_PREFIX=/usr \
    -D ZEAL_FEATURE_UPDATE_CHECK=OFF
cmake --build --preset release

mkdir -p "${tools_dir}" "${dist_dir}"
download "${linuxdeploy_url}" "${tools_dir}/linuxdeploy-x86_64.AppImage"
download "${appimage_plugin_url}" "${tools_dir}/linuxdeploy-plugin-appimage-x86_64.AppImage"
download "${qt_plugin_url}" "${tools_dir}/linuxdeploy-plugin-qt-x86_64.AppImage"

cmake -E rm -rf "${appdir}"
DESTDIR="${appdir}" cmake --install "${build_dir}"
appstreamcli validate --no-net "${appdir}/usr/share/metainfo/io.github.abnzrdev.zealrn.appdata.xml"

mkdir -p "${appdir}/apprun-hooks"
printf '%s\n' 'export QTWEBENGINE_DISABLE_SANDBOX=1' > "${appdir}/apprun-hooks/qtwebengine.sh"

export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE="$(command -v qmake6)"
export EXTRA_QT_PLUGINS="sqldrivers"
output="${dist_dir}/ZealRN-0.1.0-x86_64.AppImage"
export LDAI_OUTPUT="${output}"
export PATH="${tools_dir}:${PATH}"
"${tools_dir}/linuxdeploy-x86_64.AppImage" \
    --appdir "${appdir}" \
    --desktop-file "${appdir}/usr/share/applications/io.github.abnzrdev.zealrn.desktop" \
    --icon-file "${appdir}/usr/share/icons/hicolor/scalable/apps/zealrn.svg" \
    --plugin qt \
    --output appimage

for required in libqsqlite.so QtWebEngineProcess qtwebengine_resources.pak; do
    if ! find "${appdir}" -name "${required}" -print -quit | grep -q .; then
        echo "AppImage staging is missing ${required}." >&2
        exit 1
    fi
done

chmod +x "${output}"
echo "Created ${output}"
