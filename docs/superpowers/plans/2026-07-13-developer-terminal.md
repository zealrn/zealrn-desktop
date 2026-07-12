# Developer Terminal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a safe, lazy Developer Terminal tab to the existing bottom development dock, with optional embedded QTermWidget support and cross-platform external-terminal fallback.

**Architecture:** A shared panel owns UI and persistence, a small backend interface isolates platform code, and pure helpers own shell/directory/external-launch decisions. The existing dock object and versioned `QMainWindow` state remain authoritative.

**Tech Stack:** C++20, Qt 6 Widgets/Test, optional QTermWidget 2.x, QSettings, CMake/Ninja, Docker/Xvfb.

## Global Constraints

- Preserve `sidebar | documentation | Learning Notes` and all existing Web Playground behavior.
- Never create a shell process before the Developer Terminal is selected and acknowledged.
- Never fake an interactive terminal with `QTextEdit` and `QProcess` pipes.
- Build and show a useful external fallback when no embedded backend is available.
- Keep `.codegraph/`, build output, screenshots, logs, and temporary files unstaged; do not push.

---

### Task 1: Testable Terminal Support Rules

**Files:**
- Create: `src/libs/ui/terminalsupport.h`
- Create: `src/libs/ui/terminalsupport.cpp`
- Create: `src/libs/ui/tests/developerterminal_test.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `src/libs/ui/tests/CMakeLists.txt`

**Interfaces:**
- Produces: `availableShells()`, `validatedShell()`, `validatedWorkingDirectory()`, `externalTerminalLaunch()`, and `BottomTool` validation helpers.

- [ ] Write tests for valid/invalid saved shells, home/workspace directory fallback, Linux/Windows external launch argument construction, and invalid selected-tool fallback.
- [ ] Run the focused test and confirm it fails before implementation.
- [ ] Implement the minimum pure helpers using `QStandardPaths`, `QFileInfo`, and `QProcessEnvironment`.
- [ ] Run the focused test and full suite, review the diff, and commit `test: add developer terminal support rules`.

### Task 2: Shared Panel And Dock Integration

**Files:**
- Create: `src/libs/ui/developerterminalpanel.h`
- Create: `src/libs/ui/developerterminalpanel.cpp`
- Create: `src/libs/ui/terminalbackend.h`
- Create: `src/libs/ui/terminalbackend.cpp`
- Modify: `src/libs/core/settings.h`
- Modify: `src/libs/core/settings.cpp`
- Modify: `src/libs/core/tests/settings_test.cpp`
- Modify: `src/libs/ui/mainwindow.h`
- Modify: `src/libs/ui/mainwindow.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`

**Interfaces:**
- Produces: `DeveloperTerminalPanel`, `TerminalBackend`, and lazy `createTerminalBackend()`.
- Consumes: terminal support helpers from Task 1.

- [ ] Add failing settings tests for acknowledgement, shell, working directory, and invalid bottom-tool restoration.
- [ ] Persist the four values through the existing UI settings group.
- [ ] Implement the platform-neutral panel, safety acknowledgement, shell/directory controls, status, and unavailable state without starting a process in its constructor.
- [ ] Replace the dock content with Web Playground and Developer Terminal tabs while preserving `webPlaygroundDock`, dock state restoration, and first-launch hidden behavior.
- [ ] Add checkable View actions that reveal/select their tool and keep their state synchronized.
- [ ] Build, run all tests, smoke-test restoration, review, and commit `feat: add developer terminal shell`.

### Task 3: Optional Linux QTermWidget Backend

**Files:**
- Create: `src/libs/ui/linuxterminalbackend.h`
- Create: `src/libs/ui/linuxterminalbackend.cpp`
- Modify: `src/libs/ui/terminalbackend.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `README.md`

**Interfaces:**
- Produces: PTY-backed `TerminalBackend` when CMake finds target `qtermwidget6`.
- Consumes: selected executable and working directory from `DeveloperTerminalPanel`.

- [ ] Add `ZEALRN_ENABLE_TERMINAL`, guarded source selection, quiet `qtermwidget6` discovery, and `ZEALRN_HAVE_QTERMWIDGET`.
- [ ] Wrap QTermWidget start, clear, copy, paste, finished, history, scrollbar, inherited environment, and theme operations.
- [ ] Ensure deleting the backend closes the session and that hiding the dock preserves it.
- [ ] Document the optional Qt 6 QTermWidget dependency and external fallback.
- [ ] Validate both terminal-disabled and no-QTermWidget builds, then validate an embedded build only if a compatible package is available; review and commit `feat: add linux terminal backend`.

### Task 4: Windows And Unsupported Fallbacks

**Files:**
- Create: `src/libs/ui/windowsterminalbackend.h`
- Create: `src/libs/ui/windowsterminalbackend.cpp`
- Modify: `src/libs/ui/terminalbackend.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `.github/workflows/build-check.yaml` only if existing platform guards do not already compile the source.

**Interfaces:**
- Produces: guarded Windows backend with shell detection and external launching; unsupported platforms return the unavailable backend.

- [ ] Keep Windows headers in the Windows source only and compile it only under `WIN32`.
- [ ] Report embedded ConPTY as unavailable until a tested terminal renderer exists; do not claim runtime support.
- [ ] Verify Windows launch specifications use separate executable/arguments and selected working directory.
- [ ] Run Linux tests proving Windows-only symbols do not leak; use existing Windows CI configuration for future compile coverage.
- [ ] Review and commit `feat: add windows terminal fallback`.

### Task 5: Final Validation

- [ ] Configure/build Release with terminal enabled but QTermWidget absent.
- [ ] Configure/build Testing and run `ctest --preset testing --output-on-failure`.
- [ ] Configure/build once with `-DZEALRN_ENABLE_TERMINAL=OFF` and verify the unavailable UI path.
- [ ] Run Xvfb checks for dock tabs/actions, lazy initialization, acknowledgement, directory persistence, theme switching, hide/show, and external fallback.
- [ ] If compatible QTermWidget is available, run prompt, ANSI, Ctrl+C, resize, working directory, shell exit, and shutdown checks; otherwise record the exact limitation.
- [ ] Confirm documentation, Learning Notes, and Web Playground remain usable.
- [ ] Finish with a clean tracked tree and `.codegraph/` untracked.
