
bin_PROGRAMS = kjumpingcube
INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KDEUI) $(LIB_KFILE)

SUBDIRS = .

kjumpingcube_SOURCES = kjumpingcube.cpp kcubeboxwidget.cpp main.cpp kcubewidget.cpp cube.cpp\
                       brain.cpp cubebox.cpp

EXTRA_DIST = kjumpingcube.desktop

KDE_ICON = kjumpingcube

appsdir = $(kde_appsdir)/Games
apps_DATA = kjumpingcube.desktop

METASOURCES = AUTO

rcdir = $(kde_datadir)/kjumpingcube
rc_DATA = kjumpingcubeui.rc

messages:
	$(XGETTEXT) rc.cpp $(kjumpingcube_SOURCES) -o $(podir)/kjumpingcube.pot


