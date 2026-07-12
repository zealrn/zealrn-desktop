#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "${script_dir}/.." && pwd)"

image="${ZEALRN_DOCKER_IMAGE:-zealrn-ubuntu24-qt:latest}"
container_name="${ZEALRN_PREVIEW_CONTAINER:-zealrn-preview}"
app_binary="${ZEALRN_PREVIEW_BINARY:-./build/release/zeal}"
preview_dir="${repo_dir}/.zealrn-preview"

flatpak_docsets="${HOME}/.var/app/org.zealdocs.Zeal/data/Zeal/Zeal/docsets"
native_docsets="${HOME}/.local/share/Zeal/Zeal/docsets"
host_docsets=""

if [[ -d "${flatpak_docsets}" ]]; then
    host_docsets="${flatpak_docsets}"
elif [[ -d "${native_docsets}" ]]; then
    host_docsets="${native_docsets}"
fi

container_data="/preview/data"
container_docsets="${container_data}/Zeal/Zeal/docsets"

if [[ ! -x "${repo_dir}/${app_binary#./}" ]]; then
    echo "Missing preview binary: ${repo_dir}/${app_binary#./}" >&2
    echo "Build the release preset before running this preview." >&2
    exit 1
fi

if docker container inspect "${container_name}" >/dev/null 2>&1; then
    existing_image="$(docker container inspect -f '{{.Config.Image}}' "${container_name}")"
    if [[ "${existing_image}" != "${image}" ]]; then
        echo "Refusing to remove existing container ${container_name} using image ${existing_image}." >&2
        exit 1
    fi
    docker rm -f "${container_name}" >/dev/null
fi

mkdir -p \
    "${preview_dir}/config" \
    "${preview_dir}/data/Zeal/Zeal" \
    "${preview_dir}/cache"

docker_args=(
    run -d
    --name "${container_name}"
    --user "$(id -u):$(id -g)"
    --net=host
    -e "HOME=/tmp"
    -e "XDG_CONFIG_HOME=/preview/config"
    -e "XDG_DATA_HOME=${container_data}"
    -e "XDG_CACHE_HOME=/preview/cache"
    -e "XDG_RUNTIME_DIR=/run/user/$(id -u)"
    -e "DISPLAY=${DISPLAY:-:0}"
    -e "QTWEBENGINE_DISABLE_SANDBOX=1"
    -e "LIBGL_ALWAYS_SOFTWARE=1"
    -e "QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu"
    -v "/tmp/.X11-unix:/tmp/.X11-unix"
    -v "/run/user/$(id -u):/run/user/$(id -u)"
    -v "${repo_dir}:${repo_dir}"
    -v "${preview_dir}/config:/preview/config"
    -v "${preview_dir}/data:/preview/data"
    -v "${preview_dir}/cache:/preview/cache"
    -w "${repo_dir}"
)

if [[ -n "${XAUTHORITY:-}" ]]; then
    docker_args+=(-e "XAUTHORITY=${XAUTHORITY}")
fi

if [[ -n "${host_docsets}" ]]; then
    docset_count="$(find "${host_docsets}" -maxdepth 1 -type d -name '*.docset' | wc -l)"
    echo "Using host docsets: ${host_docsets} (${docset_count} docsets)"
    echo "Mounting read-only at: ${container_docsets}"
    docker_args+=(-v "${host_docsets}:${container_docsets}:ro")
else
    echo "Warning: no host Zeal docset directory found." >&2
    echo "Checked:" >&2
    echo "  ${flatpak_docsets}" >&2
    echo "  ${native_docsets}" >&2
    echo "The preview will start with an empty writable docset directory." >&2
fi

container_id="$(docker "${docker_args[@]}" "${image}" "${app_binary}")"

echo "Started container: ${container_name}"
echo "Container id: ${container_id}"
echo "Stop with: docker stop ${container_name}"
