# Learning Notes MVP Design

## Scope

ZealRN stores one plain-text note per documentation page. Notes are local, page-linked, searchable, and exportable. Documentation browsing, Learning Notes, Web Playground, and Developer Terminal remain independent.

## Page Identity

`MainWindow` derives a page context from the active `BrowserTab`. `DocsetRegistry::docsetForUrl()` identifies the docset. The stable identity is the docset's internal name plus the normalized path relative to `Docset::baseUrl()`.

Normalization removes the temporary HTTP origin, port, query, and fragment, cleans path segments, and preserves case. Anchors on one document share a note; identical paths in different docsets do not. Welcome, error, and non-docset pages produce an invalid context and disable saving. Saved URLs are display metadata only; reopening rebuilds a URL from the currently loaded docset base URL and saved relative path.

## Persistence

`LearningNotesStore` owns one named Qt SQL connection on the GUI thread. The database is `QStandardPaths::AppDataLocation/learning-notes.sqlite`. It uses QSQLITE, prepared statements, `QSQLITE_BUSY_TIMEOUT=5000`, transactions for migrations, and `PRAGMA user_version`.

Schema version 1 contains `notes` with the requested page metadata, content, UTC timestamps, and `UNIQUE(docset_id, page_key)`, plus updated-time and docset indexes. Empty new drafts are not inserted. Errors are returned to the UI and never discard editor text.

## Editing Flow

`LearningNotesPanel` owns the editor, status, save/autosave timer, and page metadata display. `MainWindow` supplies page changes and selected documentation text. Before context changes or shutdown, the panel synchronously flushes a dirty valid note. A failed flush blocks replacing the editor context and reports `Save failed`, preventing cross-page overwrite.

Manual Save and Ctrl+S write immediately. Edits become `Unsaved` and autosave after 1000 ms. Selection capture appends a Markdown blockquote, preserves Unicode/newlines, and ignores an immediately repeated identical selection.

## All Notes

`AllNotesDialog` uses the same store connection. A debounced prepared `LIKE` query searches title, docset name, path, and content, ordered by newest update. The dialog supports preview/edit, delete confirmation, export, and reopening a page through a signal handled by `MainWindow`.

## Export

`LearningNotesExporter` provides deterministic filename sanitization and writes UTF-8 Markdown and versioned JSON with `QSaveFile`. Searchable PDF uses `QTextDocument` and `QPrinter::PdfFormat`; user text is inserted as escaped text. Export-all creates a temporary tree and archives it with the project's existing libarchive dependency, then atomically moves the completed ZIP. Database backup flushes pending writes, checkpoints SQLite, and copies to a user-selected destination without overwriting silently.

## Failure Handling

Database, migration, export, and reopen failures are visible and non-destructive. Delete and overwrite require confirmation. The SQLite file is never placed in the repository or installation tree. Tests use temporary data paths or explicit temporary database paths.

## Testing

Focused Qt tests cover page normalization, migrations and CRUD, autosave/page switching, selection formatting, search, Unicode, filename collisions, Markdown/JSON/ZIP/PDF output, overwrite refusal, and consistent backup. Runtime validation uses existing installed docsets and restart checks.
