
appsdir = $(kde_appsdir)/Games
apps_DATA = kjumpingcube.desktop

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

SUBDIRS =

####### This part is very kjumpingcube specific
# this is the program that gets installed
bin_PROGRAMS = 	kjumpingcube

# Which sources should be compiled for kjumpingcube.
kjumpingcube_SOURCES = kjumpingcube.cpp kcubeboxwidget.cpp main.cpp kcubewidget.cpp cube.cpp\
                       brain.cpp cubebox.cpp

# let automoc handle all of the meta source files (moc)
kjumpingcube_METASOURCES = AUTO

# the library search path. 
kjumpingcube_LDFLAGS = $(all_libraries) $(KDE_RPATH)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
kjumpingcube_LDADD = $(LIB_KFILE)

# these are the headers for your project
noinst_HEADERS = kjumpingcube.h kcubeboxwidget.h cubebox.h kcubewidget.h cube.h brain.h

# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kjumpingcube_SOURCES) && mv messages.po ../po/kjumpingcube.pot

# generate source documentation
srcdoc:
	$(mkinstalldirs) $(SRCDOC_DEST)
	kdoc -H -d $(SRCDOC_DEST) kjumpingcube 	     $(noinst_HEADERS) -lqt

KDE_ICON = kjumpingcube

