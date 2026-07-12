if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(KDE_INSTALL_APPDIR "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    set(KDE_INSTALL_ICONDIR "${CMAKE_INSTALL_DATAROOTDIR}/icons")
    set(KDE_INSTALL_METAINFODIR "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")

# For development builds insert an extra release in the AppStream metadata.
if(ZEAL_VERSION_SUFFIX)
    string(TIMESTAMP ZEAL_APPSTREAM_DEV_RELEASE
        "\n    <release date=\"%Y-%m-%d\" version=\"${ZEAL_VERSION_FULL}\" type=\"development\" />"
        UTC
    )
endif()

    configure_file(
        io.github.abnzrdev.zealrn.appdata.xml.in
        io.github.abnzrdev.zealrn.appdata.xml
    )

    install(FILES ${CMAKE_BINARY_DIR}/assets/freedesktop/io.github.abnzrdev.zealrn.appdata.xml
            DESTINATION ${KDE_INSTALL_METAINFODIR}
    )

    install(FILES "io.github.abnzrdev.zealrn.desktop"
            DESTINATION ${KDE_INSTALL_APPDIR}
    )

    install(FILES "sc-apps-zeal.svg"
            DESTINATION "${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps"
            RENAME "zealrn.svg"
    )
endif()
