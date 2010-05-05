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

/**
 * SECTION:gis-opengl
 * @short_description: OpenGL based virtual globe
 *
 * #GisOpenGL is the core rendering engine used by libgis. Theoretically other
 * renderers could be writte, but they have not been. GisOpenGL uses the ROAM
 * algorithm for updating surface mesh the planet. The only thing GisOpenGL can
 * actually render on it's own is a wireframe of a sphere.
 *
 * GisOpenGL relies on #GtkGlExt and requires (at least) OpenGL 2.0.
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "gis-opengl.h"
#include "gis-util.h"
#include "roam.h"

#include "objects/gis-object.h"
#include "objects/gis-marker.h"
#include "objects/gis-callback.h"

// #define ROAM_DEBUG

/* Tessellation, "finding intersecting triangles" */
/* http://research.microsoft.com/pubs/70307/tr-2006-81.pdf */
/* http://www.opengl.org/wiki/Alpha_Blending */

/***********
 * Helpers *
 ***********/
static void _set_visuals(GisOpenGL *opengl)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Camera 1 */
	double lat, lon, elev, rx, ry, rz;
	gis_viewer_get_location(GIS_VIEWER(opengl), &lat, &lon, &elev);
	gis_viewer_get_rotation(GIS_VIEWER(opengl), &rx, &ry, &rz);
	glRotatef(rx, 1, 0, 0);
	glRotatef(rz, 0, 0, 1);

	/* Lighting */
#ifdef ROAM_DEBUG
	float light_ambient[]  = {0.7f, 0.7f, 0.7f, 1.0f};
	float light_diffuse[]  = {2.0f, 2.0f, 2.0f, 1.0f};
#else
	float light_ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
	float light_diffuse[]  = {5.0f, 5.0f, 5.0f, 1.0f};
#endif
	float light_position[] = {-13*EARTH_R, 1*EARTH_R, 3*EARTH_R, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	float material_ambient[]  = {0.2, 0.2, 0.2, 1.0};
	float material_diffuse[]  = {0.8, 0.8, 0.8, 1.0};
	float material_specular[] = {0.1, 0.1, 0.1, 1.0};
	float material_emission[] = {0.0, 0.0, 0.0, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_emission);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_COLOR_MATERIAL);

	/* Camera 2 */
	glTranslatef(0, 0, -elev2rad(elev));
	glRotatef(lat, 1, 0, 0);
	glRotatef(-lon, 0, 1, 0);

	glDisable(GL_ALPHA_TEST);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

#ifndef ROAM_DEBUG
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
#endif

	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_LINE_SMOOTH);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glShadeModel(GL_FLAT);

	g_mutex_lock(opengl->sphere_lock);
	roam_sphere_update_view(opengl->sphere);
	g_mutex_unlock(opengl->sphere_lock);
}


/********************
 * Object handleing *
 ********************/
