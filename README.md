# ZealRN

ZealRN is an offline documentation learning tool:

**Read documentation -> take notes -> experiment with code -> review later.**

It is based on [Zeal](https://github.com/zealdocs/zeal) and keeps Zeal's fast offline docset browsing while adding page-linked learning notes, an offline HTML/CSS/JavaScript playground, application appearance settings, and developer-terminal integration.

## Features

- Browse and search installed Dash/Zeal docsets offline.
- Write a persistent Start Note before opening documentation.
- Save one local plain-text note per documentation page.
- Autosave notes and capture selected documentation text as a Markdown blockquote.
- Search, edit, delete, and reopen notes from All Notes.
- Export notes as Markdown, searchable PDF, JSON, or a ZIP archive.
- Back up the local SQLite notes database.
- Experiment with HTML, CSS, and JavaScript in an isolated, network-blocked preview.
- Use System, Light, or Dark appearance.
- Use an embedded developer terminal powered by bundled xterm.js and a native Linux PTY. Windows uses safe external-terminal launching.
- Follow the first-run Quick Tour or reopen offline guidance from Help.

## Install

### User-Local AppImage

From a release checkout containing `dist/`:

```bash
./scripts/install-user-local.sh
~/.local/bin/zealrn
```

This installs without `sudo` under `~/.local/opt/zealrn/` and adds the **ZealRN** desktop entry. The installer verifies `dist/SHA256SUMS` and backs up a replaced installation.

Uninstall the application without deleting settings, notes, exports, or docsets:

```bash
./scripts/uninstall-user-local.sh
```

### Portable AppImage

```bash
chmod +x dist/ZealRN-0.1.0-x86_64.AppImage
./dist/ZealRN-0.1.0-x86_64.AppImage
```

### Debian Package

```bash
sudo apt install ./dist/zealrn_0.1.0_amd64.deb
```

The Debian package is not installed automatically by the build or user-local installer.

## Local Data

ZealRN uses a separate identity and does not reuse Zeal settings automatically.

- Settings: `~/.config/abnzrdev/ZealRN.conf`
- Data and notes: `~/.local/share/abnzrdev/ZealRN/`
- Notes database: `~/.local/share/abnzrdev/ZealRN/learning-notes.sqlite`
- Default docsets: `~/.local/share/abnzrdev/ZealRN/docsets/`

On first launch, ZealRN can reuse a native or Flatpak Zeal docset directory without copying it. Reused libraries are opened read-only in ZealRN to avoid concurrent install/remove operations. Choose another directory in Preferences to use writable ZealRN-managed docsets.

## Learning Notes

Use **Start Note** as an always-available scratchpad. Open a real docset page to switch to its page-linked note, then use **Save Note** or `Ctrl+S`. Autosave runs after a short pause. **Add Selection** appends selected documentation text without executing or modifying it.

**All Notes** searches page title, docset, relative path, and content. Export actions never overwrite a file without confirmation. PDF exports contain selectable/searchable text rather than screenshots.

The editor includes Markdown formatting, Edit/Preview modes, note-local search, zoom and line-wrap preferences, and reversible Expand and Focus modes. These presentation settings do not change stored note text or the SQLite schema.

## Web Playground

The Web Playground bundles CodeMirror locally and requires no network, Node.js, or npm at runtime. Its internal preview blocks external `http`, `https`, `ftp`, `ws`, and `wss` requests.

The preview is defense-in-depth, not a perfect security sandbox. Run only code you understand. User-created preview content is not recolored by ZealRN's appearance setting. **Open in Browser** runs outside the internal preview restrictions.

## Build

Required dependencies include CMake, Ninja, Qt 6.4.2 or later with Widgets, Svg, WebEngine, WebChannel, SQL SQLite, and Print Support, plus libarchive, SQLite, XCB keysyms, and xkbcommon on X11.

```bash
cmake --preset release -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
cmake --build --preset release

cmake --preset testing -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
cmake --build --preset testing
ctest --preset testing --output-on-failure
```

The executable is `build/release/zealrn`.

Build release packages with:

```bash
./scripts/package-appimage.sh
./scripts/package-deb.sh
```

CodeMirror developer assets are reproducible but Node.js is only needed to regenerate them:

```bash
cd tools/playground
npm ci
npm run bundle
```

The terminal frontend is also bundled locally. Node.js is only needed to regenerate its xterm.js assets:

```bash
cd tools/terminal-frontend
npm ci
npm run bundle
```

Do not commit `node_modules` or generated release artifacts.

## Documentation

- [Getting started and shortcuts](docs/getting-started.md)
- [Learning Notes](docs/learning-notes.md)
- [Developer Terminal](docs/developer-terminal.md)
- [Backups and exports](docs/backups-and-exports.md)
- [Linux testing](docs/releases/0.1.0-linux-testing.md)
- [Windows testing](docs/releases/0.1.0-windows-testing.md)

## Known Limitations

- This release provides one main note per documentation page.
- Notes are local only; there are no accounts, sync, sharing, or cloud services.
- AppImage targets a normal x86_64 Linux desktop and expects baseline system libraries such as glibc and XCB.
- The embedded Linux terminal has normal user-shell access and is not sandboxed. Windows builds launch an external terminal because this release does not include an embedded ConPTY backend.
- On Qt 6.4-6.6, changing appearance may require restarting ZealRN before downloaded documentation pages change theme.
- Windows packages are unsigned builds; SmartScreen may warn until code signing is configured.

## License And Attribution

ZealRN is licensed under the GNU General Public License version 3 or later. See [COPYING](COPYING).

ZealRN is a fork of Zeal and retains upstream copyright and attribution. Docsets are provided through the Zeal/Dash ecosystem. Bundled third-party license notices are in [LICENSES](LICENSES), [tools/playground/THIRD_PARTY_LICENSES.md](tools/playground/THIRD_PARTY_LICENSES.md), and [tools/terminal-frontend/THIRD_PARTY_LICENSES.md](tools/terminal-frontend/THIRD_PARTY_LICENSES.md).
