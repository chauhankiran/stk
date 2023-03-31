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
 * File:         gdkgc.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for creating,
 *               destroying and manipulating graphics contexts.
 */
#include <X11/Xlib.h>
#include "gdk.h"
#include "gdkprivate.h"


GdkGC*
gdk_gc_new (GdkWindow *window)
{
  GdkGC *gc;
  GdkGCPrivate *private;
  Window xwindow;
  XGCValues values;
  unsigned long values_mask;
  
  g_function_enter ("gdk_gc_new");

  private = g_new (GdkGCPrivate, 1);
  gc = (GdkGC*) private;

  xwindow = ((GdkWindowPrivate*) window)->xwindow;
  private->xdisplay = ((GdkWindowPrivate*) window)->xdisplay;

  values.function = GXcopy;
  values.fill_style = FillSolid;
  values.arc_mode = ArcPieSlice;
  values.subwindow_mode = ClipByChildren;
  values.graphics_exposures = True;
  values_mask = GCFunction | GCFillStyle | GCArcMode | GCSubwindowMode | GCGraphicsExposures;
  private->xgc = XCreateGC (private->xdisplay, xwindow, values_mask, &values);

  gc->foreground.red = 65535;
  gc->foreground.green = 65535;
  gc->foreground.blue = 65535;
  gc->foreground.pixel = 0;

  gc->background.red = 0;
  gc->background.green = 0;
  gc->background.blue = 0;
  gc->background.pixel = 0;

  gc->font = NULL;
  gc->function = GDK_COPY;
  gc->fill = GDK_SOLID;
  gc->tile = NULL;
  gc->stipple = NULL;
  gc->subwindow_mode = GDK_CLIP_BY_CHILDREN;
  gc->graphics_exposures = TRUE;

  g_function_leave ("gdk_gc_new");
  return gc;
}

void
gdk_gc_destroy (GdkGC *gc)
{
  GdkGCPrivate *private;
  
  g_function_enter ("gdk_gc_destroy");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_gc_destroy");

  private = (GdkGCPrivate*) gc;
  XFreeGC (private->xdisplay, private->xgc);

  g_free (gc);

  g_function_leave ("gdk_gc_destroy");
}

void
gdk_gc_set_foreground (GdkGC    *gc, 
		       GdkColor *color)
{
  GdkGCPrivate *private;

  g_function_enter ("gdk_gc_set_foreground");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_gc_set_foreground");

  if (!color)
    g_error ("passed NULL color to gdk_gc_set_foreground");

  private = (GdkGCPrivate*) gc;
  gc->foreground = *color;
  XSetForeground (private->xdisplay, private->xgc, gc->foreground.pixel);

  g_function_leave ("gdk_gc_set_foreground");
}

void
gdk_gc_set_background (GdkGC    *gc, 
		       GdkColor *color)
{
  GdkGCPrivate *private;

  g_function_enter ("gdk_gc_set_background");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_gc_set_background");
  
  if (!color)
    g_error ("passed NULL color to gdk_gc_set_background");

  private = (GdkGCPrivate*) gc;
  gc->background = *color;
  XSetBackground (private->xdisplay, private->xgc, gc->background.pixel);

  g_function_leave ("gdk_gc_set_background");
}

void
gdk_gc_set_font (GdkGC   *gc, 
		 GdkFont *font)
{
  GdkGCPrivate *gc_private;
  GdkFontPrivate *font_private;

  g_function_enter ("gdk_gc_set_font");

  if (!gc)
    g_error ("passed NULL gc to gdk_gc_set_font");
  
  if (!font)
    g_error ("passed NULL font to gdk_gc_set_font");

  if (gc->font != font)
    {
      gc_private = (GdkGCPrivate*) gc;
      font_private = (GdkFontPrivate*) font;
      gc->font = font;
      
      XSetFont (gc_private->xdisplay, gc_private->xgc, font_private->xfont->fid);
    }

  g_function_leave ("gdk_gc_set_font");
}

void
gdk_gc_set_function (GdkGC       *gc, 
		     GdkFunction  function)
{
  GdkGCPrivate *private;

  g_function_enter ("gdk_gc_set_function");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_gc_set_function");

  if (gc->function != function)
    {
      private = (GdkGCPrivate*) gc;
      gc->function = function;
      
      switch (gc->function)
	{
	case GDK_COPY:
	  XSetFunction (private->xdisplay, private->xgc, GXcopy);
	  break;
	case GDK_INVERT:
	  XSetFunction (private->xdisplay, private->xgc, GXinvert);
	  break;
	case GDK_XOR:
	  XSetFunction (private->xdisplay, private->xgc, GXor);
	  break;
	}
    }

  g_function_leave ("gdk_gc_set_function");
}

