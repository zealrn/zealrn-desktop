# Application Appearance Settings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete Zeal's existing System, Light, and Dark appearance path with a Linux/Docker widget fallback, safe persistence, and version-aware WebEngine behavior.

**Architecture:** Keep `Core::Settings` as the appearance source of truth, apply widget style and palette centrally in `src/app/main.cpp`, and reuse existing `Browser::Settings`/`WebPage` theme handling. Preferences uses the existing radio controls and adds only a conditional restart notice for Qt 6.4–6.6.

**Tech Stack:** C++20, Qt 6 Widgets, Qt WebEngine, QSettings, Qt Test, CMake/Ninja, Ubuntu 24.04 Docker/Xvfb.

## Global Constraints

- Preserve the existing `ContentAppearance` enum values and `content/appearance` key.
- User-facing choices are System, Light, and Dark; System remains the default.
- Add no per-widget color theme, WebEngine content injection, Chromium flag, CodeMirror, execution, packaging, or branding work.
- Keep `.codegraph/`, core dumps, build output, screenshots, and runtime profiles unstaged.
- Create exactly one final commit: `feat: add application appearance settings`.

---

### Task 1: Appearance Persistence Tests

**Files:**
- Create: `src/libs/core/tests/settings_test.cpp`
- Modify: `src/libs/core/tests/CMakeLists.txt`

**Interfaces:**
- Tests existing `Settings::ContentAppearance` stream operators and QSettings persistence.

- [x] Add QtTest cases that round-trip all three enum values through `QDataStream`, reopen temporary INI `QSettings` to verify restart restoration, and expect invalid value `99` to become `Automatic`.
- [x] Register `settings_test` beside `session_test`, build it, and verify RED because invalid values currently survive deserialization.

### Task 2: Settings Validation And System Updates

**Files:**
- Modify: `src/libs/core/settings.h`
- Modify: `src/libs/core/settings.cpp`

**Interfaces:**
- Produce: invalid appearance fallback to `Automatic`.
- Produce: `isDocumentationThemeRestartRequired() const`.
- Preserve: `isDarkModeEnabled()` as the effective theme query.

- [x] Normalize invalid values in the stream operator and after QSettings load.
- [x] Cache the pre-Qt-6.5 system palette polarity before custom palettes are applied.
- [x] React to `QStyleHints::colorSchemeChanged` on Qt 6.5+ in System mode by emitting `updated()`.
- [x] Store the startup effective docset theme and compare it to the current effective theme on Qt 6.4–6.6.
- [x] Rebuild and run `settings_test` GREEN.

### Task 3: Central Widget Appearance

**Files:**
- Modify: `src/app/main.cpp`
- Modify: `src/libs/ui/widgets/searchedit.cpp`

**Interfaces:**
- Consume: `Settings::contentAppearance`, `isDarkModeEnabled()`, and `updated()`.
- Preserve: `ProxyStyle` and native style where its palette matches.
- Fallback: Fusion plus one centralized semantic palette when explicit mode cannot be represented by the native palette.

- [x] Move application style application until after Settings loads but before `WindowManager` creates MainWindow.
- [x] Reinstall native `ProxyStyle` for System; for explicit mismatches use Fusion and centralized Light/Dark `QPalette` roles including disabled and placeholder text.
- [x] Connect `Settings::updated` to immediate style/palette reapplication.
- [x] Keep the Windows erase filter synchronized with the resolved setting.
- [x] Replace the search completion label's hardcoded gray with `QPalette::PlaceholderText`.

### Task 4: Preferences Feedback

**Files:**
- Modify: `src/libs/ui/settingsdialog.ui`
- Modify: `src/libs/ui/settingsdialog.cpp`

**Interfaces:**
- Rename only the visible Automatic label to System.
- Add `appearanceRestartLabel`, hidden unless `isDocumentationThemeRestartRequired()` is true on Qt 6.4–6.6.

- [x] Add the translated non-blocking message: `Restart ZealRN to apply the theme to documentation pages.`
- [x] Remove the old broad “Appearance (requires restart)” label mutation.
- [x] Update the message after applying a changed appearance; keep Apply/OK non-blocking.

### Task 5: Verify And Commit

**Files:**
- Include the approved design and this plan in the single final commit.

- [x] Configure/build Release and Testing in `zealrn-ubuntu24-qt:latest`; run `ctest --preset testing --output-on-failure`.
- [x] Under isolated Xvfb profiles, verify System, Light, and Dark; immediate widget/built-in-page updates; Qt 6.4 restart notice/docset restart behavior; restart restoration; readable Learning Notes and Web Playground; unchanged splitter layout; invalid fallback.
- [x] Review `git status --short`, focused diff, ignored files, and staged file list.
- [x] Stage explicit files only and commit `feat: add application appearance settings`; do not push.