static void _draw_tile(GisOpenGL *opengl, GisTile *tile, GList *triangles)
{
	if (!tile || !tile->data)
		return;
	if (!triangles)
		g_warning("GisOpenGL: _draw_tiles - No triangles to draw: edges=%f,%f,%f,%f",
			tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	//g_message("drawing %4d triangles for tile edges=%7.2f,%7.2f,%7.2f,%7.2f",
	//		g_list_length(triangles), tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	gdouble n = tile->edge.n;
	gdouble s = tile->edge.s;
	gdouble e = tile->edge.e;
	gdouble w = tile->edge.w;

	gdouble londist = e - w;
	gdouble latdist = n - s;

	gdouble xscale = tile->coords.e - tile->coords.w;
	gdouble yscale = tile->coords.s - tile->coords.n;

	for (GList *cur = triangles; cur; cur = cur->next) {
		RoamTriangle *tri = cur->data;

		gdouble lat[3] = {tri->p.r->lat, tri->p.m->lat, tri->p.l->lat};
		gdouble lon[3] = {tri->p.r->lon, tri->p.m->lon, tri->p.l->lon};

		if (lon[0] < -90 || lon[1] < -90 || lon[2] < -90) {
			if (lon[0] > 90) lon[0] -= 360;
			if (lon[1] > 90) lon[1] -= 360;
			if (lon[2] > 90) lon[2] -= 360;
		}

		gdouble xy[3][2] = {
			{(lon[0]-w)/londist, 1-(lat[0]-s)/latdist},
			{(lon[1]-w)/londist, 1-(lat[1]-s)/latdist},
			{(lon[2]-w)/londist, 1-(lat[2]-s)/latdist},
		};

		//if ((lat[0] == 90 && (xy[0][0] < 0 || xy[0][0] > 1)) ||
		//    (lat[1] == 90 && (xy[1][0] < 0 || xy[1][0] > 1)) ||
		//    (lat[2] == 90 && (xy[2][0] < 0 || xy[2][0] > 1)))
		//	g_message("w,e=%4.f,%4.f   "
		//	          "lat,lon,x,y="
		//	          "%4.1f,%4.0f,%4.2f,%4.2f   "
		//	          "%4.1f,%4.0f,%4.2f,%4.2f   "
		//	          "%4.1f,%4.0f,%4.2f,%4.2f   ",
		//		w,e,
		//		lat[0], lon[0], xy[0][0], xy[0][1],
		//		lat[1], lon[1], xy[1][0], xy[1][1],
		//		lat[2], lon[2], xy[2][0], xy[2][1]);

		/* Fix poles */
		if (lat[0] == 90 || lat[0] == -90) xy[0][0] = 0.5;
		if (lat[1] == 90 || lat[1] == -90) xy[1][0] = 0.5;
		if (lat[2] == 90 || lat[2] == -90) xy[2][0] = 0.5;

		/* Scale to tile coords */
		for (int i = 0; i < 3; i++) {
			xy[i][0] = tile->coords.w + xy[i][0]*xscale;
			xy[i][1] = tile->coords.n + xy[i][1]*yscale;
		}

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glBindTexture(GL_TEXTURE_2D, *(guint*)tile->data);
		glPolygonOffset(0, -tile->zindex);
		glBegin(GL_TRIANGLES);
		glNormal3dv(tri->p.r->norm); glTexCoord2dv(xy[0]); glVertex3dv((double*)tri->p.r);
		glNormal3dv(tri->p.m->norm); glTexCoord2dv(xy[1]); glVertex3dv((double*)tri->p.m);
		glNormal3dv(tri->p.l->norm); glTexCoord2dv(xy[2]); glVertex3dv((double*)tri->p.l);
		glEnd();
	}
}

static void _draw_tiles(GisOpenGL *opengl, GisTile *tile)
{
	/* Only draw children if possible */
	gboolean has_children = FALSE;
	GisTile *child;
	gis_tile_foreach(tile, child)
		if (child && child->data)
			has_children = TRUE;

	GList *triangles = NULL;
	if (has_children) {
		/* TODO: simplify this */
		const gdouble rows = G_N_ELEMENTS(tile->children);
		const gdouble cols = G_N_ELEMENTS(tile->children[0]);
		const gdouble lat_dist = tile->edge.n - tile->edge.s;
		const gdouble lon_dist = tile->edge.e - tile->edge.w;
		const gdouble lat_step = lat_dist / rows;
		const gdouble lon_step = lon_dist / cols;
		int row, col;
		gis_tile_foreach_index(tile, row, col) {
			GisTile *child = tile->children[row][col];
			if (child && child->data) {
				_draw_tiles(opengl, child);
			} else {
				const gdouble n = tile->edge.n-(lat_step*(row+0));
				const gdouble s = tile->edge.n-(lat_step*(row+1));
				const gdouble e = tile->edge.w+(lon_step*(col+1));
				const gdouble w = tile->edge.w+(lon_step*(col+0));
				GList *these = roam_sphere_get_intersect(opengl->sphere,
						FALSE, n, s, e, w);
				triangles = g_list_concat(triangles, these);
			}
		}
	} else {
		triangles = roam_sphere_get_intersect(opengl->sphere, FALSE,
				tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	}
	if (triangles)
		_draw_tile(opengl, tile, triangles);
	g_list_free(triangles);
}

static void _draw_marker(GisOpenGL *opengl, GisMarker *marker)
{
	GisPoint *point = gis_object_center(marker);
	gdouble px, py, pz;
	gis_viewer_project(GIS_VIEWER(opengl),
			point->lat, point->lon, point->elev,
			&px, &py, &pz);

	gint win_width  = GTK_WIDGET(opengl)->allocation.width;
	gint win_height = GTK_WIDGET(opengl)->allocation.height;
	py = win_height - py;
	if (pz > 1)
		return;

	//g_debug("GisOpenGL: draw_marker - %s pz=%f ", marker->label, pz);

	cairo_surface_t *surface = cairo_get_target(marker->cairo);
	gdouble width  = cairo_image_surface_get_width(surface);
	gdouble height = cairo_image_surface_get_height(surface);

	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
	glOrtho(0, win_width, win_height, 0, -1, 1);
	glTranslated(px - marker->xoff,
	             py - marker->yoff, 0);

	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, marker->tex);
	glDisable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	glTexCoord2f(1, 0); glVertex3f(width, 0     , 0);
	glTexCoord2f(1, 1); glVertex3f(width, height, 0);
	glTexCoord2f(0, 1); glVertex3f(0    , height, 0);
	glTexCoord2f(0, 0); glVertex3f(0    , 0     , 0);
	glEnd();
}

static void _draw_callback(GisOpenGL *opengl, GisCallback *callback)
{
	callback->callback(callback, callback->user_data);
}

static void _draw_object(GisOpenGL *opengl, GisObject *object)
{
	//g_debug("GisOpenGL: draw_object");
	/* Skip hidden objects */
	if (object->hidden)
		return;

	/* Skip out of range objects */
	if (object->lod > 0) {
		/* LOD test */
		gdouble eye[3], obj[3];
		gis_viewer_get_location(GIS_VIEWER(opengl), &eye[0], &eye[1], &eye[2]);
		gdouble elev = eye[2];
		lle2xyz(eye[0], eye[1], eye[2], &eye[0], &eye[1], &eye[2]);
		lle2xyz(object->center.lat, object->center.lon, object->center.elev,
			&obj[0], &obj[1], &obj[2]);
		gdouble dist = distd(obj, eye);
		if (object->lod < dist)
			return;

		/* Horizon testing */
		gdouble c = EARTH_R+elev;
		gdouble a = EARTH_R;
		gdouble horizon = sqrt(c*c - a*a);
		if (dist > horizon)
			return;
	}

	/* Draw */
	glMatrixMode(GL_PROJECTION); glPushMatrix();
	glMatrixMode(GL_MODELVIEW);  glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	if (GIS_IS_MARKER(object)) {
		_draw_marker(opengl, GIS_MARKER(object));
	} else if (GIS_IS_CALLBACK(object)) {
		_draw_callback(opengl, GIS_CALLBACK(object));
	} else if (GIS_IS_TILE(object)) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		g_mutex_lock(opengl->sphere_lock);
		_draw_tiles(opengl, GIS_TILE(object));
		g_mutex_unlock(opengl->sphere_lock);
	}
	glPopAttrib();
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

static void _load_object(GisOpenGL *opengl, GisObject *object)
{
	//g_debug("GisOpenGL: load_object");
	if (GIS_IS_MARKER(object)) {
		GisMarker *marker = GIS_MARKER(object);
		cairo_surface_t *surface = cairo_get_target(marker->cairo);
		gdouble width  = cairo_image_surface_get_width(surface);
		gdouble height = cairo_image_surface_get_height(surface);

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &marker->tex);
		glBindTexture(GL_TEXTURE_2D, marker->tex);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				cairo_image_surface_get_data(surface));
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//g_debug("load_texture: %d", marker->tex);
	}
}

