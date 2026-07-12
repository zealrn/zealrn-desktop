# Web Playground Shell Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a lightweight bottom Web Playground dock shell that starts hidden and safely restores its prior Qt main-window state.

**Architecture:** `MainWindow` owns one stable `QDockWidget` containing a self-contained `WebPlaygroundPanel`. The existing session TOML stores Qt's versioned main-window state as one optional blob, while the existing horizontal documentation splitter remains independent and unchanged.

**Tech Stack:** C++20, Qt 6 Widgets, QMainWindow/QDockWidget, Qt Test, CMake/Ninja, Ubuntu 24.04 Docker validation.

## Global Constraints

- Preserve `sidebar | documentation | Learning Notes` without changing its splitter behavior.
- First launch is hidden; valid later state restores visibility, bottom dock area, floating position, and relative size.
- Missing, malformed, or old-version state falls back to hidden without deleting settings.
- Add only a checkable View menu action; do not add persistent toolbar chrome.
- Keep all shell actions except Close Playground disabled and create no CodeMirror or WebEngine objects.
- Do not implement editing, execution, console capture, templates, export, branding, or packaging.
- Do not touch or stage `.codegraph/` or `core`; do not push.
- Create one final commit: `feat: add web playground shell`.

---

### Task 1: Persist Main Window Dock State

**Files:**
- Modify: `src/libs/core/tests/session_test.cpp`
- Modify: `src/libs/core/session.h`
- Modify: `src/libs/core/session.cpp`

**Interfaces:**
- Produces: `Core::WindowState::mainWindowState` as an optional `QByteArray` serialized under `main_window_state`.
- Preserves: existing `geometry`, `splitterState`, and `tocSplitterState` fields and legacy migration.

- [x] **Step 1: Write the failing round-trip and missing-key assertions**

Add `mainWindowState` test data to `saveThenLoad_roundTrip()` and assert that it round-trips. In `loadPartialFile_missingKeysUseDefaults()`, assert that an absent key yields an empty byte array:

```cpp
const QByteArray mainWindowState = QByteArray::fromHex("010203aabbcc");
ws.mainWindowState = mainWindowState;
QCOMPARE(in.windows.first().mainWindowState, mainWindowState);
QCOMPARE(session.windows.first().mainWindowState, QByteArray());
```

- [x] **Step 2: Run the focused test and verify RED**

Run inside the existing Ubuntu image after configuring the testing preset:

```bash
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --preset testing -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --build --preset testing --target CoreTests
```

Expected: compilation fails because `WindowState` has no `mainWindowState` member.

- [x] **Step 3: Implement the minimal optional session field**

Add the member and one TOML key, then read and write it with the existing blob helpers:

```cpp
QByteArray mainWindowState;
constexpr std::string_view KeyMainWindowState = "main_window_state";
ws.mainWindowState = readBlob(*tbl, KeyMainWindowState);
writeBlob(tbl, KeyMainWindowState, ws.mainWindowState);
```

- [x] **Step 4: Rebuild and run the focused test GREEN**

```bash
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --build --preset testing --target CoreTests
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  ./build/testing/src/libs/core/tests/CoreTests
```

Expected: `CoreTests` passes with no failures.

### Task 2: Add The Lightweight Playground Panel

**Files:**
- Create: `src/libs/ui/webplaygroundpanel.h`
- Create: `src/libs/ui/webplaygroundpanel.cpp`
- Modify: `src/libs/ui/CMakeLists.txt`

**Interfaces:**
- Produces: `Zeal::WidgetUi::WebPlaygroundPanel(QWidget *parent = nullptr)`.
- Produces: `WebPlaygroundPanel::closeRequested()` for the enabled Close Playground command.
- Creates: ordinary Qt Widgets only; no editor engine, WebEngine profile, page, or view.

- [x] **Step 1: Declare the focused panel interface**

```cpp
class WebPlaygroundPanel final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundPanel)
public:
    explicit WebPlaygroundPanel(QWidget *parent = nullptr);
    ~WebPlaygroundPanel() override = default;

signals:
    void closeRequested();
};
```

