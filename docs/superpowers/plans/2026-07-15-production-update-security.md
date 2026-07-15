# ZealRN Production Update And Security Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add notification-only ZealRN updates, complete the desktop/web security audit, fix confirmed findings, and produce validated 0.1.0 release candidates.

**Architecture:** Extend the existing `Core::Application`, `Core::Settings`, `WindowManager`, and Preferences paths. Keep security automation repository-local and store raw scan output only under `/tmp`.

**Tech Stack:** Qt 6 Network/Widgets/Test, CMake/CTest, GitHub Actions/CodeQL/Dependabot, npm, Trivy, LLVM/GCC sanitizers, OWASP ZAP, Vite/React/TypeScript.

## Global Constraints

- Never query or push to `zealdocs/zeal` for ZealRN updates.
- Never download or execute an installer automatically.
- Preserve existing notes, settings, docsets, exports, and original Flatpak Zeal data.
- Do not commit build output, packages, databases, raw scans, `node_modules`, or `.codegraph/`.
- Keep Windows terminals external for 0.1.0.
- Do not publish a release or create a tag.

---

### Task 1: Lock The Existing Production Baseline

**Files:**
- Verify: `src/libs/ui/developerterminalpanel.cpp`
- Verify: `src/libs/ui/learningnotespanel.cpp`
- Verify: `src/libs/ui/quicktourdialog.cpp`
- Verify: `src/libs/ui/helpdialog.cpp`

**Interfaces:**
- Consumes: existing terminal readiness, Start Note, onboarding, Help, and checklist implementations.
- Produces: a protected baseline requiring no duplicate implementation.

- [ ] Run existing terminal, Start Note, onboarding, Help, settings, and session tests.
- [ ] Confirm the live notes backup SHA256, schema, note count, and both docset counts.
- [ ] Record any baseline failure before changing updater or security code.

### Task 2: Add Secure Release Parsing And Scheduling With TDD

**Files:**
- Modify: `src/libs/core/application.h`
- Modify: `src/libs/core/application.cpp`
- Modify: `src/libs/core/settings.h`
- Modify: `src/libs/core/settings.cpp`
- Modify: `src/libs/core/tests/applicationupdates_test.cpp`
- Modify: `src/libs/core/tests/settings_test.cpp`

**Interfaces:**
- Produces: canonical repository URLs, validated release metadata, due-check logic, ETag cache state, and asynchronous secure requests.
- Consumes: `QNetworkAccessManager`, `QNetworkRequest`, `QNetworkReply`, `QVersionNumber`, and existing `Settings::save()`.

- [ ] Add failing tests for repository identity, stable/prerelease parsing, invalid JSON/URL/version/date, 304, 403, 404, response cap, skipped versions, invalid frequency, disabled checks, and daily/weekly due logic.
- [ ] Run the focused tests and confirm failures describe missing behavior.
- [ ] Implement the minimum parser/settings/request code: 15-second delay/timeout, HTTPS, safe redirects, 1 MiB cap, GitHub headers, ETag, and dedicated TLS-enforcing manager.
- [ ] Run focused tests and commit `feat: notify users about new releases`.

### Task 3: Add Update Preferences And Non-Modal UI

**Files:**
- Modify: `src/libs/ui/windowmanager.h`
- Modify: `src/libs/ui/windowmanager.cpp`
- Modify: `src/libs/ui/settingsdialog.h`
- Modify: `src/libs/ui/settingsdialog.cpp`
- Modify: `src/libs/ui/tests/helpdialog_test.cpp` or add focused update UI tests where practical.

**Interfaces:**
- Consumes: update result signals and persisted settings from Task 2.
- Produces: startup scheduling, manual Check Now, non-modal release actions, skip/remind behavior, and no duplicate notification.

- [ ] Add failing tests for startup delay, duplicate suppression, skipped releases, and manual/automatic messages.
- [ ] Add the Updates Preferences group and wire values through existing load/save paths.
- [ ] Replace modal update availability behavior with a non-modal `QMessageBox` using plain text and explicit buttons.
- [ ] Run focused tests and commit `feat: add update notification preferences`.

### Task 4: Complete Security Configuration

