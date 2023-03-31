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
 * File:         gdkglobals.c
 * Author:       Peter Mattis
 * Description:  This module contains the declarations of the
 *               global variables the library uses in multiple
 *               modules.
 */
#include <X11/Xlib.h>
#include "gdktypes.h"
#include "gdkprivate.h"

gint              gdk_debug_level = 0;
gint              gdk_show_events = 0;
gchar            *gdk_display_name = NULL;
Display          *gdk_display = NULL;
gint              gdk_screen;
Window            gdk_root_window;
GdkWindowPrivate  gdk_root_parent;
Atom              gdk_wm_delete_window;
Atom              gdk_wm_take_focus;
Atom              gdk_wm_protocols;
gchar            *gdk_progname = NULL;
gint              gdk_error_code;
gint              gdk_error_warnings = 1;
gint              gdk_motion_events = 0;
