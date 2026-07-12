# Developer Terminal Design

## Scope

Add a Developer Terminal beside Web Playground in the existing bottom dock. The terminal runs the user's normal shell environment; it is not sandboxed. Documentation, Learning Notes, and Web Playground behavior remain unchanged.

The MVP has one session, one selected shell, and one working directory. Multiple tabs, compiler adapters, command suggestions, SSH management, and note features remain out of scope.

## Dependency Decision

An embedded terminal requires both a pseudo-terminal and a real terminal emulator. A `QTextEdit` connected to `QProcess` is not sufficient.

Linux uses QTermWidget when a Qt 6-compatible installation is available. QTermWidget 2.1 is GPL-2.0-or-later, which is compatible with Zeal's GPL-3.0-or-later license. It provides PTY-backed interaction, ANSI rendering, scrollback, resizing, signals, and clean session teardown. It remains optional because Ubuntu 24.04 packages only the Qt 5 version and upstream QTermWidget 2.1 requires Qt 6.6 while Zeal supports Qt 6.4.

When embedded support is unavailable, the panel explains the limitation and keeps Open External Terminal functional. ZealRN must configure and build normally in this mode.

Microsoft ConPTY provides a UTF-8 terminal byte stream but not a terminal renderer. Implementing an untested renderer in this iteration would be unsafe. The Windows backend therefore provides platform-correct shell detection and external launch now, while preserving the backend boundary for a future ConPTY plus renderer implementation. Existing Windows CI compiles the guarded Windows source after publication; Windows runtime success is not claimed in this iteration.

## Shared Architecture

`DeveloperTerminalPanel` owns the platform-neutral toolbar, status, safety acknowledgement, shell selection, working-directory controls, and backend lifetime.

`TerminalBackend` is a small abstract `QObject` API for an optional embedded widget, session start/stop, clear/copy/paste, running state, and appearance updates. `createTerminalBackend()` selects one guarded implementation:

- `LinuxTerminalBackend` wraps QTermWidget when `ZEALRN_HAVE_QTERMWIDGET` is defined; otherwise it reports embedded support unavailable.
- `WindowsTerminalBackend` reports embedded ConPTY pending and leaves external launch available.
- `UnavailableTerminalBackend` covers other platforms and explicitly disabled builds.

Pure `terminalsupport` helpers detect shells, validate saved choices and directories, choose safe defaults, and construct external-terminal program/argument pairs. Tests exercise these rules without launching processes.

## Bottom Dock Integration

The existing `webPlaygroundDock` object name and `QMainWindow` state version remain unchanged so valid saved dock geometry continues to restore. Its title becomes “Development Tools” and its content becomes a `QTabWidget`:

1. Web Playground
2. Developer Terminal

The existing Web Playground panel remains the first tab. Selecting the terminal tab creates the panel if needed, but no shell process starts before that selection and the safety acknowledgement.

View contains two checkable actions: Show Web Playground and Show Developer Terminal. Triggering either reveals the dock and selects the corresponding tab. Hiding the dock leaves an active terminal session intact. The last selected tool is stored in the existing settings system. Missing or invalid values fall back to Web Playground, while missing or invalid dock state remains hidden through the existing versioned `restoreState()` fallback.

## Session Behavior

Before the first embedded session, the panel shows:

> The Developer Terminal has the same access as your normal shell. Commands can modify or delete files.

Acceptance is persisted. Declining starts nothing. New Session asks before replacing a running session. QTermWidget receives the selected executable, working directory, and inherited environment. Hiding or changing application appearance does not recreate it. Destruction closes the QTermWidget session, which sends SIGHUP to its PTY process.

QTermWidget does not expose the child exit code through its public widget API, so the status shows Running or Exited; an exit code is shown only when a backend can provide one. The exited state offers Restart Session and never restarts automatically.

## Shell And Directory Rules

Linux detects a valid executable `$SHELL` first, then available entries from the system shell database and common shells. Windows prefers a valid saved choice, then `pwsh.exe`, `powershell.exe`, and `cmd.exe`. Saved paths are accepted only while executable.

The working directory order is the last valid selected directory, the user's home directory, then an application-specific workspace under `QStandardPaths::AppDataLocation`. The repository is never selected automatically. Choose Working Directory persists a valid directory; Copy Path and Open Folder are non-destructive conveniences.

External launching uses `QProcess::startDetached(program, arguments, workingDirectory)` with separate arguments. Linux detection order is `xdg-terminal-exec`, `gnome-terminal`, `konsole`, `xfce4-terminal`, `kitty`, `alacritty`, `wezterm`, then `xterm`. Windows prefers Windows Terminal and falls back to PowerShell or Command Prompt. No command is composed through a shell string.

## Appearance And Failure Isolation

The shared panel inherits the application palette. QTermWidget switches between its installed light and dark color schemes without restarting the session; if a named scheme is unavailable, its existing palette remains in use. Terminal failures update only the terminal status and never affect documentation, Learning Notes, or Web Playground.

## Build And Validation

`ZEALRN_ENABLE_TERMINAL` defaults on. On Linux, CMake quietly searches for the Qt 6 `qtermwidget6` target and defines `ZEALRN_HAVE_QTERMWIDGET` only when found. The application still includes the terminal panel and external fallback when the option is off or the package is absent.

Focused tests cover shell selection, saved-shell validation, directory fallback, external launch specifications, disabled backend selection, and selected-tool setting validation. Release/testing builds and existing tests must pass without QTermWidget. Embedded Linux runtime checks are run only in an environment with a compatible QTermWidget; otherwise the exact dependency limitation and external fallback result are reported. Windows compile coverage is provided by the existing guarded Windows CI and is not represented as local runtime validation.
