#ifndef __GTK_GC_H__
#define __GTK_GC_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* GC's
 */
GdkGC* gtk_gc_get   (GdkWindow        *window,
		     GdkColor         *foreground,
		     GdkColor         *background,
		     GdkFont          *font,
		     GdkFunction       function,
		     GdkFill           fill,
		     GdkPixmap        *tile,
		     GdkPixmap        *stipple,
		     GdkSubwindowMode  subwindow_mode,
		     gint              graphics_exposures);
void gtk_gc_release (GdkGC      *gc);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_GC_H__ */
