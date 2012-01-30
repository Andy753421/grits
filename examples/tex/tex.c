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

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glu.h>

guint tex, texl, texr, mask;

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer _)
{
	if (event->keyval == GDK_q)
		gtk_main_quit();
	return FALSE;
}

gboolean on_expose(GtkWidget *drawing, GdkEventExpose *event, gpointer _)
{
	gdouble y = 0;

	/* Setup view */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1,1, -1,1, 10,-10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -5);
	glClearColor(0.5, 0.5, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Draw white background rectangle */
	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	glVertex3f(-0.25, -0.75, 0.0);
	glVertex3f(-0.25,  0.75, 0.0);
	glVertex3f( 0.25,  0.75, 0.0);
	glVertex3f( 0.25, -0.75, 0.0);
	glEnd();

	/* Setup for textures */
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_COLOR_MATERIAL);

	/* Setup mask */
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mask);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	/* Left */
	glBindTexture(GL_TEXTURE_2D, texl);
	glBegin(GL_QUADS);
	glMultiTexCoord2f(GL_TEXTURE0,  0.0, y);     glMultiTexCoord2f(GL_TEXTURE1,  0.0, y);     glVertex3f(-0.75,  0.0, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0,  0.0, 1.0);   glMultiTexCoord2f(GL_TEXTURE1,  0.0, 1.0);   glVertex3f(-0.75,  0.5, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0,  2.0, 1.0);   glMultiTexCoord2f(GL_TEXTURE1,  2.0, 1.0);   glVertex3f( 0.75,  0.5, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0,  2.0, y);     glMultiTexCoord2f(GL_TEXTURE1,  2.0, y);     glVertex3f( 0.75,  0.0, 0.0);
	glEnd();

	/* Right */
	glBindTexture(GL_TEXTURE_2D, texr);
	glBegin(GL_QUADS);
	glMultiTexCoord2f(GL_TEXTURE0, -1.0, y);     glMultiTexCoord2f(GL_TEXTURE1, -1.0, y);     glVertex3f(-0.75, 0.0, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0, -1.0, 1.0);   glMultiTexCoord2f(GL_TEXTURE1, -1.0, 1.0);   glVertex3f(-0.75, 0.5, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0,  1.0, 1.0);   glMultiTexCoord2f(GL_TEXTURE1,  1.0, 1.0);   glVertex3f( 0.75, 0.5, 0.0);
	glMultiTexCoord2f(GL_TEXTURE0,  1.0, y);     glMultiTexCoord2f(GL_TEXTURE1,  1.0, y);     glVertex3f( 0.75, 0.0, 0.0);
	glEnd();

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	/* Bottom */
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0, 0.0);   glVertex3f(-0.75, -0.5, 0.0);
	glTexCoord2f( 0.0, 1.0-y); glVertex3f(-0.75, -0.0, 0.0);
	glTexCoord2f( 1.0, 1.0-y); glVertex3f( 0.75, -0.0, 0.0);
	glTexCoord2f( 1.0, 0.0);   glVertex3f( 0.75, -0.5, 0.0);
	glEnd();

	/* Flush */
	GdkGLDrawable *gldrawable = gdk_gl_drawable_get_current();
	if (gdk_gl_drawable_is_double_buffered(gldrawable))
		gdk_gl_drawable_swap_buffers(gldrawable);
	else
		glFlush();
	return FALSE;
}
gboolean on_configure(GtkWidget *drawing, GdkEventConfigure *event, gpointer _)
{
	glViewport(0, 0,
		drawing->allocation.width,
		drawing->allocation.height);
	return FALSE;
}

guint load_mask(void)
{
	guint  tex  = 0;
	guint8 byte = 0xff;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1, 1, 0,
			GL_ALPHA, GL_UNSIGNED_BYTE, &byte);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	return tex;
}

guint load_tex(gchar *filename)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
	guchar    *pixels = gdk_pixbuf_get_pixels(pixbuf);
	int        width  = gdk_pixbuf_get_width(pixbuf);
	int        height = gdk_pixbuf_get_height(pixbuf);
	int        alpha  = gdk_pixbuf_get_has_alpha(pixbuf);
	guint      tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			(alpha ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, pixels);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	g_object_unref(pixbuf);
	return tex;
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	GtkWidget   *window   = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget   *drawing  = gtk_drawing_area_new();
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode((GdkGLConfigMode)(
			GDK_GL_MODE_RGBA   | GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA));
	g_signal_connect(window,  "destroy",         G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window,  "key-press-event", G_CALLBACK(on_key_press),  NULL);
	g_signal_connect(drawing, "expose-event",    G_CALLBACK(on_expose),     NULL);
	g_signal_connect(drawing, "configure-event", G_CALLBACK(on_configure),  NULL);
	gtk_widget_set_gl_capability(drawing, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);
	gtk_container_add(GTK_CONTAINER(window), drawing);
	gtk_widget_show_all(window);

	/* OpenGL setup */
	GdkGLContext  *glcontext  = gtk_widget_get_gl_context(GTK_WIDGET(drawing));
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(GTK_WIDGET(drawing));
	gdk_gl_drawable_gl_begin(gldrawable, glcontext);

	/* Load texture */
	mask = load_mask();
	texl = load_tex("texls.png");
	texr = load_tex("texrs.png");
	tex  = load_tex("tex.png");

	gtk_main();

	gdk_gl_drawable_gl_end(gldrawable);
}
