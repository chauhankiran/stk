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
 * File:         gdkdraw.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for drawing.
 */
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include "gdk.h"
#include "gdkprivate.h"


void
gdk_draw_move (GdkGC *gc, 
	       gint   x, 
	       gint   y)
{
  GdkGCPrivate *private;

  g_function_enter ("gdk_draw_move");

  if (!gc)
    g_error ("passed NULL gc to gdk_draw_move");

  private = (GdkGCPrivate*) gc;
  private->x = x;
  private->y = y;
  
  g_function_leave ("gdk_draw_move");
}

void
gdk_draw_move_rel (GdkGC *gc, 
		   gint   dx, 
		   gint   dy)
{
  GdkGCPrivate *private;

  g_function_enter ("gdk_draw_move_rel");

  if (!gc)
    g_error ("passed NULL gc to gdk_draw_move_rel");

  private = (GdkGCPrivate*) gc;
  private->x += dx;
  private->y += dy;
  
  g_function_leave ("gdk_draw_move_rel");
}

void
gdk_draw_line (GdkWindow *window, 
	       GdkGC     *gc, 
	       gint       x1, 
	       gint       y1, 
	       gint       x2, 
	       gint       y2)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_line");

  if (!window)
    g_error ("passed NULL window to gdk_draw_line");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_line");
  
  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  XDrawLine (window_private->xdisplay, window_private->xwindow,
	     gc_private->xgc, x1, y1, x2, y2);
  
  g_function_leave ("gdk_draw_line");
}

void
gdk_draw_line_rel (GdkWindow *window, 
		   GdkGC     *gc, 
		   gint       dx, 
		   gint       dy)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_line_rel");

  if (!window)
    g_error ("passed NULL window to gdk_draw_line_rel");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_line_rel");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  XDrawLine (window_private->xdisplay, window_private->xwindow,
	     gc_private->xgc, gc_private->x, gc_private->y,
	     gc_private->x + dx, gc_private->y + dy);

  gc_private->x += dx;
  gc_private->y += dy;

  g_function_leave ("gdk_draw_line_rel");
}

void
gdk_draw_rectangle (GdkWindow *window, 
		    GdkGC     *gc, 
		    gint       filled, 
		    gint       x, 
		    gint       y, 
		    gint       width, 
		    gint       height)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_rectangle");

  if (!window)
    g_error ("passed NULL window to gdk_draw_rectangle");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_rectangle");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  if (filled)
    XFillRectangle (window_private->xdisplay, window_private->xwindow,
		    gc_private->xgc, x, y, width, height);
  else
    XDrawRectangle (window_private->xdisplay, window_private->xwindow,
		    gc_private->xgc, x, y, width, height);

  g_function_leave ("gdk_draw_rectangle");
}

void
gdk_draw_arc (GdkWindow *window, 
	      GdkGC     *gc, 
	      gint       filled, 
	      gint       x, 
	      gint       y, 
	      gint       width, 
	      gint       height, 
	      gint       angle1, 
	      gint       angle2)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_ellipse");

  if (!window)
    g_error ("passed NULL window to gdk_draw_ellipse");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_ellipse");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  if (filled)
    XFillArc (window_private->xdisplay, window_private->xwindow,
	      gc_private->xgc, x, y, width, height, angle1, angle2);
  else
    XDrawArc (window_private->xdisplay, window_private->xwindow,
	      gc_private->xgc, x, y, width, height, angle1, angle2);

  g_function_leave ("gdk_draw_ellipse");
}

void
gdk_draw_polygon (GdkWindow *window, 
		  GdkGC     *gc, 
		  gint       filled, 
		  GdkPoint  *points, 
		  gint       npoints)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_polygon");

  if (!window)
    g_error ("passed NULL window to gdk_draw_polygon");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_polygon");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  if (filled)
    {
      XFillPolygon (window_private->xdisplay, window_private->xwindow,
		    gc_private->xgc, (XPoint*) points, npoints, Complex, CoordModeOrigin);
    }
  else
    {
      XDrawLines (window_private->xdisplay, window_private->xwindow,
		  gc_private->xgc, (XPoint*) points, npoints, CoordModeOrigin);

      if ((points[0].x != points[npoints-1].x) ||
	  (points[0].y != points[npoints-1].y))
	XDrawLine (window_private->xdisplay, window_private->xwindow,
		   gc_private->xgc, points[npoints-1].x, points[npoints-1].y,
		   points[0].x, points[0].y);
    }

  g_function_leave ("gdk_draw_polygon");
}

