/*
 *			D B 5 _ C O M B . C
 *
 *  Handle import/export of combinations (union tree) in v5 format.
 *
 *  The on-disk record looks like this:
 *	width byte
 *	n matricies (only non-identity matricies stored).
 *	n leaves
 *	len of RPN expression.  (len=0 signals all-union expression)
 *	depth of stack
 *	Section 1:  matricies
 *	Section 2:  leaves
 *	Section 3:  (Optional) RPN expression.
 *
 *  Encoding of a matrix is (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE) bytes,
 *  in network order (big-Endian, IEEE double precision).
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited.
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "db5.h"
#include "raytrace.h"

#include "./debug.h"


struct db_tree_counter_state {
	long	magic;
	long	n_mat;			/* # leaves with non-identity matricies */
	long	n_leaf;			/* # leaf nodes */
	long	n_oper;			/* # operator nodes */
	long	leafbytes;		/* # bytes for name section */
	int	non_union_seen;		/* boolean, 1 = non-unions seen */
};
#define DB_TREE_COUNTER_STATE_MAGIC	0x64546373	/* dTcs */
#define DB_CK_TREE_COUNTER_STATE(_p)	BU_CKMAG(_p, DB_TREE_COUNTER_STATE_MAGIC, "db_tree_counter_state");

/*
 *			D B _ T R E E _ C O U N T E R
 *
 *  Count number of non-identity matricies,
 *  number of leaf nodes, number of operator nodes, etc.
 *
 *  Returns -
 *	maximum depth of stack needed to unpack this tree, if serialized.
 *
 *  Notes -
 *	We over-estimate the size of the width fields used for
 *	holding the matrix subscripts.
 *	The caller is responsible for correcting by saying:
 *		tcsp->leafbytes -= tcsp->n_leaf * (8 - db5_enc_len[wid]);
 */
long
db_tree_counter( CONST union tree *tp, struct db_tree_counter_state *tcsp )
{
	long	ldepth, rdepth;

	RT_CK_TREE(tp);
	DB_CK_TREE_COUNTER_STATE(tcsp);

	switch( tp->tr_op )  {
	case OP_DB_LEAF:
		tcsp->n_leaf++;
		if( tp->tr_l.tl_mat )  tcsp->n_mat++;
		/* Over-estimate storage requirement for matrix # */
		tcsp->leafbytes += strlen(tp->tr_l.tl_name) + 1 + 8;
		return 1;

	case OP_NOT:
		/* Unary ops */
		tcsp->n_oper++;
		tcsp->non_union_seen = 1;
		return 1 + db_tree_counter( tp->tr_b.tb_left, tcsp );

	case OP_UNION:
		/* This node is known to be a binary op */
		tcsp->n_oper++;
		ldepth = db_tree_counter( tp->tr_b.tb_left, tcsp );
		rdepth = db_tree_counter( tp->tr_b.tb_right, tcsp );
		if( ldepth > rdepth )  return ldepth;
		return rdepth;

	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
		/* This node is known to be a binary op */
		tcsp->n_oper++;
		tcsp->non_union_seen = 1;
		ldepth = db_tree_counter( tp->tr_b.tb_left, tcsp );
		rdepth = db_tree_counter( tp->tr_b.tb_right, tcsp );
		if( ldepth > rdepth )  return ldepth;
		return rdepth;

	default:
		bu_log("db_tree_counter: bad op %d\n", tp->tr_op);
		bu_bomb("db_tree_counter\n");
		/* NOTREACHED */
	}
	/* NOTREACHED */
	return 0;
}

#define DB5COMB_TOKEN_LEAF		1
#define DB5COMB_TOKEN_UNION		2
#define DB5COMB_TOKEN_INTERSECT		3
#define DB5COMB_TOKEN_SUBTRACT		4
#define DB5COMB_TOKEN_XOR		5
#define DB5COMB_TOKEN_NOT		6

