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
 * File:         gdkwindow.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for creating,
 *               destroying and manipulating windows. It also
 *               handles attaching the GdkWindow structure to
 *               the appropriate X Window.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdk.h"
#include "gdkprivate.h"


static XContext xcontext;

int nevent_masks = 15;
int event_mask_table[15] =
{
  ExposureMask,
  PointerMotionMask,
  PointerMotionHintMask,
  ButtonMotionMask,
  Button1MotionMask,
  Button2MotionMask,
  Button3MotionMask,
  ButtonPressMask | OwnerGrabButtonMask,
  ButtonReleaseMask | OwnerGrabButtonMask,
  KeyPressMask,
  KeyReleaseMask,
  EnterWindowMask,
  LeaveWindowMask,
  FocusChangeMask,
  StructureNotifyMask
};


void
gdk_window_init ()
{
  XWindowAttributes xattributes;
  unsigned int width;
  unsigned int height;
  unsigned int border_width;
  unsigned int depth;
  int x, y;

  g_function_enter ("gdk_window_init");

  xcontext = 1;

  XGetGeometry (gdk_display, gdk_root_window, &gdk_root_window,
		&x, &y, &width, &height, &border_width, &depth);
  XGetWindowAttributes (gdk_display, gdk_root_window, &xattributes);

  gdk_root_parent.xdisplay = gdk_display;
  gdk_root_parent.xwindow = gdk_root_window;

  gdk_root_parent.window.window_type = GDK_WINDOW_ROOT;
  gdk_root_parent.window.visual = gdk_visual_get_system ();
  gdk_root_parent.window.colormap = gdk_colormap_get_system ();

  gdk_root_parent.window.x = x;
  gdk_root_parent.window.y = y;
  gdk_root_parent.window.width = width;
  gdk_root_parent.window.height = height;
  gdk_root_parent.window.depth = gdk_root_parent.window.visual->depth;

  gdk_root_parent.window.parent = NULL;
  gdk_root_parent.window.children = NULL;
  gdk_root_parent.window.next_sibling = NULL;
  gdk_root_parent.window.prev_sibling = NULL;

  gdk_root_parent.window.user_data = NULL;

  g_function_leave ("gdk_window_init");
}

