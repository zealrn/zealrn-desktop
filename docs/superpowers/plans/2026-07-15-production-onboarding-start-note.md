# ZealRN 0.1.0 Production Onboarding Implementation Plan

## 1. Protect And Baseline

- Record repository/remotes/disk/auth state.
- Stop only confirmed ZealRN processes.
- Back up the installed AppImage and notes database; record integrity, checksum, note count, and docset counts.
- Work on `feature/production-onboarding-start-note`.

## 2. Terminal Startup (TDD)

- Add tests for frontend readiness, one-shot auto-start eligibility, invalid shell/directory fallback, retry state, and hide/show preservation.
- Expose `TerminalView::ready` from the existing bridge.
- Add an idempotent pending-start state to `DeveloperTerminalPanel`.
- Start after acknowledgement and readiness, focus after start, and relabel the session action for exit/error.
- Commit `fix: start embedded terminal when opened`.

## 3. Start Note (TDD)

- Add store/panel tests for the reserved identity, persistence, no collision, autosave, switching, failure preservation, and exports.
- Add `LearningNotePage::startNote()` and `isStartNote()`.
- Reuse `LearningNotesPanel::setPage()` for Start Note; add Start Note and Clear controls.
- Make `MainWindow::syncLearningNotes()` select Start Note for welcome/non-docset pages.
- Commit `feat: add persistent start note`.

## 4. Onboarding, Help, And Welcome (TDD)

- Add settings tests for defaults, invalid fallback, tour state, checklist, and tip reset.
- Add a native Quick Tour wizard and startup/manual entry points.
- Add one offline Help dialog and sanitized diagnostics.
- Expand welcome-page cards and route actions through `WebBridge`.
- Add persisted checklist state and reliable completion events.
- Add Preferences controls and contextual first-use banners.
- Commit in focused onboarding/help/checklist changes.

## 5. Alpha Validation

- Configure/build release and testing presets.
- Run all CTest tests with timeouts.
- Run clean-profile Xvfb checks for tour, Start Note persistence/switching, terminal prompt, playground, help, and exports.
- Recheck the backed-up and live user database before normal-profile testing.
- Push only this feature branch to `origin` for Windows CI and resolve actual failures.

## 6. Production Identity And Documentation

- Only after the alpha gate passes, change visible/package version to `0.1.0`.
- Remove remaining user-visible alpha labels and stale upstream links.
- Update README, release/testing guides, shortcuts, notes, terminal, and backup guidance.
- Re-run Linux and Windows tests.

## 7. Packages And Installation

- Build AppImage and DEB; validate contents and checksums.
- Build Windows portable ZIP and unsigned NSIS setup through GitHub Actions; download and verify artifacts.
- Back up the current user-local install, install the new AppImage without sudo, and run installed-app acceptance tests.
- Confirm notes/docsets/settings and original Zeal remain intact.
- Leave no test processes or containers and finish with only `.codegraph/` untracked.
