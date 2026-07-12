add_subdirectory(qxtglobalshortcut)
add_subdirectory(widgets)

set(Ui_FORMS
    aboutdialog.ui
    docsetsdialog.ui
    settingsdialog.ui
)

add_library(Ui STATIC
    aboutdialog.cpp
    browsertab.cpp
    developerterminalpanel.cpp
    docsetlistitemdelegate.cpp
    docsetsdialog.cpp
    learningnotespanel.cpp
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

target_link_libraries(Ui PRIVATE Browser Sidebar QxtGlobalShortcut Widgets Registry Util)

zeal_attach_qt_pch(Ui <QDialog> <QWidget>)

find_package(Qt6 COMPONENTS WebChannel WebEngineWidgets REQUIRED)
target_link_libraries(Ui PRIVATE Qt6::WebChannel Qt6::WebEngineWidgets)

if(BUILD_TESTING)
    find_package(Qt6 COMPONENTS Test REQUIRED)
    add_subdirectory(tests)
endif()