static void _unload_object(GisOpenGL *opengl, GisObject *object)
{
	//g_debug("GisOpenGL: unload_object");
	if (GIS_IS_MARKER(object)) {
		GisMarker *marker = GIS_MARKER(object);
		glDeleteTextures(1, &marker->tex);
	}
}


/*************
 * Callbacks *
 *************/
/* The unsorted/sroted GLists are blank head nodes,
 * This way us we can remove objects from the level just by fixing up links
 * I.e. we don't need to do a lookup to remove an object if we have its GList */
struct RenderLevel {
	GList unsorted;
	GList sorted;
};

static gboolean on_configure(GisOpenGL *opengl, GdkEventConfigure *event, gpointer _)
{
	g_debug("GisOpenGL: on_configure");

	double width  = GTK_WIDGET(opengl)->allocation.width;
	double height = GTK_WIDGET(opengl)->allocation.height;

	/* Setup OpenGL Window */
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double ang = atan(height/FOV_DIST);
	gluPerspective(rad2deg(ang)*2, width/height, 1000, 10*EARTH_R);

#ifndef ROAM_DEBUG
	g_mutex_lock(opengl->sphere_lock);
	roam_sphere_update_errors(opengl->sphere);
	g_mutex_unlock(opengl->sphere_lock);
#endif

	return FALSE;
}

