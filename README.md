# ZealRN

<p align="center">
  <img src="assets/branding/zealrn-logo.svg" width="96" alt="ZealRN logo">
</p>

<p align="center"><strong>Offline documentation that helps you learn, not just search.</strong></p>

<p align="center">
  <a href="https://zealrn.github.io/zealrn-web/">Try ZealRN Web</a> ·
  <a href="https://zealrn.github.io/">Product Site</a> ·
  <a href="https://github.com/zealrn/zealrn-desktop/issues">Report an Issue</a>
</p>

ZealRN is a local-first learning workspace: read documentation, write notes linked to the page you are studying, experiment with HTML/CSS/JavaScript, and review your work later.

## Features

- Browse and search the downloadable Zeal/Dash docset catalog offline after installation.
- Write an always-available **Start Note** before opening documentation.
- Keep one persistent Markdown-compatible note per documentation page.
- Capture selected documentation text without executing it.
- Search, reopen, and export notes as Markdown, searchable PDF, JSON, ZIP, or SQLite backup.
- Experiment in an offline Web Playground with HTML, CSS, JavaScript, preview, and console output.
- Use System, Light, or Dark appearance and a first-run Quick Tour.
- Use the normal local developer environment from the Linux embedded terminal.

## Desktop And Web

| | ZealRN Desktop | ZealRN Web |
|---|---|---|
| Documentation | Downloadable docset catalog | Small starter library |
| Notes | Local SQLite database | Browser IndexedDB |
| Playground | Isolated Qt WebEngine preview | Sandboxed browser iframe |
| Terminal | Embedded xterm.js/POSIX PTY on Linux | Not available |
| Windows terminal | External Windows Terminal, PowerShell, cmd, or Git Bash | Not available |
| Install | Linux packages and Windows packages are being prepared | Installable PWA |

ZealRN 0.1.0 packages are being prepared for public release. No public binary download is linked until a release is published.

## Start Note And Learning Notes

The Start Note is a persistent scratchpad available without a docset. Opening a documentation page switches the panel to that page's note. Notes autosave, preserve Markdown text, and remain on the local device.

The editor supports Markdown actions, Edit/Preview modes, note search, zoom, focus mode, selected-text capture, and searchable exports. Back up the SQLite database or export notes regularly.

## Web Playground

The bundled CodeMirror editors and preview require no Node.js or network at runtime. Preview requests to external network schemes are blocked. This is defense-in-depth, not a perfect security sandbox; run only code you understand.

## Terminal

Linux builds include an embedded xterm.js terminal backed by a native POSIX PTY. It has the same access as the normal user shell. Windows builds open an external Windows Terminal, PowerShell, Command Prompt, or Git Bash instead; ZealRN does not claim an embedded Windows terminal.

## Install Status

Public releases are not published yet. Follow the [product site](https://zealrn.github.io/) for release status. The [Web PWA](https://zealrn.github.io/zealrn-web/) is available for trying the browser experience.

## Build From Source

Linux requires Qt 6 with WebEngine, SQLite, PrintSupport, CMake, Ninja, and the project dependencies. The normal build flow is:

```sh
cmake --preset release -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
cmake --build --preset release
cmake --preset testing -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
cmake --build --preset testing
ctest --preset testing --output-on-failure
```

The application does not require Node.js or npm after installation. Node is only used to regenerate bundled developer assets.

## Local Data

- Settings: `~/.config/abnzrdev/ZealRN.conf`
- Notes and docsets: `~/.local/share/abnzrdev/ZealRN/`
- Cache: `~/.cache/abnzrdev/ZealRN/`

ZealRN uses a separate application identity from Zeal. It can reuse an existing docset directory without copying it, but it never automatically copies or deletes docsets.

## Privacy And Security

Notes, settings, exports, and downloaded docsets stay on the local device. ZealRN has no account or analytics service. Application update checks are notification-only and target the ZealRN repository; they never install software automatically. See [SECURITY.md](SECURITY.md) and [UPSTREAM.md](UPSTREAM.md).

## Roadmap

Public package releases, broader documentation guidance, and further Windows integration are planned. The Web app remains a lightweight trial experience rather than a full replacement for Desktop.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Please report active security issues privately using [GitHub security advisories](https://github.com/zealrn/zealrn-desktop/security/advisories/new).

## Based On Zeal

ZealRN is independently maintained and based on [Zeal](https://github.com/zealdocs/zeal). It preserves the upstream GPL licensing, copyright, and attribution. ZealRN is not officially endorsed by the upstream Zeal project. See [UPSTREAM.md](UPSTREAM.md).

## License

ZealRN is available under [GPL-3.0-or-later](COPYING). Third-party notices are included with the source and installed application.
