# Offline Web Playground Design

## Scope

Turn the existing bottom dock shell into a memory-only HTML/CSS/JavaScript playground. Documentation browsing, Learning Notes, application identity, packaging, and persistence remain unchanged.

## Architecture

`WebPlaygroundPanel` remains the owner of controls and orchestration. It lazily creates exactly two WebEngine surfaces when the dock is first shown:

- one editor view backed by a dedicated off-the-record profile and a resource-only CodeMirror frontend;
- one preview view backed by a different off-the-record profile, page, and request interceptor.

MainWindow only creates the panel and dock as it does today. It does not own editor text, preview state, export logic, or WebEngine profiles.

`WebPlaygroundEditor` wraps the editor WebEngine and its minimal QWebChannel bridge. The bridge exposes readiness and content-change notifications only. Content reads, replacements, tab selection, and theme changes use asynchronous JavaScript calls to a narrow `window.zealrnEditor` API. Three independent CodeMirror `EditorState` instances preserve content and undo history across tab, dock visibility, resize, and theme changes.

`WebPlaygroundPreview` owns the preview profile, interceptor, page, and view. The profile has no storage or persistent cache. Its page rejects external navigation, popups, and permissions. The interceptor blocks network and filesystem schemes while allowing only the resource-backed preview shell and its in-document data/blob operations.

`webplaygroundproject` contains pure helpers for starter content, encoded preview payloads, standalone project files, overwrite checks, and export. These helpers are covered by focused Qt tests.

## Local CodeMirror Assets

Developer sources and npm metadata live in `tools/playground/`. Exact dependency versions and `package-lock.json` make regeneration deterministic. `npm run bundle` invokes pinned esbuild and writes a single runtime bundle to `src/app/resources/playground/editor.bundle.js`.

The bundle imports only CodeMirror state, view, language, commands, search, HTML, CSS, JavaScript, and the official One Dark theme. `node_modules` is never committed. The application loads committed HTML/JS/CSS through Qt resources and has no Node.js or network runtime dependency. Third-party package names, versions, licenses, and source links are recorded beside the assets.

## Editor Data Flow

On the dock's first `showEvent`, the panel creates the editor and preview. The editor loads `qrc:/playground/editor.html`, creates three states with starter text, establishes QWebChannel, and reports readiness asynchronously. The panel enables controls only after readiness.

Run requests one snapshot containing all three documents. When the asynchronous result arrives, the panel ignores it if the dock was hidden or a newer run superseded it. Auto Run uses one 600 ms single-shot timer and is off by default. Editor changes restart the timer only while visible.

Reset compares the current snapshot to starter content. Unchanged content resets immediately; changed content asks once, then replaces all documents, clears preview and console, and retains no old execution state.

## Preview Composition And Isolation

The preview always loads `qrc:/playground/preview.html`. C++ converts each UTF-8 document to base64 and invokes a fixed `window.zealrnPreview.render(encodedHtml, encodedCss, encodedJavaScript)` function. Base64 contains no HTML delimiters, so raw user text never enters C++-constructed script/style markup.

The preview shell decodes UTF-8, parses HTML with `DOMParser`, replaces the preview head/body, appends CSS through `style.textContent`, and appends JavaScript through `script.textContent`. This safely transports Unicode, quotes, template literals, malformed input, and closing script/style tags. User code still executes and is not trusted.

Blocked schemes are `http`, `https`, `ftp`, `ws`, `wss`, and `file`. Navigation is restricted to the preview resource. Popups return null and all feature permissions are denied. The warning states: “Code runs locally inside an isolated preview. Do not run untrusted code.”

Stop calls `stop()` and reloads the resource shell. This recovers normal loads and many renderer failures, but Qt WebEngine cannot guarantee immediate termination of every hostile infinite loop.

## Console

The preview page converts WebEngine console callbacks into timestamped entries containing severity, message, line, and a short source identifier. The panel displays them in a bounded 500-row list and removes oldest rows before appending. Clear Console affects only the list. Reloading for Run or Reset clears stale output.

## Appearance

Qt controls inherit the application palette. The editor receives a theme reconfiguration call when `Core::Settings::updated` fires; a CodeMirror compartment switches between light and One Dark without rebuilding states. Preview content is never recolored by the application.

## Open And Export

Standalone output always contains UTF-8 `index.html`, `style.css`, and `script.js`; `index.html` references the other two files. Open in Browser writes to a `QTemporaryDir` retained by the panel and opens `index.html` with `QDesktopServices`, with a warning that browser execution is outside preview restrictions.

Export asks for a destination parent and project directory name. If any target file exists, one confirmation covers replacing those three files only. The helper refuses overwrite unless explicitly allowed and returns a precise error. No unrelated path is read or written.

## Validation

Tests cover starter/reset data, encoded preview composition including Unicode and closing tags, blocked schemes, console limit, standalone output, and overwrite refusal. Docker builds and Xvfb smoke tests cover lazy creation, editing, search, tabs/history, run/auto-run/stop/reset, console, theme changes, open/export, offline runtime, documentation, and Learning Notes.

## Known Limits

- Memory-only editor contents disappear when the process exits.
- Preview isolation reduces exposure but is not a perfect security sandbox.
- Stop cannot guarantee recovery from every renderer-level denial-of-service case.
- External browser execution is outside internal preview restrictions.

