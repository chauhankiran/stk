#include "gtk.h"


int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *arrow;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Button Test", GTK_WINDOW_TOPLEVEL);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (window, vbox);

  button = gtk_push_button_new ();
  label = gtk_label_new ("Hello World!");
  gtk_box_pack (vbox, button, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_add (button, label);
  gtk_widget_show (label);
  gtk_widget_show (button);

  button = gtk_push_button_new ();
  label = gtk_label_new ("What up?");
  gtk_box_pack (vbox, button, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_add (button, label);
  gtk_widget_show (label);
  gtk_widget_show (button);

  button = gtk_push_button_new ();
  label = gtk_label_new ("Foobar");
  gtk_box_pack (vbox, button, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_add (button, label);
  gtk_widget_show (label);
  gtk_widget_show (button);

  button = gtk_push_button_new ();
  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_box_pack (vbox, button, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_add (button, arrow);
  gtk_widget_show (arrow);
  gtk_widget_show (button);

  gtk_container_set_border_width (vbox, 5);

  gtk_widget_show (vbox);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
