# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_sounddir      Where system sounds should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

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

kde_icon_DATA = kjumpingcube.xpm

# just install datas here. Use install-exec-local for scripts and etc.
# the binary itself is already installed from automake
# use mkinstalldirs, not "install -d"
# don't install a list of file. Just one file per install.
# if you have more of them, create a subdirectory with an extra Makefile 
install-data-local: 
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) $(srcdir)/mini-kjumpingcube.xpm $(kde_minidir)/kjumpingcube.xpm

# remove ALL you have installed in install-data-local or install-exec-local
uninstall-local:
	-rm -f $(kde_minidir)/kjumpingcube.xpm
