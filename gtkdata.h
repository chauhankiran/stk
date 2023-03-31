#ifndef __GTK_DATA_H__
#define __GTK_DATA_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Data routines
 */
void gtk_data_init        (GtkData     *data);
void gtk_data_unique_type (guint       *type);
void gtk_data_destroy     (GtkData     *data);
void gtk_data_attach      (GtkData     *data,
		           GtkObserver *observer);
void gtk_data_detach      (GtkData     *data,
			   GtkObserver *observer);
void gtk_data_notify      (GtkData     *data);
void gtk_data_disconnect  (GtkData     *data);

GtkData* gtk_data_int_new        (gint        value);
GtkData* gtk_data_float_new      (gfloat      value);
GtkData* gtk_data_adjustment_new (gfloat      value,
				  gfloat      lower,
				  gfloat      upper,
				  gfloat      step_increment,
				  gfloat      page_increment,
				  gfloat      page_size);
GtkData* gtk_data_widget_new     (GtkWidget  *widget);
GtkData* gtk_data_list_new       (GList      *list);

guint gtk_data_int_type        (void);
guint gtk_data_float_type      (void);
guint gtk_data_adjustment_type (void);
guint gtk_data_widget_type     (void);
guint gtk_data_list_type       (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_DATA_H__ */
