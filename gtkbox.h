#ifndef __GTK_BOX_H__
#define __GTK_BOX_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum
{
  GTK_PACK_START,
  GTK_PACK_END
} GtkPackType;


/* Boxes
 */
GtkWidget* gtk_hbox_new (gint homogeneous,
			 gint spacing);
GtkWidget* gtk_vbox_new (gint homogeneous,
			 gint spacing);

void gtk_box_pack (GtkWidget   *box,
		   GtkWidget   *child,
		   gint         expand,
		   gint         fill,
		   gint         padding,
		   GtkPackType  pack);

guint16 gtk_get_box_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_BOX_H__ */
