
bin_PROGRAMS = kjumpingcube
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes)

SUBDIRS = .

kjumpingcube_SOURCES = kjumpingcube.cpp kcubeboxwidget.cpp main.cpp kcubewidget.cpp cube.cpp\
                       brain.cpp cubebox.cpp
kjumpingcube_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kjumpingcube_LDADD =  $(LIB_KDEGAMES) $(LIB_KDEUI) $(LIB_KFILE)

EXTRA_DIST = kjumpingcube.desktop

KDE_ICON = kjumpingcube

appsdir = $(kde_appsdir)/Games/TacticStrategy
apps_DATA = kjumpingcube.desktop

METASOURCES = AUTO

rcdir = $(kde_datadir)/kjumpingcube
rc_DATA = kjumpingcubeui.rc

messages: rc.cpp
	$(XGETTEXT) rc.cpp $(kjumpingcube_SOURCES) -o $(podir)/kjumpingcube.pot

install-data-local:
	rm -rf $(DESTDIR)$(kde_appsdir)/Games/kjumpingcube.desktop