static void on_realize(GisOpenGL *opengl, gpointer _)
{
	g_debug("GisOpenGL: on_realize");

	GdkGLContext   *glcontext  = gtk_widget_get_gl_context(GTK_WIDGET(opengl));
	GdkGLDrawable  *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(opengl));
	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();

	_set_visuals(opengl);
	on_configure(opengl, NULL, NULL);
}

static gboolean _draw_level(gpointer key, gpointer value, gpointer user_data)
{
	g_debug("GisOpenGL: _draw_level - level=%-4d", (int)key);
	GisOpenGL *opengl = user_data;
	struct RenderLevel *level = value;
	int nsorted = 0, nunsorted = 0;
	GList *cur = NULL;

	/* Draw opaque objects without sorting */
	glDepthMask(TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);
	for (cur = level->unsorted.next; cur; cur = cur->next, nunsorted++)
		_draw_object(opengl, GIS_OBJECT(cur->data));

	/* Freeze depth buffer and draw transparent objects sorted */
	/* TODO: sorting */
	//glDepthMask(FALSE);
	glAlphaFunc(GL_GREATER, 0.1);
	for (cur = level->sorted.next; cur; cur = cur->next, nsorted++)
		_draw_object(opengl, GIS_OBJECT(cur->data));

	/* TODO: Prune empty levels */

	g_debug("GisOpenGL: _draw_level - drew %d,%d objects",
			nunsorted, nsorted);
	return FALSE;
}

static gboolean on_expose(GisOpenGL *opengl, GdkEventExpose *event, gpointer _)
{
	g_debug("GisOpenGL: on_expose - begin");

	glClear(GL_COLOR_BUFFER_BIT);

	_set_visuals(opengl);
#ifdef ROAM_DEBUG
	glColor4f(0.0, 0.0, 9.0, 0.6);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	roam_sphere_draw(opengl->sphere);
	//roam_sphere_draw_normals(opengl->sphere);
#else
	g_mutex_lock(opengl->objects_lock);
	g_tree_foreach(opengl->objects, _draw_level, opengl);
	g_mutex_unlock(opengl->objects_lock);
	if (opengl->wireframe) {
		glClear(GL_DEPTH_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		roam_sphere_draw(opengl->sphere);
	}
#endif

	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(opengl));
	gdk_gl_drawable_swap_buffers(gldrawable);

	g_debug("GisOpenGL: on_expose - end\n");
	return FALSE;
}

static gboolean on_key_press(GisOpenGL *opengl, GdkEventKey *event, gpointer _)
{
	g_debug("GisOpenGL: on_key_press - key=%x, state=%x, plus=%x",
			event->keyval, event->state, GDK_plus);

	guint kv = event->keyval;
	/* Testing */
	if (kv == GDK_w) {
		opengl->wireframe = !opengl->wireframe;
		gtk_widget_queue_draw(GTK_WIDGET(opengl));
	}
#ifdef ROAM_DEBUG
	else if (kv == GDK_n) roam_sphere_split_one(opengl->sphere);
	else if (kv == GDK_p) roam_sphere_merge_one(opengl->sphere);
	else if (kv == GDK_r) roam_sphere_split_merge(opengl->sphere);
	else if (kv == GDK_u) roam_sphere_update_errors(opengl->sphere);
	gtk_widget_queue_draw(GTK_WIDGET(opengl));
#endif
	return FALSE;
}

static gboolean _update_errors_cb(gpointer _opengl)
{
	GisOpenGL *opengl = _opengl;
	g_mutex_lock(opengl->sphere_lock);
	roam_sphere_update_errors(opengl->sphere);
	g_mutex_unlock(opengl->sphere_lock);
	opengl->ue_source = 0;
	return FALSE;
}
static void on_view_changed(GisOpenGL *opengl,
		gdouble _1, gdouble _2, gdouble _3)
{
	g_debug("GisOpenGL: on_view_changed");
	_set_visuals(opengl);
#ifndef ROAM_DEBUG
	if (!opengl->ue_source)
		opengl->ue_source = g_idle_add_full(G_PRIORITY_HIGH_IDLE+30,
				_update_errors_cb, opengl, NULL);
	//roam_sphere_update_errors(opengl->sphere);
#endif
}

static gboolean on_idle(GisOpenGL *opengl)
{
	//g_debug("GisOpenGL: on_idle");
	g_mutex_lock(opengl->sphere_lock);
	if (roam_sphere_split_merge(opengl->sphere))
		gtk_widget_queue_draw(GTK_WIDGET(opengl));
	g_mutex_unlock(opengl->sphere_lock);
	return TRUE;
}


