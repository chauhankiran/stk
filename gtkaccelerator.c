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
#include <ctype.h>
#include "gtkwidget.h"
#include "gtkprivate.h"


#define BASE_KEY    0x20
#define LAST_KEY    0x7E
#define NUM_KEYS    LAST_KEY - BASE_KEY
#define TABLE_SIZE  69


static void gtk_accelerator_table_init    (GtkAcceleratorTable *table);
static void gtk_accelerator_table_clean   (GtkAcceleratorTable *table);


static int key_mappings[95] =
{
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
  50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, 63, 64, 33, 34, 35, 36, 37,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 65, 66, 67, 68,
};

/*
static int inverse_key_mappings[69] =
{
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
  50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, 63, 64, 65, 92, 93, 94,
};
*/


GtkAcceleratorTable*
gtk_accelerator_table_new ()
{
  GtkAcceleratorTable *table;

  g_function_enter ("gtk_accelerator_table_new");

  table = g_new (GtkAcceleratorTable, 1);
  gtk_accelerator_table_init (table);

  g_function_leave ("gtk_accelerator_table_new");
  return table;
}

void
gtk_accelerator_table_destroy (GtkAcceleratorTable *table)
{
  g_function_enter ("gtk_accelerator_table_destroy");

  g_assert (table != NULL);
  g_assert (table->ref_count <= 0);

  gtk_accelerator_table_clean (table);
  g_free (table);

  g_function_leave ("gtk_accelerator_table_destroy");
}

void
gtk_accelerator_table_ref (GtkAcceleratorTable *table)
{
  g_function_enter ("gtk_accelerator_table_ref");

  g_assert (table != NULL);

  table->ref_count += 1;

  g_function_leave ("gtk_accelerator_table_ref");
}

void
gtk_accelerator_table_unref (GtkAcceleratorTable *table)
{
  g_function_enter ("gtk_accelerator_table_unref");

  g_assert (table != NULL);

  table->ref_count -= 1;
  if (table->ref_count <= 0)
    gtk_accelerator_table_destroy (table);

  g_function_leave ("gtk_accelerator_table_unref");
}

void
gtk_accelerator_table_install (GtkAcceleratorTable *table,
			       GtkWidget           *widget,
			       gchar                accelerator_key,
			       guint8               accelerator_mods)
{
  GtkAcceleratorEntry *entry;
  GList *entries;
  GList *temp_list;
  gint hash;

  g_function_leave ("gtk_accelerator_table_install");

  g_assert (table != NULL);
  g_assert (widget != NULL);
  g_assert ((accelerator_key >= BASE_KEY) && (accelerator_key <= LAST_KEY));

  hash = key_mappings[(int) (accelerator_key - BASE_KEY)];
  entries = table->entries[hash];

  while (entries)
    {
      entry = entries->data;

      if (entry->modifiers == accelerator_mods)
	{
	  g_assert (entry->widget);
	  g_assert (entry->widget->function_table);
	  g_assert (entry->widget->function_table->remove_accelerator);

	  (* entry->widget->function_table->remove_accelerator) (entry->widget);
	  g_free (entry);

	  if (entries == table->entries[hash])
	    table->entries[hash] = entries->next;

	  temp_list = entries;
	  if (entries->next)
	    entries->next->prev = entries->prev;
	  if (entries->prev)
	    entries->prev->next = entries->next;
	  temp_list->next = NULL;
	  temp_list->prev = NULL;
	  g_list_free (temp_list);

	  break;
	}

      entries = entries->next;
    }

  entry = g_new (GtkAcceleratorEntry, 1);
  entry->modifiers = accelerator_mods;
  entry->widget = widget;

  table->entries[hash] = g_list_prepend (table->entries[hash], entry);

  g_function_leave ("gtk_accelerator_table_install");
}

void
gtk_accelerator_table_remove (GtkAcceleratorTable *table,
			      GtkWidget           *widget)
{
  GtkAcceleratorEntry *entry;
  GList *entries;
  GList *temp_list;
  gint i;

  g_function_leave ("gtk_accelerator_table_remove");

  g_assert (table != NULL);
  g_assert (widget != NULL);

  for (i = 0; i < TABLE_SIZE; i++)
    {
      entries = table->entries[i];

      while (entries)
	{
	  entry = entries->data;

	  if (entry->widget == widget)
	    {
	      g_assert (entry->widget);
	      g_assert (entry->widget->function_table);
	      g_assert (entry->widget->function_table->remove_accelerator);

	      (* entry->widget->function_table->remove_accelerator) (entry->widget);
	      g_free (entry);

	      temp_list = entries;
	      if (entries->next)
		entries->next->prev = entries->prev;
	      if (entries->prev)
		entries->prev->next = entries->next;
	      if (table->entries[i] == entries)
		table->entries[i] = entries->next;

	      temp_list->next = NULL;
	      temp_list->prev = NULL;
	      g_list_free (temp_list);

	      goto done;
	    }

	  entries = entries->next;
	}
    }

done:
  g_function_leave ("gtk_accelerator_table_remove");
}

gint
gtk_accelerator_table_check (GtkAcceleratorTable *table,
			     gchar                accelerator_key,
			     guint8               accelerator_mods)
{
  GtkAcceleratorEntry *entry;
  GList *entries;
  gint result;
  gint hash;

  g_function_enter ("gtk_accelerator_table_check");

  g_assert (table != NULL);
  g_assert ((accelerator_key >= BASE_KEY) && (accelerator_key <= LAST_KEY));

  result = FALSE;
  hash = key_mappings[(int) (accelerator_key - BASE_KEY)];
  entries = table->entries[hash];

  while (entries)
    {
      entry = entries->data;

      if (entry->modifiers == accelerator_mods)
	{
	  gtk_widget_activate (entry->widget);
	  result = TRUE;
	  goto done;
	}

      entries = entries->next;
    }

done:
  g_function_leave ("gtk_accelerator_table_check");
  return result;
}

static void
gtk_accelerator_table_init (GtkAcceleratorTable *table)
{
  gint i;

  g_function_enter ("gtk_accelerator_table_init");

  g_assert (table != NULL);

  for (i = 0; i < TABLE_SIZE; i++)
    {
      table->entries[i] = NULL;
      table->ref_count = 0;
    }

  g_function_leave ("gtk_accelerator_table_init");
}

static void
gtk_accelerator_table_clean (GtkAcceleratorTable *table)
{
  GtkAcceleratorEntry *entry;
  GList *entries;
  gint i;

  g_function_enter ("gtk_accelerator_table_clean");

  g_assert (table != NULL);

  for (i = 0; i < TABLE_SIZE; i++)
    {
      entries = table->entries[i];
      while (entries)
	{
	  entry = entries->data;
	  entries = entries->next;

	  g_assert (entry->widget);
	  g_assert (entry->widget->function_table);
	  g_assert (entry->widget->function_table->remove_accelerator);

	  (* entry->widget->function_table->remove_accelerator) (entry->widget);
	  g_free (entry);
	}

      g_list_free (table->entries[i]);
      table->entries[i] = NULL;
    }

  g_function_leave ("gtk_accelerator_table_clean");
}
