# Offline Web Playground Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver a usable offline HTML/CSS/JavaScript playground in the existing bottom dock.

**Architecture:** One lazy resource-backed CodeMirror WebEngine hosts three independent editor states. A separate off-the-record preview profile executes base64-transported code behind strict navigation, request, popup, and permission controls. Pure C++ helpers own composition/export rules and focused tests.

**Tech Stack:** C++20, Qt 6.4 Widgets/WebEngine/WebChannel/Test, CodeMirror 6, esbuild, CMake/Ninja, Docker/Xvfb.

## Task 1: Local Editor Assets

**Create:** `tools/playground/package.json`, lockfile, `src/index.js`, `README.md`, `THIRD_PARTY_LICENSES.md`.

**Create:** `src/app/resources/playground/editor.html`, generated `editor.bundle.js`, `preview.html`.

**Modify:** `src/app/zeal.qrc`, `.gitignore` if required.

- [x] Pin only required CodeMirror/esbuild packages and generate the lockfile.
- [x] Implement three independent editor states, language support, requested editing features, search, and theme compartment.
- [x] Add the narrow asynchronous editor API and readiness/change bridge.
- [x] Bundle, verify no network references, remove `node_modules`, register resources, build, review, and commit.

## Task 2: Testable Playground Core

**Create:** `src/libs/ui/webplaygroundproject.h/.cpp`, `src/libs/ui/tests/webplayground_test.cpp`, test CMake.

**Modify:** `src/libs/ui/CMakeLists.txt`.

- [x] Add focused tests for starter/reset values, base64 composition, Unicode, closing tags, blocked schemes, console trimming, project output, and overwrite refusal.
- [x] Implement the smallest pure helpers and make tests green.
- [x] Run all tests, review, and commit.

## Task 3: Lazy Editor Integration

**Create:** `src/libs/ui/webplaygroundeditor.h/.cpp`.

**Modify:** `src/libs/ui/webplaygroundpanel.h/.cpp`, UI CMake.

- [x] Create the editor profile/view only on first panel show.
- [x] Wire QWebChannel readiness/change notifications and asynchronous snapshots.
- [x] Preserve three contents and histories across tab, hide/show, resize, and theme updates.
- [x] Enable Run/Auto Run/Reset only when ready; build, smoke test, review, and commit.

## Task 4: Isolated Preview And Console

**Create:** `src/libs/ui/webplaygroundpreview.h/.cpp`.

**Modify:** `src/libs/ui/webplaygroundpanel.h/.cpp`, UI CMake.

- [x] Add independent off-the-record profile, interceptor, restricted page, and preview view.
- [x] Add fixed base64 render calls, manual Run, 600 ms Auto Run, stale-run cancellation, Stop, and Reset.
- [x] Capture bounded and throttled console/error output and implement Clear Console.
- [x] Verify request blocking and difficult input composition, build/test, review, and commit.

## Task 5: Theme, Open, And Export

**Modify:** `src/libs/ui/webplaygroundpanel.h/.cpp`, editor wrapper, project helper/tests.

- [ ] Reconfigure editor theme on Settings updates without replacing editor state.
- [ ] Implement retained temporary standalone output and QDesktopServices launch warning.
- [ ] Implement destination selection, three-file UTF-8 export, overwrite confirmation/refusal, and clear errors.
- [ ] Build/test, review, and commit.

## Task 6: Final Validation

- [ ] Configure/build Release and Testing in the established Ubuntu 24.04 image; run all tests.
- [ ] Remove `node_modules` and validate builds/runtime again without Node.js.
- [ ] Run Xvfb checks for every requested editor, preview, console, theme, open/export, and isolation behavior.
- [ ] Confirm documentation and Learning Notes remain functional.
- [ ] Audit status/diff/staging; keep `.codegraph/` and generated runtime files unstaged; do not push.
