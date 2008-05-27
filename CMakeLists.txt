project(kjumpingcube)

add_subdirectory( pics )

########### next target ###############

set(kjumpingcube_SRCS 
   kjumpingcube.cpp 
   kcubeboxwidget.cpp 
   main.cpp 
   kcubewidget.cpp 
   cube.cpp 
   brain.cpp 
   cubebox.cpp )

kde4_add_ui_files(kjumpingcube_SRCS settings.ui )

kde4_add_kcfg_files(kjumpingcube_SRCS prefs.kcfgc )

kde4_add_app_icon(kjumpingcube_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-kjumpingcube.png")
kde4_add_executable(kjumpingcube ${kjumpingcube_SRCS})

target_link_libraries(kjumpingcube  ${KDE4_KIO_LIBS} kdegames )

install(TARGETS kjumpingcube  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES kjumpingcube.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kjumpingcube.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES kjumpingcubeui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kjumpingcube )

kde4_install_icons( ${ICON_INSTALL_DIR}   )





