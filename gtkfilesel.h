#ifndef __GTK_FILESEL_H__
#define __GTK_FILESEL_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Filesels
 */
GtkWidget* gtk_file_selection_new (char *title,
				   char *initial_filename);

void gtk_file_selection_destroy (GtkWidget *fs);

GtkWidget* gtk_file_selection_get_main_vbox (GtkWidget* fs);

void gtk_file_selection_set_ok_callback (GtkWidget *filesel,
					 GtkCallback ok_callback,
					 gpointer data);

void gtk_file_selection_set_cancel_callback (GtkWidget *filesel,
					     GtkCallback cancel_callback,
					     gpointer data);

void gtk_file_selection_set_help_callback (GtkWidget *filesel,
					   GtkCallback help_callback,
					   gpointer data);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_FILESEL_H__ */