struct rt_comb_v5_serialize_state  {
	long		magic;
	long		mat_num;	/* current matrix number */
	long		nmat;		/* # matricies, total */
	unsigned char	*matp;
	unsigned char	*leafp;
	unsigned char	*exprp;
	int		wid;
};
#define RT_COMB_V5_SERIALIZE_STATE_MAGIC	0x43357373	/* C5ss */
#define RT_CK_COMB_V5_SERIALIZE_STATE(_p)	BU_CKMAG(_p, RT_COMB_V5_SERIALIZE_STATE_MAGIC, "rt_comb_v5_serialize_state")

/*
 *			R T _ C O M B _ V 5 _ S E R I A L I Z E
 *
 *  In one single pass through the tree, serialize out all three
 *  output sections at once.
 */
void
rt_comb_v5_serialize(
	const union tree	*tp,
	struct rt_comb_v5_serialize_state	*ssp)
{
	int	n;
	int	mi;

	RT_CK_TREE(tp);
	RT_CK_COMB_V5_SERIALIZE_STATE(ssp);

	switch( tp->tr_op )  {
	case OP_DB_LEAF:
		/*
		 *  Encoding of the leaf:
		 *	A null-terminated name string, and
		 *	the matrix subscript.  -1 == identity.
		 */
		n = strlen(tp->tr_l.tl_name) + 1;
		bcopy( tp->tr_l.tl_name, ssp->leafp, n );
		ssp->leafp += n;

		if( tp->tr_l.tl_mat )
			mi = ssp->mat_num++;
		else
			mi = -1;
		BU_ASSERT_LONG( mi, <, ssp->nmat );
		ssp->leafp = db5_encode_length( ssp->leafp, mi, ssp->wid );

		/* Encoding of the matrix */
		if( tp->tr_l.tl_mat )  {
			htond( ssp->matp,
				(const unsigned char *)tp->tr_l.tl_mat,
				ELEMENTS_PER_MAT );
			ssp->matp += ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE;
		}

		/* Encoding of the "leaf" operator */
		if( ssp->exprp )
			*ssp->exprp++ = DB5COMB_TOKEN_LEAF;
		return;

	case OP_NOT:
		/* Unary ops */
		rt_comb_v5_serialize( tp->tr_b.tb_left, ssp );
		if( ssp->exprp )
			*ssp->exprp++ = DB5COMB_TOKEN_NOT;
		return;

	case OP_UNION:
		/* This node is known to be a binary op */
		rt_comb_v5_serialize( tp->tr_b.tb_left, ssp );
		rt_comb_v5_serialize( tp->tr_b.tb_right, ssp );
		if( ssp->exprp )
			*ssp->exprp++ = DB5COMB_TOKEN_UNION;
		return;
	case OP_INTERSECT:
		/* This node is known to be a binary op */
		rt_comb_v5_serialize( tp->tr_b.tb_left, ssp );
		rt_comb_v5_serialize( tp->tr_b.tb_right, ssp );
		if( ssp->exprp )
			*ssp->exprp++ = DB5COMB_TOKEN_INTERSECT;
		return;
	case OP_SUBTRACT:
		/* This node is known to be a binary op */
		rt_comb_v5_serialize( tp->tr_b.tb_left, ssp );
		rt_comb_v5_serialize( tp->tr_b.tb_right, ssp );
		if( ssp->exprp )
			*ssp->exprp++ = DB5COMB_TOKEN_SUBTRACT;
		return;
	case OP_XOR:
		/* This node is known to be a binary op */
		rt_comb_v5_serialize( tp->tr_b.tb_left, ssp );
		rt_comb_v5_serialize( tp->tr_b.tb_right, ssp );
		if( ssp->exprp )
			*ssp->exprp++ = DB5COMB_TOKEN_XOR;
		return;

	default:
		bu_log("rt_comb_v5_serialize: bad op %d\n", tp->tr_op);
		bu_bomb("rt_comb_v5_serialize\n");
	}
}

/*
 *			R T _ C O M B _ E X P O R T 5
 */
int
rt_comb_export5(
	struct bu_external		*ep,
	const struct rt_db_internal	*ip,
	double				local2mm,
	const struct db_i		*dbip,
	struct resource			*resp)
{
	struct rt_comb_internal	*comb;
	struct db_tree_counter_state		tcs;
	struct rt_comb_v5_serialize_state	ss;
	long	max_stack_depth;
	long	need;
	int	rpn_len = 0;	/* # items in RPN expression */
	int	wid;
	unsigned char	*cp;
	unsigned char	*leafp_end;
	struct bu_attribute_value_set *avsp;
	struct bu_vls	value;