GdkWindow*
gdk_window_new (GdkWindow     *parent,
		GdkWindowAttr *attributes,
		gint           attributes_mask)
{
  GdkWindow *window;
  GdkWindowPrivate *private;
  GdkWindowPrivate *parent_private;
  Display *parent_display;
  Window xparent;
  Visual *xvisual;
  XSetWindowAttributes xattributes;
  long xattributes_mask;
  XSizeHints size_hints;
  XWMHints wm_hints;
  XTextProperty text_property;
  unsigned int class;
  char *title;
  int i;

  g_function_enter ("gdk_window_new");

  if (!attributes)
    g_error ("passed NULL attributes to gdk_window_new");

  if (!parent)
    parent = (GdkWindow*) &gdk_root_parent;

  parent_private = (GdkWindowPrivate*) parent;
  xparent = parent_private->xwindow;
  parent_display = parent_private->xdisplay;

  private = g_new (GdkWindowPrivate, 1);
  window = (GdkWindow*) private;

  private->xdisplay = parent_display;
  private->destroyed = FALSE;
  xattributes_mask = 0;

  if (attributes_mask & GDK_WA_X)
    window->x = attributes->x;
  else
    window->x = 0;

  if (attributes_mask & GDK_WA_Y)
    window->y = attributes->y;
  else
    window->y = 0;

  window->width = (attributes->width > 1) ? (attributes->width) : (1);
  window->height = (attributes->height > 1) ? (attributes->height) : (1);
  window->window_type = attributes->window_type;
  window->user_data = NULL;

  window->parent = parent;
  window->children = NULL;

  if (attributes_mask & GDK_WA_VISUAL)
    window->visual = attributes->visual;
  else
    window->visual = parent->visual;
  xvisual = ((GdkVisualPrivate*) window->visual)->xvisual;

  xattributes.event_mask = 0;
  for (i = 0; i < nevent_masks; i++)
    {
      if (attributes->event_mask & (1 << (i + 1)))
	xattributes.event_mask |= event_mask_table[i];
    }

  xattributes.event_mask |= StructureNotifyMask;
  if (gdk_motion_events &&
      !(attributes->event_mask & GDK_POINTER_MOTION_HINT_MASK))
    xattributes.event_mask |= PointerMotionMask;

  if (xattributes.event_mask)
    xattributes_mask |= CWEventMask;

  if (attributes->wclass == GDK_INPUT_OUTPUT)
    {
      class = InputOutput;
      window->depth = window->visual->depth;

      if (attributes_mask & GDK_WA_COLORMAP)
	window->colormap = attributes->colormap;
      else
	window->colormap = parent->colormap;

      xattributes.background_pixel = BlackPixel (gdk_display, gdk_screen);
      xattributes.border_pixel = BlackPixel (gdk_display, gdk_screen);
      xattributes_mask |= CWBorderPixel | CWBackPixel;

      switch (window->window_type)
	{
	case GDK_WINDOW_TOPLEVEL:
	  xattributes.colormap = ((GdkColormapPrivate*) window->colormap)->xcolormap;
	  xattributes_mask |= CWColormap;

	  xparent = gdk_root_window;
	  break;

	case GDK_WINDOW_CHILD:
	  xattributes.colormap = ((GdkColormapPrivate*) window->colormap)->xcolormap;
	  xattributes_mask |= CWColormap;
	  break;

	case GDK_WINDOW_DIALOG:
	  xattributes.colormap = ((GdkColormapPrivate*) window->colormap)->xcolormap;
	  xattributes_mask |= CWColormap;

	  xparent = gdk_root_window;
	  break;

	case GDK_WINDOW_TEMP:
	  xattributes.colormap = ((GdkColormapPrivate*) window->colormap)->xcolormap;
	  xattributes_mask |= CWColormap;

	  xparent = gdk_root_window;

	  xattributes.save_under = True;
	  xattributes.override_redirect = True;
	  xattributes.cursor = None;
	  xattributes_mask |= CWSaveUnder | CWOverrideRedirect;
	  break;
	case GDK_WINDOW_ROOT:
	  g_error ("cannot make windows of type GDK_WINDOW_ROOT");
	  break;
	case GDK_WINDOW_PIXMAP:
	  g_error ("cannot make windows of type GDK_WINDOW_PIXMAP (use gdk_pixmap_new)");
	  break;
	}
    }
  else
    {
      class = InputOnly;
      window->depth = 0;
    }

  private->xwindow = XCreateWindow (private->xdisplay, xparent,
				    window->x, window->y,
				    window->width, window->height,
				    0, window->depth, class, xvisual,
				    xattributes_mask, &xattributes);

  parent->children = gdk_window_insert_sibling (parent->children, window);
  gdk_window_table_insert (window);

  switch (window->window_type)
    {
    case GDK_WINDOW_DIALOG:
      XSetTransientForHint (private->xdisplay, private->xwindow, xparent);
    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_TEMP:
      XSetWMProtocols (private->xdisplay, private->xwindow, &gdk_wm_delete_window, 1);
      XSetWMProtocols (private->xdisplay, private->xwindow, &gdk_wm_take_focus, 1);

      if (attributes_mask & GDK_WA_COLORMAP)
	XSetWMColormapWindows (private->xdisplay, private->xwindow,
			       &private->xwindow, 1);
      break;
    case GDK_WINDOW_CHILD:
      if (attributes_mask & GDK_WA_COLORMAP)
	gdk_window_add_colormap_windows (window);
      break;
    default:
      break;
    }

  size_hints.flags = PSize | PBaseSize;
  size_hints.width = window->width;
  size_hints.height = window->height;
  size_hints.base_width = window->width;
  size_hints.base_height = window->height;

  wm_hints.flags = InputHint | StateHint;
  wm_hints.input = True;
  wm_hints.initial_state = NormalState;

  XSetWMNormalHints (private->xdisplay, private->xwindow, &size_hints);
  XSetWMHints (private->xdisplay, private->xwindow, &wm_hints);

  if (attributes_mask & GDK_WA_TITLE)
    title = attributes->title;
  else
    title = gdk_progname;

  if (XStringListToTextProperty (&title, 1, &text_property))
    XSetWMName (private->xdisplay, private->xwindow, &text_property);

  gdk_window_set_cursor (window, ((attributes_mask & GDK_WA_CURSOR) ?
				  (attributes->cursor) :
				  NULL));

  g_function_leave ("gdk_window_new");
  return window;
}

