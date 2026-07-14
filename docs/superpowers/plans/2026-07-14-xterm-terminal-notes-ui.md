# Embedded Terminal and Learning Notes UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship an embedded xterm.js terminal backed by Linux PTY, retain external-terminal integration on Windows, and provide a larger Markdown-focused Learning Notes editor without changing existing note data.

**Architecture:** A local, isolated `TerminalView` and minimal `TerminalBridge` render bytes from one platform-selected `TerminalBackend`. Learning Notes remains a native Qt widget over the current store, with presentation-only settings and temporary MainWindow layout modes.

**Tech Stack:** C++20, Qt 6.4.2+ Widgets/WebEngine/WebChannel/Sql/PrintSupport/Test, xterm.js 6.0.0, esbuild 0.28.1, POSIX PTY, CMake/Ninja, GitHub Actions.

## Global Constraints

- Work only on `feature/xterm-terminal-notes-ui`; never push upstream or publish a release/tag.
- Keep `.codegraph/`, `node_modules`, builds, databases, exports, screenshots, logs, and package artifacts unstaged.
- Preserve the existing SQLite schema, page identity, autosave, exports, docsets, Web Playground, and appearance behavior.
- Installed applications require neither Node.js nor QTermWidget.
- Terminal tests have finite timeouts and cleanup every child/process handle.

---

### Task 1: Freeze Data and Frontend Dependencies

**Files:**
- Create: `tools/terminal-frontend/package.json`
- Create: `tools/terminal-frontend/package-lock.json`
- Create: `tools/terminal-frontend/src/terminal.js`
- Create: `tools/terminal-frontend/THIRD_PARTY_LICENSES.md`
- Create: `src/app/resources/terminal/index.html`
- Create: `src/app/resources/terminal/terminal.bundle.js`
- Create: `src/app/resources/terminal/terminal.bundle.css`
- Modify: `src/app/zeal.qrc`
- Modify: `.gitignore`

**Interfaces:**
- Produces: `window.zealrnTerminal` frontend API and `terminalBridge` WebChannel object contract.

- [ ] Back up the current user database with `cp --reflink=auto` where supported and record `sha256sum` before any runtime test.
- [ ] Add pinned xterm/addon/esbuild dependencies and `npm run bundle`; generate a lockfile with no global install.
- [ ] Implement xterm creation, FitAddon, SearchAddon, Base64 input/output, readiness, resize, clear, focus, copy/paste hooks, theme, and font size.
- [ ] Register generated assets and license notice in Qt resources/install rules.
- [ ] Run `npm ci && npm run bundle`, verify `node_modules` is ignored, inspect generated files, and commit `feat: add bundled xterm terminal frontend`.

### Task 2: Raw Terminal Contract, Bridge, and Isolated View

**Files:**
- Modify: `src/libs/ui/terminalbackend.h`
- Modify: `src/libs/ui/terminalbackend.cpp`
- Create: `src/libs/ui/terminalbridge.h`
- Create: `src/libs/ui/terminalbridge.cpp`
- Create: `src/libs/ui/terminalview.h`
- Create: `src/libs/ui/terminalview.cpp`
- Modify: `src/libs/ui/unavailableterminalbackend.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `src/libs/ui/tests/developerterminal_test.cpp`

**Interfaces:**
- `TerminalBackend::start(const TerminalProfile &, const QString &, QSize)`
- `TerminalBackend::write(const QByteArray &)`, `resize(QSize)`, `interrupt()`, `terminate()`
- Signals: `outputReceived(QByteArray)`, `started(TerminalProfile)`, `exited(int,bool)`, `errorOccurred(QString)`
- Bridge slots: `frontendReady()`, `sendInput(QString)`, `resizeTerminal(int,int)`, `startSession(QString,QString)`, `interruptSession()`, `terminateSession()`, `copySelection(QString)`, `requestPaste()`

- [ ] Add failing tests for Base64 round trips, split UTF-8 bytes, readiness queueing, output batching, cap behavior, and disabled backend selection.
- [ ] Run the focused test and verify failures name the missing bridge/contract.
- [ ] Replace the widget-owning backend contract and implement the bounded bridge queue.
- [ ] Add an off-the-record view/profile/page/interceptor allowing only terminal qrc and Qt WebChannel resources.
- [ ] Run focused tests, release compile, `git diff --check`, and commit `refactor: replace qtermwidget terminal architecture`.

### Task 3: Linux PTY Backend

**Files:**
- Replace: `src/libs/ui/linuxterminalbackend.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Create: `src/libs/ui/tests/posixptybackend_test.cpp`
- Modify: `src/libs/ui/tests/CMakeLists.txt`

