# Generated automatically from Makefile.in by configure.
.SUFFIXES:
.SUFFIXES: .c .o

srcdir		=	.

SHELL		=	/bin/sh

TESTSRC		=	$(srcdir)/button.c		\
			$(srcdir)/entry.c		\
			$(srcdir)/filesel.c		\
			$(srcdir)/frame.c		\
			$(srcdir)/gamma_correct.c	\
			$(srcdir)/listbox.c		\
			$(srcdir)/optionmenu.c		\
			$(srcdir)/popup.c		\
			$(srcdir)/ruler.c		\
			$(srcdir)/scale.c		\
			$(srcdir)/scroll.c		\
			$(srcdir)/scrollbar.c		\
			$(srcdir)/table.c		\
			$(srcdir)/toggle.c
TESTS		=	$(TESTSRC:.c=)

OTHER_FILES	=	Makefile.in .depend
ALL_FILES	:=	$(foreach file, $(OTHER_FILES) $(wildcard *.[ch]), $(srcdir)/$(file))

CC		=	gcc
RANLIB		=	ranlib
DEPEND		=	$(CC) -MM

LIBS		=	-lgtk -lgdk  -lglib -lc -lm
CFLAGS		=	-g -pipe -Wall -ansi -pedantic -I../ -I../glib  -I/usr/X11R6/include
LDFLAGS		=	-L../ -L../glib  -L/usr/X11R6/lib 

all: $(TESTS)

# .c: ../libgdk.a ../libgtk.a
# 	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

%: %.c ../libgdk.a ../libgtk.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

clean:
	rm -f *~ *.o $(TESTS)

distclean: clean

files:
	@echo $(ALL_FILES)

depend:
	$(DEPEND) $(CFLAGS) $(TESTSRC) > .depend

.depend:
	touch .depend

include .depend
