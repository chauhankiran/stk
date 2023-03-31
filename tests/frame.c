#include "gtk.h"


int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *label;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Frame Test", GTK_WINDOW_TOPLEVEL);

  frame = gtk_frame_new (NULL);
  frame = gtk_frame_new ("Frame test w/ a long string");
  gtk_frame_set_type (frame, GTK_SHADOW_ETCHED_IN);
  gtk_frame_set_label_align (frame, 0.25, 0.5);
  gtk_container_set_border_width (frame, 5);
  gtk_container_add (window, frame);
  gtk_widget_show (frame);

  button = gtk_push_button_new ();
  gtk_container_set_border_width (button, 5);
  gtk_container_add (frame, button);
  gtk_widget_show (button);

  label = gtk_label_new ("Hello World!");
  gtk_container_add (button, label);
  gtk_widget_show (label);

  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