**Interfaces:**
- Produces `createTerminalBackend()` with a native PTY on Unix when terminal support is enabled.

- [ ] Add timeout-bounded tests for echo, working directory, ANSI, Unicode, `stty size`, interrupt, exit code, and child reaping.
- [ ] Run the test and verify it fails because the QTermWidget backend cannot satisfy the raw contract.
- [ ] Implement `forkpty`, nonblocking notifier I/O, queued writes, resize, process-group termination, and `waitpid` reaping.
- [ ] Link `libutil` only where required and remove all QTermWidget discovery/definitions/linking.
- [ ] Run PTY tests repeatedly, inspect child processes, build with terminal on/off, and commit `feat: add linux pty terminal backend`.

### Task 4: Terminal Panel Integration

**Files:**
- Modify: `src/libs/ui/developerterminalpanel.h`
- Modify: `src/libs/ui/developerterminalpanel.cpp`
- Modify: `src/libs/ui/terminalsupport.h`
- Modify: `src/libs/ui/terminalsupport.cpp`
- Modify: `src/libs/core/settings.h`
- Modify: `src/libs/core/settings.cpp`
- Modify: `src/libs/core/tests/settings_test.cpp`
- Modify: `src/libs/ui/mainwindow.cpp`

**Interfaces:**
- Consumes `TerminalView` and platform `TerminalProfile` discovery.
- Produces lazy one-session terminal UX with persisted profile/directory/font size.

- [ ] Add failing tests for profile preference order, saved-profile validation, terminal font bounds, and external fallback argument separation.
- [ ] Implement labeled profiles, lazy view/backend construction, Stop/Search/Restart/font controls, and existing safety acknowledgement.
- [ ] Connect status, exit code, copy/paste, resize, appearance, and `Ctrl+`` without recreating sessions.
- [ ] Run terminal/settings tests and an Xvfb startup smoke test; commit `feat: integrate embedded developer terminal`.

### Task 5: Windows External Terminal Fallback

**Files:**
- Modify: `src/libs/ui/terminalsupport.cpp`
- Modify: `src/libs/ui/tests/developerterminal_test.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `.github/workflows/windows-alpha.yml`

**Interfaces:**
- Produces Windows shell discovery and safe external-terminal launch specifications without an embedded backend.

- [x] Keep PowerShell/cmd/Git Bash profile discovery and argument-safe external launching.
- [x] Select the unavailable embedded backend on Windows while keeping the terminal panel and external action functional.
- [x] Remove the unreliable ConPTY implementation and its process tests after repeated CI failures.
- [ ] Run Windows shell/fallback tests and package validation on `windows-2022`.

### Task 6: Markdown Editing Helpers and Zoom

**Files:**
- Create: `src/libs/ui/learningnotesmarkdown.h`
- Create: `src/libs/ui/learningnotesmarkdown.cpp`
- Create: `src/libs/ui/tests/learningnotesmarkdown_test.cpp`
- Modify: `src/libs/ui/tests/CMakeLists.txt`
- Modify: `src/libs/core/settings.h`
- Modify: `src/libs/core/settings.cpp`
- Modify: `src/libs/core/tests/settings_test.cpp`

**Interfaces:**
- `LearningNotesMarkdown::format(QTextCursor, Format)` returns the logical cursor/selection.
- `LearningNotesMarkdown::counts(QString, QString)` returns words, characters, selected characters.
- `LearningNotesMarkdown::clampZoom(int)` returns 80-200 with default 115.

