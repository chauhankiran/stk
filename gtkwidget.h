#ifndef __GTK_WIDGET_H__
#define __GTK_WIDGET_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Widgets
 */
void  gtk_widget_unique_type           (guint16             *type);
gint  gtk_widget_destroy               (GtkWidget           *widget);
void  gtk_widget_show                  (GtkWidget           *widget);
void  gtk_widget_hide                  (GtkWidget           *widget);
void  gtk_widget_map                   (GtkWidget           *widget);
void  gtk_widget_unmap                 (GtkWidget           *widget);
void  gtk_widget_realize               (GtkWidget           *widget);
void  gtk_widget_draw                  (GtkWidget           *widget,
					GdkRectangle        *area,
					gint                 is_expose);
void  gtk_widget_draw_focus            (GtkWidget           *widget);
gint  gtk_widget_event                 (GtkWidget           *widget,
					GdkEvent            *event);
void  gtk_widget_size_request          (GtkWidget           *widget,
					GtkRequisition      *requisition);
void  gtk_widget_size_allocate         (GtkWidget           *widget,
					GtkAllocation       *allocation);
gint  gtk_widget_is_child              (GtkWidget           *widget,
					GtkWidget           *child);
gint  gtk_widget_is_immediate_child    (GtkWidget           *widget,
					GtkWidget           *child);
gint  gtk_widget_locate                (GtkWidget           *widget,
					GtkWidget          **child,
					gint                 x,
					gint                 y);
void  gtk_widget_activate              (GtkWidget           *widget);
void  gtk_widget_set_state             (GtkWidget           *widget,
					GtkStateType         state);
void  gtk_widget_set_sensitive         (GtkWidget           *widget,
					gint                 sensitive);
void  gtk_widget_install_accelerator   (GtkWidget           *widget,
					GtkAcceleratorTable *table,
					gchar                accelerator_key,
					guint8               accelerator_mods);
void  gtk_widget_remove_accelerator    (GtkWidget           *widget,
					GtkAcceleratorTable *table);
void  gtk_widget_add_accelerator_table (GtkWidget           *widget,
					GtkAcceleratorTable *table);
void  gtk_widget_grab_focus            (GtkWidget           *widget);
void  gtk_widget_grab_default          (GtkWidget           *widget);
void  gtk_widget_move                  (GtkWidget           *widget,
					gint                 x,
					gint                 y);
gint  gtk_widget_intersect             (GtkWidget           *widget,
					GdkRectangle        *area,
					GdkRectangle        *dest);
void  gtk_widget_reparent              (GtkWidget           *widget,
					GtkWidget           *new_parent);
void  gtk_widget_popup                 (GtkWidget           *widget,
					gint                 x,
					gint                 y);
void  gtk_widget_set_uposition         (GtkWidget           *widget,
					gint                 x,
					gint                 y);
void  gtk_widget_set_usize             (GtkWidget           *widget,
					gint                 width,
					gint                 height);

void       gtk_widget_set_defaults   (GtkWidget  *widget);
void       gtk_widget_set_style      (GtkWidget  *widget,
				      GtkStyle   *style);
void       gtk_widget_set_user_data  (GtkWidget  *widget,
				      gpointer    data);
gpointer   gtk_widget_get_user_data  (GtkWidget  *widget);
GtkWidget* gtk_widget_get_toplevel   (GtkWidget *widget);


void  gtk_widget_default_show                (GtkWidget    *widget);
void  gtk_widget_default_hide                (GtkWidget    *widget);
void  gtk_widget_default_map                 (GtkWidget    *widget);
void  gtk_widget_default_unmap               (GtkWidget    *widget);
void  gtk_widget_default_draw_focus          (GtkWidget    *widget);
gint  gtk_widget_default_event               (GtkWidget    *widget,
					      GdkEvent     *event);
void  gtk_widget_default_activate            (GtkWidget    *widget);
void  gtk_widget_default_set_state           (GtkWidget    *widget,
					      GtkStateType  state);
gint  gtk_widget_default_install_accelerator (GtkWidget    *widget,
					      gchar         accelerator_key,
					      guint8        accelerator_mods);
void  gtk_widget_default_remove_accelerator  (GtkWidget    *widget);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_WIDGET_H__ */