- [x] **Step 2: Build the shell with translated labels and disabled future actions**

Use a compact vertical layout containing a top `QTabBar` for HTML/CSS/JavaScript, a framed editor placeholder, a `QTabWidget` for Preview/Console placeholders, the local-isolation warning, and command rows. Disable Run, Auto Run, Stop, Reset, Clear Console, Open in Browser, and Export Project. Connect only Close Playground to `closeRequested()`:

```cpp
WebPlaygroundPanel::WebPlaygroundPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *editorControls = new QHBoxLayout();
    auto *editorTabs = new QTabBar(this);
    editorTabs->addTab(tr("HTML"));
    editorTabs->addTab(tr("CSS"));
    editorTabs->addTab(tr("JavaScript"));
    editorControls->addWidget(editorTabs);
    editorControls->addStretch();

    auto *runButton = new QPushButton(tr("Run"), this);
    runButton->setEnabled(false);
    editorControls->addWidget(runButton);

    auto *autoRun = new QCheckBox(tr("Auto Run"), this);
    autoRun->setEnabled(false);
    editorControls->addWidget(autoRun);
    layout->addLayout(editorControls);

    auto *editorPlaceholder = new QLabel(tr("Code editor will load when editing is implemented."), this);
    editorPlaceholder->setAlignment(Qt::AlignCenter);
    editorPlaceholder->setFrameShape(QFrame::StyledPanel);
    layout->addWidget(editorPlaceholder, 1);

    auto *outputTabs = new QTabWidget(this);
    auto *previewPlaceholder = new QLabel(tr("Preview is not available yet."), outputTabs);
    previewPlaceholder->setAlignment(Qt::AlignCenter);
    outputTabs->addTab(previewPlaceholder, tr("Preview"));
    auto *consolePlaceholder = new QLabel(tr("Console output is not available yet."), outputTabs);
    consolePlaceholder->setAlignment(Qt::AlignCenter);
    outputTabs->addTab(consolePlaceholder, tr("Console"));
    layout->addWidget(outputTabs, 1);

    auto *warning = new QLabel(tr("Code runs locally inside an isolated preview."), this);
    warning->setWordWrap(true);
    layout->addWidget(warning);

    auto *commands = new QHBoxLayout();
    for (const QString &text : {tr("Stop"), tr("Reset"), tr("Clear Console"), tr("Open in Browser"),
                                tr("Export Project")}) {
        auto *button = new QPushButton(text, this);
        button->setEnabled(false);
        commands->addWidget(button);
    }
    commands->addStretch();
    auto *closeButton = new QPushButton(tr("Close Playground"), this);
    connect(closeButton, &QPushButton::clicked, this, &WebPlaygroundPanel::closeRequested);
    commands->addWidget(closeButton);
    layout->addLayout(commands);
}
```

- [x] **Step 3: Register the source with the existing Ui target**

Add `webplaygroundpanel.cpp` beside `learningnotespanel.cpp` in `src/libs/ui/CMakeLists.txt`. CMake AUTOMOC discovers the `Q_OBJECT` header through the included source dependency.

- [x] **Step 4: Compile the Ui target**

```bash
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --build --preset testing --target Ui
```

Expected: `Ui` builds successfully.

### Task 3: Integrate The Versioned Bottom Dock

**Files:**
- Modify: `src/libs/ui/mainwindow.h`
- Modify: `src/libs/ui/mainwindow.cpp`

**Interfaces:**
- Consumes: `WebPlaygroundPanel::closeRequested()`.
- Consumes: `Core::WindowState::mainWindowState`.
- Produces: one `QDockWidget` with object name `webPlaygroundDock` and a native checkable View action.

- [x] **Step 1: Add the dock member and state version**

Forward-declare `QDockWidget`, add `QDockWidget *m_webPlaygroundDock = nullptr;`, and define:

```cpp
constexpr int MainWindowStateVersion = 1;
```

- [x] **Step 2: Construct the hidden bottom dock before menu setup**