void
gdk_window_destroy (GdkWindow *window)
{
  GdkWindowPrivate *private;
  GdkWindow *temp_window;

  g_function_enter ("gdk_window_destroy");

  if (!window)
    g_error ("passed NULL window to gdk_window_destroy");

  switch (window->window_type)
    {
    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_CHILD:
    case GDK_WINDOW_DIALOG:
    case GDK_WINDOW_TEMP:
      private = (GdkWindowPrivate*) window;

      if (!private->destroyed)
	{
	  private->destroyed = TRUE;
	  XDestroyWindow (private->xdisplay, private->xwindow);

	  temp_window = window->children;
	  while (temp_window)
	    {
	      private = (GdkWindowPrivate*) temp_window;
	      private->destroyed = TRUE;
	      temp_window = temp_window->next_sibling;
	    }
	}
      break;

    case GDK_WINDOW_ROOT:
      g_error ("attempted to destroy root window");
      break;

    case GDK_WINDOW_PIXMAP:
      g_warning ("called gdk_window_destroy on a pixmap (use gdk_pixmap_destroy)");
      gdk_pixmap_destroy (window);
      break;
    }

  g_function_leave ("gdk_window_destroy");
}

void
gdk_window_real_destroy (GdkWindow *window)
{
  g_function_enter ("gdk_real_window_destroy");

  if (!window)
    g_error ("passed NULL window to gdk_real_window_destroy");

  window->parent->children = gdk_window_remove_sibling (window->parent->children, window);
  gdk_window_table_remove (window);

  g_free (window);

  g_function_leave ("gdk_real_window_destroy");
}

void
gdk_window_show (GdkWindow *window)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_show");

  if (!window)
    g_error ("passed NULL window to gdk_window_show");

  private = (GdkWindowPrivate*) window;
  XRaiseWindow (private->xdisplay, private->xwindow);
  XMapWindow (private->xdisplay, private->xwindow);

  g_function_leave ("gdk_window_show");
}

void
gdk_window_hide (GdkWindow *window)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_hide");

  if (!window)
    g_error ("passed NULL window to gdk_window_hide");

  private = (GdkWindowPrivate*) window;
  XUnmapWindow (private->xdisplay, private->xwindow);

  g_function_leave ("gdk_window_hide");
}

void
gdk_window_move (GdkWindow *window,
		 gint       x,
		 gint       y)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_move");

  if (!window)
    g_error ("passed NULL window to gdk_window_move");

  private = (GdkWindowPrivate*) window;
  XMoveWindow (private->xdisplay, private->xwindow, x, y);

  window->x = x;
  window->y = y;

  g_function_leave ("gdk_window_move");
}

void
gdk_window_reparent (GdkWindow *window,
		     GdkWindow *new_parent,
		     gint       x,
		     gint       y)
{
  GdkWindowPrivate *window_private;
  GdkWindowPrivate *parent_private;

  g_function_enter ("gdk_window_reparent");

  if (!window)
    g_error ("passed NULL window to gdk_window_reparent");

  if (!new_parent)
    new_parent = (GdkWindow*) &gdk_root_parent;

  window_private = (GdkWindowPrivate*) window;
  parent_private = (GdkWindowPrivate*) new_parent;

  XReparentWindow (window_private->xdisplay,
		   window_private->xwindow,
		   parent_private->xwindow,
		   x, y);

  g_function_leave ("gdk_window_reparent");
}

void
gdk_window_clear (GdkWindow *window)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_clear");

  if (!window)
    g_error ("passed NULL window to gdk_window_clear");

  private = (GdkWindowPrivate*) window;

  XClearWindow (private->xdisplay, private->xwindow);

  g_function_leave ("gdk_window_clear");
}

