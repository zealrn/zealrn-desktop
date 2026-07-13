# ZealRN Windows Alpha Release Design

## Scope

This iteration keeps the Linux catalog-fixed AppImage installed, changes the application update checker to use only `abnzrdev/zealrn`, and produces Windows x64 portable and installer artifacts through GitHub Actions. It does not publish a release or tag.

## Update Checking

`Core::Application` will request GitHub's public releases endpoint for `abnzrdev/zealrn`. A small pure parser will ignore drafts and prereleases, parse `tag_name`, and distinguish an empty valid release list from malformed data. The existing application signals will gain an explicit no-releases result so the UI can show “No ZealRN releases have been published yet.” without treating upstream Zeal as an update source.

Automatic checks remain controlled by the existing setting. Quiet checks suppress no-release and network dialogs; manual checks display a concise result. No request targets `zealdocs/zeal`.

## Windows Build

The build uses `windows-2022`, MSVC 2022, Qt 6.11.1, the existing vcpkg manifest, CMake presets, and Qt's generated deployment script. CPack ZIP creates the portable payload and CPack NSIS creates an unsigned current-user installer. Both derive from the same CMake install tree so Qt WebEngine, SQLite, PrintSupport, plugins, resources, licenses, and application assets remain consistent.

The workflow runs release and testing configurations, CTest, `zealrn.exe --version`, package-content validation, SHA-256 generation, and artifact upload. It never creates a GitHub Release.

## Compatibility

Windows continues using the existing external terminal detection (`wt.exe`, `pwsh.exe`, `powershell.exe`, `cmd.exe`); embedded ConPTY is out of scope. QStandardPaths keeps settings and data separate from the program directory and from original Zeal. The NSIS package uses a ZealRN-specific name and uninstall identity and preserves user data because only installed program files are managed.

## Linux Launcher

The user-local installer will install a small launcher script rather than a direct AppImage symlink. It first launches normally, retries with `QT_QPA_PLATFORM=xcb` only for a platform-plugin startup failure, and stays quiet during normal app-menu launch. The uninstaller continues removing only program integration files.