/*********************
 * GisViewer methods *
 *********************/
/**
 * gis_opengl_new:
 * @plugins: the plugins store to use
 * @prefs:   the preferences object to use
 *
 * Create a new OpenGL renderer.
 *
 * Returns: the new #GisOpenGL
 */
GisViewer *gis_opengl_new(GisPlugins *plugins, GisPrefs *prefs)
{
	g_debug("GisOpenGL: new");
	GisViewer *opengl = g_object_new(GIS_TYPE_OPENGL, NULL);
	gis_viewer_setup(opengl, plugins, prefs);
	return opengl;
}

static void gis_opengl_center_position(GisViewer *_opengl, gdouble lat, gdouble lon, gdouble elev)
{
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	glRotatef(lon, 0, 1, 0);
	glRotatef(-lat, 1, 0, 0);
	glTranslatef(0, 0, elev2rad(elev));
}

static void gis_opengl_project(GisViewer *_opengl,
		gdouble lat, gdouble lon, gdouble elev,
		gdouble *px, gdouble *py, gdouble *pz)
{
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	gdouble x, y, z;
	lle2xyz(lat, lon, elev, &x, &y, &z);
	gluProject(x, y, z,
		opengl->sphere->view->model,
		opengl->sphere->view->proj,
		opengl->sphere->view->view,
		px, py, pz);
}

