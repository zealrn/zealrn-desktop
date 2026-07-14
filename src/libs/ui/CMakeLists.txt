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
    terminalbridge.cpp
    terminalsupport.cpp
    terminalview.cpp
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
    if(WIN32)
        set(TerminalBackend_SOURCE windowsterminalbackend.cpp)
        target_compile_definitions(Ui PUBLIC ZEALRN_HAVE_CONPTY PRIVATE _WIN32_WINNT=0x0A00)
        message(STATUS "Developer Terminal: Windows ConPTY backend enabled")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(TerminalBackend_SOURCE linuxterminalbackend.cpp)
        target_compile_definitions(Ui PUBLIC ZEALRN_HAVE_POSIX_PTY)
        find_library(UTIL_LIBRARY util)
        if(UTIL_LIBRARY)
            target_link_libraries(Ui PRIVATE ${UTIL_LIBRARY})
        endif()
        message(STATUS "Developer Terminal: native POSIX PTY backend enabled")
    else()
        message(STATUS "Developer Terminal: native backend unavailable; external terminal fallback enabled")
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