	RT_CK_DB_INTERNAL( ip );
	RT_CK_RESOURCE(resp);

	if( ip->idb_type != ID_COMBINATION ) bu_bomb("rt_comb_export5() type not ID_COMBINATION");
	comb = (struct rt_comb_internal *)ip->idb_ptr;
	RT_CK_COMB(comb);

	/* First pass -- count number of non-identity matricies,
	 * number of leaf nodes, number of operator nodes.
	 */
	bzero( (char *)&tcs, sizeof(tcs) );
	tcs.magic = DB_TREE_COUNTER_STATE_MAGIC;
	if( comb->tree )
		max_stack_depth = db_tree_counter( comb->tree, &tcs );
	else
		max_stack_depth = 0;	/* some combinations have no tree */

	if( tcs.non_union_seen )  {
		/* RPN expression needs one byte for each leaf or operator node */
		rpn_len = tcs.n_leaf + tcs.n_oper;
	} else {
		rpn_len = 0;
	}

	wid = db5_select_length_encoding(
		tcs.n_mat | tcs.n_leaf | tcs.leafbytes |
		rpn_len | max_stack_depth );

	/* Apply correction factor to tcs.leafbytes now that we know 'wid'.
	 * Ignore the slight chance that a smaller 'wid' might now be possible.
	 */
	tcs.leafbytes -= tcs.n_leaf * (8 - db5_enc_len[wid]);

	/* Second pass -- determine amount of on-disk storage needed */
	need =  1 +			/* width code */
		db5_enc_len[wid] + 	/* size for nmatricies */
		db5_enc_len[wid] +	/* size for nleaves */
		db5_enc_len[wid] +	/* size for leafbytes */
		db5_enc_len[wid] +	/* size for len of RPN */
		db5_enc_len[wid] +	/* size for max_stack_depth */
		tcs.n_mat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE) +	/* sizeof matrix array */
		tcs.leafbytes +		/* size for leaf nodes */
		rpn_len;		/* storage for RPN expression */

	BU_INIT_EXTERNAL(ep);
	ep->ext_nbytes = need;
#if 0
	ep->ext_buf = bu_malloc( need, "rt_comb_export5 ext_buf" );
#else
	ep->ext_buf = bu_calloc( 1, need, "rt_comb_export5 ext_buf" );
