set(UIFILES
    canvas_options.glade
    preview_options.glade
    dialog_workspaces.glade
)

foreach (UIFILE IN ITEMS ${UIFILES})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${UIFILE}
        ${SYNFIG_BUILD_ROOT}/share/synfig/ui/${UIFILE}
        COPYONLY
    )

    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/${UIFILE}
        DESTINATION share/synfig/ui
    )
endforeach()
