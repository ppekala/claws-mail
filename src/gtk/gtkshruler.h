/* GTKSHRuler
 * Copyright (C) 2000-2004 Alfons Hoogervorst & The Sylpheed Claws Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_SHRULER_H__
#define __GTK_SHRULER_H__


#include <gdk/gdk.h>
#include <gtk/gtkhruler.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_SHRULER(obj)            (GTK_CHECK_CAST ((obj), gtk_shruler_get_type (), GtkSHRuler))
#define GTK_SHRULER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), gtk_shruler_get_type (), GtkSHRulerClass))
#define GTK_IS_SHRULER(obj)         (GTK_CHECK_TYPE ((obj), gtk_shruler_get_type ()))


typedef struct _GtkSHRuler        GtkSHRuler;
typedef struct _GtkSHRulerClass   GtkSHRulerClass;

struct _GtkSHRuler
{
	GtkHRuler ruler;
};

struct _GtkSHRulerClass
{
 	GtkHRulerClass parent_class;
};


guint      gtk_shruler_get_type (void);
GtkWidget* gtk_shruler_new      (void);
void       gtk_shruler_set_pos  (GtkSHRuler *ruler,
				 gfloat      pos);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_SHRULER_H__ */
