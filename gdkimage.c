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
 * File:         gdkimage.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for creating,
 *               destroying and manipulating images.
 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "gdk.h"
#include "gdkprivate.h"


static void gdk_image_put_normal (GdkWindow *window,
				  GdkGC     *gc,
				  GdkImage  *image,
				  gint       xsrc,
				  gint       ysrc,
				  gint       xdest,
				  gint       ydest,
				  gint       width,
				  gint       height);
static void gdk_image_put_shared (GdkWindow *window,
				  GdkGC     *gc,
				  GdkImage  *image,
				  gint       xsrc,
				  gint       ysrc,
				  gint       xdest,
				  gint       ydest,
				  gint       width,
				  gint       height);


static GList *image_list = NULL;


void
gdk_image_exit ()
{
  GdkImage *image;

  g_function_enter ("gdk_image_exit");

  while (image_list)
    {
      image = image_list->data;
      gdk_image_destroy (image);
    }

  g_function_leave ("gdk_image_exit");
}

GdkImage*
gdk_image_new (GdkImageType  type,
	       GdkVisual    *visual,
	       gint          width,
	       gint          height)
{
  GdkImage *image;
  GdkImagePrivate *private;
  XShmSegmentInfo *x_shm_info;
  Visual *xvisual;

  g_function_enter ("gdk_image_new");

  switch (type)
    {
    case GDK_IMAGE_FASTEST:
      image = gdk_image_new (GDK_IMAGE_SHARED, visual, width, height);

      if (!image)
	image = gdk_image_new (GDK_IMAGE_NORMAL, visual, width, height);
      break;

    default:
      private = g_new (GdkImagePrivate, 1);
      image = (GdkImage*) private;

      private->xdisplay = gdk_display;
      private->image_put = NULL;

      image->type = type;
      image->visual = visual;
      image->width = width;
      image->height = height;
      image->depth = visual->depth;

      xvisual = ((GdkVisualPrivate*) visual)->xvisual;

      switch (type)
	{
	case GDK_IMAGE_NORMAL:
	  private->image_put = gdk_image_put_normal;

	  private->ximage = XCreateImage (private->xdisplay, xvisual, visual->depth,
					  ZPixmap, 0, 0, width, height, 32, 0);

	  private->ximage->data = g_new (char, private->ximage->bytes_per_line *
					  private->ximage->height);
	  break;

	case GDK_IMAGE_SHARED:
	  private->image_put = gdk_image_put_shared;

	  private->x_shm_info = g_new (XShmSegmentInfo, 1);
	  x_shm_info = private->x_shm_info;

	  private->ximage = XShmCreateImage (private->xdisplay, xvisual, visual->depth,
					     ZPixmap, NULL, x_shm_info, width, height);

	  x_shm_info->shmid = shmget (IPC_PRIVATE,
				      private->ximage->bytes_per_line * private->ximage->height,
				      IPC_CREAT | 0777);

	  if (x_shm_info->shmid < 0)
	    g_error ("shmget failed!");

	  x_shm_info->readOnly = False;
	  x_shm_info->shmaddr = shmat (x_shm_info->shmid, 0, 0);
	  private->ximage->data = x_shm_info->shmaddr;

	  if (x_shm_info->shmaddr < (char*) 0)
	    g_error ("shmat failed!");

	  gdk_error_code = 0;
	  gdk_error_warnings = 0;

	  XShmAttach (private->xdisplay, x_shm_info);
	  XSync (private->xdisplay, False);

	  gdk_error_warnings = 1;
	  if (gdk_error_code == -1)
	    {
	      XDestroyImage (private->ximage);
	      shmdt (x_shm_info->shmaddr);
	      shmctl (x_shm_info->shmid, IPC_RMID, 0);
	      g_free (private->x_shm_info);
	      g_free (image);
	      image = NULL;
	    }
	  break;

	case GDK_IMAGE_FASTEST:
	  g_error ("danger will robinson! you should never see this");
	  break;
	}

      if (image)
	{
	  image->byte_order = private->ximage->byte_order;
	  image->mem = private->ximage->data;
	  image->bpl = private->ximage->bytes_per_line;

	  switch (private->ximage->bits_per_pixel)
	    {
	    case 8:
	      image->bpp = 1;
	      break;
	    case 16:
	      image->bpp = 2;
	      break;
	    case 24:
	      image->bpp = 3;
	      break;
	    case 32:
	      image->bpp = 4;
	      break;
	    }

	  image_list = g_list_prepend (image_list, image);
	}
    }

  g_function_leave ("gdk_image_new");
  return image;
}

