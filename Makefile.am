
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes)
METASOURCES = AUTO
KDE_ICON = kjumpingcube

bin_PROGRAMS = kjumpingcube
kjumpingcube_SOURCES = kjumpingcube.cpp kcubeboxwidget.cpp main.cpp \
                       kcubewidget.cpp cube.cpp brain.cpp cubebox.cpp \
                       settings.ui prefs.kcfgc

kjumpingcube_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kjumpingcube_LDADD = $(LIB_KDEGAMES) $(LIB_KFILE)
kjumpingcube_DEPENDENCIES = $(LIB_KDEGAMES_DEP)

xdg_apps_DATA = kjumpingcube.desktop
kde_kcfg_DATA = kjumpingcube.kcfg

rcdir = $(kde_datadir)/kjumpingcube
rc_DATA = kjumpingcubeui.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kjumpingcube.pot

