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
 * File:         gdkcursor.c
 * Author:       Spencer Kimball
 * Description:  This module contains the routines for creating,
 *               destroying cursors.
 */
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include "gdk.h"
#include "gdkprivate.h"


GdkCursor*
gdk_cursor_new (GdkCursorType cursor_type)
{
  GdkCursorPrivate *private;
  GdkCursor *cursor;
  Cursor xcursor;

  g_function_enter ("gdk_cursor_new");

  switch (cursor_type)
    {
    case GDK_LEFT_ARROW:
      xcursor = XCreateFontCursor (gdk_display, XC_top_left_arrow);
      break;
    case GDK_RIGHT_ARROW:
      xcursor = XCreateFontCursor (gdk_display, XC_arrow);
      break;
    case GDK_TEXT_CURSOR:
      xcursor = XCreateFontCursor (gdk_display, XC_xterm);
      break;
    case GDK_DIRECTIONAL:
      xcursor = XCreateFontCursor (gdk_display, XC_crosshair);
      break;
    case GDK_PENCIL:
      xcursor = XCreateFontCursor (gdk_display, XC_pencil);
      break;
    case GDK_CROSS:
      xcursor = XCreateFontCursor (gdk_display, XC_cross);
      break;
    case GDK_TCROSS:
      xcursor = XCreateFontCursor (gdk_display, XC_tcross);
      break;
    case GDK_FLEUR:
      xcursor = XCreateFontCursor (gdk_display, XC_fleur);
      break;
    case GDK_BI_ARROW_HORZ:
      xcursor = XCreateFontCursor (gdk_display, XC_sb_h_double_arrow);
      break;
    case GDK_BI_ARROW_VERT:
      xcursor = XCreateFontCursor (gdk_display, XC_sb_v_double_arrow);
      break;
    default:
      g_error ("unknown cursor specified");
      xcursor = None;
      break;
    }
  
  private = g_new (GdkCursorPrivate, 1);
  private->xdisplay = gdk_display;
  private->xcursor = xcursor;
  cursor = (GdkCursor*) private;
  cursor->type = cursor_type;

  g_function_leave ("gdk_cursor_new");
  return cursor;
}

void
gdk_cursor_destroy (GdkCursor *cursor)
{
  GdkCursorPrivate *private;

  g_function_enter ("gdk_cursor_destroy");

  if (!cursor)
    g_error ("passed NULL for cursor to gdk_cursor_destroy");

  private = (GdkCursorPrivate *) cursor;
  XFreeCursor (private->xdisplay, private->xcursor);

  g_free (private);

  g_function_leave ("gdk_cursor_destroy");
}
