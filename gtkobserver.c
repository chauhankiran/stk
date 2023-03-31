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
#include "gtkobserver.h"


gint
gtk_observer_update (GtkObserver *observer,
		     GtkData     *data)
{
  gint return_val;
  
  g_function_enter ("gtk_observer_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);
  g_assert (observer->update != NULL);

  return_val = (* observer->update) (observer, data);
  
  g_function_leave ("gtk_observer_update");
  return return_val;
}

void
gtk_observer_disconnect (GtkObserver *observer,
			 GtkData     *data)
{
  g_function_enter ("gtk_observer_update");

  g_assert (observer != NULL);
  g_assert (data != NULL);
  g_assert (observer->disconnect != NULL);

  (* observer->disconnect) (observer, data);
  
  g_function_leave ("gtk_observer_update");
}
