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

#ifndef __GIS_UTIL_H__
#define __GIS_UTIL_H__

#include <glib.h>

/**
 * EARTH_R:
 *
 * Radius of the earth
 */
#define EARTH_R (6371000)

/**
 * EARTH_C:
 *
 * Circumference of the earth at the equator
 */
#define EARTH_C (2*G_PI*EARTH_R)

/**
 * NORTH:
 *
 * Latitude at the north poll
 */
#define NORTH  90

/**
 * SOUTH:
 *
 * Latitude at the south poll
 */
#define SOUTH -90

/**
 * EAST:
 *
 * Eastern most longitude
 */
#define EAST  180

/**
 * WEST:
 *
 * Western most longitude
 */
#define WEST -180

/***************
 * Conversions *
 ***************/
/**
 * azim2lon:
 * @azim: the azimuth in radians
 *
 * Convert azimuth to longitude
 *
 * Returns: the longitude 
 */
#define azim2lon(azim) ((azim)*180/G_PI)

/**
 * lon2azim:
 * @lon: the longitude
 *
 * Convert longitude to azimuth 
 *
 * Returns: the azimuth in radians
 */
#define lon2azim(lon)  ((lon)*G_PI/180)

/**
 * incl2lat:
 * @incl: the inclination in radians
 *
 * Convert inclination to latitude
 *
 * Returns: the latitude
 */
#define incl2lat(incl) (90-(incl)*180/G_PI)

/**
 * lat2incl:
 * @lat: the latitude
 *
 * Convert latitude to inclination
 *
 * Returns: the inclination in radians
 */
#define lat2incl(lat)  ((90-(lat))*G_PI/180)

/**
 * rad2elev:
 * @rad: the radius in meters
 *
 * Convert radius to elevation
 *
 * Returns: the elevation in meters above the earth surface
 */
#define rad2elev(rad)  ((rad)-EARTH_R)

/**
 * elev2rad:
 * @elev: the elevation in meters above the earth surface
 *
 * Convert elevation to radius
 *
 * Returns: the radius in meters
 */
#define elev2rad(elev) ((elev)+EARTH_R)

/**
 * deg2rad:
 * @deg: the angle in degrees
 *
 * Convert degrees to radians
 *
 * Returns: the angle in radians
 */
#define deg2rad(deg) (((deg)*G_PI)/180.0)

/**
 * rad2deg:
 * @rad: the angle in radians
 *
 * Convert radians to degrees
 *
 * Returns: the angle in degrees
 */
#define rad2deg(rad) (((rad)*180.0)/G_PI)


/*************
 * Datatypes *
 *************/

/* GisPoint */
typedef struct _GisPoint GisPoint;
struct _GisPoint {
	gdouble lat, lon, elev;
};

void gis_point_set_lle(GisPoint *point,
		gdouble lat, gdouble lon, gdouble elev);

/* GisBounds */
typedef struct _GisBounds GisBounds;
struct _GisBounds {
	gdouble n, s, e, w;
};

void gis_bounds_set_bounds(GisBounds *bounds,
		gdouble n, gdouble s, gdouble e, gdouble w);


/********
 * Misc *
 ********/
/**
 * FOV_DIST:
 *
 * Used by GisOpenGL to set up the drawing window
 */
#define FOV_DIST   2000.0

/**
 * MPPX:
 * @dist: the distance between the eye and the point in question
 *
 * Get the resolution that a point would be drawn at on the screen
 *
 * Returns: the resolution in meters per pixel
 */
#define MPPX(dist) (4*dist/FOV_DIST)

void lle2xyz(gdouble lat, gdouble lon, gdouble elev,
		gdouble *x, gdouble *y, gdouble *z);

void xyz2lle(gdouble x, gdouble y, gdouble z,
		gdouble *lat, gdouble *lon, gdouble *elev);

void xyz2ll(gdouble x, gdouble y, gdouble z,
		gdouble *lat, gdouble *lon);

gdouble ll2m(gdouble lon_dist, gdouble lat);

gdouble distd(gdouble *a, gdouble *b);

gdouble lon_avg(gdouble a, gdouble b);

#endif
