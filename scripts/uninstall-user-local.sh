#!/usr/bin/env bash
set -euo pipefail

install_dir="${HOME}/.local/opt/zealrn"
launcher="${HOME}/.local/bin/zealrn"
desktop_file="${HOME}/.local/share/applications/io.github.abnzrdev.zealrn.desktop"
icon_file="${HOME}/.local/share/icons/hicolor/scalable/apps/zealrn.svg"

if [[ -L "${launcher}" ]] && [[ "$(readlink "${launcher}")" == "${install_dir}/"* ]]; then
    rm "${launcher}"
elif [[ -f "${launcher}" ]] && grep -q '^# ZealRN user-local launcher$' "${launcher}"; then
    rm "${launcher}"
fi
rm -f "${desktop_file}" "${icon_file}"
rm -rf "${install_dir}"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${HOME}/.local/share/applications" >/dev/null 2>&1 || true
fi

echo "Removed the user-local ZealRN application."
echo "Settings, notes, exports, backups, and docsets were not removed."
