# Security Policy

## Supported versions

Security fixes are provided for the latest published ZealRN release and the current `main` branch. Older development builds and unmodified upstream Zeal releases are outside this project's support scope.

## Reporting a vulnerability

Do not open a public issue for an active vulnerability. Use [GitHub private vulnerability reporting](https://github.com/zealrn/zealrn-desktop/security/advisories/new) and include:

- the affected ZealRN version and operating system;
- a concise description of the impact;
- reproducible steps or a minimal proof of concept;
- any suggested mitigation, if known.

Reports are acknowledged as soon as practical. The project will validate the issue, assess severity, coordinate a fix, and credit the reporter when requested. Please allow time for a corrected build before public disclosure.

## Scope

In scope:

- ZealRN application and packaging code;
- docset download and extraction;
- notes, imports, exports, and local storage;
- WebEngine playground and terminal boundaries;
- update notifications and official build workflows.

Out of scope:

- vulnerabilities in an unmodified upstream Zeal build;
- commands a user intentionally runs in Developer Terminal;
- third-party documentation content itself;
- social engineering and denial-of-service reports without a product-specific flaw;
- unsigned alpha/test packages triggering operating-system reputation warnings.

Dependency vulnerabilities are welcome when they affect reachable ZealRN functionality. Include the package, affected version, advisory, and reachable code path where possible.

There is currently no bug bounty program. Do not test against systems or data you do not own or have permission to use.
