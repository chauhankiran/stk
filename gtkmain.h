#ifndef __GTK_MAIN_H__
#define __GTK_MAIN_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Initialization, exit, mainloop and miscellaneous routines
 */
void gtk_init           (int    *argc,
		         char ***argv);
void gtk_exit           (int     error_code);
void gtk_main           (void);
void gtk_main_iteration (void);

void gtk_grab_add    (GtkWidget *widget);
void gtk_grab_remove (GtkWidget *widget);

gint gtk_timeout_add    (guint32     interval,
			 GtkFunction function,
			 gpointer    data);
void gtk_timeout_remove (gint        tag);

void       gtk_get_current_event (GdkEvent *event);
GtkWidget* gtk_get_event_widget  (GdkEvent *event);


/* Widget creation variables
 */
GdkVisual*   gtk_peek_visual   (void);
GdkColormap* gtk_peek_colormap (void);
GtkStyle*    gtk_peek_style    (void);

void gtk_push_visual   (GdkVisual   *visual);
void gtk_push_colormap (GdkColormap *colormap);
void gtk_push_style    (GtkStyle    *style);

void gtk_pop_visual    (void);
void gtk_pop_colormap  (void);
void gtk_pop_style     (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MAIN_H__ */
