#include "gtk.h"

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *scrollbar;
  
  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Scrollbar Test", GTK_WINDOW_TOPLEVEL);
  
  frame = gtk_frame_new (NULL);
  gtk_frame_set_type (frame, GTK_SHADOW_NONE);
  gtk_container_add (window, frame);

  scrollbar = gtk_vscrollbar_new (NULL);
  gtk_container_add (frame, scrollbar);

  gtk_widget_show (scrollbar);
  gtk_widget_show (frame);
  gtk_widget_show (window);

  gtk_main ();
  
  return 0;
}
