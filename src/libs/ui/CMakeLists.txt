add_subdirectory(qxtglobalshortcut)
add_subdirectory(widgets)

set(Ui_FORMS
    aboutdialog.ui
    docsetsdialog.ui
    settingsdialog.ui
)

add_library(Ui STATIC
    allnotesdialog.cpp
    aboutdialog.cpp
    browsertab.cpp
    developerterminalpanel.cpp
    docsetcatalog.cpp
    docsetlistitemdelegate.cpp
    docsetsdialog.cpp
    learningnotespanel.cpp
    learningnotesexporter.cpp
    learningnotesstore.cpp
    mainwindow.cpp
    searchitemdelegate.cpp
    searchsidebar.cpp
    settingsdialog.cpp
    sidebarviewprovider.cpp
    terminalbackend.cpp
    terminalsupport.cpp
    webplaygroundeditor.cpp
    webplaygroundpanel.cpp
    webplaygroundpreview.cpp
    webplaygroundproject.cpp
    windowmanager.cpp
    ${Ui_FORMS} # For Qt Creator.
)

set(TerminalBackend_SOURCE unavailableterminalbackend.cpp)
if(ZEALRN_ENABLE_TERMINAL)
    target_compile_definitions(Ui PRIVATE ZEALRN_ENABLE_TERMINAL)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(qtermwidget6 CONFIG QUIET)
        if(TARGET qtermwidget6)
            set(TerminalBackend_SOURCE linuxterminalbackend.cpp)
            target_link_libraries(Ui PRIVATE qtermwidget6)
            target_compile_definitions(Ui PUBLIC ZEALRN_HAVE_QTERMWIDGET)
            message(STATUS "Developer Terminal: QTermWidget backend enabled")
        else()
            message(STATUS "Developer Terminal: QTermWidget unavailable; external terminal fallback enabled")
        endif()
    elseif(WIN32)
        set(TerminalBackend_SOURCE windowsterminalbackend.cpp)
        message(STATUS "Developer Terminal: Windows external terminal fallback enabled")
    endif()
else()
    message(STATUS "Developer Terminal: disabled; external terminal fallback enabled")
endif()
target_sources(Ui PRIVATE ${TerminalBackend_SOURCE})

target_link_libraries(Ui PRIVATE Browser Sidebar QxtGlobalShortcut Widgets Registry Util)

zeal_attach_qt_pch(Ui <QDialog> <QWidget>)

find_package(LibArchive REQUIRED)
find_package(Qt6 COMPONENTS PrintSupport Sql WebChannel WebEngineWidgets REQUIRED)
target_link_libraries(Ui PRIVATE LibArchive::LibArchive Qt6::PrintSupport Qt6::Sql Qt6::WebChannel Qt6::WebEngineWidgets)

if(BUILD_TESTING)
    find_package(Qt6 COMPONENTS Test REQUIRED)
    add_subdirectory(tests)
endif()
