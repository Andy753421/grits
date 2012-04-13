/*
 * Copyright (C) 2009-2011 Andy Spencer <andy753421@gmail.com>
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
 * SECTION:env
 * @short_description: Environment plugin
 *
 * #GritsPluginEnv provides environmental information such as sky images. It can
 * also paint a blank overlay on the surface so that other plugins can draw
 * transparent overlays nicely.
 */

#include <math.h>

#include <grits.h>
#include <GL/glut.h>

#include "env.h"

/***********
 * Helpers *
 ***********/
static void expose_sky(GritsCallback *sky, GritsOpenGL *opengl, gpointer _env)
{
	GritsPluginEnv *env = GRITS_PLUGIN_ENV(_env);
	g_debug("GritsPluginEnv: expose_sky");

	gdouble lat, lon, elev;
	grits_viewer_get_location(env->viewer, &lat, &lon, &elev);

	/* Misc */
	gdouble rg   = MAX(0, 1-(elev/40000));
	gdouble blue = MAX(0, 1-(elev/100000));
	glClearColor(MIN(0.4,rg), MIN(0.4,rg), MIN(1,blue), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	/* Attempt to render an atmosphere */
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_MODELVIEW);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	grits_viewer_center_position(env->viewer, lat, lon, -EARTH_R);

	gdouble ds  = EARTH_R+elev;     // distance to self
	gdouble da  = EARTH_R+300000;   // distance to top of atmosphere
	gdouble dg  = EARTH_R-100000;   // distance to top of atmosphere
	gdouble ang = acos(EARTH_R/ds); // angle to horizon
	ang = MAX(ang,0.1);

	gdouble ar  = sin(ang)*da;      // top of quad fan "atomosphere"j
	gdouble az  = cos(ang)*da;      //

	gdouble gr  = sin(ang)*dg;      // bottom of quad fan "ground"
	gdouble gz  = cos(ang)*dg;      //

	glBegin(GL_QUAD_STRIP);
	for (gdouble i = 0; i <= 2*G_PI; i += G_PI/30) {
		glColor4f(0.3, 0.3, 1.0, 1.0); glVertex3f(gr*sin(i), gr*cos(i), gz);
		glColor4f(0.3, 0.3, 1.0, 0.0); glVertex3f(ar*sin(i), ar*cos(i), az);
	}
	glEnd();
}

static void expose_hud(GritsCallback *hud, GritsOpenGL *opengl, gpointer _env)
{
	GritsPluginEnv *env = GRITS_PLUGIN_ENV(_env);
	g_debug("GritsPluginEnv: expose_hud");
	gdouble x, y, z;
	grits_viewer_get_rotation(env->viewer, &x, &y, &z);

	/* Setup projection */
	gint win_width  = GTK_WIDGET(opengl)->allocation.width;
	gint win_height = GTK_WIDGET(opengl)->allocation.height;
	float scale     = MIN(win_width, win_height) / 10.0;
	float offset    = scale+20;
	glTranslatef(offset, offset, 0);

	/* Setup lighting */
	float light_ambient[]  = {0.1f, 0.1f, 0.0f, 1.0f};
	float light_diffuse[]  = {0.9f, 0.9f, 0.9f, 1.0f};
	float light_position[] = {-50.0f, -40.0f, -80.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	/* Setup state */
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	/* Draw compas */
	glRotatef(x, 1, 0, 0);
	glRotatef(-z, 0, 0, 1);
	glRotatef(-90, 0, 0, 1);
	glRotatef(-90, 1, 0, 0);
	glColor4f(0.9, 0.9, 0.7, 1.0);
	glutSolidTeapot(scale * 0.5);
}

static void click_hud(GritsCallback *hud, GritsViewer *viewer)
{
	grits_viewer_set_rotation(viewer, 0, 0, 0);
}

/***********
 * Methods *
 ***********/
/**
 * grits_plugin_env_new:
 * @viewer: the #GritsViewer to use for drawing
 * @prefs:  the #GritsPrefs for storing configurations
 *
 * Create a new instance of the environment plugin.
 *
 * Returns: the new #GritsPluginEnv
 */
GritsPluginEnv *grits_plugin_env_new(GritsViewer *viewer, GritsPrefs *prefs)
{
	g_debug("GritsPluginEnv: new");
	GritsPluginEnv *env = g_object_new(GRITS_TYPE_PLUGIN_ENV, NULL);
	env->viewer = g_object_ref(viewer);

	/* Add sky */
	GritsCallback *sky = grits_callback_new(expose_sky, env);
	GritsCallback *hud = grits_callback_new(expose_hud, env);
	grits_viewer_add(viewer, GRITS_OBJECT(sky), GRITS_LEVEL_BACKGROUND, FALSE);
	grits_viewer_add(viewer, GRITS_OBJECT(hud), GRITS_LEVEL_HUD, FALSE);
	g_signal_connect(hud, "clicked", G_CALLBACK(click_hud), viewer);
	env->refs = g_list_prepend(env->refs, sky);
	env->refs = g_list_prepend(env->refs, hud);

	/* Add background */
	//GritsTile *background = grits_tile_new(NULL, NORTH, SOUTH, EAST, WEST);
	//glGenTextures(1, &env->tex);
	//background->data = &env->tex;
	//grits_viewer_add(viewer, GRITS_OBJECT(background), GRITS_LEVEL_BACKGROUND, FALSE);
	//env->refs = g_list_prepend(env->refs, background);

	return env;
}


/****************
 * GObject code *
 ****************/
/* Plugin init */
static void grits_plugin_env_plugin_init(GritsPluginInterface *iface);
G_DEFINE_TYPE_WITH_CODE(GritsPluginEnv, grits_plugin_env, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(GRITS_TYPE_PLUGIN,
			grits_plugin_env_plugin_init));
static void grits_plugin_env_plugin_init(GritsPluginInterface *iface)
{
	g_debug("GritsPluginEnv: plugin_init");
	/* Add methods to the interface */
}
/* Class/Object init */
static void grits_plugin_env_init(GritsPluginEnv *env)
{
	g_debug("GritsPluginEnv: init");
	/* Set defaults */
}
static void grits_plugin_env_dispose(GObject *gobject)
{
	g_debug("GritsPluginEnv: dispose");
	GritsPluginEnv *env = GRITS_PLUGIN_ENV(gobject);
	/* Drop references */
	if (env->viewer) {
		for (GList *cur = env->refs; cur; cur = cur->next)
			grits_viewer_remove(env->viewer, cur->data);
		g_list_free(env->refs);
		g_object_unref(env->viewer);
		glDeleteTextures(1, &env->tex);
		env->viewer = NULL;
	}
	G_OBJECT_CLASS(grits_plugin_env_parent_class)->dispose(gobject);
}
static void grits_plugin_env_class_init(GritsPluginEnvClass *klass)
{
	g_debug("GritsPluginEnv: class_init");
	int argc = 1; char *argv[] = {"", NULL};
	glutInit(&argc, argv);
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose = grits_plugin_env_dispose;
}
