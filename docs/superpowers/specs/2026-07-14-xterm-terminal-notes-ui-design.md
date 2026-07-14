# Embedded Terminal and Learning Notes UI Design

## Scope

This change replaces the optional QTermWidget path with a bundled xterm.js renderer backed by a native Linux pseudoterminal. It also enlarges and improves the existing page-linked Markdown note editor without changing note identity, the SQLite schema, autosave semantics, or exports. Windows retains the existing external-terminal integration.

The existing documentation view, bottom Development Tools dock, Web Playground, All Notes dialog, appearance modes, docset catalog, and release identity remain intact. There is one terminal session, no remote transport, no runtime Node.js, and no rich-text note storage.

## Terminal Architecture

`DeveloperTerminalPanel` continues to own the toolbar, safety acknowledgement, selected shell, working directory, status, and external-terminal fallback. It lazily creates a `TerminalView` when the terminal tab is first shown.

`TerminalView` owns a dedicated off-the-record `QWebEngineProfile`, a navigation-blocking page, a request interceptor, a `QWebChannel`, and a `TerminalBridge`. It loads only `qrc:/terminal/index.html`. Requests outside the terminal and Qt WebChannel resources are blocked; popups, downloads, permissions, remote navigation, local file access, and persistent WebEngine storage are disabled.

`TerminalBridge` is the only object exposed to JavaScript. Frontend input and backend output are Base64 strings so UTF-8 fragments and terminal control bytes survive transport unchanged. Backend output is accumulated for at most one event-loop slice, emitted in bounded batches, queued until xterm reports readiness, and capped to prevent an unbounded hidden-panel backlog. JavaScript calls are asynchronous; C++ never waits for `runJavaScript`.

`TerminalBackend` no longer owns a widget. Its interface is raw terminal operations: start, write, resize, interrupt, terminate, running state, and output/start/exit/error signals. CMake selects `PosixPtyBackend` on Linux and `UnavailableBackend` on Windows, unsupported platforms, or when `ZEALRN_ENABLE_TERMINAL=OFF`.

## xterm.js Frontend

The frontend lives under `tools/terminal-frontend/` and pins:

- `@xterm/xterm` 6.0.0
- `@xterm/addon-fit` 0.11.0
- `@xterm/addon-search` 0.16.0
- `esbuild` 0.28.1 as a development-only bundler

The generated JavaScript and CSS are committed under `src/app/resources/terminal/` and registered in `src/app/zeal.qrc`. Installed packages do not need Node.js. The frontend provides 10,000-line scrollback, fit/resize reporting, search, selection, explicit copy/paste, ANSI/VT rendering, true color, Unicode, mouse input, light/dark palettes, and persistent font size. Input is sent through xterm's `onData`; output bytes are decoded incrementally by the browser's terminal parser rather than by C++.

## Linux PTY

`PosixPtyBackend` uses `forkpty` and links `libutil` where required. The child receives the selected working directory, inherited user environment, `TERM=xterm-256color`, and the selected interactive shell. The parent keeps a nonblocking PTY master, uses `QSocketNotifier` for reads and queued writes, applies `TIOCSWINSZ`, and reaps the child with `waitpid`.

Termination targets the PTY child process group with escalating HUP, TERM, then KILL only if needed. EOF and exit status are reported once, file descriptors are closed once, and destruction cannot leave a zombie. No sudo or automatic command is run.

## Windows Terminal Integration

Repeated Windows CI validation showed that the alpha ConPTY backend could not reliably complete input, exit, and teardown across runner environments. It is excluded rather than shipping an unreliable process host. Windows builds retain shell discovery and explicit external launching for Windows Terminal, PowerShell 7, Windows PowerShell, Command Prompt, and Git for Windows Bash. No command is started automatically.

## Terminal UX and State

The toolbar retains Shell, New Session, Directory, Clear, Copy, Paste, Open External, and Close, and adds Stop, Search, and font-size control. A session starts only after the existing one-time unrestricted-shell warning is acknowledged. Hiding the dock or switching appearance preserves the session. New Session confirms replacement, Stop terminates explicitly, Clear clears xterm scrollback only, and an exited shell shows its exit code plus Restart. `Ctrl+`` opens and focuses the terminal when no existing shortcut conflicts.

Shell, directory, terminal font size, and warning acknowledgement use the existing settings architecture. The terminal is not described as sandboxed.

## Learning Notes Layout

The right splitter pane defaults to 410 px and has a 300 px minimum. Existing three-pane splitter state is restored when valid; old, missing, or undersized state receives safe defaults. The documentation pane remains the stretchable center.

`LearningNotesPanel` keeps the existing store, stable page key, one-second autosave, page-switch flush, Add Selection, All Notes, exports, and Ctrl+S. Its new vertical structure is:

1. compact title, page identity, save state, and expand/focus controls;
2. Markdown formatting and zoom toolbars;
3. Edit/Preview switch;
4. large editor or native preview;
5. find bar and count/timestamp row;
6. primary note actions.

The editor stays a `QPlainTextEdit` using the system fixed-width font. Toolbar actions operate on `QTextCursor`, wrapping selections or inserting short placeholders. Stored content remains plain Markdown.

## Zoom, Preview, Find, and Counts

Note zoom defaults to 115%, is clamped to 80-200%, and persists in settings. `Ctrl++`, `Ctrl+-`, `Ctrl+0`, toolbar controls, and Ctrl+wheel update editor and preview presentation only.

Preview uses a separate `QTextBrowser`/`QTextDocument` with GitHub Markdown and `MarkdownNoHTML`. It never replaces the editor document, so editor text, cursor, selection, and undo history survive mode changes. Preview links require an explicit click and confirmation before `QDesktopServices`; remote resources are not loaded.

The note-local find bar uses `QPlainTextEdit::find`, supports next/previous, and wraps once. Counts are derived from current plain text and selection. Line wrap is a user toggle. The footer shows words, characters or selected characters, and the last successful save time.

## Expanded and Focus Modes

Expanded mode records current splitter sizes, gives Learning Notes roughly 42% of available width, and restores the exact prior sizes on exit. Focus mode additionally records and temporarily hides the sidebar and Development Tools dock while keeping documentation and notes visible. It never destroys Development Tools widgets or terminal processes. Escape exits focus first, then expanded mode. Normal splitter sizes and dock visibility are not overwritten while a temporary mode is active.

## Data Safety

No database migration is needed. Before runtime tests, the current `learning-notes.sqlite` and checksum are copied to a timestamped backup outside the repository. Automated tests use temporary database paths and settings. Existing notes are loaded and exported without rewriting them unless the user edits and saves normally.

## Testing and Packaging

Focused Qt tests cover the bridge codec/batching, shell discovery, Linux PTY lifecycle, formatting helpers, zoom bounds, preview safety, find/count behavior, and splitter restoration. Windows tests cover shell detection, external-launch construction, and the unavailable-backend fallback on `windows-2022`. Every process test has a timeout.

The Linux AppImage and Debian package include xterm resources and no QTermWidget or Node runtime. Windows deployment does not include ConPTY code. Generated package artifacts remain ignored. Only the feature branch may be pushed to `origin` for Windows CI; upstream, releases, and tags are untouched.

## Self-Review

- No placeholder or speculative multi-session API is included.
- Terminal rendering, transport, and platform process ownership have separate responsibilities.
- Notes UI preferences stay in settings; note content and identity stay in the existing SQLite schema.
- Linux and Windows backends implement the same raw-byte contract.
- Unsupported platforms and terminal-disabled builds still compile and retain the external fallback.
