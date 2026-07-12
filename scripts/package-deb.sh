#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="${repo_dir}/build/release"
dist_dir="${repo_dir}/dist"

cmake --preset release -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja -D CMAKE_INSTALL_PREFIX=/usr \
    -D ZEAL_FEATURE_UPDATE_CHECK=OFF
cmake --build --preset release
mkdir -p "${dist_dir}"

(cd "${build_dir}" && cpack -G DEB)
install -m 0644 "${build_dir}/zealrn_0.1.0-alpha_amd64.deb" "${dist_dir}/zealrn_0.1.0-alpha_amd64.deb"
echo "Created ${dist_dir}/zealrn_0.1.0-alpha_amd64.deb"
