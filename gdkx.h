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
#ifndef __GDK_X_H__
#define __GDK_X_H__


#include "gdkprivate.h"


#define GDK_ROOT_WINDOW()             gdk_root_window
#define GDK_DISPLAY()                 gdk_display
#define GDK_WINDOW_XDISPLAY(win)      (((GdkWindowPrivate*) win->private)->xdisplay)
#define GDK_WINDOW_XWINDOW(win)       (((GdkWindowPrivate*) win->private)->xwindow)
#define GDK_IMAGE_XDISPLAY(image)     (((GdkImagePrivate*) image->private)->xdisplay)
#define GDK_IMAGE_XIMAGE(image)       (((GdkImagePrivate*) image->private)->ximage)
#define GDK_GC_XDISPLAY(gc)           (((GdkGCPrivate*) gc->private)->xdisplay)
#define GDK_GC_XGC(gc)                (((GdkGCPrivate*) gc->private)->xgc)
#define GDK_COLORMAP_XDISPLAY(cmap)   (((GdkColormapPrivate*) cmap->private)->xdisplay)
#define GDK_COLORMAP_XCOLORMAP(cmap)  (((GdkColormapPrivate*) cmap->private)->xcolormap)
#define GDK_FONT_XDISPLAY(font)       (((GdkFontPrivate*) font->private)->xdisplay)
#define GDK_FONT_XFONT(font)          (((GdkFontPrivate*) font->private)->xfont)


#endif /* __GDK_X_H__ */
