srcdir = ./

all: info dvi

info: $(srcdir)/gdk.texi $(srcdir)/gtk.texi
	makeinfo $(srcdir)/gdk.texi
	makeinfo $(srcdir)/gtk.texi

dvi: $(srcdir)/gdk.texi $(srcdir)/gtk.texi
	texi2dvi $(srcdir)/gdk.texi
	texi2dvi $(srcdir)/gtk.texi

files:
	@echo $(srcdir)/Makefile $(srcdir)/gdk.texi $(srcdir)/gtk.texi

clean:
	-rm -f $(srcdir)/*~ $(srcdir)/g[dt]k.?? $(srcdir)/g[dt]k.??? $(srcdir)/g[dt]k $(srcdir)/g[dt]k.info*

distclean: clean
