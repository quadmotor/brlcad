/*
 *  			P L A N E . H
 *
 *  This header file describes the plane_specific structure, which
 *  is for ARS processing, and the tri_specific structure,
 *  which is for ARB and PG processing.
 *  
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 *
 *  $Header$
 */
#ifndef PLANE_H
#define PLANE_H seen

#define MAXPTS	4			/* All we need are 4 points */
#define pl_A	pl_points[0]		/* Synonym for A point */

struct plane_specific  {
	int	pl_npts;		/* number of points on plane */
	point_t	pl_points[MAXPTS];	/* Actual points on plane */
	vect_t	pl_Xbasis;		/* X (B-A) vector (for 2d coords) */
	vect_t	pl_Ybasis;		/* Y (C-A) vector (for 2d coords) */
	vect_t	pl_N;			/* Unit-length Normal (outward) */
	fastf_t	pl_NdotA;		/* Normal dot A */
	fastf_t	pl_2d_x[MAXPTS];	/* X 2d-projection of points */
	fastf_t	pl_2d_y[MAXPTS];	/* Y 2d-projection of points */
	fastf_t	pl_2d_com[MAXPTS];	/* pre-computed common-term */
	struct plane_specific *pl_forw;	/* Forward link */
	char	pl_code[MAXPTS+1];	/* Face code string.  Decorative. */
};

/*
 *  Describe the tri_specific structure.
 */
struct tri_specific  {
	point_t	tri_A;			/* triangle vertex (A) */
	vect_t	tri_BA;			/* B - A (second point) */
	vect_t	tri_CA;			/* C - A (third point) */
	vect_t	tri_wn;			/* facet normal (non-unit) */
	vect_t	tri_N;			/* unit normal vector */
	struct tri_specific *tri_forw;	/* Next facet */
};

#endif PLANE_H
