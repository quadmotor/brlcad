/* 
 *			H I D E L I N E . C
 * 
 * Description -
 *	Takes the vector  list for the  current  display  and  raytraces
 *	along those vectors.  If the first point hit in the model is the
 *	same as that  vector,  continue  the line  through  that  point;
 *	otherwise,  stop  drawing  that  vector  or  draw  dotted  line.
 *	Produces Unix-plot type output.
 *
 *	The command is "H file.pl [stepsize] [%epsilon]". Stepsize is the
 *	number of segments into which the window size should be broken.
 *	%Epsilon specifies how close two points must be before they are
 *	considered equal. A good values for stepsize and %epsilon are 128
 *	and 1, respectively.
 *
 * Author -  
 *	Mark Huston Bowden  
 *	Research  Institute,  E-47 
 *	University of Alabama in Huntsville  
 *	Huntsville, AL 35899
 *	(205) 895-6467	UAH
 *	(205) 876-1089	Redstone Arsenal
 *
 * History -
 *	01 Aug 88		Began initial coding
 */

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "raytrace.h"
#include "./ged.h"
#include "./solid.h"
#include "./dm.h"

#define MAXOBJECTS	3000

#define VIEWSIZE	(2*Viewscale)
#define TRUE	1
#define FALSE	0

#define MOVE(v)	  VMOVE(last_move,(v))

#define DRAW(v)	{ vect_t a,b;\
		  MAT4X3PNT(a,model2view,last_move);\
		  MAT4X3PNT(b,model2view,(v));\
		  pdv_3line(plotfp, a, b ); }

extern struct db_i *dbip;	/* current database instance */

extern mat_t view2model;
extern mat_t model2view;

fastf_t epsilon;
vect_t aim_point;
struct solid *sp;

/*
 * hit_headon - routine called by rt_shootray if ray hits model
 */

static int
hit_headon(ap,PartHeadp)
register struct application *ap;
struct partition *PartHeadp;
{
	register char diff_solid;
	vect_t	diff;
	register fastf_t len;

	if (PartHeadp->pt_forw->pt_forw != PartHeadp)
		rt_log("hit_headon: multiple partitions\n");

	VJOIN1(PartHeadp->pt_forw->pt_inhit->hit_point,ap->a_ray.r_pt,
	    PartHeadp->pt_forw->pt_inhit->hit_dist, ap->a_ray.r_dir);
	VSUB2(diff,PartHeadp->pt_forw->pt_inhit->hit_point,aim_point);

	diff_solid = strcmp(sp->s_path[0]->d_namep,
	    PartHeadp->pt_forw->pt_inseg->seg_stp->st_name);
	len = MAGNITUDE(diff);

	if (	NEAR_ZERO(len,epsilon)
	    ||
	    ( diff_solid &&
	    VDOT(diff,ap->a_ray.r_dir) > 0 )
	    )
		return(1);
	else
		return(0);
}

/*
 * hit_tangent - routine called by rt_shootray if ray misses model
 *
 *     We know we are shooting at the model since we are aiming at the
 *     vector list MGED created. However, shooting at an edge or shooting
 *     tangent to a curve produces only one intersection point at which
 *     time rt_shootray reports a miss. Therefore, this routine is really
 *     a "hit" routine.
 */

static int
hit_tangent(ap,PartHeadp)
register struct application *ap;
struct partition *PartHeadp;
{
	return(1);		/* always a hit */
}

/*
 * hit_overlap - called by rt_shootray if ray hits an overlap
 */

static int
hit_overlap(ap,PartHeadp)
register struct application *ap;
struct partition *PartHeadp;
{
	return(0);		/* never a hit */
}

/*
 *			F _ H I D E L I N E
 */
