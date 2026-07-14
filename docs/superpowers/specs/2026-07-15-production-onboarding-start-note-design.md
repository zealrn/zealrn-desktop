# ZealRN 0.1.0 Production Onboarding Design

## Scope

Make the existing desktop application immediately useful on a clean profile without changing its core documentation, notes, playground, or terminal architecture. The release adds reliable terminal startup, a persistent Start Note, first-run guidance, offline help, a compact welcome checklist, and production packaging metadata.

## Existing Architecture

- `MainWindow` owns the documentation tabs, the three-pane splitter, and the bottom Development Tools dock.
- `DeveloperTerminalPanel` lazily creates `TerminalView`; `TerminalBridge` already buffers PTY output until xterm.js calls `frontendReady()`.
- `LearningNotesPanel` owns the Markdown editor, autosave, preview, zoom, find, exports, and a `LearningNotesStore` using schema version 1.
- `MainWindow::syncLearningNotes()` maps only docset URLs to notes and currently passes an invalid page for the welcome page.
- `WebBridge` is already the narrow action channel used by the built-in welcome page.
- `Core::Settings` is the single persistent settings system.

## Terminal Startup

`TerminalView` will expose a C++ `ready` signal when `TerminalBridge::ready` fires. `DeveloperTerminalPanel` will request automatic startup when shown, but start only after all of these are true:

1. the embedded backend is available;
2. the xterm.js frontend is ready;
3. the user accepted the existing unrestricted-shell warning;
4. no session is already running or starting.

The request is idempotent. Existing output buffering remains unchanged, so prompt bytes produced before frontend readiness are retained. Hiding the dock does not terminate a session. Exit and startup failure reuse the existing New Session button, relabelled to Restart Session or Retry as appropriate. Windows keeps the external-terminal fallback and never claims an embedded terminal.

## Start Note

The Start Note is a normal `LearningNote` using the existing table and unique constraint:

- `docset_id`: `__zealrn_system__`
- `docset_name`: `ZealRN`
- `page_key`: `start-note`
- `page_path`: empty
- `page_url`: empty
- `page_title`: `Start Note`

This reserved identity cannot collide with a downloaded docset and requires no migration. `LearningNotesPanel` reuses the existing editor, autosave, preview, zoom, find, All Notes, and export behavior. It gains a Start Note action and a guarded Clear action. Invalid or non-docset pages select Start Note instead of disabling the editor. A real documentation page still selects its page-linked note after flushing pending Start Note changes.

## Onboarding And Help

A native `QWizard` provides the seven requested Quick Tour pages. Startup display is controlled by settings that distinguish whether the tour has ever been shown, whether it was completed, and whether it was explicitly requested for the next launch. Closing or skipping does not mark completion; it also does not repeatedly interrupt every launch unless requested.

One native Help dialog contains offline Getting Started, keyboard shortcuts, notes/backups, playground, terminal, and docset sections. Report a Problem points only to `abnzrdev/zealrn` and offers sanitized diagnostics containing version, OS, Qt version, terminal capability, database schema, and docset count.

## Welcome And Checklist

The existing `qrc` welcome page gains local action cards for Docset Library, Start Note, Web Playground, Developer Terminal, Quick Tour, note import, and Help. The existing `WebBridge` remains the only page-to-application API.

The compact checklist is persisted in `Core::Settings`. Completion is recorded only for reliable in-app events: existing/downloaded docsets, Start Note save, docset page open, page-note save, playground open, terminal open, and note backup/export. It is dismissible and resettable. No network is required.

## Preferences And Tips

The General preferences page gains startup/onboarding options and reset buttons. Invalid persisted values use safe defaults. Small dismissible first-use banners use the same settings object and appear in Learning Notes, Web Playground, and Developer Terminal. Resetting tips clears only their acknowledgement keys.

## Data Safety

- No SQLite schema change.
- Every page switch flushes the current note before replacing editor content.
- Failed saves retain editor text and dirty state.
- Production validation starts with temporary profiles.
- Existing user data is backed up and checksummed before installed-app testing.
- Original Flatpak Zeal data is never mounted or modified.

## Release Gate

Feature tests and alpha-package runtime checks run before changing the version to `0.1.0`. After the version change, Linux and Windows builds/tests/package checks run again. Windows packages document external terminal behavior. No tag, release, or upstream push is created.
