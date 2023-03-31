#ifndef __GTK_MISC_H__
#define __GTK_MISC_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Labels, images and pixmaps
 */
GtkWidget* gtk_label_new  (gchar         *label);
GtkWidget* gtk_image_new  (GdkImage      *image);
GtkWidget* gtk_arrow_new  (GtkArrowType   arrow_type,
                           GtkShadowType  shadow_type);
GtkWidget* gtk_pixmap_new (GdkPixmap     *normal,
			   GdkPixmap     *active,
			   GdkPixmap     *prelight);

void gtk_label_get  (GtkWidget  *widget,
		     gchar     **label);
void gtk_image_get  (GtkWidget  *widget,
		     GdkImage  **image);
void gtk_pixmap_get (GtkWidget  *widget,
		     GdkPixmap **normal,
		     GdkPixmap **active,
		     GdkPixmap **prelight);

void gtk_label_set  (GtkWidget  *widget,
		     gchar      *label);
void gtk_image_set  (GtkWidget  *widget,
		     GdkImage   *image);
void gtk_pixmap_set (GtkWidget  *widget,
		     GdkPixmap  *normal,
		     GdkPixmap  *active,
		     GdkPixmap  *prelight);

void gtk_label_set_alignment  (GtkWidget  *label,
			       gdouble     xalign,
			       gdouble     yalign);
void gtk_image_set_alignment  (GtkWidget  *image,
			       gdouble     xalign,
			       gdouble     yalign);
void gtk_arrow_set_alignment  (GtkWidget  *arrow,
			       gdouble     xalign,
			       gdouble     yalign);
void gtk_pixmap_set_alignment (GtkWidget  *pixmap,
			       gdouble     xalign,
			       gdouble     yalign);

guint16 gtk_get_label_type  (void);
guint16 gtk_get_image_type  (void);
guint16 gtk_get_arrow_type  (void);
guint16 gtk_get_pixmap_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MISC_H__ */
