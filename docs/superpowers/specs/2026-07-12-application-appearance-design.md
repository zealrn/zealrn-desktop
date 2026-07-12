# Application Appearance Settings Design

## Scope

Complete Zeal's existing application appearance path for System, Light, and Dark modes. The work covers Qt Widgets, existing documentation WebEngine pages, and built-in pages. It does not add CodeMirror, a playground preview, code execution, packaging, branding, or per-widget theme stylesheets.

## Existing Architecture

Zeal already has the correct ownership boundaries:

- `Core::Settings::ContentAppearance` stores `Automatic`, `Light`, or `Dark`; `Automatic` is the existing internal name for user-facing System mode.
- Preferences already exposes three appearance radio buttons and persists the enum through `QSettings`.
- `Settings::isDarkModeEnabled()` resolves the effective mode.
- `ProxyStyle` and palette-aware widgets provide application-wide widget rendering.
- `Browser::Settings` and `WebPage` apply `ForceDarkMode` to docset content on Qt 6.7+.
- `WebPage` updates built-in `qrc` pages through their existing `data-theme` support and keeps their WebEngine background synchronized with the application palette.
- Built-in welcome and not-found resources already contain light and dark design tokens.

The implementation extends these paths rather than adding a second theme service.

## Approaches Considered

1. Extend `Core::Settings`, the existing startup style function, and current WebPage logic. This is selected because appearance state, update signals, style installation, and WebEngine theming already live there.
2. Add an `AppearanceManager` object. This would duplicate Settings signals and application-style ownership for one setting.
3. Add widget-specific stylesheets. This would bypass palette roles, create light-mode cleanup problems, and conflict with native platform styles.

## Preference And Persistence

Preferences changes the visible label from “Automatic” to “System” while preserving enum value `0` and the existing `content/appearance` key. System remains the default.

The `ContentAppearance` stream deserializer accepts only the three declared values. Any other numeric value becomes `Automatic`, giving malformed or future unsupported values a deterministic System fallback.

Focused tests use temporary INI-backed `QSettings` instances to verify:

- System, Light, and Dark serialize and deserialize unchanged.
- Reopening settings restores each saved value, representing application restart.
- An invalid serialized enum value restores as System.

## Widget Appearance

The existing style setup in `src/app/main.cpp` becomes the single widget appearance application point. It continues wrapping the selected base style in `ProxyStyle` so existing custom primitives and icons are preserved.

System mode uses the current platform style and platform-derived standard palette. On Qt 6.5+, system color-scheme changes trigger the existing Settings update path and reapply the application style. On Qt 6.4, where Qt cannot report color-scheme changes, System keeps the platform appearance detected at startup.

Explicit Light and Dark first use Qt's platform style and palette when their polarity matches the requested mode. If the platform cannot provide a matching palette, the application uses the installed Fusion style as a fallback and applies one centralized palette. This primarily covers Linux containers and incomplete platform themes without forcing Fusion when the native style already works.

The fallback palette sets semantic `QPalette` roles once for windows, text inputs, alternate rows, buttons, tooltips, selections, links, placeholder text, borders, and disabled foregrounds. Individual MainWindow, sidebar, tab, Learning Notes, Web Playground, dialog, menu, tooltip, scrollbar, and button widgets receive no hardcoded colors.

The appearance function is connected to `Settings::updated` and called once after Settings loads but before `WindowManager` creates the first MainWindow. This prevents a visible light window before a persisted dark setting is restored. Changing Preferences reapplies widgets immediately.

## Preferences Feedback

On Qt 6.4–6.6, changing the effective light/dark mode updates widgets and built-in pages immediately, but existing Qt WebEngine cannot switch downloaded documentation dynamically. Preferences shows a small, non-blocking label only when the effective mode changes:

> Restart ZealRN to apply the theme to documentation pages.

The notice is not a dialog and does not interrupt Apply or OK. Qt 6.7+ does not show it because `ForceDarkMode` updates docset pages live.

## WebEngine Behavior

Qt 6.7+ continues using the existing page-level `QWebEngineSettings::ForceDarkMode` behavior. Internal `qrc` pages keep `ForceDarkMode` disabled and use their authored theme tokens, preventing inversion of Zeal-owned pages.

Qt 6.4–6.6 retains Zeal's existing startup-only docset dark-mode configuration unchanged. No new Chromium flags are added. Changing effective appearance requires restart for downloaded documentation, as communicated by Preferences.

No downloaded docset file is modified, no persistent content injection is introduced, and no user-created Web Playground preview content exists or is recolored in this iteration.

## Update Flow

1. Settings loads `content/appearance`, normalizing invalid values to System.
2. Before the first MainWindow, startup applies the resolved style and palette.
3. Browser settings and each WebPage read the same effective dark-mode result.
4. Applying Preferences saves the enum and emits the existing `updated` signal.
5. Widgets and built-in pages update immediately.
6. Qt 6.7+ docset pages update immediately; Qt 6.4–6.6 show the restart notice when required.

## Error And Fallback Behavior

- Missing appearance: System.
- Invalid serialized appearance: System.
- Unknown system scheme: use the current platform palette; in the Docker environment this resolves to light.
- Explicit mode with unusable platform palette: use Fusion plus the centralized matching palette.
- Failure to switch downloaded documentation live on Qt 6.4–6.6: retain the current page and show the non-blocking restart notice.

## Validation

Docker validation covers Release and Testing builds plus all existing and new tests. Isolated runtime profiles verify System, Light, and Dark appearance, restart restoration, immediate widget and built-in-page changes, Learning Notes readability, Web Playground shell readability, unchanged sidebar/splitter geometry, Qt 6.4 documentation behavior after restart, and invalid-value fallback.

## Non-Goals

No CodeMirror, HTML/CSS/JavaScript execution, preview WebEngine, console collection, Python playground, packaging, branding, docset mutation, theme injection, or unrelated refactoring.
