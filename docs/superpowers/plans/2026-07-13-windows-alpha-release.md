# Windows Alpha Release Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Install the corrected Linux alpha, isolate ZealRN update checks from upstream Zeal, and build validated Windows x64 portable and installer artifacts in GitHub Actions.

**Architecture:** Keep the existing Qt/CMake application architecture. Add testable GitHub release parsing inside Core, reuse Qt's generated Windows deployment install script, and package that installed tree with CPack ZIP and NSIS.

**Tech Stack:** C++20, Qt 6.11.1, CMake/CPack, vcpkg, MSVC 2022, NSIS, GitHub Actions, gh CLI.

## Global Constraints

- Work only on `feature/windows-alpha-release` and push only to `origin` (`abnzrdev/zealrn`).
- Never push to `upstream`, publish a release, or create a tag.
- Produce `ZealRN-0.1.0-alpha-win64-portable.zip`, `ZealRN-0.1.0-alpha-win64-setup.exe`, and `SHA256SUMS`.
- Keep Windows terminal support as the existing honest external-terminal fallback.
- Keep `.codegraph/`, build trees, and generated `dist/` artifacts untracked.

---

### Task 1: Preserve and reinstall Linux ZealRN

**Files:**
- Modify: `scripts/install-user-local.sh`
- Modify: `scripts/uninstall-user-local.sh`

- [x] **Step 1: Verify the AppImage SHA-256 and stop only confirmed ZealRN processes.**
- [x] **Step 2: Back up installed binaries and desktop integration.**
- [x] **Step 3: Run the existing uninstaller and verify notes remain byte-identical.**
- [x] **Step 4: Reinstall the verified AppImage and validate the desktop entry.**
- [x] **Step 5: Add and test automatic XCB fallback without changing normal launches.**
- [x] **Step 6: Commit the launcher adjustment with its shell self-check.**

### Task 2: Isolate application update identity

**Files:**
- Modify: `src/libs/core/application.h`
- Modify: `src/libs/core/application.cpp`
- Modify: `src/libs/core/tests/CMakeLists.txt`
- Create: `src/libs/core/tests/applicationupdates_test.cpp`
- Modify: `src/libs/ui/mainwindow.cpp`

- [x] **Step 1: Add failing tests for the `abnzrdev/zealrn` API URL, empty release lists, draft/prerelease filtering, stable versions, and malformed JSON.**
- [x] **Step 2: Run the focused test and confirm the expected failure.**
- [x] **Step 3: Implement the minimal parser and explicit no-releases signal.**
- [x] **Step 4: Connect manual result dialogs using ZealRN wording and links only.**
- [x] **Step 5: Run focused and full Linux tests.**
- [x] **Step 6: Commit `fix: use zealrn update identity`.**

### Task 3: Add Windows packaging

**Files:**
- Modify: `src/app/CMakeLists.txt`
- Create: `scripts/package-windows.ps1`
- Create: `scripts/validate-windows-package.ps1`

- [x] **Step 1: Configure the direct NSIS installer for current-user installation after CPack's admin-only template proved unsuitable.**
- [x] **Step 2: Add a PowerShell packager that stages release output, renames artifacts deterministically, and creates SHA-256 sums.**
- [x] **Step 3: Add deployment validation for required executable, Qt WebEngine, platform, SQLite, and license files plus source-path rejection.**
- [x] **Step 4: Validate CMake configuration and NSIS syntax locally where platform-independent checks permit.**
- [x] **Step 5: Commit `build: add windows installer packaging`.**

### Task 4: Add Windows GitHub Actions validation

**Files:**
- Create: `.github/workflows/windows-alpha.yml`

- [x] **Step 1: Add manual Windows 2022 workflow with Qt/vcpkg cache setup.**
- [x] **Step 2: Build release and testing configurations and run CTest.**
- [x] **Step 3: run `zealrn.exe --version`, package, validate, checksum, and upload the exact artifacts.**
- [x] **Step 4: Validate workflow syntax and commit `build: add windows release workflow`.**

### Task 5: Document manual Windows testing

**Files:**
- Create: `docs/releases/0.1.0-alpha-windows-testing.md`

- [x] **Step 1: Document checksum, portable, installer, persistence, docset, notes, export, playground, appearance, terminal fallback, and uninstall tests.**
- [x] **Step 2: Document Windows data locations and unsigned SmartScreen limitation.**
- [x] **Step 3: Commit `docs: add windows alpha testing guide`.**

### Task 6: Build and retrieve artifacts

**Files:**
- Generated only: `dist/windows/*` (ignored)

- [x] **Step 1: Run release/testing Linux builds and all tests.**
- [ ] **Step 2: Push only `feature/windows-alpha-release` to origin.**
- [ ] **Step 3: Dispatch and monitor `windows-alpha.yml`; inspect and fix root causes for any failures.**
- [ ] **Step 4: Download successful artifacts to `dist/windows/` and verify SHA-256 locally.**
- [ ] **Step 5: Launch the installed Linux app and verify preserved data/catalog behavior without modifying original Zeal.**
- [ ] **Step 6: Confirm a clean tracked tree, no running preview containers, no release, and no tag.**
