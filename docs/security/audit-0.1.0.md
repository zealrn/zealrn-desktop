# ZealRN 0.1.0 Security Audit

Date: 2026-07-15

## Scope

This audit covers the ZealRN desktop application, Linux and Windows release candidates, and the separately hosted ZealRN Web PWA. It reviews release updates, docset downloads and extraction, notes and backup import/export, WebEngine isolation, the local terminal, diagnostics, package contents, browser storage, the service worker, and repository automation.

The companion threat model is in `docs/security/threat-model.md`. Raw scanner output was kept outside the repositories because reports can contain local paths and request details.

## Tools And Results

| Tool | Version / environment | Result |
| --- | --- | --- |
| CodeQL | CLI 2.26.0, GitHub Actions | Desktop C++, JavaScript/TypeScript, and Actions jobs passed. Web JavaScript/TypeScript passed. No open alerts. |
| Trivy | 0.72.0 | Both repositories: 0 vulnerabilities, 0 secrets, and 0 misconfigurations. License inventories contained 24 desktop and 27 web records. |
| npm audit | npm 10.9.8, Node.js 22.23.1 | 0 vulnerabilities in the CodeMirror tooling, terminal frontend, and ZealRN Web. |
| ASan / UBSan | GCC 13.3, Ubuntu 24.04, Qt 6.4.2 | All 19 CTest targets passed with application checks enabled. |
| LeakSanitizer | GCC 13.3, Ubuntu 24.04, Qt 6.4.2 | 14 targets passed without reports. Five Qt WebEngine/fontconfig/SQLite tests reported process-lifetime framework allocations; see Accepted Risks. |
| OWASP ZAP | 2.17.0, local production preview | Passive-only plan found 30 URLs, 0 high, 2 medium, 1 low, and 1 informational alert. No active scan job was used. |
| CTest | Ubuntu 24.04, Qt 6.4.2 | 19/19 passed. |
| Vitest / Playwright | Chromium, Vite 8.1.4 | 39/39 unit tests and 8/8 browser tests passed, including offline and accessibility coverage. |
| Windows CI | Windows Server 2022, Qt 6.11.1, MSVC 2022 14.44 | Tests, deployment validation, portable ZIP, NSIS installer, and artifact checksums passed. |

## Confirmed Findings And Fixes

### Web playground isolation

The PWA playground previously used `srcdoc`, which inherited the application page's CSP and made the user-code boundary harder to reason about. It now loads a dedicated local runner page in a sandboxed iframe without `allow-same-origin`. The runner accepts source only from its parent and matching channel, creates user JavaScript through a blob URL, blocks network/frame/object/form destinations through its CSP, and does not receive application storage objects.

### Notes backup resource limits

Browser backup imports previously lacked explicit aggregate limits. Imports now reject JSON above 5 MiB, more than 10,000 notes, metadata above 4,096 characters, note content above 1 MiB, invalid timestamps, and duplicate page identities before writing anything.

### Release update boundary

The updater now uses only `abnzrdev/zealrn`, validates semantic versions and GitHub release URLs, limits responses to 1 MiB, uses HTTPS with safe redirects and a dedicated TLS-enforcing network manager, and stores validated ETags. It never downloads or executes an asset. A packaging default that disabled update notifications in release builds was corrected and covered by a default-on test.

### Repository security automation

Both repositories now have CodeQL, grouped Dependabot configuration, secret scanning, push protection, and private vulnerability reporting. Repository ownership metadata points to the ZealRN maintainer rather than upstream Zeal.

No confirmed critical or high-severity reachable vulnerability and no committed secret were found.

## Desktop Boundary Review

- Docset extraction canonicalizes paths, rejects absolute paths and traversal, writes only regular files/directories inside the destination, and does not materialize archive links or special files.
- The update checker has no shell or installer execution path and cannot inherit the optional docset TLS-ignore setting.
- Note imports validate structure before writes. Exports sanitize filenames, escape rich text used for PDF output, and write only to explicit destinations.
- Web Playground and Developer Terminal use separate off-the-record WebEngine profiles and local-resource request interceptors. Popups, permission requests, external navigation, and unrelated WebChannel objects are blocked.
- Terminal data crosses the bridge as bounded Base64 batches. Starting a shell requires acknowledgement; paste and command execution are never automatic. The terminal intentionally has normal user permissions.
- Copy Diagnostics contains only product version, operating system, Qt version, backend type, database schema, and docset count.

## ZAP Triage

The passive scan reported:

- `CSP: style-src unsafe-inline` (medium): accepted for 0.1.0. CodeMirror and application components create runtime styles. Script execution remains restricted to local assets, the playground is separately sandboxed, and no `unsafe-inline` script policy is present.
- `Missing Anti-clickjacking Header` (medium): accepted hosting limitation. GitHub Pages does not provide repository-controlled response headers, and `frame-ancestors` cannot be enforced from a CSP meta element. The PWA has no account, server session, privileged operation, or cloud-held note content.
- `X-Content-Type-Options Header Missing` (low): local Vite preview and GitHub Pages hosting limitation. Inspected responses used correct explicit content types. The application cannot configure Pages response headers.
- `Modern Web Application` (informational): expected.

These findings do not create a confirmed high-severity reachable vulnerability. A future deployment behind configurable response headers should add `frame-ancestors 'none'` and `X-Content-Type-Options: nosniff`.

## Accepted Risks And Limitations

- Release artifacts are not cryptographically signed. The published SHA-256 manifests detect accidental corruption but do not replace signing. The Windows installer is unsigned and SmartScreen may warn.
- The GitHub API and release page are trusted through operating-system TLS and strict repository URL validation. ZealRN does not implement an independent release-signing framework in 0.1.0.
- A user may explicitly enable the legacy ignore-SSL-errors option for docset traffic. Release update requests use a separate manager and always retain TLS verification.
- Docset archives do not have one global decompressed-size ceiling because legitimate documentation sets vary greatly. Extraction remains path-contained, partial work is temporary, and disk exhaustion is still possible.
- ASan's `new-delete-type-mismatch` check was disabled for the sanitizer run after it reproduced inside Qt WebEngine 6.4.2 destruction rather than application code. LSan reports from five GUI/database tests originate in process-lifetime Qt WebEngine, fontconfig, and SQLite allocations. No application-owned invalid access or undefined behavior was observed.
- Developer Terminal is intentionally not sandboxed. Its warning accurately states that commands have the same access as the user's normal shell. Windows 0.1.0 opens an external terminal; it does not claim an embedded backend.
- ZealRN Web notes remain in browser storage. Clearing site data can remove them, so regular exported backups remain necessary.

## Package And Data Validation

The AppImage starts with a clean profile, creates separate ZealRN settings/data paths, initializes SQLite, and does not load source-tree files. The user-local reinstall preserved the existing database byte-for-byte, including schema version 1 and five notes. Four ZealRN docsets and fifteen original Flatpak Zeal docsets remained unchanged.

The Debian package metadata and installed paths were inspected. The AppImage includes Qt WebEngine and SQLite runtime components. The Windows deployment includes `zealrn.exe`, Qt WebEngine process/resources, the SQLite driver, Visual C++ runtime dependencies, and bundled third-party license notices.

## Release Decision

No unresolved release-blocking security finding remains. ZealRN 0.1.0 is suitable as an unsigned release candidate for manual publication approval. Publication should preserve the generated SHA-256 manifests and clearly disclose the unsigned Windows installer, unrestricted local terminal, browser-storage backup requirement, and GitHub Pages response-header limitations.
