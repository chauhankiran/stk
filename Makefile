# Generated automatically from Makefile.in by configure.
.SUFFIXES:
.SUFFIXES: .c .o .so



prefix		= /usr/local
exec_prefix	= ${prefix}
bindir		= $(exec_prefix)/bin
libdir		= $(exec_prefix)/lib
includedir	= $(prefix)/include/gtk
datadir		= $(prefix)/share
mandir		= $(prefix)/man/man1
manext		= .1
srcdir		= .

SHELL		= /bin/sh

MINOR		= 1
MAJOR		= 0
VERSION		= $(MAJOR).$(MINOR)
PACKAGE		= gtk
DISTRIBUTION	= $(PACKAGE)-$(VERSION)
RELEASE		= $(PACKAGE)
SYSTEM		= @system@

GDKSRCS		=	$(srcdir)/gdk.c			\
			$(srcdir)/gdkcolor.c		\
			$(srcdir)/gdkcursor.c		\
			$(srcdir)/gdkdraw.c		\
			$(srcdir)/gdkfont.c		\
			$(srcdir)/gdkgc.c		\
			$(srcdir)/gdkglobals.c		\
			$(srcdir)/gdkimage.c		\
			$(srcdir)/gdkpixmap.c		\
			$(srcdir)/gdkrectangle.c	\
			$(srcdir)/gdkvisual.c		\
			$(srcdir)/gdkwindow.c

GTKSRCS		=	$(srcdir)/gtk.c			\
			$(srcdir)/gtkaccelerator.c	\
			$(srcdir)/gtkalignment.c	\
			$(srcdir)/gtkbox.c		\
			$(srcdir)/gtkbutton.c		\
			$(srcdir)/gtkcallback.c		\
			$(srcdir)/gtkcontainer.c	\
			$(srcdir)/gtkdata.c		\
			$(srcdir)/gtkdraw.c		\
			$(srcdir)/gtkdrawingarea.c	\
			$(srcdir)/gtkentry.c		\
			$(srcdir)/gtkevent.c		\
			$(srcdir)/gtkfilesel.c		\
			$(srcdir)/gtkframe.c		\
			$(srcdir)/gtkgc.c		\
			$(srcdir)/gtkglobals.c		\
			$(srcdir)/gtklist.c		\
			$(srcdir)/gtklistbox.c		\
			$(srcdir)/gtkmenu.c		\
			$(srcdir)/gtkmisc.c		\
			$(srcdir)/gtkobserver.c		\
			$(srcdir)/gtkoptionmenu.c	\
			$(srcdir)/gtkruler.c		\
			$(srcdir)/gtkscale.c		\
			$(srcdir)/gtkscroll.c		\
			$(srcdir)/gtkscrollbar.c	\
			$(srcdir)/gtkstyle.c		\
			$(srcdir)/gtktable.c		\
			$(srcdir)/gtkwidget.c		\
			$(srcdir)/gtkwindow.c		\
			$(srcdir)/fnmatch.c

GDKOBJS		=	$(GDKSRCS:.c=.o)
GTKOBJS		=	$(GTKSRCS:.c=.o)

LIBGDK		=	libgdk
LIBGTK		=	libgtk
PROGRAM		=	testgtk

ALLSRCS		=	$(GDKSRCS) $(GTKSRCS) $(srcdir)/$(PROGRAM).c

OTHER_FILES	=	Makefile.in Makefile.shared.in configure.in config.h.in \
			configure config.guess config.sub install-sh SPECS TODO \
			gtk.prj .depend
ALL_FILES	:=	$(foreach file, $(OTHER_FILES) $(wildcard *.[ch]), $(srcdir)/$(file))
SUBDIRS		=	$(srcdir)/docs		\
			$(srcdir)/glib		\
			$(srcdir)/tests

CC		=	gcc
LD		=	ld
RANLIB		=	ranlib
DEPEND		=	$(CC) -MM
INSTALL		=	/usr/bin/install -c
LN_S		=	ln -s

LIBS		=	-lgtk -lgdk -lX11 -lXext -lglib -lc -lm
CFLAGS		=	-g -pipe -Wall -ansi -pedantic -I./ -Iglib  -I/usr/X11R6/include
LDFLAGS		=	-L./ -Lglib  -L/usr/X11R6/lib 


.PHONY: glib tests info dvi clean distclean mostlyclean dist release files depend

all: glib $(LIBGDK).a $(LIBGTK).a $(PROGRAM)