void
gdk_gc_set_fill (GdkGC   *gc, 
		 GdkFill  fill)
{
  GdkGCPrivate *private;

  g_function_enter ("gdk_gc_set_fill");
  
  if (!gc)
    g_error ("passed NULL gc to gdk_gc_set_fill");

  if (gc->fill != fill)
    {
      private = (GdkGCPrivate*) gc;
      gc->fill = fill;
      
      switch (gc->fill)
	{
	case GDK_SOLID:
	  XSetFillStyle (private->xdisplay, private->xgc, FillSolid);
	  break;
	case GDK_TILED:
	  XSetFillStyle (private->xdisplay, private->xgc, FillTiled);
	  break;
	case GDK_STIPPLED:
	  XSetFillStyle (private->xdisplay, private->xgc, FillStippled);
	  break;
	case GDK_OPAQUE_STIPPLED:
	  XSetFillStyle (private->xdisplay, private->xgc, FillOpaqueStippled);
	  break;
	}
    }

  g_function_leave ("gdk_gc_set_fill");
}

void 
gdk_gc_set_tile (GdkGC     *gc,
		 GdkPixmap *tile)
{
  GdkGCPrivate *private;
  GdkPixmapPrivate *pixmap_private;
  Pixmap pixmap;

  g_function_enter ("gdk_gc_set_tile");
  
  g_assert (gc);
  private = (GdkGCPrivate*) gc;
  
  if (gc->tile != tile)
    {
      gc->tile = tile;
      
      pixmap = None;
      if (tile)
	{
	  pixmap_private = (GdkPixmapPrivate*) tile;
	  pixmap = pixmap_private->xwindow;
	}
      
      XSetTile (private->xdisplay, private->xgc, pixmap);
    }
  
  g_function_leave ("gdk_gc_set_tile");
}

void 
gdk_gc_set_stipple (GdkGC     *gc,
		    GdkPixmap *stipple)
{
  GdkGCPrivate *private;
  GdkPixmapPrivate *pixmap_private;
  Pixmap pixmap;

  g_function_enter ("gdk_gc_set_stipple");
  
  g_assert (gc);
  private = (GdkGCPrivate*) gc;
  
  if (gc->stipple != stipple)
    {
      gc->stipple = stipple;
      
      pixmap = None;
      if (stipple)
	{
	  pixmap_private = (GdkPixmapPrivate*) stipple;
	  pixmap = pixmap_private->xwindow;
	}
      
      XSetStipple (private->xdisplay, private->xgc, pixmap);
    }
  
  g_function_leave ("gdk_gc_set_stipple");
}

void 
gdk_gc_set_subwindow (GdkGC            *gc,
		      GdkSubwindowMode  mode)
{
  GdkGCPrivate *private;
  
  g_function_enter ("gdk_gc_set_subwindow");
  
  g_assert (gc);
  private = (GdkGCPrivate*) gc;

  if (gc->subwindow_mode != mode)
    {
      gc->subwindow_mode = mode;
      XSetSubwindowMode (private->xdisplay, private->xgc, mode);
    }
  
  g_function_leave ("gdk_gc_set_subwindow");
}

void 
gdk_gc_set_exposures (GdkGC *gc,
		      gint   exposures)
{
  GdkGCPrivate *private;
  
  g_function_enter ("gdk_gc_set_exposures");
  
  g_assert (gc);
  private = (GdkGCPrivate*) gc;

  if (gc->graphics_exposures != exposures)
    {
      gc->graphics_exposures = exposures;
      XSetGraphicsExposures (private->xdisplay, private->xgc, exposures);
    }
  
  g_function_leave ("gdk_gc_set_exposures");
}

void
gdk_gc_set_line_attributes (GdkGC       *gc,
			    gint         line_width,
			    GdkLineStyle line_style,
			    GdkCapStyle  cap_style,
			    GdkJoinStyle join_style)
{
  GdkGCPrivate *private;
  int xline_style;
  int xcap_style;
  int xjoin_style;
  
  g_function_enter ("gdk_gc_set_line_attributes");
  
  g_assert (gc);
  private = (GdkGCPrivate*) gc;

  switch (line_style)
    {
    case GDK_LINE_SOLID:
      xline_style = LineSolid;
      break;
    case GDK_LINE_ON_OFF_DASH:
      xline_style = LineOnOffDash;
      break;
    case GDK_LINE_DOUBLE_DASH:
      xline_style = LineDoubleDash;
      break;
    }

  switch (cap_style)
    {
    case GDK_CAP_NOT_LAST:
      xcap_style = CapNotLast;
      break;
    case GDK_CAP_BUTT:
      xcap_style = CapButt;
      break;
    case GDK_CAP_ROUND:
      xcap_style = CapRound;
      break;
    case GDK_CAP_PROJECTING:
      xcap_style = CapProjecting;
      break;
    }

  switch (join_style)
    {
    case GDK_JOIN_MITER:
      xjoin_style = JoinMiter;
      break;
    case GDK_JOIN_ROUND:
      xjoin_style = JoinRound;
      break;
    case GDK_JOIN_BEVEL:
      xjoin_style = JoinBevel;
      break;
    }

  XSetLineAttributes (private->xdisplay, private->xgc, line_width,
		      xline_style, xcap_style, xjoin_style);
  
  g_function_leave ("gdk_gc_set_line_attributes");
}
