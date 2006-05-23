project(kjumpingcube)
include_directories( ${CMAKE_SOURCE_DIR}/libkdegames  )


########### next target ###############

set(kjumpingcube_SRCS 
   kjumpingcube.cpp 
   kcubeboxwidget.cpp 
   main.cpp 
   kcubewidget.cpp 
   cube.cpp 
   brain.cpp 
   cubebox.cpp )

kde4_automoc(${kjumpingcube_SRCS})

kde4_add_ui_files(kjumpingcube_SRCS settings.ui )

kde4_add_kcfg_files(kjumpingcube_SRCS prefs.kcfgc )

kde4_add_executable(kjumpingcube ${kjumpingcube_SRCS})

target_link_libraries(kjumpingcube  ${KDE4_KIO_LIBS} kdegames )

install_targets(/bin kjumpingcube )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kjumpingcube.desktop )
install_files( ${KCFG_INSTALL_DIR} FILES kjumpingcube.kcfg )
install_files( ${DATA_INSTALL_DIR}/kjumpingcube FILES kjumpingcubeui.rc )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )




#original Makefile.am contents follow:

#
#INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes)
#METASOURCES = AUTO
#KDE_ICON = kjumpingcube
#
#bin_PROGRAMS = kjumpingcube
#kjumpingcube_SOURCES = kjumpingcube.cpp kcubeboxwidget.cpp main.cpp \
#                       kcubewidget.cpp cube.cpp brain.cpp cubebox.cpp \
#                       settings.ui prefs.kcfgc
#
#kjumpingcube_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#kjumpingcube_LDADD = $(LIB_KDEGAMES) $(LIB_KFILE)
#kjumpingcube_DEPENDENCIES = $(LIB_KDEGAMES_DEP)
#
#xdg_apps_DATA = kjumpingcube.desktop
#kde_kcfg_DATA = kjumpingcube.kcfg
#
#rcdir = $(kde_datadir)/kjumpingcube
#rc_DATA = kjumpingcubeui.rc
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kjumpingcube.pot
#
