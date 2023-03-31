#ifndef __GTK_RULER_H__
#define __GTK_RULER_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Metrics
 */
#define PIXELS       0
#define INCHES       1
#define CENTIMETERS  2


/* Rulers
 */
GtkWidget* gtk_hruler_new (GtkDataAdjustment *adjustment);
GtkWidget* gtk_vruler_new (GtkDataAdjustment *adjustment);

GtkData* gtk_ruler_get_adjustment (GtkWidget *ruler);
void     gtk_ruler_set_metric     (GtkWidget *ruler, gint metric);

guint16 gtk_get_ruler_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_RULER_H__ */