install: all
	$(INSTALL) -d $(libdir)
	$(INSTALL) -d $(includedir)
	$(INSTALL) *.h $(includedir)
	if test -e $(LIBGDK).a; then \
	  $(INSTALL) $(LIBGDK).a $(libdir); \
	fi
	if test -e $(LIBGTK).a; then \
	  $(INSTALL) $(LIBGTK).a $(libdir); \
	fi
	if test -e $(LIBGDK).$(SO).$(VERSION); then \
	  $(INSTALL) $(LIBGDK).$(SO).$(VERSION) $(libdir); \
	  $(RM) -f $(libdir)/$(LIBGDK).$(SO); \
	  $(LN_S) -s $(libdir)/$(LIBGDK).$(SO).$(VERSION) $(libdir)/$(LIBGDK).$(SO); \
	fi
	if test -e $(LIBGTK).$(SO).$(VERSION); then \
	  $(INSTALL) $(LIBGTK).$(SO).$(VERSION) $(libdir); \
	  $(RM) -f $(libdir)/$(LIBGTK).$(SO); \
	  $(LN_S) -s $(libdir)/$(LIBGTK).$(SO).$(VERSION) $(libdir)/$(LIBGTK).$(SO); \
	fi

uninstall:
	rm -fr $(includedir)
	rm -f $(libdir)/$(LIBGDK).a
	rm -f $(libdir)/$(LIBGTK).a
	rm -f $(libdir)/$(LIBGDK).$(SO).$(VERSION)
	rm -f $(libdir)/$(LIBGDK).$(SO)
	rm -f $(libdir)/$(LIBGTK).$(SO).$(VERSION)
	rm -f $(libdir)/$(LIBGTK).$(SO)

config.h: config.h.in
	./configure

$(LIBGDK).a: $(GDKOBJS)
	-rm -f $@
	ar rc $@ $(GDKOBJS)
	$(RANLIB) $@

$(LIBGTK).a: $(GTKOBJS)
	-rm -f $@
	ar rc $@ $(GTKOBJS)
	$(RANLIB) $@

$(PROGRAM): $(PROGRAM).o $(LIBGTK).a $(LIBGDK).a
	$(CC) -o $(PROGRAM) $(PROGRAM).o $(LDFLAGS) $(LIBS)

glib:
	(cd glib; $(MAKE))

tests:
	(cd tests; $(MAKE))

info:
	(cd docs; $(MAKE) info)

dvi:
	(cd docs; $(MAKE) dvi)

clean: sharedclean
	rm -f *.o *.so *~ *.tar.gz $(PROGRAM) $(LIBGDK).a $(LIBGTK).a
	(cd docs; $(MAKE) clean)
	(cd glib; $(MAKE) clean)
	(cd tests; $(MAKE) clean)

distclean: clean
	rm -f config.h config.cache config.status config.log
	(cd docs; $(MAKE) distclean)
	(cd glib; $(MAKE) distclean)
	(cd tests; $(MAKE) distclean)

mostlyclean: clean

realclean: distclean

dist:
	@echo Making distribution: $(DISTRIBUTION).tar.gz
	@rm -rf $(DISTRIBUTION)
	@mkdir $(DISTRIBUTION)
	@ln $(ALL_FILES) $(DISTRIBUTION)
	@set -e; for i in $(SUBDIRS); do mkdir $(DISTRIBUTION)/$$i; done
	@set -e; for i in $(SUBDIRS); do ln `$(MAKE) -C $$i files srcdir=$$i | grep -v "make\["` $(DISTRIBUTION)/$$i; done
	@tar cf - $(DISTRIBUTION) | gzip -9 > $(DISTRIBUTION).tar.gz
	@rm -rf $(DISTRIBUTION)

release:
	@echo Making release: $(RELEASE)`date +"%y%m%d"`.tgz
	@rm -rf $(RELEASE)
	@mkdir $(RELEASE)
	@ln $(ALL_FILES) $(RELEASE)
	@set -e; for i in $(SUBDIRS); do mkdir $(RELEASE)/$$i; done
	@set -e; for i in $(SUBDIRS); do ln `$(MAKE) -C $$i files srcdir=$$i | grep -v "make\["` $(RELEASE)/$$i; done
	@tar cf - $(RELEASE) | gzip -9 > $(RELEASE)`date +"%y%m%d"`.tgz
	@rm -rf $(RELEASE)

files:
	@echo $(ALL_FILES)

depend:
	@if eval test ! -f "config.h"; then \
	  touch config.h; \
	  touch .rm_config_h; \
	else \
	  rm -f .rm_config_h; \
	fi
	$(DEPEND) $(CFLAGS) $(ALLSRCS) > .depend
	@if eval test -f ".rm_config_h"; then \
	  rm -f config.h; \
	  rm -f .rm_config_h; \
	fi
	(cd glib; $(MAKE) depend)
	(cd tests; $(MAKE) depend)

.depend:
	touch .depend

include .depend
include Makefile.shared