int
f_hideline(argc, argv)
int	argc;
char	**argv;
{
	FILE 	*plotfp;
	char 	visible;
	extern int 	hit_headon(),hit_tangent(),hit_overlap();
	int 	i,numobjs;
	char 	*objname[MAXOBJECTS],title[1];
	fastf_t 	len,u,step;
	FAST float 	ratio;
	vect_t	last_move;
	struct rt_i	*rtip;
	struct resource resource;
	struct application a;
	vect_t temp;
	vect_t last,dir;
	register struct rt_vlist	*vp;

	if ((plotfp = fopen(argv[1],"w")) == NULL) {
		rt_log("f_hideline: unable to open \"%s\" for writing.\n",
		    argv[1]);
		return CMD_BAD;
	}
	pl_space(plotfp,-2048,-2048,2048,2048);

	/*  Build list of objects being viewed */
	numobjs = 0;
	FOR_ALL_SOLIDS(sp) {
		for (i = 0; i < numobjs; i++)  {
			if( objname[i] == sp->s_path[0]->d_namep )
				break;
		}
		if (i == numobjs)
			objname[numobjs++] = sp->s_path[0]->d_namep;
	}

	rt_log("Generating hidden-line drawing of the following regions:\n");
	for (i = 0; i < numobjs; i++)
		rt_log("\t%s\n",objname[i]);

	/* Initialization for librt */
	if ((rtip = rt_dirbuild(dbip->dbi_filename,title,0)) == RTI_NULL) {
		rt_log("f_hideline: unable to open model file \"%s\"\n",
		    dbip->dbi_filename);
		return CMD_BAD;
	}
	a.a_hit = hit_headon;
	a.a_miss = hit_tangent;
	a.a_overlap = hit_overlap;
	a.a_rt_i = rtip;
	a.a_resource = &resource;
	a.a_level = 0;
	a.a_onehit = 1;
	a.a_diverge = 0;
	a.a_rbeam = 0;

	if (argc > 2) {
		sscanf(argv[2],"%f",&step);
		step = Viewscale/step;
		sscanf(argv[3],"%f",&epsilon);
		epsilon *= Viewscale/100;
	} else {
		step = Viewscale/256;
		epsilon = 0.1*Viewscale;
	}

	for (i = 0; i < numobjs; i++)
		if (rt_gettree(rtip,objname[i]) == -1)
			rt_log("f_hideline: rt_gettree failed on \"%s\"\n",objname[i]);

	/* Crawl along the vectors raytracing as we go */
	VSET(temp,0,0,-1);				/* looking at model */
	MAT4X3VEC(a.a_ray.r_dir,view2model,temp);
	VUNITIZE(a.a_ray.r_dir);

	FOR_ALL_SOLIDS(sp) {

		ratio = sp->s_size / VIEWSIZE;		/* ignore if small or big */
		if (ratio >= dmp->dmr_bound || ratio < 0.001)
			continue;

		rt_log("Solid\n");
		for( RT_LIST_FOR( vp, rt_vlist, &(sp->s_vlist) ) )  {
			register int	i;
			register int	nused = vp->nused;
			register int	*cmd = vp->cmd;
			register point_t *pt = vp->pt;
			for( i = 0; i < nused; i++,cmd++,pt++ )  {
				rt_log("\tVector\n");
				switch( *cmd )  {
				case RT_VLIST_POLY_START:
					break;
				case RT_VLIST_POLY_MOVE:
				case RT_VLIST_LINE_MOVE:
					/* move */
					VMOVE(last, *pt);
					MOVE(last);
					break;
				case RT_VLIST_POLY_DRAW:
				case RT_VLIST_POLY_END:
				case RT_VLIST_LINE_DRAW:
					/* setup direction && length */
					VSUB2(dir, *pt, last);
					len = MAGNITUDE(dir);
					VUNITIZE(dir);
					visible = FALSE;
					rt_log("\t\tDraw 0 -> %g, step %g\n", len, step);
					for (u = 0; u <= len; u += step) {
						VJOIN1(aim_point,last,u,dir);
						MAT4X3PNT(temp,model2view,aim_point);
						temp[Z] = 100;			/* parallel project */
						MAT4X3PNT(a.a_ray.r_pt,view2model,temp);
						if (rt_shootray(&a)) {
							if (!visible) {
								visible = TRUE;
								MOVE(aim_point);
							}
						} else {
							if (visible) {
								visible = FALSE;
								DRAW(aim_point);
							}
						}
					}
					if (visible)
						DRAW(aim_point);
					VMOVE(last, *pt); /* new last vertex */
				}
			}
		}
	}
	fclose(plotfp);
	return CMD_OK;
}
