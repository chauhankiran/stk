#include "gtk.h"


char* items[] =
{
  "hello world",
  "blah",
  "foobar",
  "argh",
  "spencer is a wuss",
  "why is spencer such",
  "a wuss",
  "it is simple",
  "he was born a wuss",
};
int nitems = sizeof (items) / sizeof (items[0]);


int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *listbox;
  GtkWidget *list;
  GtkWidget *list_item;
  GList *list_items;
  int i;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Listbox Test", GTK_WINDOW_TOPLEVEL);

  listbox = gtk_listbox_new ();
  gtk_listbox_set_shadow_type (listbox, GTK_SHADOW_IN);
  gtk_container_add (window, listbox);

  list = gtk_listbox_get_list (listbox);
  /*  gtk_list_set_selection_mode (list, GTK_SELECTION_SINGLE); */
  gtk_list_set_selection_mode (list, GTK_SELECTION_BROWSE);
  /*  gtk_list_set_selection_mode (list, GTK_SELECTION_MULTIPLE); */
  /*  gtk_list_set_selection_mode (list, GTK_SELECTION_EXTENDED); */
  gtk_widget_set_usize (list, 100, 100);

  for (i = 0; i < nitems; i++)
    {
      list_item = gtk_list_item_new_with_label ("test");
      gtk_container_add (list, list_item);
      gtk_widget_show (list_item);
    }

  list_items = NULL;
  for (i = 0; i < nitems; i++)
    {
      list_item = gtk_list_item_new_with_label (items[i]);
      list_items = g_list_append (list_items, list_item);
      gtk_widget_show (list_item);
    }
  gtk_list_insert_items (list, list_items, 3);
  gtk_list_clear_items (list, 1, 4);

  gtk_widget_show (listbox);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
