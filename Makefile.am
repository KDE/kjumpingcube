
bin_PROGRAMS = kjumpingcube
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes)
LDFLAGS = -L$(top_builddir)/libkdegames $(all_libraries) $(KDE_RPATH)
LDADD = -lkdegames $(LIB_KDEUI) $(LIB_KFILE)

SUBDIRS = .

kjumpingcube_SOURCES = kjumpingcube.cpp kcubeboxwidget.cpp main.cpp kcubewidget.cpp cube.cpp\
                       brain.cpp cubebox.cpp

EXTRA_DIST = kjumpingcube.desktop

KDE_ICON = kjumpingcube

appsdir = $(kde_appsdir)/Games/TacticStrategy
apps_DATA = kjumpingcube.desktop

METASOURCES = AUTO

rcdir = $(kde_datadir)/kjumpingcube
rc_DATA = kjumpingcubeui.rc

messages: rc.cpp
	$(XGETTEXT) rc.cpp $(kjumpingcube_SOURCES) -o $(podir)/kjumpingcube.pot


