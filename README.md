# ZealRN

**An offline, interactive learning workspace for developers.**

ZealRN extends the [Zeal](https://zealdocs.org/) documentation browser into a complete learning environment. Instead of only searching and reading documentation, users can read, practise commands, and take notes without switching between multiple applications.

## Why ZealRN?

Learning programming usually requires several separate tools:

- A browser or documentation reader
- A terminal for testing commands
- A notepad for saving explanations and examples
- A code editor for practice

ZealRN brings these learning tools into one focused desktop workspace.

## Main Features

### Offline Documentation

Search and read programming documentation locally without depending on an internet connection.

### Integrated Terminal

Run commands and practise examples while reading the related documentation.

Linux includes an embedded terminal. Windows opens an external terminal such as Windows Terminal, PowerShell, Command Prompt, or Git Bash.

### Built-in Notepad

Write page-linked notes, save useful examples, and record what you learn without leaving the application.

Notes can be exported as Markdown, PDF, JSON, ZIP, or a database backup and shared with AI agents as project context.

### Web Playground

Practise HTML, CSS, and JavaScript with a built-in editor, preview, and console.

### Focused Learning Workspace

Keep documentation, practice, and personal notes together in one environment with fewer distractions.

## How It Is Different from Zeal

Zeal is an offline documentation browser.

ZealRN builds on that foundation and adds tools for active learning:

| Zeal | ZealRN |
| --- | --- |
| Read documentation | Read and actively practise |
| Search docsets | Search docsets while using a terminal |
| Reference tool | Interactive learning workspace |
| Documentation only | Documentation, terminal, playground, and notes |

## Project Goal

The goal of ZealRN is to help developers move from simply reading documentation to learning by doing.

A user should be able to:

1. Find a programming topic.
2. Read its documentation.
3. Test commands in the integrated terminal.
4. Save explanations and examples in the notepad.
5. Export useful context for AI agents.
6. Continue learning without changing applications.

## Built From Zeal

ZealRN is built from the open-source [Zeal documentation browser](https://zealdocs.org/). The original Zeal developers and contributors created the documentation browsing foundation.

ZealRN focuses on extending that foundation with interactive learning and productivity features. The original GPL license, copyright notices, contribution history, and attribution are preserved.

See [UPSTREAM.md](UPSTREAM.md) for details.

## Current Development

The main learning workflow includes:

- Offline documentation browsing
- An integrated Linux terminal
- External terminal support on Windows
- Page-linked learning notes
- Markdown, PDF, JSON, ZIP, and database exports
- A built-in Web Playground
- A unified learning interface

## Build and Installation

Linux requires Qt 6 with WebEngine, SQLite, PrintSupport, CMake, Ninja, and the project dependencies. The normal build flow is:

```sh
cmake --preset release -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
cmake --build --preset release
cmake --preset testing -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
cmake --build --preset testing
ctest --preset testing --output-on-failure
```

The application does not require Node.js or npm after installation. Node is only used to regenerate bundled developer assets.

## Local Data

- Settings: `~/.config/abnzrdev/ZealRN.conf`
- Notes and docsets: `~/.local/share/abnzrdev/ZealRN/`
- Cache: `~/.cache/abnzrdev/ZealRN/`

ZealRN uses a separate application identity from Zeal. It can reuse an existing docset directory without copying it, but it never automatically copies or deletes docsets.

## Privacy And Security

Notes, settings, exports, and downloaded docsets stay on the local device. ZealRN has no account or analytics service. Application update checks are notification-only and target the ZealRN repository; they never install software automatically. See [SECURITY.md](SECURITY.md) and [UPSTREAM.md](UPSTREAM.md).

## Roadmap

Public package releases, broader documentation guidance, and further Windows integration are planned. The Web app remains a lightweight trial experience rather than a full replacement for Desktop.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Please report active security issues privately using [GitHub security advisories](https://github.com/zealrn/zealrn-desktop/security/advisories/new).

## Based On Zeal

ZealRN is independently maintained and based on [Zeal](https://github.com/zealdocs/zeal). It preserves the upstream GPL licensing, copyright, and attribution. ZealRN is not officially endorsed by the upstream Zeal project. See [UPSTREAM.md](UPSTREAM.md).

## License

ZealRN is available under [GPL-3.0-or-later](COPYING). Third-party notices are included with the source and installed application.