void
gdk_window_clear_area (GdkWindow *window,
		       gint       x,
		       gint       y,
		       gint       width,
		       gint       height)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_clear_area");

  if (!window)
    g_error ("passed NULL window to gdk_window_clear_area");

  private = (GdkWindowPrivate*) window;

  XClearArea (private->xdisplay, private->xwindow,
	      x, y, width, height, False);

  g_function_leave ("gdk_window_clear_area");
}

void
gdk_window_raise (GdkWindow *window)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_raise");

  if (!window)
    g_error ("passed NULL window to gdk_window_raise");

  private = (GdkWindowPrivate*) window;

  XRaiseWindow (private->xdisplay, private->xwindow);

  g_function_leave ("gdk_window_raise");
}

void
gdk_window_lower (GdkWindow *window)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_lower");

  if (!window)
    g_error ("passed NULL window to gdk_window_lower");

  private = (GdkWindowPrivate*) window;

  XLowerWindow (private->xdisplay, private->xwindow);

  g_function_leave ("gdk_window_lower");
}

void
gdk_window_set_user_data (GdkWindow *window,
			  gpointer   user_data)
{
  g_function_enter ("gdk_window_set_user_data");

  if (!window)
    g_error ("passed NULL window to gdk_window_set_user_data");

  window->user_data = user_data;

  g_function_leave ("gdk_window_set_user_data");
}

void
gdk_window_set_size (GdkWindow *window,
		     gint       width,
		     gint       height)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_set_size");

  if (!window)
    g_error ("passed NULL window to gdk_window_set_size");

  private = (GdkWindowPrivate*) window;

  if ((window->width != width) || (window->height != height))
    {
      XResizeWindow (private->xdisplay, private->xwindow, width, height);

      if (window->window_type == GDK_WINDOW_CHILD)
	{
	  window->width = width;
	  window->height = height;
	}
    }

  g_function_leave ("gdk_window_set_size");
}

void
gdk_window_set_sizes (GdkWindow *window,
		      gint       min_width,
		      gint       min_height,
		      gint       max_width,
		      gint       max_height,
		      gint       flags)
{
  GdkWindowPrivate *private;
  XSizeHints size_hints;

  g_function_enter ("gdk_window_set_sizes");

  if (!window)
    g_error ("passwd NULL window to gdk_window_set_sizes");

  private = (GdkWindowPrivate*) window;
  size_hints.flags = 0;

  if (flags & GDK_MIN_SIZE)
    {
      size_hints.flags |= PMinSize;
      size_hints.min_width = min_width;
      size_hints.min_height = min_height;
    }

  if (flags & GDK_MAX_SIZE)
    {
      size_hints.flags |= PMaxSize;
      size_hints.max_width = max_width;
      size_hints.max_height = max_height;
    }

  if (flags)
    XSetWMNormalHints (private->xdisplay, private->xwindow, &size_hints);

  g_function_leave ("gdk_window_set_sizes");
}

void gdk_window_set_position (GdkWindow  *window,
			      gint        x,
			      gint        y)
{
  GdkWindowPrivate *private;
  XSizeHints size_hints;

  g_function_enter ("gdk_window_set_position");

  if (!window)
    g_error ("passwd NULL window to gdk_window_set_sizes");

  private = (GdkWindowPrivate*) window;

  size_hints.flags = USPosition|PPosition;
  size_hints.x = x;
  size_hints.y = y;

  XSetWMNormalHints (private->xdisplay, private->xwindow, &size_hints);

  g_function_leave ("gdk_window_set_position");
}

void
gdk_window_set_title (GdkWindow *window,
		      gchar     *title)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_set_title");

  if (!window)
    g_error ("passed NULL window to gdk_window_title");

  private = (GdkWindowPrivate*) window;
  XStoreName (private->xdisplay, private->xwindow, title);

  g_function_leave ("gdk_window_set_title");
}

void
gdk_window_set_background (GdkWindow *window,
			   GdkColor  *color)
{
  GdkWindowPrivate *private;

  g_function_enter ("gdk_window_set_background");

  if (!window)
    g_error ("passed NULL window to gdk_window_set_background");

  private = (GdkWindowPrivate*) window;
  XSetWindowBackground (private->xdisplay, private->xwindow, color->pixel);

  g_function_leave ("gdk_window_set_background");
}

