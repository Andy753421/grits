#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gis-tile.h"
#include "gis-wms.h"
#include "gis-util.h"

struct CacheState {
	GtkWidget *image;
	GtkWidget *status;
	GtkWidget *progress;
};


void chunk_callback(gsize cur, gsize total, gpointer _state)
{
	struct CacheState *state = _state;
	g_message("chunk_callback: %d/%d", cur, total);

	if (state->progress == NULL) {
		state->progress = gtk_progress_bar_new();
		gtk_box_pack_end(GTK_BOX(state->status), state->progress, FALSE, FALSE, 0);
		gtk_widget_show(state->progress);
	}

	if (cur == total)
		gtk_widget_destroy(state->progress);
	else
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), (gdouble)cur/total);
}

gpointer do_cache(gpointer _image)
{
	GtkImage *image = _image;
	g_message("Creating tile");
	GisTile *tile = gis_tile_new(NULL, NORTH, SOUTH, EAST, WEST);
	tile->children[0][1] = gis_tile_new(tile, NORTH, 0, 0, WEST);
	tile = tile->children[0][1];

	g_message("Fetching image");
	GisWms *bmng_wms = gis_wms_new(
		"http://www.nasa.network.com/wms", "bmng200406", "image/jpeg",
		"bmng_test/", "jpg", 512, 256);
	const char *path = gis_wms_make_local(bmng_wms, tile);

	g_message("Loading image: [%s]", path);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, NULL);
	gdk_threads_enter();
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
	gdk_threads_leave();

	g_message("Cleaning up");
	gis_wms_free(bmng_wms);
	gis_tile_free(tile, NULL, NULL);
	return NULL;
}

gboolean key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_q)
		gtk_main_quit();
	return TRUE;
}

int main(int argc, char **argv)
{
	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&argc, &argv);

	GtkWidget *win        = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *vbox1      = gtk_vbox_new(FALSE, 0);
	GtkWidget *vbox2      = gtk_vbox_new(FALSE, 0);
	GtkWidget *status     = gtk_statusbar_new();
	GtkWidget *scroll     = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *bmng_image = gtk_image_new();
	GtkWidget *srtm_image = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(win), vbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), vbox2);
	gtk_box_pack_start(GTK_BOX(vbox2), bmng_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), srtm_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), status, FALSE, FALSE, 0);
	g_signal_connect(win, "key-press-event", G_CALLBACK(key_press_cb), NULL);
	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	g_thread_create(do_cache, bmng_image, FALSE, NULL);

	gtk_widget_show_all(win);
	gtk_main();

	return 0;
}