**Files:**
- Modify: `.github/workflows/analyze-codeql.yaml`
- Modify: `.github/dependabot.yml`
- Create: `SECURITY.md`
- Create in web: `.github/workflows/codeql.yml`
- Create in web: `.github/dependabot.yml`
- Create in web: `SECURITY.md`

**Interfaces:**
- Produces: C++ and JavaScript/TypeScript CodeQL plus grouped npm/Actions Dependabot coverage.

- [ ] Update desktop CodeQL to security-extended/security-and-quality with its real CMake build and manual dispatch.
- [ ] Add web JavaScript/TypeScript CodeQL with minimum permissions and manual dispatch.
- [ ] Add grouped Dependabot entries for Actions and all three npm roots.
- [ ] Verify repository Dependabot alerts, secret scanning, and push protection through `gh`; enable where supported.
- [ ] Commit focused security configuration in each repository.

### Task 5: Audit And Harden Desktop Attack Surfaces

**Files:**
- Modify only files implicated by confirmed findings.
- Test: `src/libs/core/tests/extractor_test.cpp`
- Test: updater, notes exporter/importer, playground, terminal, and diagnostics tests.

**Interfaces:**
- Consumes: untrusted release JSON, archives, notes imports, WebEngine content, clipboard, and terminal data.
- Produces: tests proving containment, TLS enforcement, bounded inputs/output, and no unintended execution.

- [ ] Run focused hostile-input tests for update JSON, archive traversal/links, note JSON sizing/escaping, playground requests/popups/permissions, terminal URL/bridge limits, and diagnostics redaction.
- [ ] Fix only confirmed defects at the shared boundary and add one regression test per defect.
- [ ] Commit each independent confirmed fix with a `security:` prefix.

### Task 6: Audit ZealRN Web

**Files:**
- Modify only confirmed vulnerable web files.
- Test: existing Vitest and Playwright suites plus focused regression tests.

**Interfaces:**
- Consumes: sandboxed iframe messages, imported JSON, IndexedDB, service-worker caches, and external URLs.
- Produces: bounded imports, CSP-compatible production output, safe message handling, and documented GitHub Pages limitations.

- [ ] Run `npm ci`, audit, lint, typecheck, unit tests, build, and Playwright.
- [ ] Review iframe sandbox/CSP, postMessage source/channel checks, backup validation, URLs, service worker, and workflow permissions.
- [ ] Run passive ZAP baseline against a local production preview.
- [ ] Fix confirmed application-owned findings and commit them on a web security branch.

### Task 7: Run Local Security Scanners And Sanitizers

**Files:**
- Create: `docs/security/threat-model.md`
- Create: `docs/security/audit-0.1.0.md`

**Interfaces:**
- Produces: reproducible tool/version summary, finding triage, fixes, accepted risks, and release decision.

- [ ] Run Trivy filesystem vulnerability/secret/misconfiguration/license scans for both repositories with JSON/SARIF under `/tmp`.
- [ ] Run `npm audit` in desktop playground, desktop terminal frontend, and web roots.
- [ ] Configure ASan/UBSan/LSan build and run focused desktop tests with timeouts.
- [ ] Triage every high/critical/secret result and write the concise audit report without private paths.
- [ ] Commit security documentation.

### Task 8: CI, Packages, Installation, And Final Gate

**Files:**
- Generated and ignored: `dist/**`, `build/**`

**Interfaces:**
- Consumes: all committed desktop/web changes and security findings.
- Produces: tested Linux AppImage/DEB and Windows ZIP/NSIS release candidates.

- [ ] Run fresh Linux release/testing builds and all CTest tests.
- [ ] Push only owned feature branches, dispatch desktop/web CodeQL and Windows CI, and inspect actual results.
- [ ] Rebuild Linux and Windows artifacts, verify checksums and deployed runtime contents.
- [ ] Back up and reinstall the AppImage user-locally; launch from `~/.local/bin/zealrn` using clean and normal profiles.
- [ ] Recheck notes SHA/content, schema, note count, ZealRN/original Zeal docset counts, processes, containers, and tracked trees.
- [ ] Keep version 0.1.0 only when every release blocker is cleared; otherwise document the blocker and retain release-candidate status.
