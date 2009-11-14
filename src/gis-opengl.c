/*
 * Copyright (C) 2009 Andy Spencer <spenceal@rose-hulman.edu>
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

/* Tessellation, "finding intersecting triangles" */
/* http://research.microsoft.com/pubs/70307/tr-2006-81.pdf */
/* http://www.opengl.org/wiki/Alpha_Blending */

#include <config.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "gis-opengl.h"
#include "gis-util.h"
#include "roam.h"

#define FOV_DIST   2000.0
#define MPPX(dist) (4*dist/FOV_DIST)

// #define ROAM_DEBUG

/***********
 * Helpers *
 ***********/
static void _gis_opengl_begin(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));

	GdkGLContext   *glcontext  = gtk_widget_get_gl_context(GTK_WIDGET(self));
	GdkGLDrawable  *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(self));

	if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
		g_assert_not_reached();
}

static void _gis_opengl_end(GisOpenGL *self)
{
	g_assert(GIS_IS_OPENGL(self));
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(self));
	gdk_gl_drawable_gl_end(gldrawable);
}

/*************
 * ROAM Code *
 *************/
static void _set_visuals(GisOpenGL *self)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Camera 1 */
	double lat, lon, elev, rx, ry, rz;
	gis_viewer_get_location(GIS_VIEWER(self), &lat, &lon, &elev);
	gis_viewer_get_rotation(GIS_VIEWER(self), &rx, &ry, &rz);
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
	float material_specular[] = {0.0, 0.0, 0.0, 1.0};
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

	/* Misc */
	gdouble rg   = MAX(0, 1-(elev/20000));
	gdouble blue = MAX(0, 1-(elev/50000));
	glClearColor(MIN(0.65,rg), MIN(0.65,rg), MIN(1,blue), 1.0f);
	glColor4f(1, 1, 1, 1);

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

	roam_sphere_update_view(self->sphere);
}


/*************
 * Callbacks *
 *************/
static void on_realize(GisOpenGL *self, gpointer _)
{
	g_debug("GisOpenGL: on_realize");
	_set_visuals(self);
	roam_sphere_update_errors(self->sphere);
}
static gboolean on_configure(GisOpenGL *self, GdkEventConfigure *event, gpointer _)
{
	g_debug("GisOpenGL: on_configure");
	_gis_opengl_begin(self);

	double width  = GTK_WIDGET(self)->allocation.width;
	double height = GTK_WIDGET(self)->allocation.height;
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double ang = atan(height/FOV_DIST);
	gluPerspective(rad2deg(ang)*2, width/height, 1, 20*EARTH_R);

#ifndef ROAM_DEBUG
	roam_sphere_update_errors(self->sphere);
#endif

	_gis_opengl_end(self);
	return FALSE;
}

static void on_expose_plugin(GisPlugin *plugin, gchar *name, GisOpenGL *self)
{
	_set_visuals(self);
	glMatrixMode(GL_PROJECTION); glPushMatrix();
	glMatrixMode(GL_MODELVIEW);  glPushMatrix();
	gis_plugin_expose(plugin);
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}
static gboolean on_expose(GisOpenGL *self, GdkEventExpose *event, gpointer _)
{
	g_debug("GisOpenGL: on_expose - begin");
	_gis_opengl_begin(self);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef ROAM_DEBUG
	gis_plugins_foreach(GIS_VIEWER(self)->plugins, G_CALLBACK(on_expose_plugin), self);

	if (self->wireframe) {
		_set_visuals(self);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		roam_sphere_draw(self->sphere);
	}
#else
	_set_visuals(self);
	glColor4f(0.0, 0.0, 9.0, 0.6);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	roam_sphere_draw(self->sphere);

	//roam_sphere_draw_normals(self->sphere);
#endif

	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(self));
	gdk_gl_drawable_swap_buffers(gldrawable);

	_gis_opengl_end(self);
	g_debug("GisOpenGL: on_expose - end\n");
	return FALSE;
}

