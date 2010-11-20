/*
 * Copyright (C) 2009-2010 Andy Spencer <andy753421@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GRITS_OBJECT_H__
#define __GRITS_OBJECT_H__

#include <glib.h>
#include <glib-object.h>
#include "grits-util.h"

/* GritsObject */
#define GRITS_TYPE_OBJECT            (grits_object_get_type())
#define GRITS_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),   GRITS_TYPE_OBJECT, GritsObject))
#define GRITS_IS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),   GRITS_TYPE_OBJECT))
#define GRITS_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST   ((klass), GRITS_TYPE_OBJECT, GritsObjectClass))
#define GRITS_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE   ((klass), GRITS_TYPE_OBJECT))
#define GRITS_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GRITS_TYPE_OBJECT, GritsObjectClass))

typedef struct _GritsObject      GritsObject;
typedef struct _GritsObjectClass GritsObjectClass;

struct _GritsObject {
	GObject    parent_instance;
	GritsPoint center;
	gboolean   hidden;
	gdouble    lod;
};

#include "grits-opengl.h"
struct _GritsObjectClass {
	GObjectClass parent_class;

	/* Move some of these to GObject? */
	void (*draw) (GritsObject *object, GritsOpenGL *opengl);
};

GType grits_object_get_type(void);

/* Implemented by sub-classes */
void grits_object_draw(GritsObject *object, GritsOpenGL *opengl);

/**
 * grits_object_center:
 * @object: The #GritsObject to get the center of
 * 
 * Get the #GritsPoint representing the center of an object
 *
 * Returns: the center point
 */
#define grits_object_center(object) \
	(&GRITS_OBJECT(object)->center)

#endif