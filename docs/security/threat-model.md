# ZealRN 0.1.0 Threat Model

## Scope and assets

This model covers the desktop application, its Linux and Windows packages, and the GitHub-based update notification flow. The primary assets are:

- learning notes and their SQLite database;
- installed docsets and user-selected export locations;
- application settings and local session state;
- the user's filesystem and processes reachable through Developer Terminal;
- package and update identity for `zealrn/zealrn-desktop`.

ZealRN is a local, single-user desktop application. It has no accounts, cloud sync, telemetry, remote terminal, or background installer.

## Trust boundaries

| Boundary | Trusted side | Untrusted input | Main controls |
|---|---|---|---|
| Docset catalog/download | ZealRN and OS trust store | HTTPS responses and archives | TLS verification, redirect policy, response validation, temporary files, contained extraction |
| Update notification | Canonical repository constant | GitHub release JSON | HTTPS, bounded response, semantic-version validation, trusted release URLs, notification only |
| Documentation WebEngine | Application profile and local docset server | Downloaded documentation HTML | Centralized navigation/profile policy; docset files are not modified |
| Web Playground | Dedicated off-the-record profile | User HTML/CSS/JavaScript | Request interceptor, no persistence, denied navigation/popups/permissions, bounded console |
| Terminal frontend | Local Qt resource and PTY backend | Terminal output and keyboard input | Resource-only WebEngine view, minimal Base64 WebChannel bridge, no network server |
| Notes/imports | SQLite store and export code | User text and selected files | Prepared SQL, versioned schema, atomic writes, validated imports, escaped PDF/Markdown metadata |
| Filesystem exports | User-selected destination | Filenames and note metadata | Sanitized names, overwrite confirmation, temporary output before replacement |
| Build/release | Owned GitHub repository and CI | Dependencies and workflow inputs | CodeQL, Dependabot, secret scanning, push protection, checksums |

## Terminal boundary

Developer Terminal is intentionally not a sandbox. On Linux it launches a normal local PTY shell with the user's permissions. Commands can read, modify, or delete any data available to that user. ZealRN requires first-use acknowledgement, does not auto-paste or auto-execute commands, and does not elevate privileges. Windows 0.1.0 opens a selected external terminal; it does not embed ConPTY.

The terminal WebEngine frontend is only a renderer. It loads local Qt resources, has no remote server, and exposes only terminal input, resize, session lifecycle, clipboard requests, and encoded output through WebChannel.

## Update and package trust

The application checks GitHub releases for `zealrn/zealrn-desktop` and never upstream Zeal. Checks send ordinary HTTP metadata such as IP address and user agent to GitHub, but no account identifier, note content, docset list, or telemetry identifier. A release response can only produce a notification and open a verified GitHub release page. ZealRN does not download, execute, or install an update automatically.

Release packages are currently unsigned. SHA256 checksums provide corruption detection but not publisher authentication when obtained from the same untrusted channel. Windows SmartScreen and Linux desktop environments may warn about unsigned packages.

## Residual risks

- User JavaScript may consume CPU or memory inside the isolated playground; Stop/reload is best effort.
- Large but valid docset archives can consume substantial disk space. Extraction rejects traversal and special-file entries, but 0.1.0 does not impose a universal decompression-size cap because docset sizes vary widely.
- Downloaded documentation is third-party HTML rendered locally. ZealRN does not guarantee that every docset is benign.
- A compromised operating system, user account, shell startup file, or system trust store is outside the application boundary.
- Unsigned packages remain vulnerable to distribution-channel substitution unless users verify checksums through a trusted channel.

## Security invariants

- TLS errors are never ignored for catalogs or updates.
- Archives cannot write outside their selected extraction directory.
- Update metadata never starts a process or installer.
- Notes are not uploaded by ZealRN.
- Playground content cannot access the documentation profile, note database, or terminal bridge.
- Terminal commands run only after explicit user acknowledgement and input.
