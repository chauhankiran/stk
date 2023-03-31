# stk

Specific Toolkit

## Introduction

At this moment, the repo contains old GTK code copied from GIMP v0.60 downloaded from [historical](https://ftp.gimp.org/gimp/historical) folder on [https://ftp.gimp.org](https://ftp.gimp.org). All the GTK files are copied directly in root folder to hack it in raw form. 

## Getting Started

Getting started with this library is quite easy. It depends on two specific libraries - X11 and Xext.  You can easily download development library for these libraries using following commands.

```bash
$ sudo apt install libx11-dev
```

and

```bash
$ sudo apt install libxext-dev
```

I've created one sample `test_window.c` program that will display default 0x0 window. To run this program I've created a simple Makefile. This Makefile contain only one command that compile all these files with `test_window.c` so that you don't have to type a really big command in terminal.

```bash
$ make
```

You might get some warning. As of now, ignore those warning as we're trying to compile as lease 2 decade old programs with newer version of compiler.

After that you should have an executable as `test_window`. You can run this easily as,

```bash
$ ./test_window
```

If you get error like following.

```bash
$ ./test_window
** ERROR **: sigsegv caught
#0: g_error
#1: gdk_signal
#2: gtk_gc_find_by_val
#3: gtk_gc_get
#4: gtk_style_attach
#5: gtk_init
```

Re-run the program again and check if you're seeing a new icon in menu bar or somewhere. Because, at this moment the default window size is 0x0. So, it might not visible with first look. You need look for it.

## Why This Exist?

For fun and learn!
