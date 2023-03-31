#ifndef __GTK_ALIGNMENT_H__
#define __GTK_ALIGNMENT_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Alignments
 */
GtkWidget* gtk_alignment_new  (gdouble     xalign,
			       gdouble     yalign,
			       gdouble     xscale,
			       gdouble     yscale);

void gtk_alignment_set (GtkWidget  *widget,
			gdouble     xalign,
			gdouble     yalign,
			gdouble     xscale,
			gdouble     yscale);

guint16 gtk_get_alignment_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_ALIGNMENT_H__ */
