/* GTK - The General Toolkit (written for the GIMP)
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
#include "gtktypes.h"

GtkContainer *gtk_root = NULL;
GdkColor      gtk_default_foreground = { 0, 0, 0, 0 };
GdkColor      gtk_default_background = { 0, 55000, 55000, 55000 };
GdkColor      gtk_default_selected_foreground = { 0, 0, 0, 0 };
GdkColor      gtk_default_selected_background = { 0, 45000, 45000, 55000 };
gint          gtk_default_shadow_thickness = 2;
char         *gtk_default_font_name = NULL;