GdkImage *
gdk_image_get (GdkWindow    *window,
	       gint          x,
	       gint          y,
	       gint          width,
	       gint          height)
{
  GdkImage *image;
  GdkImagePrivate *private;
  GdkWindowPrivate *win_private;

  g_function_enter ("gdk_image_get");

  if (!window)
    g_error ("passed NULL window to gdk_image_get");
  win_private = (GdkWindowPrivate *) window;

  private = g_new (GdkImagePrivate, 1);
  image = (GdkImage*) private;

  private->xdisplay = gdk_display;
  private->image_put = gdk_image_put_normal;
  private->ximage = XGetImage (private->xdisplay,
			       win_private->xwindow,
			       x, y, width, height,
			       AllPlanes, ZPixmap);

  image->type = GDK_IMAGE_NORMAL;
  image->visual = window->visual;
  image->width = width;
  image->height = height;
  image->depth = private->ximage->depth;

  image->mem = private->ximage->data;
  image->bpl = private->ximage->bytes_per_line;
  image->bpp = 1;

  image_list = g_list_prepend (image_list, image);

  g_function_leave ("gdk_image_get");
  return image;
}

guint32
gdk_image_get_pixel (GdkImage *image,
		     gint x,
		     gint y)
{
  guint32 pixel;
  GdkImagePrivate *private;

  g_function_enter ("gdk_image_get_pixel");

  if (!image)
    g_error ("passed NULL image to gdk_image_get_pixel");

  private = (GdkImagePrivate *) image;

  pixel = XGetPixel (private->ximage, x, y);

  g_function_leave ("gdk_image_get_pixel");
  return pixel;
}

void
gdk_image_put_pixel (GdkImage *image,
		     gint x,
		     gint y,
		     guint32 pixel)
{
  GdkImagePrivate *private;

  g_function_enter ("gdk_image_put_pixel");

  if (!image)
    g_error ("passed NULL image to gdk_image_put_pixel");

  private = (GdkImagePrivate *) image;

  pixel = XPutPixel (private->ximage, x, y, pixel);

  g_function_leave ("gdk_image_put_pixel");
}

void
gdk_image_destroy (GdkImage *image)
{
  GdkImagePrivate *private;
  XShmSegmentInfo *x_shm_info;

  g_function_enter ("gdk_image_destroy");

  if (!image)
    g_error ("passed NULL image to gdk_image_destroy");

  private = (GdkImagePrivate*) image;
  switch (image->type)
    {
    case GDK_IMAGE_NORMAL:
      XDestroyImage (private->ximage);
      break;

    case GDK_IMAGE_SHARED:
      XShmDetach (private->xdisplay, private->x_shm_info);
      XDestroyImage (private->ximage);

      x_shm_info = private->x_shm_info;
      shmdt (x_shm_info->shmaddr);
      shmctl (x_shm_info->shmid, IPC_RMID, 0);

      g_free (private->x_shm_info);
      break;

    case GDK_IMAGE_FASTEST:
      g_error ("danger will robinson! you should never see this");
      break;
    }

  image_list = g_list_remove (image_list, image);

  g_free (image);

  g_function_leave ("gdk_image_destroy");
}

static void
gdk_image_put_normal (GdkWindow *window,
		      GdkGC     *gc,
		      GdkImage  *image,
		      gint       xsrc,
		      gint       ysrc,
		      gint       xdest,
		      gint       ydest,
		      gint       width,
		      gint       height)
{
  GdkWindowPrivate *window_private;
  GdkImagePrivate *image_private;
  GdkGCPrivate *gc_private;

  g_function_enter ("gdk_image_put_normal");

  if (!window)
    g_error ("passed NULL window to gdk_image_put_normal");

  if (!image)
    g_error ("passed NULL image to gdk_image_put_normal");

  if (!gc)
    g_error ("passed NULL gc to gdk_image_put_normal");

  window_private = (GdkWindowPrivate*) window;
  image_private = (GdkImagePrivate*) image;
  gc_private = (GdkGCPrivate*) gc;

  if (image->type != GDK_IMAGE_NORMAL)
    g_error ("image type is not GDK_IMAGE_NORMAL");

  XPutImage (window_private->xdisplay, window_private->xwindow,
	     gc_private->xgc, image_private->ximage,
	     xsrc, ysrc, xdest, ydest, width, height);

  g_function_leave ("gdk_image_put_normal");
}

static void
gdk_image_put_shared (GdkWindow *window,
		      GdkGC     *gc,
		      GdkImage  *image,
		      gint       xsrc,
		      gint       ysrc,
		      gint       xdest,
		      gint       ydest,
		      gint       width,
		      gint       height)
{
  GdkWindowPrivate *window_private;
  GdkImagePrivate *image_private;
  GdkGCPrivate *gc_private;

  g_function_enter ("gdk_image_put_shared");

  if (!window)
    g_error ("passed NULL window to gdk_image_put_shared");

  if (!image)
    g_error ("passed NULL image to gdk_image_put_shared");

  if (!gc)
    g_error ("passed NULL gc to gdk_image_put_shared");

  window_private = (GdkWindowPrivate*) window;
  image_private = (GdkImagePrivate*) image;
  gc_private = (GdkGCPrivate*) gc;

  if (image->type != GDK_IMAGE_SHARED)
    g_error ("image type is not GDK_IMAGE_SHARED");

  XShmPutImage (window_private->xdisplay, window_private->xwindow,
		gc_private->xgc, image_private->ximage,
		xsrc, ysrc, xdest, ydest, width, height, False);

  g_function_leave ("gdk_image_put_shared");
}