#endif

	/* Build combination's on-disk header section */
	cp = (unsigned char *)ep->ext_buf;
	*cp++ = wid;
	cp = db5_encode_length( cp, tcs.n_mat, wid );
	cp = db5_encode_length( cp, tcs.n_leaf, wid );
	cp = db5_encode_length( cp, tcs.leafbytes, wid );
	cp = db5_encode_length( cp, rpn_len, wid );
	cp = db5_encode_length( cp, max_stack_depth, wid );

	/*
	 *  The output format has three sections:
	 *	Section 1:  matricies
	 *	Section 2:  leaf nodes
	 *	Section 3:  Optional RPN expression
	 *
	 *  We have pre-computed the exact size of all three sections,
	 *  so they can all be searialized together in one pass.
	 *  Establish pointers to the start of each section.
	 */
	ss.magic = RT_COMB_V5_SERIALIZE_STATE_MAGIC;
	ss.wid = wid;
	ss.mat_num = 0;
	ss.nmat = tcs.n_mat;
	ss.matp = cp;
	ss.leafp = cp + tcs.n_mat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE);
	leafp_end = ss.leafp + tcs.leafbytes;
	if( rpn_len )
		ss.exprp = leafp_end;
	else
		ss.exprp = NULL;

	if( comb->tree )
		rt_comb_v5_serialize( comb->tree, &ss );

	BU_ASSERT_LONG( ss.mat_num, ==, tcs.n_mat );
	BU_ASSERT_PTR( ss.matp, ==, cp + tcs.n_mat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE) );
	BU_ASSERT_PTR( ss.leafp, ==, leafp_end );
	if( rpn_len )
		BU_ASSERT_PTR( ss.exprp, <=, ((unsigned char *)ep->ext_buf) + ep->ext_nbytes );

	/* Encode all the other stuff as attributes. */
	bu_vls_init( &value );
	/* WARNING:  We remove const from the ip pointer!!! */
	avsp = (struct bu_attribute_value_set *)&ip->idb_avs;
	if( avsp->magic != BU_AVS_MAGIC )
		bu_avs_init( avsp, 32, "rt_comb v5 attributes" );
	if( comb->region_flag )  {
		/* Presence of this attribute means this comb is a region. */
		/* Current code values are 0, 1, and 2; all are regions. */
		bu_vls_trunc( &value, 0 );
		bu_vls_printf( &value, "%d", comb->is_fastgen );
		bu_avs_add_vls( avsp, "region", &value );
	}
	if( comb->inherit )
		bu_avs_add( avsp, "inherit", "1" );
	if( comb->rgb_valid )  {
		bu_vls_trunc( &value, 0 );
		bu_vls_printf( &value, "%d/%d/%d", V3ARGS(comb->rgb) );
		bu_avs_add_vls( avsp, "rgb", &value );
	}
	/* optical shader string goes out in Tcl format */
	if( bu_vls_strlen( &comb->shader ) > 0 )
		bu_avs_add_vls( avsp, "oshader", &comb->shader );
	if( bu_vls_strlen( &comb->material ) > 0 )
		bu_avs_add_vls( avsp, "material", &comb->material );
	if( comb->temperature > 0 )  {
		bu_vls_trunc( &value, 0 );
		bu_vls_printf( &value, "%f", comb->temperature );
		bu_avs_add_vls( avsp, "temp", &value );
	}
	/* GIFT compatability */
	if( comb->region_id != 0 )  {
		bu_vls_trunc( &value, 0 );
		bu_vls_printf( &value, "%d", comb->region_id );
		bu_avs_add_vls( avsp, "region_id", &value );
	}
	if( comb->aircode != 0 )  {
		bu_vls_trunc( &value, 0 );
		bu_vls_printf( &value, "%d", comb->aircode );
		bu_avs_add_vls( avsp, "aircode", &value );
	}
	if( comb->GIFTmater != 0 )  {
		bu_vls_trunc( &value, 0 );
		bu_vls_printf( &value, "%d", comb->GIFTmater );
		bu_avs_add_vls( avsp, "giftmater", &value );
	}
	if( comb->los != 0 )  {
		bu_vls_trunc( &value, 0 );
		bu_vls_printf( &value, "%d", comb->los );
		bu_avs_add_vls( avsp, "los", &value );
	}

	bu_vls_free( &value );
	return 0;	/* OK */
}

/*
 *			R T _ C O M B _ I M P O R T 5
 *
 *  Read a combination object in v5 external (on-disk) format,
 *  and convert it into the internal format described in h/rtgeom.h
 *
 *  This is an unusual conversion, because some of the data is taken
 *  from attributes, not just from the object body.
 *  By the time this is called, the attributes will already have been
 *  cracked into ip->idb_avs, we get the attributes from there.
 *
 *  Returns -
 *	0	OK
 *	-1	FAIL
 */
int
rt_comb_import5(
	struct rt_db_internal	*ip,
	const struct bu_external *ep,
	const mat_t		mat,
	const struct db_i	*dbip,
	struct resource		*resp)
{
	struct rt_comb_internal	*comb;
	unsigned char	*cp;
	int		wid;
	long		nmat, nleaf, rpn_len, max_stack_depth;
	long		leafbytes;
	unsigned char	*matp;
	unsigned char	*leafp;
	unsigned char	*leafp_end;
	unsigned char	*exprp;
#define MAX_V5_STACK	8000
	union tree	*stack[MAX_V5_STACK];
	union tree	**sp;			/* stack pointer */
	const char	*ap;
	int		i;

	RT_CK_DB_INTERNAL( ip );
	BU_CK_EXTERNAL(ep);
	RT_CK_DBI(dbip);
	RT_CK_RESOURCE(resp);

