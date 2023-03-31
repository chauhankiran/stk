/* GTK - The General Toolkit (written for the GIMP)
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
#include "gtkdraw.h"


void 
gtk_draw_hline (GdkWindow *window, 
		GdkGC     *lightgc, 
		GdkGC     *darkgc, 
		gint       x1, 
		gint       x2, 
		gint       y, 
		gint       thickness)
{
  gint thickness_light;
  gint thickness_dark;
  gint i;

  g_function_enter ("gtk_draw_hline");

  g_assert (window != NULL);
  g_assert (lightgc != NULL);
  g_assert (darkgc != NULL);

  thickness_light = thickness / 2;
  thickness_dark = thickness - thickness_light;
  
  for (i = 0; i < thickness_dark; i++)
    {
      gdk_draw_line (window, lightgc, x2 - i - 1, y + i, x2, y + i);
      gdk_draw_line (window, darkgc, x1, y + i, x2 - i - 1, y + i);
    }

  y += thickness_dark;
  for (i = 0; i < thickness_light; i++)
    {
      gdk_draw_line (window, darkgc, x1, y + i, x1 + thickness_light - i - 1, y + i);
      gdk_draw_line (window, lightgc, x1 + thickness_light - i - 1, y + i, x2, y + i);
    }

  g_function_leave ("gtk_draw_hline");
}

void 
gtk_draw_vline (GdkWindow *window, 
		GdkGC     *lightgc, 
		GdkGC     *darkgc, 
		gint       y1, 
		gint       y2, 
		gint       x, 
		gint       thickness)
{
  gint thickness_light;
  gint thickness_dark;
  gint i;

  g_function_enter ("gtk_draw_vline");

  g_assert (window != NULL);
  g_assert (lightgc != NULL);
  g_assert (darkgc != NULL);

  thickness_light = thickness / 2;
  thickness_dark = thickness - thickness_light;
  
  for (i = 0; i < thickness_dark; i++)
    {
      gdk_draw_line (window, lightgc, x + i, y2 - i - 1, x + i, y2);
      gdk_draw_line (window, darkgc, x + i, y1, x + i, y2 - i - 1);
    }

  x += thickness_dark;
  for (i = 0; i < thickness_light; i++)
    {
      gdk_draw_line (window, darkgc, x + i, y1, x + i, y1 + thickness_light - i);
      gdk_draw_line (window, lightgc, x + i, y1 + thickness_light - i, x + i, y2);
    }

  g_function_leave ("gtk_draw_vline");
}

void 
gtk_draw_shadow (GdkWindow     *window, 
		 GdkGC         *lightgc, 
		 GdkGC         *darkgc, 
		 GdkGC         *backgc, 
		 GtkShadowType  shadow_type, 
		 gint           x, 
		 gint           y, 
		 gint           width, 
		 gint           height, 
		 gint           thickness)
{
  GdkGC *gc1;
  GdkGC *gc2;
  gint thickness_light;
  gint thickness_dark;
  gint i;

  g_function_enter ("gtk_draw_shadow");

  g_assert (window != NULL);
  g_assert (lightgc != NULL);
  g_assert (darkgc != NULL);

  if (backgc)
    gdk_draw_rectangle (window, backgc, TRUE,
			x, y, width, height);

  switch (shadow_type)
    {
    case GTK_SHADOW_NONE:
      gc1 = NULL;
      gc2 = NULL;
      break;
    case GTK_SHADOW_IN:
    case GTK_SHADOW_ETCHED_IN:
      gc1 = lightgc;
      gc2 = darkgc;
      break;
    case GTK_SHADOW_OUT:
    case GTK_SHADOW_ETCHED_OUT:
      gc1 = darkgc;
      gc2 = lightgc;
      break;
    default:
      g_error ("unknown shadow type: %d", shadow_type);
      gc1 = NULL;
      gc2 = NULL;
      break;
    }

  switch (shadow_type)
    {
    case GTK_SHADOW_NONE:
      break;

    case GTK_SHADOW_IN:
    case GTK_SHADOW_OUT:
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1,
		       x + i, 
		       y + height - 1 - i,
		       x + width - 1 - i,
		       y + height - 1 - i);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1, 
		       x + width - 1 - i,
		       y + height - 1 - i,
		       x + width - 1 - i,
		       y + i);
      
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2, 
		       x + width - 1 - i,
		       y + i,
		       x + i,
		       y + i);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2, 
		       x + i,
		       y + i,
		       x + i,
		       y + height - 1 - i);
      break;
      
    case GTK_SHADOW_ETCHED_IN:
    case GTK_SHADOW_ETCHED_OUT:
      thickness_light = thickness / 2;
      thickness_dark = thickness - thickness_light;

      for (i = 0; i < thickness_dark; i++)
	{
	  gdk_draw_line (window, gc1, 
			 x + i, 
			 y + height - i - 1, 
			 x + width - i - 1, 
			 y + height - i - 1);
	  gdk_draw_line (window, gc1, 
			 x + width - i - 1, 
			 y + i, 
			 x + width - i - 1, 
			 y + height - i - 1);
	  
	  gdk_draw_line (window, gc2, 
			 x + i, 
			 y + i, 
			 x + width - i - 2, 
			 y + i);
	  gdk_draw_line (window, gc2, 
			 x + i, 
			 y + i, 
			 x + i, 
			 y + height - i - 2);
	}

      for (i = 0; i < thickness_light; i++)
	{
	  gdk_draw_line (window, gc1,
			 x + thickness_dark + i,
			 y + thickness_dark + i,
			 x + width - thickness_dark - i - 1,
			 y + thickness_dark + i);
	  gdk_draw_line (window, gc1,
			 x + thickness_dark + i,
			 y + thickness_dark + i,
			 x + thickness_dark + i,
			 y + height - thickness_dark - i - 1);

	  gdk_draw_line (window, gc2, 
			 x + thickness_dark + i,
			 y + height - thickness_light - i - 1,
			 x + width - thickness_light - 1,
			 y + height - thickness_light - i - 1);
	  gdk_draw_line (window, gc2,
			 x + width - thickness_light - i - 1,
			 y + thickness_dark + i,
			 x + width - thickness_light - i - 1,
			 y + height - thickness_light - 1);
	}
      break;
      
    default:
      g_error ("unknown shadow type: %d", shadow_type);
      break;
    }
  
  g_function_leave ("gtk_draw_shadow");
}

void 
gtk_draw_arrow (GdkWindow     *window, 
		GdkGC         *lightgc, 
		GdkGC         *darkgc, 
		GdkGC         *backgc, 
		GtkArrowType   arrow_type, 
		GtkShadowType  shadow_type, 
		gint           x, 
		gint           y, 
		gint           width, 
		gint           height, 
		gint           thickness)
{
  GdkGC *gc1;
  GdkGC *gc2;
  gint half_width;
  gint half_height;
  GdkPoint points[3];
  gint i;
  
  g_function_enter ("gtk_draw_arrow");

  g_assert (window != NULL);
  g_assert (lightgc != NULL);
  g_assert (darkgc != NULL);

  switch (shadow_type)
    {
    case GTK_SHADOW_NONE:
      goto done;
      break;
    case GTK_SHADOW_IN:
      gc1 = lightgc;
      gc2 = darkgc;
      break;
    case GTK_SHADOW_OUT:
      gc1 = darkgc;
      gc2 = lightgc;
      break;
    default:
      g_error ("unsupported shadow type: %d", shadow_type);
      gc1 = NULL;
      gc2 = NULL;
      break;
    }

  half_width = width / 2;
  half_height = height / 2;

  switch (arrow_type)
    {
    case GTK_ARROW_UP:
      if (backgc)
	{
	  points[0].x = x + half_width;
	  points[0].y = y;
	  points[1].x = x;
	  points[1].y = y + height - 1;
	  points[2].x = x + width - 1;
	  points[2].y = y + height - 1;

	  gdk_draw_polygon (window, backgc, 
			    TRUE, points, 3);
	}

      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1,
		       x + i,
		       y + height - i - 1,
		       x + width - i - 1,
		       y + height - i - 1);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1,
		       x + width - i - 1,
		       y + height - i - 1,
		       x + half_width,
		       y + i);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2,
		       x + half_width,
		       y + i,
		       x + i,
		       y + height - i - 1);
      break;
    case GTK_ARROW_DOWN:
      if (backgc)
	{
	  points[0].x = x + width - 1;
	  points[0].y = y;
	  points[1].x = x;
	  points[1].y = y;
	  points[2].x = x + half_width;
	  points[2].y = y + height - 1;

	  gdk_draw_polygon (window, backgc, 
			    TRUE, points, 3);
	}

      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2,
		       x + width - i - 1,
		       y + i,
		       x + i,
		       y + i);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2,
		       x + i,
		       y + i,
		       x + half_width,
		       y + height - i - 1);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1,
		       x + half_width,
		       y + height - i - 1,
		       x + width - i - 1,
		       y + i);
      break;
    case GTK_ARROW_LEFT:
      if (backgc)
	{
	  points[0].x = x;
	  points[0].y = y + half_height;
	  points[1].x = x + width - 1;
	  points[1].y = y + height - 1;
	  points[2].x = x + width - 1;
	  points[2].y = y;

	  gdk_draw_polygon (window, backgc, 
			    TRUE, points, 3);
	}

      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1,
		       x + i,
		       y + half_height,
		       x + width - i - 1,
		       y + height - i - 1);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1,
		       x + width - i - 1,
		       y + height - i - 1,
		       x + width - i - 1,
		       y + i);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2,
		       x + width - i - 1,
		       y + i,
		       x + i,
		       y + half_width);
      break;
    case GTK_ARROW_RIGHT:
      if (backgc)
	{
	  points[0].x = x + width - 1;
	  points[0].y = y + half_height;
	  points[1].x = x;
	  points[1].y = y;
	  points[2].x = x;
	  points[2].y = y + height - 1;

	  gdk_draw_polygon (window, backgc, 
			    TRUE, points, 3);
	}

      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2,
		       x + width - i - 1,
		       y + half_height,
		       x + i,
		       y + i);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc2,
		       x + i,
		       y + i,
		       x + i,
		       y + height - i - 1);
      for (i = 0; i < thickness; i++)
	gdk_draw_line (window, gc1,
		       x + i,
		       y + height - i - 1,
		       x + width - i - 1,
		       y + half_height);
      break;
    }

done:
  g_function_leave ("gtk_draw_arrow");
}

void 
gtk_draw_diamond (GdkWindow     *window, 
		  GdkGC         *lightgc, 
		  GdkGC         *darkgc, 
		  GdkGC         *backgc, 
		  GtkShadowType  shadow_type, 
		  gint           x, 
		  gint           y, 
		  gint           width, 
		  gint           height, 
		  gint           thickness)
{
  GdkGC *gc1;
  GdkGC *gc2;
  gint half_width;
  gint half_height;
  gint i;
  
  g_function_enter ("gtk_draw_diamond");

  g_assert (window != NULL);
  g_assert (lightgc != NULL);
  g_assert (darkgc != NULL);

  switch (shadow_type)
    {
    case GTK_SHADOW_NONE:
      goto done;
      break;
    case GTK_SHADOW_IN:
      gc1 = lightgc;
      gc2 = darkgc;
      break;
    case GTK_SHADOW_OUT:
      gc1 = darkgc;
      gc2 = lightgc;
      break;
    default:
      g_error ("unsupported shadow type: %d", shadow_type);
      gc1 = NULL;
      gc2 = NULL;
      break;
    }

  half_width = width / 2;
  half_height = height / 2;

  for (i = 0; i < thickness+1; i++)
    {
      gdk_draw_line (window, gc1,
		     x + i, 
		     y + half_height,
		     x + half_width,
		     y + height - i);
      gdk_draw_line (window, gc1,
		     x + half_width,
		     y + height - i,
		     x + width - i,
		     y + half_height);
    }

  for (i = 0; i < thickness+1; i++)
    {
      gdk_draw_line (window, gc2,
		     x + i,
		     y + half_height,
		     x + half_width,
		     y + i);
      gdk_draw_line (window, gc2,
		     x + half_width,
		     y + i,
		     x + width - i,
		     y + half_height);
    }

done:
  g_function_leave ("gtk_draw_diamond");
}
