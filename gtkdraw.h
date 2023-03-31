#ifndef __GTK_DRAW_H__
#define __GTK_DRAW_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Drawing routines
 */
void gtk_draw_hline   (GdkWindow     *window,
		       GdkGC         *lightgc,
		       GdkGC         *darkgc,
		       gint           x1,
		       gint           x2,
		       gint           y,
		       gint           thickness);
void gtk_draw_vline   (GdkWindow     *window,
		       GdkGC         *lightgc,
		       GdkGC         *darkgc,
		       gint           y1,
		       gint           y2,
		       gint           x,
		       gint           thickness);
void gtk_draw_shadow  (GdkWindow     *window,
		       GdkGC         *lightgc,
		       GdkGC         *darkgc,
		       GdkGC         *backgc,
		       GtkShadowType  shadow_type,
		       gint           x,
		       gint           y,
		       gint           width,
		       gint           height,
		       gint           thickness);
void gtk_draw_arrow   (GdkWindow     *window,
		       GdkGC         *lightgc,
		       GdkGC         *darkgc,
		       GdkGC         *backgc,
		       GtkArrowType   arrow_type,
		       GtkShadowType  shadow_type,
		       gint           x,
		       gint           y,
		       gint           width,
		       gint           height,
		       gint           thickness);
void gtk_draw_diamond (GdkWindow     *window,
		       GdkGC         *lightgc,
		       GdkGC         *darkgc,
		       GdkGC         *backgc,
		       GtkShadowType  shadow_type,
		       gint           x,
		       gint           y,
		       gint           width,
		       gint           height,
		       gint           thickness);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_DRAW_H__ */