- [ ] Add failing table-driven tests for every formatting action with/without selection, counts, Unicode, and zoom bounds/settings persistence.
- [ ] Implement minimal cursor-based wrappers/prefixes/placeholders and count/zoom helpers.
- [ ] Run focused tests and commit `feat: add markdown note editing helpers`.

### Task 7: Learning Notes Panel Redesign

**Files:**
- Modify: `src/libs/ui/learningnotespanel.h`
- Modify: `src/libs/ui/learningnotespanel.cpp`
- Modify: `src/libs/ui/tests/learningnotespanel_test.cpp`
- Modify: `src/libs/ui/mainwindow.h`
- Modify: `src/libs/ui/mainwindow.cpp`

**Interfaces:**
- New panel signals: `expandedModeRequested(bool)`, `focusModeRequested(bool)`.
- Existing store/page/export signals remain unchanged.

- [ ] Add failing tests for zoom persistence/application, toolbar edits, preview safety, undo preservation, find wrap, counts, autosave, hide/show, and existing note loading.
- [ ] Rebuild the panel with compact metadata, Markdown toolbar, zoom controls, Edit/Preview stack, safe `QTextBrowser`, find bar, counts/timestamp, line wrap, and existing actions.
- [ ] Increase splitter default/minimum to 410/300 while preserving valid old three-pane state.
- [ ] Implement reversible expanded/focus layout snapshots in MainWindow; Escape restores exact splitter/sidebar/dock state without destroying Development Tools.
- [ ] Run focused and existing store/export tests, Xvfb theme checks, and commit `feat: improve learning notes editor`.

### Task 8: Linux Validation and Packaging

**Files:**
- Modify only if required: `scripts/build-linux-release.sh`
- Modify only if required: `scripts/install-user-local.sh`
- Modify: `docs/releases/0.1.0-alpha.md`

**Interfaces:**
- Produces ignored AppImage/Debian artifacts containing xterm assets and native PTY backend.

- [ ] Configure/build release and testing presets; run all CTest tests and repeat PTY lifecycle tests under timeout.
- [ ] Run temporary-profile Xvfb checks for terminal prompt/ANSI/Unicode/resize/Ctrl+C and notes load/zoom/format/preview/find/focus/export.
- [ ] Compare the live SQLite checksum/content against its backup and verify no unintended rewrite.
- [ ] Build AppImage/Debian/SHA256SUMS, inspect dependencies to confirm no QTermWidget/Node, back up the current install, reinstall, and launch `~/.local/bin/zealrn`.
- [ ] Commit packaging/docs source changes only as `build: package embedded terminal and notes ui`.

### Task 9: Windows CI, Packages, and Final Verification

**Files:**
- Modify only for actual failures: `.github/workflows/windows-alpha.yml`, Windows backend/tests, packaging scripts.

**Interfaces:**
- Produces ignored Windows portable ZIP, unsigned setup EXE, and checksums downloaded under `dist/windows/`.

- [ ] Push only `feature/xterm-terminal-notes-ui` to `origin`, dispatch Windows CI, and monitor to completion.
- [ ] Fix actual compile/test/package failures with focused commits and rerun; never weaken tests.
- [ ] Download artifacts, verify SHA256SUMS and contents, and confirm xterm resources plus no Node/QTermWidget runtime.
- [ ] Re-run Linux installed-app smoke checks, verify no preview containers/process leaks, check tracked cleanliness, and prepare the requested final report.

## Plan Self-Review

- The user-approved Windows fallback replaces the original ConPTY requirement after repeated CI failures; unsupported-platform and terminal-off builds remain covered.
- QTermWidget removal happens before platform implementations, so no stale dual architecture remains.
- Notes data compatibility is validated before and after runtime tests; no schema migration is planned.
- Platform-specific headers stay in platform source files.
- Generated frontend bundles are intentional resources; dependency trees and package artifacts remain unstaged.
