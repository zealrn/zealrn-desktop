#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "${script_dir}/.." && pwd)"
dist_dir="${repo_dir}/dist"
checksums="${dist_dir}/SHA256SUMS"

appimage="$(find "${dist_dir}" -maxdepth 1 -type f -name 'ZealRN-*-x86_64.AppImage' -print | sort | tail -n 1)"
if [[ -z "${appimage}" ]]; then
    echo "No ZealRN AppImage found under ${dist_dir}." >&2
    exit 1
fi
if [[ ! -f "${checksums}" ]]; then
    echo "Missing checksum file: ${checksums}" >&2
    exit 1
fi

artifact_name="$(basename "${appimage}")"
expected="$(awk -v name="${artifact_name}" '$2 == name { print $1 }' "${checksums}")"
actual="$(sha256sum "${appimage}" | awk '{ print $1 }')"
if [[ -z "${expected}" || "${actual}" != "${expected}" ]]; then
    echo "Checksum verification failed for ${artifact_name}." >&2
    exit 1
fi

install_dir="${HOME}/.local/opt/zealrn"
installed_appimage="${install_dir}/${artifact_name}"
launcher="${HOME}/.local/bin/zealrn"
desktop_file="${HOME}/.local/share/applications/io.github.abnzrdev.zealrn.desktop"
icon_file="${HOME}/.local/share/icons/hicolor/scalable/apps/zealrn.svg"
backup_path=""

if [[ -d "${install_dir}" && ! -f "${installed_appimage}" ]]; then
    backup_path="${HOME}/.local/opt/zealrn-backup-$(date +%Y%m%d-%H%M%S)"
    mv "${install_dir}" "${backup_path}"
fi

mkdir -p "${install_dir}" "$(dirname "${launcher}")" "$(dirname "${desktop_file}")" "$(dirname "${icon_file}")"
if [[ ! -f "${installed_appimage}" ]] || ! cmp -s "${appimage}" "${installed_appimage}"; then
    if [[ -f "${installed_appimage}" ]]; then
        backup_path="${HOME}/.local/opt/zealrn-backup-$(date +%Y%m%d-%H%M%S)"
        mkdir -p "${backup_path}"
        mv "${installed_appimage}" "${backup_path}/"
    fi
    install -m 0755 "${appimage}" "${installed_appimage}"
fi

ln -sfn "${installed_appimage}" "${launcher}"
install -m 0644 "${repo_dir}/assets/freedesktop/sc-apps-zeal.svg" "${icon_file}"

escaped_launcher="${launcher//\\/\\\\}"
escaped_launcher="${escaped_launcher//\"/\\\"}"
cat > "${desktop_file}" <<EOF
[Desktop Entry]
Version=1.0
Name=ZealRN
GenericName=Documentation Learning Tool
Comment=Read, annotate, and experiment with offline documentation
Keywords=documentation;docs;docset;offline;notes;learning;developer;programming;
Exec="${escaped_launcher}" %u
Icon=zealrn
Terminal=false
Type=Application
Categories=Development;Documentation;
StartupWMClass=ZealRN
EOF

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$(dirname "${desktop_file}")" >/dev/null 2>&1 || true
fi

echo "Installed executable: ${launcher}"
echo "Application menu name: ZealRN"
if [[ -n "${backup_path}" ]]; then
    echo "Previous installation backup: ${backup_path}"
fi
echo "Uninstall with: ${repo_dir}/scripts/uninstall-user-local.sh"
