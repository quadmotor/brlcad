/*
 *  			W R A Y . C
 *
 *  Write a VLD-standard ray on the given file pointer.
 *  VLD-standard rays are defined by /vld/include/ray.h,
 *  included here for portability.  A variety of VLD programs
 *  exist to manipulate these files, including rayvect.
 *
 *  To obtain a UNIX-plot of a ray file, the procedure is:
 *	/vld/bin/rayvect -mMM < file.ray > file.vect
 *	/vld/bin/vectplot -mMM < file.vect > file.plot
 *	tplot -Tmeg file.plot		# or equivalent
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
 */
#ifndef lint
static char RCSwray[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"


/* /vld/include/ray.h -- ray segment data format (D A Gwyn) */
/* binary ray segment data record; see ray(4V) (SCCS vers 1.4) */
struct vldray
{
	float	ox;			/* origin coordinates */
	float	oy;
	float	oz;
	float	rx;			/* ray vector */
	float	ry;
	float	rz;
	float	na;			/* origin surface normal */
	float	ne;
	float	pa;			/* principal direction */
	float	pe;
	float	pc;			/* principal curvature */
	float	sc;			/* secondary curvature */
	long	ob;			/* object code */
	long	rt;			/* ray tag */
};

/* The normal is expected to be pointing out from the object */
#define WRAY_NORMAL(_ray, _norm)	\
	_ray.na = atan2( _norm[Y], _norm[X] ); \
	_ray.ne = asin( _norm[Z] );

/*
 *  The 32-bit ray tag field (rt) is encoded as follows:
 *	13 bits for screen X,
 *	13 bits for screen Y,
 *	 6 bits for ray level.
 *
 *  This admits of different ray tags for every ray in a raytrace
 *  up to 4096x4096 pixels, with up to 64 levels of recursion.
 *  It is not clear just why this had to be encoded; it would have
 *  been more useful for the file to have several fields for this.
 *
 *  0                   1                   2                   3 3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Screen Y        |          Screen X       |    Level  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define WRAY_TAG(_ray, _ap)	{ \
		if( (_ray.rt = _ap->a_level) > 0x3F || _ray.rt < 0 ) \
			_ray.rt = 0x3F; \
		_ray.rt |= ((_ap->a_x & 0x1FFF) << 6 ) | \
			   ((_ap->a_y & 0x1FFF) << (6+13) ); \
	}

/*
 *  			W R A Y
 */
void
wray( pp, ap, fp )
register struct partition *pp;
register struct application *ap;
FILE *fp;
{
	LOCAL struct vldray vldray;
	register struct hit *hitp= pp->pt_inhit;

	VMOVE( &(vldray.ox), hitp->hit_point );
	VSUB2( &(vldray.rx), pp->pt_outhit->hit_point,
		hitp->hit_point );

	if( pp->pt_inflip )  {
		VREVERSE( hitp->hit_normal, hitp->hit_normal );
		pp->pt_inflip = 0;
	}
	WRAY_NORMAL( vldray, hitp->hit_normal );

	vldray.pa = vldray.pe = vldray.pc = vldray.sc = 0;	/* no curv */

	/*  Air is marked by zero or negative region ID codes.
	 *  When air is encountered, the air code is taken from reg_aircode.
	 *  The negative of the air code is used for the "ob" field, to
	 *  distinguish air from other regions.
	 */
	if( (vldray.ob = pp->pt_regionp->reg_regionid) <= 0 )
		vldray.ob = -(pp->pt_regionp->reg_aircode);

	WRAY_TAG( vldray, ap );

	if( fwrite( &vldray, sizeof(struct vldray), 1, fp ) != 1 )
		rt_bomb("rway:  write error");
}

/*
 *  			W R A Y P T S
 *  
 *  Write a VLD-standard ray for a section of a ray specified
 *  by the "in" and "out" distances along the ray.  This is usually
 *  used for logging passage through "air" (ie, no solid).
 *  The "inorm" flag holds an inward pointing normal (typ. a r_dir value)
 *  that will be flipped on output, so that the "air solid"
 *  has a proper outward pointing normal.
 */
void
wraypts( in, inorm, out, id, ap, fp )
vect_t	in;
vect_t	inorm;
vect_t	out;
int	id;
struct application *ap;
FILE	*fp;
{
	LOCAL struct vldray vldray;
	vect_t	norm;

	VMOVE( &(vldray.ox), in );
	VSUB2( &(vldray.rx), out, in );

	VREVERSE( norm, inorm );
	WRAY_NORMAL( vldray, norm );

	vldray.pa = vldray.pe = vldray.pc = vldray.sc = 0;	/* no curv */

	vldray.ob = id;

	WRAY_TAG( vldray, ap );

	fwrite( &vldray, sizeof(struct vldray), 1, fp );
}

/*
 *			W R A Y P A I N T
 *
 *  Write "paint" into a VLD standard rayfile.
 */
void
wraypaint( start, norm, paint, ap, fp )
vect_t start, norm;
struct application *ap;
FILE *fp;
{
	LOCAL struct vldray vldray;

	VMOVE( &(vldray.ox), start );
	VSETALL( &(vldray.rx), 0 );

	WRAY_NORMAL( vldray, norm );

	vldray.pa = vldray.pe = vldray.pc = vldray.sc = 0;	/* no curv */

	vldray.ob = paint;

	WRAY_TAG( vldray, ap );

	fwrite( &vldray, sizeof(struct vldray), 1, fp );
}
