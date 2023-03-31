/* GDK - The General Drawing Kit (written for the GIMP)
 * Copyright (C) 1995 Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GDK_H__
#define __GDK_H__


#include "gdktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Initialization, exit and events
 */
void gdk_init            (int    *argc,
			  char ***argv);
void gdk_exit            (int     error_code);

gint gdk_events_pending  (void);
gint gdk_event_get       (GdkEvent *event);
void gdk_event_put       (GdkEvent *event);
void gdk_events_record   (char     *filename);
void gdk_events_playback (char     *filename);
void gdk_events_stop     (void);

void gdk_set_debug_level (int  level);
void gdk_set_show_events (int  show_events);

guint32 gdk_time_get  (void);
guint32 gdk_timer_get (void);
void    gdk_timer_set (guint32  milliseconds);

gint gdk_input_add    (gint              source,
		       GdkInputCondition condition,
		       GdkInputFunction  function,
		       gpointer          data);
void gdk_input_remove (gint              tag);

gint gdk_pointer_grab   (GdkWindow *     window,
			 gint            owner_events,
			 GdkEventMask    event_mask,
			 GdkWindow *     confine_to,
			 GdkCursor *     cursor,
			 guint32         time);
void gdk_pointer_ungrab (guint32         time);

gint gdk_screen_width  (void);
gint gdk_screen_height (void);

void gdk_flush (void);


/* Visuals
 */
gint          gdk_visual_get_best_depth      (void);
GdkVisualType gdk_visual_get_best_type       (void);
GdkVisual*    gdk_visual_get_system          (void);
GdkVisual*    gdk_visual_get_best            (void);
GdkVisual*    gdk_visual_get_best_with_depth (gint           depth);
GdkVisual*    gdk_visual_get_best_with_type  (GdkVisualType  visual_type);
GdkVisual*    gdk_visual_get_best_with_both  (gint           depth,
					      GdkVisualType  visual_type);

void gdk_query_depths       (gint           **depths,
			     gint            *count);
void gdk_query_visual_types (GdkVisualType  **visual_types,
			     gint            *count);
void gdk_query_visuals      (GdkVisual      **visuals,
			     gint            *count);


/* Windows
 */
GdkWindow* gdk_window_new       (GdkWindow     *parent,
				 GdkWindowAttr *attributes,
				 gint           attributes_mask);
void       gdk_window_destroy   (GdkWindow     *window);

void       gdk_window_show       (GdkWindow    *window);
void       gdk_window_hide       (GdkWindow    *window);
void       gdk_window_move       (GdkWindow    *window,
				  gint          x,
				  gint          y);
void       gdk_window_reparent   (GdkWindow    *window,
				  GdkWindow    *new_parent,
				  gint          x,
				  gint          y);
void       gdk_window_clear      (GdkWindow    *window);
void       gdk_window_clear_area (GdkWindow    *window,
				  gint          x,
				  gint          y,
				  gint          width,
				  gint          height);
void       gdk_window_raise      (GdkWindow *window);
void       gdk_window_lower      (GdkWindow *window);

void       gdk_window_set_user_data  (GdkWindow       *window,
				      gpointer         user_data);
void       gdk_window_set_size       (GdkWindow       *window,
				      gint             width,
				      gint             height);
void       gdk_window_set_sizes      (GdkWindow       *window,
				      gint             min_width,
				      gint             min_height,
				      gint             max_width,
				      gint             max_height,
				      gint             flags);
void       gdk_window_set_position   (GdkWindow       *window,
				      gint             x,
				      gint             y);
void       gdk_window_set_title      (GdkWindow       *window,
				      gchar           *title);
void       gdk_window_set_background (GdkWindow       *window,
				      GdkColor        *color);
void       gdk_window_set_cursor     (GdkWindow       *window,
				      GdkCursor       *cursor);
void       gdk_window_set_colormap   (GdkWindow       *window,
				      GdkColormap     *colormap);
void       gdk_window_get_user_data  (GdkWindow       *window,
				      gpointer        *data);
gint       gdk_window_get_origin     (GdkWindow       *window,
				      gint            *x,
				      gint            *y);
GdkWindow* gdk_window_get_pointer    (GdkWindow       *window,
				      gint            *x,
				      gint            *y,
				      GdkModifierType *mask);
GdkWindow* gdk_window_get_parent     (GdkWindow       *window);
GdkWindow* gdk_window_get_toplevel   (GdkWindow       *window);

/* Cursors
 */
GdkCursor* gdk_cursor_new     (GdkCursorType   cursor_type);
void       gdk_cursor_destroy (GdkCursor      *cursor);


/* GCs
 */
GdkGC* gdk_gc_new            (GdkWindow        *window);
void   gdk_gc_destroy        (GdkGC            *gc);
void   gdk_gc_set_foreground (GdkGC            *gc,
			      GdkColor         *color);
void   gdk_gc_set_background (GdkGC            *gc,
			      GdkColor         *color);
void   gdk_gc_set_font       (GdkGC            *gc,
			      GdkFont          *font);
void   gdk_gc_set_function   (GdkGC            *gc,
			      GdkFunction       function);
void   gdk_gc_set_fill       (GdkGC            *gc,
			      GdkFill           fill);
void   gdk_gc_set_tile       (GdkGC            *gc,
			      GdkPixmap        *tile);
void   gdk_gc_set_stipple    (GdkGC            *gc,
			      GdkPixmap        *stipple);
