#ifndef __GTK_STYLE_H__
#define __GTK_STYLE_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Style routines
 */
GtkStyle* gtk_style_new     (gint        shadow_thickness);
void      gtk_style_init    (GtkStyle   *style);
void      gtk_style_destroy (GtkStyle   *style);
GtkStyle* gtk_style_attach  (GtkStyle   *style,
			     GdkWindow  *window);
void      gtk_style_detach  (GtkStyle   *style);
void      gtk_style_ref     (GtkStyle   *style);
void      gtk_style_unref   (GtkStyle   *style);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_STYLE_H__ */