void
gdk_window_set_cursor (GdkWindow     *window,
		       GdkCursor     *cursor)
{
  GdkWindowPrivate *window_private;
  GdkCursorPrivate *cursor_private;
  Cursor xcursor;

  g_function_enter ("gdk_window_set_cursor");

  if (!window)
    g_error ("passed NULL window to gdk_window_set_cursor");

  window_private = (GdkWindowPrivate*) window;
  cursor_private = (GdkCursorPrivate*) cursor;

  if (!cursor)
    xcursor = None;
  else
    xcursor = cursor_private->xcursor;

  XDefineCursor (window_private->xdisplay, window_private->xwindow, xcursor);

  g_function_leave ("gdk_window_set_cursor");
}

void
gdk_window_set_colormap (GdkWindow   *window,
			 GdkColormap *colormap)
{
  GdkWindowPrivate *window_private;
  GdkColormapPrivate *colormap_private;

  g_function_enter ("gdk_window_set_colormap");

  if (!window)
    g_error ("passed NULL window to gdk_window_set_colormap");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_window_set_colormap");

  window_private = (GdkWindowPrivate*) window;
  colormap_private = (GdkColormapPrivate*) colormap;

  XSetWindowColormap (window_private->xdisplay,
		      window_private->xwindow,
		      colormap_private->xcolormap);

  if (window->window_type != GDK_WINDOW_TOPLEVEL)
    gdk_window_add_colormap_windows (window);

  g_function_leave ("gdk_window_set_colormap");
}

void
gdk_window_get_user_data (GdkWindow *window,
			  gpointer  *data)
{
  g_function_enter ("gdk_window_get_user_data");

  if (!window)
    g_error ("passed NULL window to gdk_window_get_user_data");

  *data = window->user_data;

  g_function_leave ("gdk_window_get_user_data");
}

gint
gdk_window_get_origin (GdkWindow *window,
		       gint      *x,
		       gint      *y)
{
  GdkWindowPrivate *private;
  gint return_val;
  Window child;

  g_function_enter ("gdk_window_get_origin");

  if (!window)
    g_error ("passed NULL window to gdk_window_get_origin");

  if (!x || !y)
    g_error ("passed NULL x or y value to gdk_window_get_origin");

  private = (GdkWindowPrivate*) window;

  return_val = XTranslateCoordinates (private->xdisplay,
				      private->xwindow,
				      gdk_root_window,
				      0, 0, x, y,
				      &child);

  g_function_leave ("gdk_window_get_origin");
  return return_val;
}

GdkWindow*
gdk_window_get_pointer (GdkWindow       *window,
			gint            *x,
			gint            *y,
			GdkModifierType *mask)
{
  GdkWindowPrivate *private;
  GdkWindow *return_val;
  Window root;
  Window child;
  int rootx, rooty;
  int winx, winy;
  unsigned int xmask;

  g_function_enter ("gdk_window_get_pointer");

  if (!window)
    window = (GdkWindow*) &gdk_root_parent;

  private = (GdkWindowPrivate*) window;

  return_val = NULL;
  if (XQueryPointer (private->xdisplay, private->xwindow, &root, &child,
		     &rootx, &rooty, &winx, &winy, &xmask))
    {
      if (x) *x = winx;
      if (y) *y = winy;
      if (mask) *mask = xmask;

      if (child)
	return_val = gdk_window_table_lookup (child);
    }

  g_function_leave ("gdk_window_get_pointer");
  return return_val;
}

GdkWindow*
gdk_window_get_parent (GdkWindow *window)
{
  g_function_enter ("gdk_window_get_parent");

  if (!window)
    g_error ("passed NULL window to gdk_window_get_parent");

  g_function_leave ("gdk_window_get_parent");
  return window->parent;
}

GdkWindow*
gdk_window_get_toplevel (GdkWindow *window)
{
  g_function_enter ("gdk_window_get_toplevel");

  if (!window)
    g_error ("passed NULL window to gdk_window_get_toplevel");

  while (window->parent && (window->window_type != GDK_WINDOW_TOPLEVEL))
    window = window->parent;

  g_function_leave ("gdk_window_get_toplevel");
  return window;
}