```cpp
m_webPlaygroundDock = new QDockWidget(tr("Web Playground"), this);
m_webPlaygroundDock->setObjectName(QStringLiteral("webPlaygroundDock"));
m_webPlaygroundDock->setAllowedAreas(Qt::BottomDockWidgetArea);
auto *webPlaygroundPanel = new WebPlaygroundPanel(m_webPlaygroundDock);
m_webPlaygroundDock->setWidget(webPlaygroundPanel);
addDockWidget(Qt::BottomDockWidgetArea, m_webPlaygroundDock);
m_webPlaygroundDock->hide();
connect(webPlaygroundPanel, &WebPlaygroundPanel::closeRequested, m_webPlaygroundDock, &QDockWidget::hide);
```

This leaves the dock hidden before any state restoration and creates no expensive runtime.

- [x] **Step 3: Add the native View menu action**

After the Sidebar action, add the dock's existing action rather than creating duplicate toggle logic:

```cpp
action = m_webPlaygroundDock->toggleViewAction();
action->setText(tr("Web &Playground"));
menu->addAction(action);
addAction(action);
```

- [x] **Step 4: Restore valid state and keep all other cases hidden**

After restoring the independent horizontal splitter:

```cpp
if (windowState.mainWindowState.isEmpty()
    || !restoreState(windowState.mainWindowState, MainWindowStateVersion)) {
    m_webPlaygroundDock->hide();
}
```

The state version rejects old layouts. The dock was hidden before restoration, so missing and malformed blobs also remain hidden.

- [x] **Step 5: Save versioned state without changing splitter persistence**

In `MainWindow::~MainWindow()`:

```cpp
windowState.mainWindowState = saveState(MainWindowStateVersion);
```

- [x] **Step 6: Build the application**

```bash
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --build --preset testing
```

Expected: the application and all testing targets build successfully.

### Task 4: Validate And Commit The Shell

**Files:**
- Modify: `docs/superpowers/plans/2026-07-12-web-playground-shell.md` checkboxes only.

**Interfaces:**
- Verifies the complete shell behavior and repository scope.

- [x] **Step 1: Configure and build Release**

```bash
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --preset release -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --build --preset release
```

Expected: both commands exit zero.

- [x] **Step 2: Configure, build, and run all tests**

```bash
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --preset testing -D CMAKE_MAKE_PROGRAM=/usr/bin/ninja
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  cmake --build --preset testing
docker run --rm --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:$PWD" -w "$PWD" zealrn-ubuntu24-qt:latest \
  ctest --preset testing --output-on-failure
```

Expected: every test passes.

- [x] **Step 3: Run isolated Xvfb state smoke checks**

Launch with temporary `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_CACHE_HOME`, and `XDG_STATE_HOME`. Verify the dock is absent on first launch, show it through View > Web Playground, resize it, close and launch again to confirm visibility/size restoration, then replace `main_window_state` with invalid data and verify the next launch remains usable with the dock hidden. Confirm sidebar, documentation, and Learning Notes remain visible and unchanged.

- [x] **Step 4: Review repository scope**

```bash
git diff --check
git status --short
git diff --stat
git diff -- src/libs/core/session.h src/libs/core/session.cpp \
  src/libs/core/tests/session_test.cpp src/libs/ui/CMakeLists.txt \
  src/libs/ui/webplaygroundpanel.h src/libs/ui/webplaygroundpanel.cpp \
  src/libs/ui/mainwindow.h src/libs/ui/mainwindow.cpp \
  docs/superpowers/plans/2026-07-12-web-playground-shell.md
```

Expected: only the plan and shell-required source/test files changed; `.codegraph/` and `core` remain untracked and unstaged.

- [x] **Step 5: Commit exactly the reviewed files**

```bash
git add docs/superpowers/plans/2026-07-12-web-playground-shell.md \
  src/libs/core/session.h src/libs/core/session.cpp src/libs/core/tests/session_test.cpp \
  src/libs/ui/CMakeLists.txt src/libs/ui/webplaygroundpanel.h \
  src/libs/ui/webplaygroundpanel.cpp src/libs/ui/mainwindow.h src/libs/ui/mainwindow.cpp
git commit -m "feat: add web playground shell"
```

Do not push.
