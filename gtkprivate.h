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
#ifndef __GTK_PRIVATE_H__
#define __GTK_PRIVATE_H__


#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef void (*GtkWindowResizeHook) (GtkWidget *, gint *, gint *, gint *, gint *);


void gtk_window_set_resize_hook (GtkWidget           *widget,
				 GtkWindowResizeHook  resize);

void gtk_accelerator_table_install (GtkAcceleratorTable *table,
				    GtkWidget           *widget,
				    gchar                accelerator_key,
				    guint8               accelerator_mods);
void gtk_accelerator_table_remove  (GtkAcceleratorTable *table,
				    GtkWidget           *widget);
gint gtk_accelerator_table_check   (GtkAcceleratorTable *table,
				    gchar                accelerator_key,
				    guint8               accelerator_mods);


extern GtkContainer *gtk_root;
extern GdkColor      gtk_default_foreground;
extern GdkColor      gtk_default_background;
extern GdkColor      gtk_default_selected_foreground;
extern GdkColor      gtk_default_selected_background;
extern gint          gtk_default_shadow_thickness;
extern char         *gtk_default_font_name;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_PRIVATE_H__ */
