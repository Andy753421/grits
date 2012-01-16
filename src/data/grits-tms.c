/*
 * Copyright (C) 2009, 2012 Andy Spencer <spenceal@rose-hulman.edu>
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


#include <config.h>
#include <stdio.h>
#include <math.h>
#include <glib.h>

#include "grits-tms.h"

static gchar *_make_uri(GritsTms *tms, GritsTile *tile)
{

#if 0
	/* This doesn't make any sense.. */
	gdouble lon_rad = deg2rad(tile->edge.n + tile->edge.s)/2;
	gdouble lat_rad = deg2rad(tile->edge.e + tile->edge.w)/2;
	g_message("%lf,%lf", lat_rad, lon_rad);

	/* Reproject the coordinates to the Mercator projection: */
	gdouble x = lon_rad;
	gdouble y = log(tan(lat_rad) + 1.0/cos(lat_rad));

	/* Transform range of x and y to 0 - 1 and shift origin to top left */
	x = (1.0 + (x / G_PI)) / 2.0;
	y = (1.0 - (y / G_PI)) / 2.0;

	/* Calculate the number of tiles across the map, n, using 2^zoom */
	gint zoom = 0;
	for (GritsTile *tmp = tile->parent; tmp; tmp = tmp->parent)
		zoom++;
	gint n = pow(2, zoom);

	/* Multiply x and y by n. Round results down to give tilex and tiley. */
	gint xtile = x * n;
	gint ytile = y * n;

	g_message("xy=%f,%f  zoom=%d  n=%d  xy_tiles=%d,%d",
			x, y, zoom, n, xtile, ytile);
#endif

#if 1
	/* This is broken */
	gint zoom = 0;
	for (GritsTile *tmp = tile->parent; tmp; tmp = tmp->parent)
		zoom++;
	gint breath = pow(2,zoom);

	gdouble lon_pos =  (tile->edge.e+tile->edge.w)/2 + 180;
	gdouble lat_pos = -(tile->edge.n+tile->edge.s)/2 +  90 - 4.9489;

	gdouble lon_total = 360;
	gdouble lat_total = 85.0511*2;

	gdouble lon_pct = lon_pos / lon_total;
	gdouble lat_pct = lat_pos / lat_total;

	gint xtile = lon_pct * breath;
	gint ytile = lat_pct * breath;

	//g_message("bbok=%f,%f,%f,%f",
	//		tile->edge.n, tile->edge.s,
	//		tile->edge.e, tile->edge.w);
	//g_message("pos=%f,%f total=%f,%f pct=%f,%f tile=%d,%d",
	//		lon_pos,   lat_pos,
	//		lon_total, lat_total,
	//		lon_pct,   lat_pct,
	//		xtile,     ytile);
#endif

	// http://tile.openstreetmap.org/<zoom>/<xtile>/<ytile>.png
	return g_strdup_printf("%s/%d/%d/%d.%s",
			tms->uri_prefix, zoom, xtile, ytile, tms->extension);
}

gchar *grits_tms_fetch(GritsTms *tms, GritsTile *tile, GritsCacheType mode,
		GritsChunkCallback callback, gpointer user_data)
{
	/* Get file path */
	gchar *uri   = _make_uri(tms, tile);
	gchar *tilep = grits_tile_get_path(tile);
	gchar *local = g_strdup_printf("%s%s", tilep, tms->extension);
	gchar *path  = grits_http_fetch(tms->http, uri, local,
			mode, callback, user_data);
	g_free(uri);
	g_free(tilep);
	g_free(local);
	return path;
}

GritsTms *grits_tms_new(const gchar *uri_prefix,
		const gchar *prefix, const gchar *extension)
{
	GritsTms *tms = g_new0(GritsTms, 1);
	tms->http         = grits_http_new(prefix);
	tms->uri_prefix   = g_strdup(uri_prefix);
	tms->extension    = g_strdup(extension);
	return tms;
}

void grits_tms_free(GritsTms *tms)
{
	grits_http_free(tms->http);
	g_free(tms->uri_prefix);
	g_free(tms->extension);
	g_free(tms);
}
