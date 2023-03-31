#include "gtk.h"


int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *label;
  GtkData *owner;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Button Test", GTK_WINDOW_TOPLEVEL);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (window, vbox);

  button = gtk_radio_button_new (NULL);
  label = gtk_label_new ("Hello World!");
  gtk_label_set_alignment (label, 0.0, 0.5);
  gtk_box_pack (vbox, button, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_add (button, label);
  gtk_widget_show (label);
  gtk_widget_show (button);
  owner = gtk_button_get_owner (button);

  button = gtk_radio_button_new (owner);
  label = gtk_label_new ("What up?");
  gtk_label_set_alignment (label, 0.0, 0.5);
  gtk_box_pack (vbox, button, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_add (button, label);
  gtk_widget_show (label);
  gtk_widget_show (button);

  button = gtk_radio_button_new (owner);
  label = gtk_label_new ("Foobar");
  gtk_label_set_alignment (label, 0.0, 0.5);
  gtk_box_pack (vbox, button, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_add (button, label);
  gtk_widget_show (label);
  gtk_widget_show (button);

  gtk_container_set_border_width (vbox, 5);
  
  gtk_widget_show (vbox);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
