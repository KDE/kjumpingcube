
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KDEGAMES) $(LIB_KFILE)
METASOURCES = AUTO
KDE_ICON = kjumpingcube

bin_PROGRAMS = kjumpingcube
kjumpingcube_SOURCES = kjumpingcube.cpp kcubeboxwidget.cpp main.cpp \
					   kcubewidget.cpp cube.cpp \
                       brain.cpp cubebox.cpp

appsdir = $(kde_appsdir)/Games/TacticStrategy
apps_DATA = kjumpingcube.desktop

EXTRA_DIST = $(apps_DATA)

rcdir = $(kde_datadir)/kjumpingcube
rc_DATA = kjumpingcubeui.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kjumpingcube.pot

install-data-local:
	rm -rf $(DESTDIR)$(kde_appsdir)/Games/kjumpingcube.desktop
