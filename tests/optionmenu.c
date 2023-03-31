#include "gtk.h"


GtkWidget* 
create_menu ()
{
  char *item_labels[] =
  {
    "foo",
    "bar",
    "argh",
    "oof",
    "spencer is a",
    "fucking weeny",
    "he should be",
    "programming",
    "but instead he's",
    "playing hackey",
    "sack",
  };
  int nitems = sizeof (item_labels) / sizeof (item_labels[0]);
  
  GtkWidget *menu;
  GtkWidget *item;
  int i;

  menu = gtk_menu_new ();

  for (i = 0; i < nitems; i++)
    {
      item = gtk_menu_item_new_with_label (item_labels[i]);
      gtk_container_add (menu, item);
      gtk_widget_show (item);
    }

  return menu;
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *option_menu;
  GtkWidget *menu;
  
  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Test", GTK_WINDOW_TOPLEVEL);
  option_menu = gtk_option_menu_new ();
  gtk_container_add (window, option_menu);
  
  menu = create_menu ();
  gtk_option_menu_set_menu (option_menu, menu);

  gtk_widget_show (option_menu);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
