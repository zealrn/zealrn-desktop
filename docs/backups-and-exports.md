# Notes Backups And Exports

Current notes can be exported as UTF-8 Markdown, searchable text-based PDF, or versioned JSON. **Export All Notes** creates a ZIP with one Markdown file per note and a JSON index.

**Backup Database** checkpoints SQLite before copying `learning-notes.sqlite`. Keep backups outside the ZealRN data directory when practical.

Default Linux locations are:

- Settings: `~/.config/abnzrdev/ZealRN.conf`
- Notes and data: `~/.local/share/abnzrdev/ZealRN/`
- Session layout: `~/.local/state/zealrn/session.toml`

The user-local uninstaller and system package removal leave settings, notes, exports, and docsets in place. Clearing these directories manually is destructive, so export a backup first.
