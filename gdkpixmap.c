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

/*
 * File:         gdkpixmap.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for creating
 *               and destroying pixmaps.
 */
#include <X11/Xlib.h>
#include "gdk.h"
#include "gdkprivate.h"


GdkPixmap*
gdk_pixmap_new (GdkWindow *window, 
		gint       width, 
		gint       height, 
		gint       depth)
{
  GdkPixmap *pixmap;
  GdkWindowPrivate *private;
  GdkWindowPrivate *window_private;

  g_function_enter ("gdk_pixmap_new");

  if (!window)
    window = (GdkWindow*) &gdk_root_parent;

  if (depth == -1)
    depth = window->depth;
  
  private = g_new (GdkWindowPrivate, 1);
  pixmap = (GdkPixmap*) private;

  window_private = (GdkWindowPrivate*) window;
  
  private->xdisplay = window_private->xdisplay;
  private->xwindow = XCreatePixmap (private->xdisplay, window_private->xwindow, 
				    width, height, depth);

  pixmap->window_type = GDK_WINDOW_PIXMAP;
  pixmap->visual = window->visual;
  pixmap->colormap = window->colormap;
  
  pixmap->x = 0;
  pixmap->y = 0;
  pixmap->width = width;
  pixmap->height = height;
  pixmap->depth = depth;

  pixmap->parent = window->parent;
  pixmap->children = NULL;
  window->children = gdk_window_insert_sibling (window->children, pixmap);
  gdk_window_table_insert (pixmap);
  
  g_function_leave ("gdk_pixmap_new");
  return pixmap;
}

GdkPixmap *
gdk_bitmap_create_from_data (GdkWindow *window, 
			     gchar     *data,
			     gint       width, 
			     gint       height)
{
  GdkPixmap *pixmap;
  GdkWindowPrivate *private;
  GdkWindowPrivate *window_private;

  g_function_enter ("gdk_bitmap_create_from_data");

  if (!data)
    g_error ("passed NULL data to gdk_bitmap_create_from_data");

  if (!window)
    window = (GdkWindow*) &gdk_root_parent;

  private = g_new (GdkWindowPrivate, 1);
  pixmap = (GdkPixmap*) private;

  window_private = (GdkWindowPrivate*) window;
  
  private->xdisplay = window_private->xdisplay;
  private->xwindow = XCreateBitmapFromData (private->xdisplay,
					    window_private->xwindow,
					    data, width, height);

  pixmap->window_type = GDK_WINDOW_PIXMAP;
  pixmap->visual = window->visual;
  pixmap->colormap = window->colormap;
  
  pixmap->x = 0;
  pixmap->y = 0;
  pixmap->width = width;
  pixmap->height = height;
  pixmap->depth = 1;

  pixmap->parent = window->parent;
  pixmap->children = NULL;
  window->children = gdk_window_insert_sibling (window->children, pixmap);
  gdk_window_table_insert (pixmap);
  
  g_function_leave ("gdk_bitmap_create_from_data");
  return pixmap;
}

void
gdk_pixmap_destroy (GdkPixmap *pixmap)
{
  GdkWindowPrivate *private;
  
  g_function_enter ("gdk_pixmap_destroy");
  
  if (!pixmap)
    g_error ("passed NULL pixmap to gdk_pixmap_destroy");
  
  private = (GdkPixmapPrivate*) pixmap;
  XFreePixmap (private->xdisplay, private->xwindow);

  pixmap->parent->children = gdk_window_remove_sibling (pixmap->parent->children, pixmap);
  gdk_window_table_remove (pixmap);
  
  g_free (pixmap);

  g_function_leave ("gdk_pixmap_destroy");
}
