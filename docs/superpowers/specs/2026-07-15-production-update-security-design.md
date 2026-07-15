# ZealRN Production Update And Security Design

## Scope

This release candidate extends the existing 0.1.0 production-onboarding work with a notification-only updater and a documented security audit. The terminal auto-start, Start Note, Quick Tour, offline Help, checklist, contextual tips, Linux packages, and Windows packages already exist and remain unchanged except where verification exposes a defect.

The separate ZealRN Web repository receives only security configuration and confirmed security fixes. Its product layout, PWA storage model, and playground behavior are not redesigned.

## Options Considered

1. **Extend `Core::Application` and existing settings/UI paths (chosen).** This preserves the current update action, shared network architecture, and Preferences conventions. It is the smallest compatible change.
2. Add a general update-service framework. This would isolate the feature but creates an abstraction with one consumer and unnecessary lifecycle complexity.
3. Delegate updates to an external helper or package manager. This breaks portable builds, adds platform-specific behavior, and conflicts with notification-only requirements.

## Update Architecture

`Core::Application` remains the update owner. A dedicated `QNetworkAccessManager` is used for update requests so the legacy user option to ignore docset TLS errors can never weaken release checks.

The canonical repository slug is defined once as `abnzrdev/zealrn`. Stable checks use GitHub's latest published release endpoint. When prereleases are enabled, the client uses the repository releases collection and selects the newest valid non-draft semantic version. Requests use HTTPS, `NoLessSafeRedirectPolicy`, a 15-second timeout, GitHub JSON/API-version headers, `If-None-Match`, and a 1 MiB response cap.

The parser accepts only the expected object or array schema. It validates semantic versions, publication timestamps, and release URLs under `https://github.com/abnzrdev/zealrn/releases/`. Release body HTML is never rendered.

Settings store the automatic-check toggle, Daily/Weekly/Never frequency, prerelease channel, last attempt, last success, ETag, skipped version, and the last validated release metadata. Invalid persisted values fall back to enabled, Daily, stable-only behavior.

`WindowManager` schedules an automatic check 15 seconds after startup only when enabled and due. Automatic offline, timeout, 403, 404, malformed, or unchanged results remain silent. Manual checks explain no-release, current-version, and errors. A newer release opens a non-modal message with View Release, Download Page, Remind Me Later, Skip This Version, and Close actions. No asset is downloaded or executed.

## Existing First-Launch Features

The existing behavior is the release baseline:

- Developer Terminal waits for safety acknowledgement, xterm.js readiness, and PTY readiness before starting exactly one shell.
- Start Note uses reserved identity `__zealrn_system__` plus `start-note` in the existing SQLite schema.
- Quick Tour, offline Help, checklist, contextual tips, last-page restoration, and production identity remain in place.
- A welcome Import Notes action may import only versioned ZealRN JSON with a strict size limit and explicit conflict handling. It must not accept arbitrary archives or overwrite notes silently.

## Threat Model

### Protected Data

- SQLite notes and backups
- Downloaded docsets and settings
- User-selected export destinations
- Terminal input/output and inherited user permissions
- Release decision state such as skipped versions
- Browser IndexedDB notes in ZealRN Web

### Trusted Components

- Signed operating-system trust store and Qt TLS implementation
- Bundled application resources
- SQLite, libarchive, Qt WebEngine, xterm.js, CodeMirror, and pinned build dependencies
- GitHub HTTPS API responses only after schema and origin validation

### Untrusted Inputs

- Release API JSON and redirect targets
- Docset archives and metadata
- Imported note JSON
- Documentation HTML and links
- Web Playground user code and console messages
- Clipboard and terminal keystrokes
- Browser URL parameters, service-worker responses, and imported web backups

### Boundaries

- Update requests never share the TLS-ignore path and never execute assets.
- Docsets extract only regular files/directories contained by the destination; links and traversal entries are rejected or ignored.
- Documentation, terminal, and playground use distinct WebEngine profiles and minimal bridges.
- The terminal is intentionally unrestricted after explicit acknowledgement; it never auto-runs or auto-pastes commands.
- Desktop writes remain under `QStandardPaths` locations or explicit user-selected export paths.
- ZealRN Web playground code runs in a sandboxed iframe without same-origin access; browser notes remain in IndexedDB.

## Security Automation

- Desktop CodeQL keeps a real CMake C/C++ build and uses security-extended/security-and-quality queries.
- ZealRN Web receives JavaScript/TypeScript CodeQL.
- Dependabot covers GitHub Actions and each committed npm project with grouped low-noise updates and no auto-merge.
- Local Trivy scans cover vulnerabilities, secrets, misconfiguration, and licenses; raw reports remain outside repositories.
- Every npm project runs `npm audit` without automatic forced fixes.
- Linux sanitizer builds run ASan, UBSan, and compatible leak checks against focused tests.
- ZealRN Web receives a passive ZAP baseline scan against a local production preview.

## Finding Policy

Confirmed critical/high reachable vulnerabilities, committed secrets, path traversal, insecure TLS, unsafe update execution, and application-created sandbox escapes block 0.1.0. False positives and accepted risks require written justification in `docs/security/audit-0.1.0.md`. Tool warnings alone do not justify code changes.

## Release Gate

Version 0.1.0 remains valid only if desktop Linux tests, Windows CI, web tests, package validation, data-integrity checks, security scans, and sanitizer checks pass without an unresolved release blocker. Packages are rebuilt but not published or tagged.
