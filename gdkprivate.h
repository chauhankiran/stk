/* GDK - The General Drawing Kit (written for the GIMP)
 * Copyright (C) 1995 Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GDK_PRIVATE_H__
#define __GDK_PRIVATE_H__


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _GdkWindowPrivate    GdkWindowPrivate;
typedef struct _GdkWindowPrivate    GdkPixmapPrivate;
typedef struct _GdkImagePrivate     GdkImagePrivate;
typedef struct _GdkGCPrivate        GdkGCPrivate;
typedef struct _GdkColormapPrivate  GdkColormapPrivate;
typedef struct _GdkVisualPrivate    GdkVisualPrivate;
typedef struct _GdkFontPrivate      GdkFontPrivate;
typedef struct _GdkCursorPrivate    GdkCursorPrivate;

struct _GdkWindowPrivate
{
  GdkWindow window;
  Window xwindow;
  Display *xdisplay;
  unsigned int destroyed : 1;
};

struct _GdkImagePrivate
{
  GdkImage image;
  XImage *ximage;
  Display *xdisplay;
  gpointer x_shm_info;

  void (*image_put) (GdkWindow *window,
		     GdkGC     *gc,
		     GdkImage  *image,
		     gint       xsrc,
		     gint       ysrc,
		     gint       xdest,
		     gint       ydest,
		     gint       width,
		     gint       height);
};

struct _GdkGCPrivate
{
  GdkGC gc;
  GC xgc;
  Display *xdisplay;
  gint16 x;
  gint16 y;
};

struct _GdkColormapPrivate
{
  GdkColormap colormap;
  Colormap xcolormap;
  Display *xdisplay;
  GdkVisual *visual;
  gint private;
  gint next_color;
};

struct _GdkVisualPrivate
{
  GdkVisual visual;
  Visual *xvisual;
};

struct _GdkFontPrivate
{
  GdkFont font;
  XFontStruct *xfont;
  Display *xdisplay;
};

struct _GdkCursorPrivate
{
  GdkCursor cursor;
  Cursor xcursor;
  Display *xdisplay;
};

void gdk_window_init (void);
void gdk_visual_init (void);

void gdk_image_exit (void);

void gdk_window_real_destroy (GdkWindow *window);
void gdk_window_table_insert (GdkWindow *window);
void gdk_window_table_remove (GdkWindow *window);
GdkWindow* gdk_window_table_lookup (Window xwindow);
GdkWindow* gdk_window_insert_sibling (GdkWindow *windows, GdkWindow *sibling);
GdkWindow* gdk_window_remove_sibling (GdkWindow *windows, GdkWindow *sibling);

void gdk_window_add_colormap_windows (GdkWindow *window);


extern gint              gdk_debug_level;
extern gint              gdk_show_events;
extern gint              gdk_stack_trace;
extern gchar            *gdk_display_name;
extern Display          *gdk_display;
extern gint              gdk_screen;
extern Window            gdk_root_window;
extern GdkWindowPrivate  gdk_root_parent;
extern Atom              gdk_wm_delete_window;
extern Atom              gdk_wm_take_focus;
extern Atom              gdk_wm_protocols;
extern gchar            *gdk_progname;
extern gint              gdk_error_code;
extern gint              gdk_error_warnings;
extern gint              gdk_motion_events;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GDK_PRIVATE_H__ */
