#include "gtk.h"

int
main (int argc, char *argv[])
{
  GtkWidget *window1;
  GtkWidget *window2;
  GtkWidget *frame;
  GtkWidget *ruler;
  GtkData *adj;
  
  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window1 = gtk_window_new ("Horizontal Ruler Test", GTK_WINDOW_TOPLEVEL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_type (frame, GTK_SHADOW_NONE);
  gtk_container_add (window1, frame);

  adj = gtk_data_adjustment_new (50.0, 0.0, 100.0, 0.0, 0.0, 0.0);

  ruler = gtk_hruler_new ((GtkDataAdjustment*) adj);
  gtk_widget_set_usize (ruler, 200, 0);
  gtk_container_add (frame, ruler);

  gtk_widget_show (ruler);
  gtk_widget_show (frame);
  gtk_widget_show (window1);

  window2 = gtk_window_new ("Vertical Ruler Test", GTK_WINDOW_TOPLEVEL);
  
  frame = gtk_frame_new (NULL);
  gtk_frame_set_type (frame, GTK_SHADOW_NONE);
  gtk_container_add (window2, frame);

  ruler = gtk_vruler_new ((GtkDataAdjustment*) adj);
  gtk_widget_set_usize (ruler, 0, 200);
  gtk_container_add (frame, ruler);

  gtk_widget_show (ruler);
  gtk_widget_show (frame);
  gtk_widget_show (window2);

  gtk_main ();
  
  return 0;
}
