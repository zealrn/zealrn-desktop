# Learning Notes Release Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver page-linked local notes, exports, ZealRN identity, Linux packages, and a safe user-local installation.

**Architecture:** `MainWindow` derives stable page context and delegates all editor state to `LearningNotesPanel`; a GUI-thread `LearningNotesStore` owns QSQLITE. Export helpers remain UI-independent. Existing CMake install rules feed both CPack DEB and upstream-style linuxdeploy AppImage packaging.

**Tech Stack:** C++23, Qt 6 Widgets/WebEngine/Sql/PrintSupport/Test, SQLite via QSQLITE, libarchive, CMake/CPack, linuxdeploy.

## Global Constraints

- Work only on `feature/learning-notes-release`; do not push or publish.
- Preserve documentation, Web Playground, appearance, and terminal behavior.
- Store notes only under `QStandardPaths::AppDataLocation`.
- Keep `.codegraph/`, build output, databases, exports, and `dist/` unstaged.
- Use explicit staging and small commits.

---

### Task 1: Stable Page Context And Store

**Files:**
- Create: `src/libs/ui/learningnotepage.h`
- Create: `src/libs/ui/learningnotesstore.h`
- Create: `src/libs/ui/learningnotesstore.cpp`
- Create: `src/libs/ui/tests/learningnotesstore_test.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `src/libs/ui/tests/CMakeLists.txt`

**Interfaces:**
- Produces: `LearningNotePage::fromUrl(const Registry::Docset *, const QUrl &, const QString &)` and store CRUD/search methods using `LearningNote` values.

- [ ] Write tests for normalization, separate docsets, migration, CRUD, uniqueness, Unicode, search, and rollback.
- [ ] Run the focused test and confirm it fails before implementation.
- [ ] Implement the value types and minimal QSQLITE store with schema version 1.
- [ ] Run the focused test and all UI tests.
- [ ] Commit `feat: persist learning notes in sqlite`.

### Task 2: Page-Linked Editor And Selection

**Files:**
- Modify: `src/libs/ui/learningnotespanel.h`
- Modify: `src/libs/ui/learningnotespanel.cpp`
- Modify: `src/libs/ui/mainwindow.h`
- Modify: `src/libs/ui/mainwindow.cpp`
- Modify: `src/libs/ui/browsertab.h`
- Modify: `src/libs/browser/webcontrol.h`
- Modify: `src/libs/browser/webcontrol.cpp`
- Create: `src/libs/ui/tests/learningnotespanel_test.cpp`

**Interfaces:**
- Consumes: `LearningNotePage`, `LearningNotesStore`.
- Produces: page switching, save/flush, `appendSelection(QString)`, and reopen request signals.

- [ ] Write tests for status, dirty state, autosave, page switch, flush failure, and selection formatting.
- [ ] Run the focused test and confirm failure.
- [ ] Add metadata, Save/Add Selection/All Notes/Export controls, Ctrl+S, and 1000 ms autosave.
- [ ] Forward selected text and active page changes from browser/MainWindow without modifying doc pages.
- [ ] Run focused tests and a runtime page-switch check.
- [ ] Commit `feat: connect notes to documentation pages`.

### Task 3: All Notes Browser

**Files:**
- Create: `src/libs/ui/allnotesdialog.h`
- Create: `src/libs/ui/allnotesdialog.cpp`
- Modify: `src/libs/ui/learningnotespanel.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `src/libs/ui/tests/learningnotesstore_test.cpp`

**Interfaces:**
- Consumes: store search/update/delete and page values.
- Produces: `openDocumentationRequested(LearningNotePage)`.

- [ ] Extend tests for title/docset/path/content search and delete.
- [ ] Implement the debounced search/list/editor dialog with delete confirmation.
- [ ] Wire reopen and current-note refresh.
- [ ] Run focused and UI tests.
- [ ] Commit `feat: add all notes search`.

### Task 4: Note Exports

**Files:**
- Create: `src/libs/ui/learningnotesexporter.h`
- Create: `src/libs/ui/learningnotesexporter.cpp`
- Create: `src/libs/ui/tests/learningnotesexporter_test.cpp`
- Modify: `src/libs/ui/learningnotespanel.cpp`
- Modify: `src/libs/ui/allnotesdialog.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`
- Modify: `src/libs/ui/tests/CMakeLists.txt`

**Interfaces:**
- Produces: filename sanitization and Markdown, JSON, PDF, ZIP, and database-backup functions with explicit errors.

- [ ] Write failing tests for Unicode, sanitization, duplicate names, overwrite refusal, archive contents, PDF text, and backup consistency.
- [ ] Implement atomic Markdown/JSON output, QTextDocument PDF, and libarchive ZIP.
- [ ] Add current/all export menus and overwrite confirmation.
- [ ] Run tests and validate PDF with `pdftotext`.
- [ ] Commit `feat: export learning notes`.

### Task 5: ZealRN Identity And Existing Docsets

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `src/app/main.cpp`
- Modify: `src/app/CMakeLists.txt`
- Modify: `src/libs/ui/mainwindow.cpp`
- Modify: `src/libs/ui/aboutdialog.ui`
- Create/modify: `assets/freedesktop/io.github.abnzrdev.zealrn.desktop`
- Create/modify: `assets/freedesktop/io.github.abnzrdev.zealrn.appdata.xml.in`
- Modify: `assets/freedesktop/CMakeLists.txt`
- Add focused startup/settings tests under `src/libs/core/tests/`.

**Interfaces:**
- Produces: separate identity and first-run docset reuse chooser.

- [ ] Write tests for candidate detection and independent settings identity.
- [ ] Set identity before settings access and update visible/install metadata.
- [ ] Add first-run reuse/custom/empty selection without copying data.
- [ ] Build and verify original Zeal settings/data are unchanged.
- [ ] Commit `chore: add zealrn application identity`.

### Task 6: Linux Packaging And User Installer

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `.gitignore`
- Create: `scripts/package-appimage.sh`
- Create: `scripts/package-deb.sh`
- Create: `scripts/install-user-local.sh`
- Create: `scripts/uninstall-user-local.sh`
- Modify: `README.md`

**Interfaces:**
- Produces: ignored `dist/` AppImage, DEB, checksums, and idempotent user-local install scripts.

- [ ] Add CPack DEB metadata and upstream-style linuxdeploy script.
- [ ] Add checksum-verifying installer/uninstaller with replacement backup.
- [ ] Run release/testing builds and all CTest tests in Ubuntu 24.04 Docker.
- [ ] Build packages; inspect DEB and run AppImage with a clean home.
- [ ] Stop only `zealrn-preview`, install user-locally, run acceptance tests, uninstall/reinstall, and leave ZealRN installed.
- [ ] Confirm no containers remain and original Zeal is untouched.
- [ ] Commit packaging scripts as `build: add linux release packaging`, installer as `build: add user-local installer`, and docs as `docs: prepare zealrn 0.1 alpha`.
