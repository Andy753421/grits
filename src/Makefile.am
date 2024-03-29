SUBDIRS = data objects . plugins

AM_CFLAGS   = -Wall --std=gnu99 -I$(top_srcdir)/src
AM_CPPFLAGS = $(GLIB_CFLAGS) $(GTK_CFLAGS) $(GL_CFLAGS) $(SOUP_CFLAGS)
AM_LDADD    = $(GLIB_LIBS) $(GTK_LIBS) $(GL_LIBS) $(SOUP_LIBS)
AM_LDFLAGS  = 

# System dependant flags
if SYS_WIN
AM_LDFLAGS += -mwindows -no-undefined
endif
if SYS_MAC
AM_CFLAGS  += -ObjC
AM_LDFLAGS += -framework AppKit
else
AM_LDFLAGS += -Wl,--as-needed -Wl,--no-undefined
endif

# Headers
grits_includedir = $(includedir)/grits
grits_include_HEADERS = \
	grits.h         \
	grits-viewer.h  \
	grits-prefs.h   \
	grits-opengl.h  \
	grits-plugin.h  \
	grits-util.h    \
	gtkgl.h         \
	gpqueue.h       \
	roam.h

# Pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = grits.pc

# GTK 2/3 compatability
EXTRA_DIST = compat.h

# Library
lib_LTLIBRARIES = libgrits.la

libgrits_la_SOURCES = grits.h \
	grits-viewer.c  grits-viewer.h  \
	grits-prefs.c   grits-prefs.h   \
	grits-opengl.c  grits-opengl.h  \
	grits-plugin.c  grits-plugin.h  \
	grits-marshal.c grits-marshal.h \
	grits-util.c    grits-util.h    \
	roam.c          roam.h          \
	gtkgl.c         gtkgl.h         \
	gpqueue.c       gpqueue.h
libgrits_la_CPPFLAGS = $(AM_CPPFLAGS) \
	-DPKGDATADIR="\"$(DOTS)$(datadir)/$(GRITS_SUBDIR)\"" \
	-DPLUGINSDIR="\"$(DOTS)$(libdir)/$(GRITS_SUBDIR)\""
libgrits_la_LIBADD  = $(AM_LDADD) -lm \
	objects/libgrits-objects.la \
	data/libgrits-data.la
libgrits_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIB_VERSION)

# Demo program
bin_PROGRAMS = grits-demo

grits_demo_SOURCES = grits-demo.c
grits_demo_LDADD   = $(AM_LDADD) libgrits.la

# Test programs
noinst_PROGRAMS = grits-test tile-test

grits_test_SOURCES = grits-test.c
grits_test_LDADD   = $(AM_LDADD) libgrits.la

tile_test_SOURCES = tile-test.c
tile_test_LDADD   = $(AM_LDADD) libgrits.la

# Clean
MAINTAINERCLEANFILES = Makefile.in

# Generated code 
BUILT_SOURCES = grits-marshal.c grits-marshal.h

.list.c:
	glib-genmarshal --prefix=grits_cclosure_marshal --body   $< > $@
.list.h:
	glib-genmarshal --prefix=grits_cclosure_marshal --header $< > $@

# Debugging rules
BUILT_SOURCES += .libs/$(GRITS_SUBDIR)

.libs/$(GRITS_SUBDIR):
	mkdir -p .libs
	ln -sf ../plugins/.libs $@

test: all
	G_MESSAGES_DEBUG=all \
	./grits-test

gdb: all
	G_MESSAGES_DEBUG=all \
	gdb ./.libs/grits-test

ddd: all
	G_MESSAGES_DEBUG=all \
	ddd ./grits-test

memcheck: all
	G_SLICE=always-malloc                   \
	G_DEBUG=gc-friendly,resident-modules    \
	valgrind --track-origins=yes            \
	         --leak-check=full              \
	         --leak-resolution=high         \
	         --num-callers=50               \
	         --suppressions=gtk.suppression \
	         --suppressions=/usr/lib/valgrind/default.supp \
	         .libs/grits-test               \
	2> valgrind.out
