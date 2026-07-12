# Web Playground Shell Design

## Scope

This iteration adds only the Web Playground shell. It does not bundle CodeMirror, create a preview `QWebEngineView`, execute code, capture console output, export projects, or change Zeal identity or packaging.

The existing central layout remains unchanged:

```text
sidebar | documentation | Learning Notes
```

The playground is a bottom dock surrounding that central layout, not another child of its horizontal splitter.

## Approaches Considered

1. A bottom `QDockWidget` owned by `MainWindow`. This is the selected approach because Qt already provides docking, resizing, visibility actions, and versioned layout persistence without changing `BrowserTab` or the documentation splitter.
2. A vertical splitter around the documentation stack. This would tightly couple playground visibility to the central layout and require custom size and visibility persistence.
3. A per-tab playground inside `BrowserTab`. This would duplicate state across tabs and touch documentation navigation ownership before per-page playgrounds are required.

## Components

`WebPlaygroundPanel` is a dedicated `QWidget` in `src/libs/ui/webplaygroundpanel.h` and `src/libs/ui/webplaygroundpanel.cpp`. It owns a lightweight shell containing:

- HTML, CSS, and JavaScript editor tabs.
- Run and Auto Run controls.
- Preview and Console output tabs.
- Stop, Reset, Clear Console, Open in Browser, Export Project, and Close Playground controls.
- A readable notice that code will run locally inside an isolated preview.

Controls whose behavior belongs to later phases remain disabled. The panel emits a close request for the explicit Close Playground control; this is the only reason it needs `Q_OBJECT`.

`MainWindow` owns one `QDockWidget` named `webPlaygroundDock`. It places the panel in the bottom dock area, allows bottom docking only, connects the panel close request to `hide()`, and adds the dock's native checkable `toggleViewAction()` to the View menu as “Web Playground”. No toolbar is added because the menu action satisfies the requested discoverability with less persistent chrome.

## State And Startup

`Core::WindowState` gains one `QByteArray` for the versioned `QMainWindow` dock state. Session TOML reads and writes it as an optional blob, preserving compatibility with existing session files and leaving the current geometry, documentation splitter, and table-of-contents splitter fields unchanged.

After the dock exists and has its stable `objectName`, `MainWindow` calls `restoreState()` with a dedicated state version. If the blob is absent, malformed, or has a different version, restoration fails and the dock is explicitly hidden. A valid state restores visibility, dock area, and relative dock size. Destruction saves the current dock state without altering the existing splitter-state save.

First launch is therefore hidden. Later launches restore the last valid state. Older Zeal/ZealRN state remains readable because the new TOML key is optional. Existing saved settings are never erased.

## Lazy Boundary

The shell constructs ordinary Qt widgets only. It creates no CodeMirror runtime, WebEngine profile, page, preview, timer, or execution process. Hidden startup therefore performs no editor initialization or user-code execution. Later editor and preview phases will initialize their runtime when the dock first becomes visible.

## Error Handling

- Missing or invalid dock state: keep the playground hidden.
- Unsupported state version: keep the playground hidden.
- Existing two- or three-pane splitter state: restore using the current independent splitter logic.
- Disabled shell actions: remain visibly unavailable rather than pretending to perform later functionality.

## Validation

Focused session tests cover round-tripping the new optional dock-state blob and loading older session data without it. Build and existing tests run in the established Ubuntu 24.04 Docker workflow. Runtime smoke checks verify that the playground starts hidden, the View action shows and hides it, dock resizing works, a second launch restores valid visibility and size, and corrupted or absent state falls back to hidden while the sidebar, documentation, and Learning Notes panes remain unchanged.

## Non-Goals

This shell does not add CodeMirror assets, code editing, preview rendering, code execution, console capture, templates, documentation selection capture, export, branding, AppImage packaging, or Flatpak packaging.