	ip->idb_type = ID_COMBINATION;
	ip->idb_meth = &rt_functab[ID_COMBINATION];
	BU_GETSTRUCT( comb, rt_comb_internal );
	ip->idb_ptr = (genptr_t)comb;
	comb->magic = RT_COMB_MAGIC;
	bu_vls_init( &comb->shader );
	bu_vls_init( &comb->material );
	comb->temperature = -1;

	cp = ep->ext_buf;
	wid = *cp++;
	cp += db5_decode_length( &nmat, cp, wid );
	cp += db5_decode_length( &nleaf, cp, wid );
	cp += db5_decode_length( &leafbytes, cp, wid );
	cp += db5_decode_length( &rpn_len, cp, wid );
	cp += db5_decode_length( &max_stack_depth, cp, wid );
	matp = cp;
	leafp = cp + nmat * (ELEMENTS_PER_MAT * SIZEOF_NETWORK_DOUBLE);
	exprp = leafp + leafbytes;
	leafp_end = exprp;

	if( rpn_len == 0 )  {
		int	i;
		for( i = nleaf-1; i >= 0; i-- )  {
			union tree	*tp;
			long		mi;

			RT_GET_TREE( tp, resp );
			tp->tr_l.magic = RT_TREE_MAGIC;
			tp->tr_l.tl_op = OP_DB_LEAF;
			tp->tr_l.tl_name = bu_strdup( (const char *)leafp );
			leafp += strlen( (const char *)leafp) + 1;

			/* Get matrix index */
			mi = 4095;			/* sanity */
			leafp += db5_decode_signed( &mi, leafp, wid );

			if( mi < 0 )  {
				/* Signal identity matrix */
				tp->tr_l.tl_mat = NULL;
			} else {
				/* Unpack indicated matrix mi */
				BU_ASSERT_LONG( mi, <, nmat );
				tp->tr_l.tl_mat = (matp_t)bu_malloc(
					sizeof(mat_t), "v5comb mat");
				ntohd( (unsigned char *)tp->tr_l.tl_mat,
					&matp[mi*ELEMENTS_PER_MAT*SIZEOF_NETWORK_DOUBLE],
					ELEMENTS_PER_MAT);
			}

			if( comb->tree == TREE_NULL )  {
				comb->tree = tp;
			} else {
				union tree	*unionp;
				RT_GET_TREE( unionp, resp );
				unionp->tr_b.magic = RT_TREE_MAGIC;
				unionp->tr_b.tb_op = OP_UNION;
				unionp->tr_b.tb_left = comb->tree;
				unionp->tr_b.tb_right = tp;
				comb->tree = unionp;
			}
		}
		BU_ASSERT_PTR( leafp, ==, leafp_end );
		goto finish;
	}

	/*
	 *  Bring the RPN expression back from the disk,
	 *  populating leaves and matricies in the order they are encountered.
	 */
	if( max_stack_depth > MAX_V5_STACK )  {
		bu_log("Combination needs stack depth %d, only have %d, aborted\n",
			max_stack_depth, MAX_V5_STACK);
		return -1;
	}
	sp = &stack[0];

