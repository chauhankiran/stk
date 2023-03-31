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
#include "gtkdata.h"
#include "gtkobserver.h"


void
gtk_data_init (GtkData *data)
{
  g_function_enter ("gtk_data_unique_type");

  g_assert (data != NULL);

  data->type = 0;
  data->observers = NULL;
  data->in_call = FALSE;
  data->need_destroy = FALSE;

  g_function_leave ("gtk_data_unique_type");
}

void
gtk_data_unique_type (guint *type)
{
  static guint next_type = 1;

  g_function_enter ("gtk_data_unique_type");

  g_assert (type != NULL);

  *type = next_type++;

  g_function_leave ("gtk_data_unique_type");
}

void
gtk_data_destroy (GtkData *data)
{
  g_function_enter ("gtk_data_destroy");

  g_assert (data != NULL);

  if (data->in_call)
    {
      data->need_destroy = TRUE;
    }
  else
    {
      gtk_data_disconnect (data);
      g_list_free (data->observers);
      g_free (data);
    }

  g_function_leave ("gtk_data_destroy");
}

void
gtk_data_attach (GtkData     *data,
		 GtkObserver *observer)
{
  g_function_enter ("gtk_data_attach");

  g_assert (data != NULL);
  g_assert (observer != NULL);

  data->observers = g_list_prepend (data->observers, observer);

  g_function_leave ("gtk_data_attach");
}

void
gtk_data_detach (GtkData     *data,
		 GtkObserver *observer)
{
  g_function_enter ("gtk_data_detach");

  g_assert (data != NULL);
  g_assert (observer != NULL);

  data->observers = g_list_remove (data->observers, observer);

  g_function_leave ("gtk_data_detach");
}

void
gtk_data_notify (GtkData *data)
{
  GList *list;

  g_function_enter ("gtk_data_notify");

  g_assert (data != NULL);

  data->in_call = TRUE;
  list = data->observers;

  while (list)
    {
      if (gtk_observer_update ((GtkObserver*) list->data, data))
	list = data->observers;
      else
	list = list->next;

      if (data->need_destroy)
	break;
    }

  data->in_call = FALSE;

  if (data->need_destroy)
    gtk_data_destroy (data);

  g_function_leave ("gtk_data_notify");
}

void
gtk_data_disconnect (GtkData *data)
{
  GList *list;

  g_function_enter ("gtk_data_disconnect");

  g_assert (data != NULL);

  list = data->observers;
  while (list)
    {
      gtk_observer_disconnect ((GtkObserver*) list->data, data);
      list = list->next;
    }

  g_function_leave ("gtk_data_disconnect");
}


GtkData*
gtk_data_int_new (gint value)
{
  GtkDataInt *data;

  g_function_enter ("gtk_data_int_new");

  data = g_new (GtkDataInt, 1);

  gtk_data_init ((GtkData*) data);
  data->data.type = gtk_data_int_type ();
  data->value = value;

  g_function_leave ("gtk_data_int_new");
  return ((GtkData*) data);
}

GtkData*
gtk_data_float_new (gfloat value)
{
  GtkDataFloat *data;

  g_function_enter ("gtk_data_float_new");

  data = g_new (GtkDataFloat, 1);

  gtk_data_init ((GtkData*) data);
  data->data.type = gtk_data_float_type ();
  data->value = value;

  g_function_leave ("gtk_data_float_new");
  return ((GtkData*) data);
}

GtkData*
gtk_data_adjustment_new (gfloat value,
			 gfloat lower,
			 gfloat upper,
			 gfloat step_increment,
			 gfloat page_increment,
			 gfloat page_size)
{
  GtkDataAdjustment *data;

  g_function_enter ("gtk_data_adjustment_new");

  data = g_new (GtkDataAdjustment, 1);

  gtk_data_init ((GtkData*) data);
  data->data.type = gtk_data_adjustment_type ();
  data->value = value;
  data->lower = lower;
  data->upper = upper;
  data->step_increment = step_increment;
  data->page_increment = page_increment;
  data->page_size = page_size;

  g_function_leave ("gtk_data_adjustment_new");
  return ((GtkData*) data);
}

GtkData*
gtk_data_widget_new (GtkWidget *widget)
{
  GtkDataWidget *data;

  g_function_enter ("gtk_data_widget_new");

  data = g_new (GtkDataWidget, 1);

  gtk_data_init ((GtkData*) data);
  data->data.type = gtk_data_widget_type ();
  data->widget = widget;

  g_function_leave ("gtk_data_widget_new");
  return ((GtkData*) data);
}

GtkData*
gtk_data_list_new (GList *list)
{
  GtkDataList *data;

  g_function_enter ("gtk_data_list_new");

  data = g_new (GtkDataList, 1);

  gtk_data_init ((GtkData*) data);
  data->data.type = gtk_data_list_type ();
  data->list = list;

  g_function_leave ("gtk_data_list_new");
  return ((GtkData*) data);
}

guint
gtk_data_int_type ()
{
  static guint int_data_type = 0;

  g_function_enter ("gtk_data_int_type");

  if (!int_data_type)
    gtk_data_unique_type (&int_data_type);

  g_function_leave ("gtk_data_int_type");
  return int_data_type;
}

guint
gtk_data_float_type ()
{
  static guint float_data_type = 0;

  g_function_enter ("gtk_data_float_type");

  if (!float_data_type)
    gtk_data_unique_type (&float_data_type);

  g_function_leave ("gtk_data_float_type");
  return float_data_type;
}

guint
gtk_data_adjustment_type ()
{
  static guint adjustment_data_type = 0;

  g_function_enter ("gtk_data_adjustment_type");

  if (!adjustment_data_type)
    gtk_data_unique_type (&adjustment_data_type);

  g_function_leave ("gtk_data_adjustment_type");
  return adjustment_data_type;
}

guint
gtk_data_widget_type ()
{
  static guint widget_data_type = 0;

  g_function_enter ("gtk_data_widget_type");

  if (!widget_data_type)
    gtk_data_unique_type (&widget_data_type);

  g_function_leave ("gtk_data_widget_type");
  return widget_data_type;
}

guint
gtk_data_list_type ()
{
  static guint list_data_type = 0;

  g_function_enter ("gtk_data_list_type");

  if (!list_data_type)
    gtk_data_unique_type (&list_data_type);

  g_function_leave ("gtk_data_list_type");
  return list_data_type;
}
