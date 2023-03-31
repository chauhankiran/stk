#ifndef __GTK_SCALE_H__
#define __GTK_SCALE_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_POS_LEFT    0
#define GTK_POS_RIGHT   1
#define GTK_POS_TOP     2
#define GTK_POS_BOTTOM  3


/* Scales
 */
GtkWidget* gtk_hscale_new (GtkDataAdjustment *adjustment);
GtkWidget* gtk_vscale_new (GtkDataAdjustment *adjustment);

GtkData* gtk_scale_get_adjustment (GtkWidget *scale);
void     gtk_scale_set_draw_value (GtkWidget *scale,
				   gint       draw_value);
void     gtk_scale_set_value_pos  (GtkWidget *scale,
				   gint       value_pos);
void     gtk_scale_set_digits     (GtkWidget *scale,
				   gint       digits);

guint16 gtk_get_scale_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_SCALE_H__ */