static void gis_opengl_set_height_func(GisViewer *_opengl, GisTile *tile,
		RoamHeightFunc height_func, gpointer user_data, gboolean update)
{
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	if (!tile)
		return;
	/* TODO: get points? */
	g_mutex_lock(opengl->sphere_lock);
	GList *triangles = roam_sphere_get_intersect(opengl->sphere, TRUE,
			tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	for (GList *cur = triangles; cur; cur = cur->next) {
		RoamTriangle *tri = cur->data;
		RoamPoint *points[] = {tri->p.l, tri->p.m, tri->p.r, tri->split};
		for (int i = 0; i < G_N_ELEMENTS(points); i++) {
			if (tile->edge.n >= points[i]->lat && points[i]->lat >= tile->edge.s &&
			    tile->edge.e >= points[i]->lon && points[i]->lon >= tile->edge.w) {
				points[i]->height_func = height_func;
				points[i]->height_data = user_data;
				roam_point_update_height(points[i]);
			}
		}
	}
	g_list_free(triangles);
	g_mutex_unlock(opengl->sphere_lock);
}

static void _gis_opengl_clear_height_func_rec(RoamTriangle *root)
{
	if (!root)
		return;
	RoamPoint *points[] = {root->p.l, root->p.m, root->p.r, root->split};
	for (int i = 0; i < G_N_ELEMENTS(points); i++) {
		points[i]->height_func = NULL;
		points[i]->height_data = NULL;
		roam_point_update_height(points[i]);
	}
	_gis_opengl_clear_height_func_rec(root->kids[0]);
	_gis_opengl_clear_height_func_rec(root->kids[1]);
}

static void gis_opengl_clear_height_func(GisViewer *_opengl)
{
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	for (int i = 0; i < G_N_ELEMENTS(opengl->sphere->roots); i++)
		_gis_opengl_clear_height_func_rec(opengl->sphere->roots[i]);
}

static gpointer gis_opengl_add(GisViewer *_opengl, GisObject *object,
		gint key, gboolean sort)
{
	g_assert(GIS_IS_OPENGL(_opengl));
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	g_mutex_lock(opengl->objects_lock);
	_load_object(opengl, object);
	struct RenderLevel *level = g_tree_lookup(opengl->objects, (gpointer)key);
	if (!level) {
		level = g_new0(struct RenderLevel, 1);
		g_tree_insert(opengl->objects, (gpointer)key, level);
	}
	GList *list = sort ? &level->sorted : &level->unsorted;
	/* Put the link in the list */
	GList *link = g_new0(GList, 1);
	link->data = object;
	link->prev = list;
	link->next = list->next;
	if (list->next)
		list->next->prev = link;
	list->next = link;
	g_mutex_unlock(opengl->objects_lock);
	return link;
}

static GisObject *gis_opengl_remove(GisViewer *_opengl, gpointer _link)
{
	g_assert(GIS_IS_OPENGL(_opengl));
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	g_mutex_lock(opengl->objects_lock);
	GList *link = _link;
	GisObject *object = link->data;
	_unload_object(opengl, object);
	/* Just unlink and free it, link->prev is assured */
	link->prev->next = link->next;
	if (link->next)
		link->next->prev = link->prev;
	g_free(link);
	g_object_unref(object);
	g_mutex_unlock(opengl->objects_lock);
	return object;
}

/****************
 * GObject code *
 ****************/
static int _objects_cmp(gconstpointer _a, gconstpointer _b, gpointer _)
{
	gint a = (int)_a, b = (int)_b;
	return a < b ? -1 :
	       a > b ?  1 : 0;
}
static void _objects_free(gpointer value)
{
	struct RenderLevel *level = value;
	if (level->sorted.next)
		g_list_free(level->sorted.next);
	if (level->unsorted.next)
		g_list_free(level->unsorted.next);
	g_free(level);
}

G_DEFINE_TYPE(GisOpenGL, gis_opengl, GIS_TYPE_VIEWER);
static void gis_opengl_init(GisOpenGL *opengl)
{
	g_debug("GisOpenGL: init");
	/* OpenGL setup */
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
	if (!glconfig)
		g_error("Failed to create glconfig");
	if (!gtk_widget_set_gl_capability(GTK_WIDGET(opengl),
				glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_error("GL lacks required capabilities");
	g_object_unref(glconfig);

	opengl->objects = g_tree_new_full(_objects_cmp, NULL, NULL, _objects_free);
	opengl->objects_lock = g_mutex_new();
	opengl->sphere = roam_sphere_new(opengl);
	opengl->sphere_lock = g_mutex_new();

#ifndef ROAM_DEBUG
	opengl->sm_source[0] = g_timeout_add_full(G_PRIORITY_HIGH_IDLE+30, 33,  (GSourceFunc)on_idle, opengl, NULL);
	opengl->sm_source[1] = g_timeout_add_full(G_PRIORITY_HIGH_IDLE+10, 500, (GSourceFunc)on_idle, opengl, NULL);
#endif

	gtk_widget_add_events(GTK_WIDGET(opengl), GDK_KEY_PRESS_MASK);
	g_signal_connect(opengl, "realize",          G_CALLBACK(on_realize),      NULL);
	g_signal_connect(opengl, "configure-event",  G_CALLBACK(on_configure),    NULL);
	g_signal_connect(opengl, "expose-event",     G_CALLBACK(on_expose),       NULL);

	g_signal_connect(opengl, "key-press-event",  G_CALLBACK(on_key_press),    NULL);

	g_signal_connect(opengl, "location-changed", G_CALLBACK(on_view_changed), NULL);
	g_signal_connect(opengl, "rotation-changed", G_CALLBACK(on_view_changed), NULL);
}
static void gis_opengl_dispose(GObject *_opengl)
{
	g_debug("GisOpenGL: dispose");
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	if (opengl->sm_source[0]) {
		g_source_remove(opengl->sm_source[0]);
		opengl->sm_source[0] = 0;
	}
	if (opengl->sm_source[1]) {
		g_source_remove(opengl->sm_source[1]);
		opengl->sm_source[1] = 0;
	}
	if (opengl->ue_source) {
		g_source_remove(opengl->ue_source);
		opengl->ue_source = 0;
	}
	G_OBJECT_CLASS(gis_opengl_parent_class)->dispose(_opengl);
}
static void gis_opengl_finalize(GObject *_opengl)
{
	g_debug("GisOpenGL: finalize");
	GisOpenGL *opengl = GIS_OPENGL(_opengl);
	roam_sphere_free(opengl->sphere);
	g_tree_destroy(opengl->objects);
	g_mutex_free(opengl->objects_lock);
	g_mutex_free(opengl->sphere_lock);
	G_OBJECT_CLASS(gis_opengl_parent_class)->finalize(_opengl);
}
static void gis_opengl_class_init(GisOpenGLClass *klass)
{
	g_debug("GisOpenGL: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gis_opengl_finalize;
	gobject_class->dispose = gis_opengl_dispose;

	GisViewerClass *viewer_class = GIS_VIEWER_CLASS(klass);
	viewer_class->center_position   = gis_opengl_center_position;
	viewer_class->project           = gis_opengl_project;
	viewer_class->clear_height_func = gis_opengl_clear_height_func;
	viewer_class->set_height_func   = gis_opengl_set_height_func;
	viewer_class->add               = gis_opengl_add;
	viewer_class->remove            = gis_opengl_remove;
}
