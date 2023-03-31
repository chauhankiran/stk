#ifndef __GTK_SCROLL_H__
#define __GTK_SCROLL_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Scrolled areas and scrolled windows
 */
GtkWidget* gtk_scrolled_area_new   (GtkDataAdjustment *hadjustment,
				    GtkDataAdjustment *vadjustment);
GtkWidget* gtk_scrolled_window_new (GtkDataAdjustment *hadjustment,
				    GtkDataAdjustment *vadjustment);

GtkData* gtk_scrolled_area_get_hadjustment (GtkWidget *scrolled_area);
GtkData* gtk_scrolled_area_get_vadjustment (GtkWidget *scrolled_area);
void     gtk_scrolled_area_set_align       (GtkWidget *scrolled_area,
					    gfloat     xalign,
					    gfloat     yalign);

GtkWidget* gtk_scrolled_window_get_scrolled_area (GtkWidget     *scrolled_window);
GtkWidget* gtk_scrolled_window_get_hscrollbar    (GtkWidget     *scrolled_window);
GtkWidget* gtk_scrolled_window_get_vscrollbar    (GtkWidget     *scrolled_window);
void       gtk_scrolled_window_set_shadow_type   (GtkWidget     *scrolled_window,
						  GtkShadowType  type);

guint16 gtk_get_scrolled_area_type   (void);
guint16 gtk_get_scrolled_window_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_SCROLL_H__ */