GdkWindow*
gdk_window_find (Window xwindow)
{
  GdkWindow *return_val;

  g_function_enter ("gdk_window_find");
  return_val = gdk_window_table_lookup (xwindow);
  g_function_leave ("gdk_window_find");

  return return_val;
}


void
gdk_window_table_insert (GdkWindow *window)
{
  GdkWindowPrivate *private;
  int val;

  g_function_enter ("gdk_window_table_insert");

  if (!window)
    g_error ("passed NULL window to gdk_window_table_insert");

  private = (GdkWindowPrivate*) window;

  val = XSaveContext (private->xdisplay, private->xwindow,
		      xcontext, (XPointer) window);

  g_function_leave ("gdk_window_table_insert");
}

void
gdk_window_table_remove (GdkWindow *window)
{
  GdkWindowPrivate *private;
  int val;

  g_function_enter ("gdk_window_table_remove");

  if (!window)
    g_error ("passed NULL window to gdk_window_table_remove");

  private = (GdkWindowPrivate*) window;
  val = XDeleteContext (private->xdisplay, private->xwindow, xcontext);

  g_function_leave ("gdk_window_table_remove");
}

GdkWindow*
gdk_window_table_lookup (Window xwindow)
{
  GdkWindow *window;
  int val;

  g_function_enter ("gdk_window_table_lookup");

  val = XFindContext (gdk_display, xwindow,
		      xcontext, (XPointer*) &window);

  g_function_leave ("gdk_window_table_lookup");
  return window;
}

GdkWindow*
gdk_window_insert_sibling (GdkWindow *windows,
			   GdkWindow *sibling)
{
  g_function_enter ("gdk_window_insert_sibling");

  sibling->next_sibling = windows;
  sibling->prev_sibling = NULL;

  if (windows)
    windows->prev_sibling = sibling;

  g_function_leave ("gdk_window_insert_sibling");
  return sibling;
}

GdkWindow*
gdk_window_remove_sibling (GdkWindow *windows,
			   GdkWindow *sibling)
{
  GdkWindow *return_val;

  g_function_enter ("gdk_window_remove_sibling");

  return_val = windows;
  if (sibling)
    {
      if (sibling->prev_sibling)
	sibling->prev_sibling->next_sibling = sibling->next_sibling;
      if (sibling->next_sibling)
	sibling->next_sibling->prev_sibling = sibling->prev_sibling;

      if (sibling == windows)
	return_val = sibling->next_sibling;
      else
	return_val = windows;
    }

  g_function_leave ("gdk_window_remove_sibling");
  return return_val;
}

void
gdk_window_add_colormap_windows (GdkWindow *window)
{
  GdkWindow *toplevel;
  GdkWindowPrivate *toplevel_private;
  GdkWindowPrivate *window_private;
  Window *old_windows;
  Window *new_windows;
  int i, count;

  g_function_enter ("gdk_window_add_colormap_windows");

  if (!window)
    g_error ("passed NULL window to gdk_window_add_colormap_windows");

  toplevel = gdk_window_get_toplevel (window);
  toplevel_private = (GdkWindowPrivate*) toplevel;
  window_private = (GdkWindowPrivate*) window;

  if (!XGetWMColormapWindows (toplevel_private->xdisplay,
			      toplevel_private->xwindow,
			      &old_windows, &count))
    {
      old_windows = NULL;
      count = 0;
    }

  for (i = 0; i < count; i++)
    if (old_windows[i] == window_private->xwindow)
      goto done;

  new_windows = g_new (Window, count + 1);

  for (i = 0; i < count; i++)
    new_windows[i] = old_windows[i];
  new_windows[count] = window_private->xwindow;

  XSetWMColormapWindows (toplevel_private->xdisplay,
			 toplevel_private->xwindow,
			 new_windows, count + 1);

  g_free (new_windows);
  if (old_windows)
    XFree (old_windows);

 done:
  g_function_leave ("gdk_window_add_colormap_windows");
}