	for( i=0; i < rpn_len; i++,exprp++ )  {
		union tree	*tp;
		long		mi;

		RT_GET_TREE( tp, resp );
		tp->tr_b.magic = RT_TREE_MAGIC;

		switch( *exprp )  {
		case DB5COMB_TOKEN_LEAF:
			tp->tr_l.tl_op = OP_DB_LEAF;
			tp->tr_l.tl_name = bu_strdup( (const char *)leafp );
			leafp += strlen( (const char *)leafp) + 1;

			/* Get matrix index */
			mi = 4095;			/* sanity */
			leafp += db5_decode_signed( &mi, leafp, wid );

			if( mi < 0 )  {
				/* Signal identity matrix */
				tp->tr_l.tl_mat = NULL;
			} else {
				/* Unpack indicated matrix mi */
				BU_ASSERT_LONG( mi, <, nmat );
				tp->tr_l.tl_mat = (matp_t)bu_malloc(
					sizeof(mat_t), "v5comb mat");
				ntohd( (unsigned char *)tp->tr_l.tl_mat,
					&matp[mi*ELEMENTS_PER_MAT*SIZEOF_NETWORK_DOUBLE],
					ELEMENTS_PER_MAT);
			}
			break;

		case DB5COMB_TOKEN_UNION:
		case DB5COMB_TOKEN_INTERSECT:
		case DB5COMB_TOKEN_SUBTRACT:
		case DB5COMB_TOKEN_XOR:
			/* These are all binary operators */
			tp->tr_b.tb_regionp = REGION_NULL;
			tp->tr_b.tb_right = *--sp;
			RT_CK_TREE(tp->tr_b.tb_right);
			tp->tr_b.tb_left = *--sp;
			RT_CK_TREE(tp->tr_b.tb_left);
			switch( *exprp )  {
			case DB5COMB_TOKEN_UNION:
				tp->tr_b.tb_op = OP_UNION;
				break;
			case DB5COMB_TOKEN_INTERSECT:
				tp->tr_b.tb_op = OP_INTERSECT;
				break;
			case DB5COMB_TOKEN_SUBTRACT:
				tp->tr_b.tb_op = OP_SUBTRACT;
				break;
			case DB5COMB_TOKEN_XOR:
				tp->tr_b.tb_op = OP_XOR;
				break;
			}
			break;

		case DB5COMB_TOKEN_NOT:
			/* This is a unary operator */
			tp->tr_b.tb_regionp = REGION_NULL;
			tp->tr_b.tb_left = *--sp;
			RT_CK_TREE(tp->tr_b.tb_left);
			tp->tr_b.tb_right = TREE_NULL;
			tp->tr_b.tb_op = OP_NOT;
			break;
		default:
			bu_log("rt_comb_import5() unknown RPN expression token=%d, import aborted\n", *exprp);
			return -1;
		}

		/* Push this node on the stack */
		*sp++ = tp;
	}
	BU_ASSERT_PTR( leafp, ==, leafp_end );

	/* There should only be one thing left on the stack, the result */
	BU_ASSERT_PTR( sp, ==, &stack[1] );

	comb->tree = stack[0];
	RT_CK_TREE(comb->tree);

finish:
	if( ip->idb_avs.magic != BU_AVS_MAGIC )  return 0;	/* OK */

	/* Unpack the attributes */
	if( (ap = bu_avs_get( &ip->idb_avs, "rgb" )) != NULL )  {
		int	ibuf[3];
		if( sscanf( ap, "%d/%d/%d", ibuf, ibuf+1, ibuf+2 ) == 3 )  {
			VMOVE( comb->rgb, ibuf );
			comb->rgb_valid = 1;
		} else {
			bu_log("unable to parse 'rgb' attribute '%s'\n", ap);
		}
	}
	if( (ap = bu_avs_get( &ip->idb_avs, "inherit" )) != NULL ) {
		comb->inherit = atoi( ap );
	}
	if( (ap = bu_avs_get( &ip->idb_avs, "region" )) != NULL )  {
		int	ibuf[1];
		if( sscanf( ap, "%d", ibuf ) == 1 )  {
			/* Presence of this attribute implies it is a region  */
			comb->region_flag = 1;
			/* Value of this parameter is the FASTGEN code */
			comb->is_fastgen = ibuf[0];

			/* get the other "region" attributes */
			if( (ap = bu_avs_get( &ip->idb_avs, "region_id" )) != NULL )  {
				comb->region_id = atoi( ap );
			}
			if( (ap = bu_avs_get( &ip->idb_avs, "aircode" )) != NULL )  {
				comb->aircode = atoi( ap );
			}
			if( (ap = bu_avs_get( &ip->idb_avs, "giftmater" )) != NULL )  {
				comb->GIFTmater = atoi( ap );
				bu_vls_printf( &comb->material, "gift%d", comb->GIFTmater );
			}
			if( (ap = bu_avs_get( &ip->idb_avs, "los" )) != NULL )  {
				comb->los = atoi( ap );
			}
		} else {
			bu_log("unable to parse 'region' attribute '%s'\n", ap);
		}
	}
	if( (ap = bu_avs_get( &ip->idb_avs, "oshader" )) != NULL )  {
		bu_vls_strcat( &comb->shader, ap );
	}

	return 0;			/* OK */
}
