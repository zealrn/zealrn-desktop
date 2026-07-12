# ZealRN Linux Alpha Release Design

## Identity

ZealRN is a separate application named `ZealRN`, executable/package `zealrn`, application ID `io.github.abnzrdev.zealrn`, version `0.1.0-alpha`. Startup sets organization and application identity before settings or `QStandardPaths` access. Internal C++ namespaces remain unchanged.

Linux metadata receives ZealRN-specific filenames and identifiers. Existing GPL notices, source attribution, and bundled third-party licenses remain installed. The alpha may reuse the existing icon artwork under distinct ZealRN filenames; replacing artwork is outside this release.

## Existing Docsets

On first launch with no configured ZealRN docset directory, startup detects common native and Flatpak Zeal paths. A small chooser offers reuse, another directory, or an empty library. It never copies docsets automatically. A chosen path remains configurable through existing settings.

Concurrent use is treated conservatively: browsing shared docsets is supported, while users are warned not to install/remove the same docset from two running applications. No broad filesystem access or automatic ownership changes are introduced.

## Build And Packages

CMake keeps one install tree. CPack generates `zealrn_0.1.0-alpha_amd64.deb` with discovered runtime dependencies. A local AppImage script follows upstream's linuxdeploy workflow: install to AppDir, deploy Qt WebEngine and plugins, then create `ZealRN-0.1.0-alpha-x86_64.AppImage`.

Generated artifacts live in ignored `dist/`. Node is not needed at runtime because CodeMirror assets are already resources. QSQLITE and Qt Print Support are packaged. Developer Terminal capability is reported from the build: embedded Linux terminal when QTermWidget is present, otherwise the existing external-terminal fallback.

## User-Local Install

`scripts/install-user-local.sh` verifies `dist/SHA256SUMS`, installs the AppImage to `~/.local/opt/zealrn`, creates `~/.local/bin/zealrn`, and installs the desktop entry under `~/.local/share/applications`. Replacements receive timestamped backups. The uninstall script removes only installed program integration, never config, notes, exports, or docsets.

## Validation

Release and testing presets build in the disposable Ubuntu 24.04 environment. CTest must pass before packaging. AppImage runs against a clean home and then as the installed user-local app. `dpkg-deb --info` and `--contents` validate the Debian package. Checksums are generated after final artifacts. The original Zeal installation and data are inspected before and after acceptance testing and remain untouched.
