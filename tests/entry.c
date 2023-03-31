#include "gtk.h"


int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *entry;
  
  gdk_set_debug_level (0);
  gdk_set_show_events (1);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Test", GTK_WINDOW_TOPLEVEL);

  vbox = gtk_vbox_new (TRUE, 0);
  gtk_container_add (window, vbox);
  gtk_widget_show (vbox);
  
  entry = gtk_text_entry_new ();
  gtk_text_entry_set_text (entry, "testing testing");
  gtk_container_add (vbox, entry);
  gtk_widget_show (entry);
  
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
