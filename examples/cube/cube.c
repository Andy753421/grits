#include <gtk/gtk.h>
#include <GL/glut.h>
#include "gtkgl.h"

static gdouble rotate = 0;

gboolean on_spin(GtkWidget *spin)
{
	/* Spin the cube when the toggle button is pressed */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(spin)))
		rotate++;
	gtk_widget_queue_draw(gtk_widget_get_toplevel(spin));
	return TRUE;
}

gboolean on_draw(GtkWidget *draw)
{
	GtkAllocation alloc;
	gtk_widget_get_allocation(draw, &alloc);

	/* Start OpenGL Commands */
	gtk_gl_begin(draw);

	/* Setup View */
	glViewport(0, 0, alloc.width, alloc.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (gdouble)alloc.width/alloc.height, 0.01, 10);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0, 0, -3);
	glRotated(rotate+20, 1, 0, 0);
	glRotated(rotate+30, 0, 1, 0);

	/* Draw a Cube */
	glClearColor(0, 0, 0.15, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glLineWidth(4);

	glColor3f(1, 0, 0);
	glutSolidCube(1);

	glColor3f(1, 1, 1);
	glutWireCube(1);

	/* Done with OpenGL */
	gtk_gl_end(draw);

	return TRUE;
}

int main(int argc, char **argv)
{
	/* Library initialization */
	gtk_init(&argc, &argv);
	glutInit(&argc, argv);

	/* Create widgets */
	GtkWidget *win  = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *draw = gtk_drawing_area_new();
	GtkWidget *spin = gtk_toggle_button_new_with_label("Spin");

	/* Pack everything together */
	gtk_container_add(GTK_CONTAINER(win), box);
	gtk_box_pack_start(GTK_BOX(box), draw, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), spin, FALSE, FALSE, 0);

	/* Connect signals */
	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
	g_signal_connect(draw, "draw", G_CALLBACK(on_draw), NULL);
	g_timeout_add(1000/60, (GSourceFunc)on_spin, spin);

	/* Enable OpenGL on drawing frame */
	gtk_gl_enable(draw);

	/* Show and run main loop */
	gtk_widget_show_all(win);
	gtk_main();

	return 0;
}
