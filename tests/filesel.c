#include "gtk.h"
#include <stdio.h>

void ok_callback(GtkWidget* fs_window, gpointer client_data, gpointer filename)
{
  fprintf(stdout, "ok callback: %s", (gchar*)filename);
}

void cancel_callback(GtkWidget* fs_window, gpointer client_data, gpointer filename)
{
  gtk_file_selection_destroy(fs_window);
  gtk_exit(0);
}

void help_callback(GtkWidget* fs_window, gpointer client_data, gpointer filename)
{
  fprintf(stdout, "help is for wusses\n");
}

int
main (int argc, char *argv[])
{
  GtkWidget *filesel;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  filesel = gtk_file_selection_new ("Josh Is A Wussnut", NULL);

  gtk_file_selection_set_ok_callback(filesel, ok_callback, NULL);
  gtk_file_selection_set_cancel_callback(filesel, cancel_callback, NULL);
  gtk_file_selection_set_help_callback(filesel, help_callback, NULL);

  gtk_widget_show (filesel);

  gtk_main ();

  return 0;
}
