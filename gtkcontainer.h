#ifndef __GTK_CONTAINER_H__
#define __GTK_CONTAINER_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Containers
 */
void gtk_container_add           (GtkWidget         *container,
				  GtkWidget         *widget);
void gtk_container_remove        (GtkWidget         *container,
				  GtkWidget         *widget);
void gtk_container_need_resize   (GtkContainer      *container,
				  GtkWidget         *widget);
void gtk_container_focus_advance (GtkContainer      *container,
				  GtkWidget        **widget,
				  GtkDirectionType   direction);
void gtk_container_foreach       (GtkContainer      *container,
				  GtkCallback        callback,
				  gpointer           callback_data);

void gtk_container_set_defaults     (GtkWidget  *widget);
void gtk_container_set_border_width (GtkWidget  *widget,
				     gint        border_width);

void gtk_container_default_need_resize   (GtkContainer      *container,
					  GtkWidget         *widget);
void gtk_container_default_focus_advance (GtkContainer      *container,
					  GtkWidget        **child,
					  GtkDirectionType   direction);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_CONTAINER_H__ */