static gboolean on_key_press(GisOpenGL *self, GdkEventKey *event, gpointer _)
{
	g_debug("GisOpenGL: on_key_press - key=%x, state=%x, plus=%x",
			event->keyval, event->state, GDK_plus);

	guint kv = event->keyval;
	gdk_threads_leave();
	/* Testing */
	if (kv == GDK_w) {
		self->wireframe = !self->wireframe;
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
#ifdef ROAM_DEBUG
	else if (kv == GDK_n) roam_sphere_split_one(self->sphere);
	else if (kv == GDK_p) roam_sphere_merge_one(self->sphere);
	else if (kv == GDK_r) roam_sphere_split_merge(self->sphere);
	else if (kv == GDK_u) roam_sphere_update_errors(self->sphere);
	gdk_threads_enter();
	gtk_widget_queue_draw(GTK_WIDGET(self));
#else
	gdk_threads_enter();
#endif

	return TRUE;
}

static gboolean _update_errors_cb(gpointer sphere)
{
	roam_sphere_update_errors(sphere);
	return FALSE;
}
static void on_view_changed(GisOpenGL *self,
		gdouble _1, gdouble _2, gdouble _3)
{
	g_debug("GisOpenGL: on_view_changed");
	gdk_threads_enter();
	_gis_opengl_begin(self);
	_set_visuals(self);
#ifndef ROAM_DEBUG
	g_idle_add_full(G_PRIORITY_HIGH_IDLE+30, _update_errors_cb, self->sphere, NULL);
	//roam_sphere_update_errors(self->sphere);
#endif
	_gis_opengl_end(self);
	gdk_threads_leave();
}

static gboolean on_idle(GisOpenGL *self)
{
	//g_debug("GisOpenGL: on_idle");
	gdk_threads_enter();
	_gis_opengl_begin(self);
	if (roam_sphere_split_merge(self->sphere))
		gtk_widget_queue_draw(GTK_WIDGET(self));
	_gis_opengl_end(self);
	gdk_threads_leave();
	return TRUE;
}


/*********************
 * GisViewer methods *
 *********************/
GisViewer *gis_opengl_new(GisPlugins *plugins)
{
	g_debug("GisOpenGL: new");
	GisViewer *self = g_object_new(GIS_TYPE_OPENGL, NULL);
	self->plugins = plugins;
	return self;
}

static void gis_opengl_center_position(GisViewer *_self, gdouble lat, gdouble lon, gdouble elev)
{
	GisOpenGL *self = GIS_OPENGL(_self);
	glRotatef(lon, 0, 1, 0);
	glRotatef(-lat, 1, 0, 0);
	glTranslatef(0, 0, elev2rad(elev));
}

static void gis_opengl_project(GisViewer *_self,
		gdouble lat, gdouble lon, gdouble elev,
		gdouble *px, gdouble *py, gdouble *pz)
{
	GisOpenGL *self = GIS_OPENGL(_self);
	gdouble x, y, z;
	lle2xyz(lat, lon, elev, &x, &y, &z);
	gluProject(x, y, z,
		self->sphere->view->model,
		self->sphere->view->proj,
		self->sphere->view->view,
		px, py, pz);
}

static void gis_opengl_render_tile(GisViewer *_self, GisTile *tile)
{
	GisOpenGL *self = GIS_OPENGL(_self);
	if (!tile || !tile->data)
		return;
	GList *triangles = roam_sphere_get_intersect(self->sphere,
			tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	if (!triangles)
		g_warning("GisOpenGL: render_tiles - No triangles to draw: edges=%f,%f,%f,%f",
			tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	//g_message("rendering %4d triangles for tile edges=%7.2f,%7.2f,%7.2f,%7.2f",
	//		g_list_length(triangles), tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	for (GList *cur = triangles; cur; cur = cur->next) {
		RoamTriangle *tri = cur->data;

		gdouble lat[3] = {tri->p.r->lat, tri->p.m->lat, tri->p.l->lat};
		gdouble lon[3] = {tri->p.r->lon, tri->p.m->lon, tri->p.l->lon};

		if (lon[0] < -90 || lon[1] < -90 || lon[2] < -90) {
			if (lon[0] > 90) lon[0] -= 360;
			if (lon[1] > 90) lon[1] -= 360;
			if (lon[2] > 90) lon[2] -= 360;
		}

		gdouble n = tile->edge.n;
		gdouble s = tile->edge.s;
		gdouble e = tile->edge.e;
		gdouble w = tile->edge.w;

		gdouble londist = e - w;
		gdouble latdist = n - s;

		gdouble xy[][3] = {
			{(lon[0]-w)/londist, 1-(lat[0]-s)/latdist},
			{(lon[1]-w)/londist, 1-(lat[1]-s)/latdist},
			{(lon[2]-w)/londist, 1-(lat[2]-s)/latdist},
		};

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, *(guint*)tile->data);
		glBegin(GL_TRIANGLES);
		glNormal3dv(tri->p.r->norm); glTexCoord2dv(xy[0]); glVertex3dv((double*)tri->p.r);
		glNormal3dv(tri->p.m->norm); glTexCoord2dv(xy[1]); glVertex3dv((double*)tri->p.m);
		glNormal3dv(tri->p.l->norm); glTexCoord2dv(xy[2]); glVertex3dv((double*)tri->p.l);
		glEnd();
	}
	g_list_free(triangles);
}

static void gis_opengl_render_tiles(GisViewer *_self, GisTile *tile)
{
	GisOpenGL *self = GIS_OPENGL(_self);
	/* Only render children if possible */
	gboolean has_children = TRUE;
	GisTile *child;
	gis_tile_foreach(tile, child)
		if (!child || !child->data)
			has_children = FALSE;
	if (has_children)
		/* Only render children */
		gis_tile_foreach(tile, child)
			gis_opengl_render_tiles(_self, child);
	else
		/* No children, render this tile */
		gis_opengl_render_tile(_self, tile);
}

static void gis_opengl_set_height_func(GisViewer *_self, GisTile *tile,
		RoamHeightFunc height_func, gpointer user_data, gboolean update)
{
	GisOpenGL *self = GIS_OPENGL(_self);
	if (!tile)
		return;
	/* TODO: get points? */
	GList *triangles = roam_sphere_get_intersect(self->sphere,
			tile->edge.n, tile->edge.s, tile->edge.e, tile->edge.w);
	for (GList *cur = triangles; cur; cur = cur->next) {
		RoamTriangle *tri = cur->data;
		RoamPoint *points[] = {tri->p.l, tri->p.m, tri->p.r, tri->split};
		for (int i = 0; i < G_N_ELEMENTS(points); i++) {
			points[i]->height_func = height_func;
			points[i]->height_data = user_data;
			roam_point_update_height(points[i]);
		}
	}
	g_list_free(triangles);
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

static void gis_opengl_clear_height_func(GisViewer *_self)
{
	GisOpenGL *self = GIS_OPENGL(_self);
	for (int i = 0; i < G_N_ELEMENTS(self->sphere->roots); i++)
		_gis_opengl_clear_height_func_rec(self->sphere->roots[i]);
}

static void gis_opengl_begin(GisViewer *_self)
{
	g_assert(GIS_IS_OPENGL(_self));
	_gis_opengl_begin(GIS_OPENGL(_self));
}

static void gis_opengl_end(GisViewer *_self)
{
	g_assert(GIS_IS_OPENGL(_self));
	_gis_opengl_end(GIS_OPENGL(_self));
}

/****************
 * GObject code *
 ****************/
G_DEFINE_TYPE(GisOpenGL, gis_opengl, GIS_TYPE_VIEWER);
static void gis_opengl_init(GisOpenGL *self)
{
	g_debug("GisOpenGL: init");
	/* OpenGL setup */
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
	if (!glconfig)
		g_error("Failed to create glconfig");
	if (!gtk_widget_set_gl_capability(GTK_WIDGET(self),
				glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_error("GL lacks required capabilities");
	g_object_unref(glconfig);

	gtk_widget_set_size_request(GTK_WIDGET(self), 600, 550);
	gtk_widget_set_events(GTK_WIDGET(self),
			GDK_BUTTON_PRESS_MASK |
			GDK_ENTER_NOTIFY_MASK |
			GDK_KEY_PRESS_MASK);
	g_object_set(self, "can-focus", TRUE, NULL);

	self->sphere = roam_sphere_new(self);

#ifndef ROAM_DEBUG
	self->sm_source[0] = g_timeout_add_full(G_PRIORITY_HIGH_IDLE+30, 33,  (GSourceFunc)on_idle, self, NULL);
	self->sm_source[1] = g_timeout_add_full(G_PRIORITY_HIGH_IDLE+10, 500, (GSourceFunc)on_idle, self, NULL);
#endif

	g_signal_connect(self, "realize",            G_CALLBACK(on_realize),      NULL);
	g_signal_connect(self, "configure-event",    G_CALLBACK(on_configure),    NULL);
	g_signal_connect(self, "expose-event",       G_CALLBACK(on_expose),       NULL);

	g_signal_connect(self, "key-press-event",    G_CALLBACK(on_key_press),    NULL);

	g_signal_connect(self, "location-changed",   G_CALLBACK(on_view_changed), NULL);
	g_signal_connect(self, "rotation-changed",   G_CALLBACK(on_view_changed), NULL);
}
static void gis_opengl_dispose(GObject *_self)
{
	g_debug("GisOpenGL: dispose");
	GisOpenGL *self = GIS_OPENGL(_self);
	if (self->sm_source[0]) {
		g_source_remove(self->sm_source[0]);
		self->sm_source[0] = 0;
	}
	if (self->sm_source[1]) {
		g_source_remove(self->sm_source[1]);
		self->sm_source[1] = 0;
	}
	if (self->sphere) {
		roam_sphere_free(self->sphere);
		self->sphere = NULL;
	}
	G_OBJECT_CLASS(gis_opengl_parent_class)->dispose(_self);
}
static void gis_opengl_class_init(GisOpenGLClass *klass)
{
	g_debug("GisOpenGL: class_init");
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose     = gis_opengl_dispose;

	GisViewerClass *viewer_class = GIS_VIEWER_CLASS(klass);
	viewer_class->center_position   = gis_opengl_center_position;
	viewer_class->project           = gis_opengl_project;
	viewer_class->clear_height_func = gis_opengl_clear_height_func;
	viewer_class->set_height_func   = gis_opengl_set_height_func;
	viewer_class->render_tile       = gis_opengl_render_tile;
	viewer_class->render_tiles      = gis_opengl_render_tiles;
	viewer_class->begin             = gis_opengl_begin;
	viewer_class->end               = gis_opengl_end;
}