void 
gdk_draw_string (GdkWindow *window, 
		 GdkGC     *gc, 
		 gint       x, 
		 gint       y, 
		 gchar     *string)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_string");
  
  if (!window)
    g_error ("passed NULL window to gdk_draw_string");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_string");

  if (!string)
    g_error ("passed NULL string to gdk_draw_string");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  XDrawString (window_private->xdisplay, window_private->xwindow,
	       gc_private->xgc, x, y, string, strlen (string));
  
  g_function_leave ("gdk_draw_string");
}

void 
gdk_draw_text (GdkWindow *window, 
	       GdkGC     *gc, 
	       gint       x, 
	       gint       y, 
	       gchar     *text,
	       gint       text_length)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_text");
  
  if (!window)
    g_error ("passed NULL window to gdk_draw_text");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_text");

  if (!text)
    g_error ("passed NULL text to gdk_draw_text");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  XDrawString (window_private->xdisplay, window_private->xwindow,
	       gc_private->xgc, x, y, text, text_length);
  
  g_function_leave ("gdk_draw_text");
}

void
gdk_draw_pixmap (GdkWindow *window, 
		 GdkGC     *gc, 
		 GdkPixmap *pixmap, 
		 gint       xsrc, 
		 gint       ysrc, 
		 gint       xdest, 
		 gint       ydest, 
		 gint       width, 
		 gint       height)
{
  GdkWindowPrivate *window_private;
  GdkWindowPrivate *pixmap_private;
  GdkGCPrivate *gc_private;
  
  g_function_enter ("gdk_draw_pixmap");

  if (!window)
    g_error ("passed NULL window to gdk_draw_pixmap");
  
  if (!pixmap)
    g_error ("passed NULL pixmap to gdk_draw_pixmap");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_pixmap");

  window_private = (GdkWindowPrivate*) window;
  pixmap_private = (GdkPixmapPrivate*) pixmap;
  gc_private = (GdkGCPrivate*) gc;

  XCopyArea (window_private->xdisplay, 
	     pixmap_private->xwindow,
	     window_private->xwindow,
	     gc_private->xgc, 
	     xsrc, ysrc, 
	     width, height, 
	     xdest, ydest);

  g_function_leave ("gdk_draw_pixmap");
}

void
gdk_draw_image (GdkWindow *window, 
		GdkGC     *gc, 
		GdkImage  *image, 
		gint       xsrc, 
		gint       ysrc, 
		gint       xdest, 
		gint       ydest, 
		gint       width, 
		gint       height)
{
  GdkImagePrivate *image_private;
  
  g_function_enter ("gdk_draw_image");

  if (!window)
    g_error ("passed NULL window to gdk_draw_image");
  
  if (!image)
    g_error ("passed NULL image to gdk_draw_image");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_image");

  image_private = (GdkImagePrivate*) image;

  if (!image_private->image_put)
    g_error ("unable to draw image...NULL function pointer");

  (* image_private->image_put) (window, gc, image, xsrc, ysrc, 
				xdest, ydest, width, height);

  g_function_leave ("gdk_draw_image");
}

void
gdk_draw_points (GdkWindow  *window, 
		 GdkGC      *gc, 
		 GdkPoint   *points,
		 gint        npoints)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;

  g_function_enter ("gdk_draw_points");

  if (!window)
    g_error ("passed NULL window to gdk_draw_points");
  
  if (!points && npoints)
    g_error ("passed NULL segments to gdk_draw_points");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_points");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  XDrawPoints (window_private->xdisplay,
	       window_private->xwindow, 
	       gc_private->xgc,
	       (XPoint *) points,
	       npoints,
	       CoordModeOrigin);

  g_function_leave ("gdk_draw_points");
}

void
gdk_draw_segments (GdkWindow  *window, 
		   GdkGC      *gc, 
		   GdkSegment *segs, 
		   gint        nsegs)
{
  GdkWindowPrivate *window_private;
  GdkGCPrivate *gc_private;

  g_function_enter ("gdk_draw_segments");

  if (!window)
    g_error ("passed NULL window to gdk_draw_segments");
  
  if (!segs && nsegs)
    g_error ("passed NULL segments to gdk_draw_segments");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_draw_segments");

  window_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  XDrawSegments (window_private->xdisplay,
		 window_private->xwindow, 
		 gc_private->xgc,
		 (XSegment *) segs,
		 nsegs);

  g_function_leave ("gdk_draw_segments");
}
