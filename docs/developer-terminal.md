# Developer Terminal

On Linux, opening **View > Show Developer Terminal** starts the saved valid shell, `$SHELL`, or the first available Bash, Zsh, Fish, or `sh` after the one-time safety acknowledgement. The embedded xterm.js view uses a native POSIX PTY. Hiding the dock preserves the session; closing ZealRN terminates it.

The terminal is not sandboxed. Commands have the same access as the normal user shell and can modify or delete files. ZealRN never runs documentation selections, pastes commands, or invokes `sudo` automatically.

On Windows 10 and 11, 0.1.0 uses external terminal integration. It detects Windows Terminal, PowerShell 7, Windows PowerShell, Command Prompt, and Git Bash. This release does not claim an embedded Windows terminal.

The Web Playground is different: its browser preview blocks external network requests and has no shell access.