void   gdk_gc_set_subwindow  (GdkGC            *gc,
			      GdkSubwindowMode  mode);
void   gdk_gc_set_exposures  (GdkGC            *gc,
			      gint              exposures);
void   gdk_gc_set_line_attributes (GdkGC       *gc,
				   gint         line_width,
				   GdkLineStyle line_style,
				   GdkCapStyle  cap_style,
				   GdkJoinStyle join_style);


/* Pixmaps
 */
GdkPixmap* gdk_pixmap_new              (GdkWindow  *window,
					gint        width,
					gint        height,
					gint        depth);
GdkPixmap* gdk_bitmap_create_from_data (GdkWindow  *window,
					gchar      *data,
					gint        width,
					gint        height);
void       gdk_pixmap_destroy          (GdkPixmap  *pixmap);



/* Images
 */
GdkImage* gdk_image_new     (GdkImageType  type,
			     GdkVisual    *visual,
			     gint          width,
			     gint          height);
GdkImage *gdk_image_get     (GdkWindow    *window,
			     gint          x,
			     gint          y,
			     gint          width,
			     gint          height);
void      gdk_image_put_pixel (GdkImage     *image,
			       gint          x,
			       gint          y,
			       guint32       pixel);
guint32   gdk_image_get_pixel (GdkImage     *image,
			       gint          x,
			       gint          y);
void      gdk_image_destroy   (GdkImage     *image);


/* Color
 */
GdkColormap* gdk_colormap_new     (GdkVisual   *visual,
				   gint         allocate);
void         gdk_colormap_destroy (GdkColormap *colormap);

GdkColormap* gdk_colormap_get_system       (void);
gint         gdk_colormap_get_system_size  (void);

void gdk_colormap_change (GdkColormap   *colormap,
			  gint           ncolors);
void gdk_colors_store    (GdkColormap   *colormap,
			  GdkColor      *colors,
			  gint           ncolors);
gint gdk_colors_alloc    (GdkColormap   *colormap,
			  gint           contiguous,
			  gulong        *planes,
			  gint           nplanes,
			  gulong        *pixels,
			  gint           npixels);
gint gdk_color_white     (GdkColormap   *colormap,
			  GdkColor      *color);
gint gdk_color_black     (GdkColormap   *colormap,
			  GdkColor      *color);
gint gdk_color_parse     (gchar         *spec,
			  GdkColor      *color);
gint gdk_color_alloc     (GdkColormap   *colormap,
			  GdkColor      *color);
gint gdk_color_change    (GdkColormap   *colormap,
			  GdkColor      *color);


/* Fonts
 */
GdkFont* gdk_font_load    (gchar    *font_name);
void     gdk_font_free    (GdkFont  *font);
gint     gdk_string_width (GdkFont  *font,
			   gchar    *string);
gint     gdk_text_width   (GdkFont  *font,
			   gchar    *text,
			   gint      text_length);
gint     gdk_char_width   (GdkFont  *font,
			   gchar     character);


/* Drawing
 */
void gdk_draw_move       (GdkGC      *gc,
			  gint        x,
			  gint        y);
void gdk_draw_move_rel   (GdkGC      *gc,
			  gint        dx,
			  gint        dy);
void gdk_draw_line       (GdkWindow  *window,
			  GdkGC      *gc,
			  gint        x1,
			  gint        y1,
			  gint        x2,
			  gint        y2);
void gdk_draw_line_rel   (GdkWindow  *window,
			  GdkGC      *gc,
			  gint        dx,
			  gint        dy);
void gdk_draw_rectangle  (GdkWindow  *window,
			  GdkGC      *gc,
			  gint        filled,
			  gint        x,
			  gint        y,
			  gint        width,
			  gint        height);
void gdk_draw_arc        (GdkWindow  *window,
			  GdkGC      *gc,
			  gint        filled,
			  gint        x,
			  gint        y,
			  gint        width,
			  gint        height,
			  gint        angle1,
			  gint        angle2);
void gdk_draw_polygon    (GdkWindow  *window,
			  GdkGC      *gc,
			  gint        filled,
			  GdkPoint   *points,
			  gint        npoints);
void gdk_draw_string     (GdkWindow  *window,
			  GdkGC      *gc,
			  gint        x,
			  gint        y,
			  gchar      *string);
void gdk_draw_text       (GdkWindow  *window,
			  GdkGC      *gc,
			  gint        x,
			  gint        y,
			  gchar      *text,
			  gint        text_length);
void gdk_draw_pixmap     (GdkWindow  *window,
			  GdkGC      *gc,
			  GdkPixmap  *pixmap,
			  gint        xsrc,
			  gint        ysrc,
			  gint        xdest,
			  gint        ydest,
			  gint        width,
			  gint        height);
void gdk_draw_image      (GdkWindow  *window,
			  GdkGC      *gc,
			  GdkImage   *image,
			  gint        xsrc,
			  gint        ysrc,
			  gint        xdest,
			  gint        ydest,
			  gint        width,
			  gint        height);
void gdk_draw_points     (GdkWindow  *window,
			  GdkGC      *gc,
			  GdkPoint   *points,
			  gint        npoints);
void gdk_draw_segments   (GdkWindow  *window,
			  GdkGC      *gc,
			  GdkSegment *segs,
			  gint        nsegs);

/* Rectangle utilities
 */
gint gdk_rectangle_intersect (GdkRectangle *src1,
			      GdkRectangle *src2,
			      GdkRectangle *dest);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GDK_H__ */
