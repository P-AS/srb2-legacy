// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_fps.h
/// \brief Uncapped framerate stuff.

#include "r_fps.h"

#include "r_main.h"
#include "g_game.h"
#include "i_video.h"
#include "r_plane.h"
#include "p_spec.h"
#include "r_state.h"
#include "p_polyobj.h"
#include "z_zone.h"
#include "d_net.h" //MAXSPLITSCREENPLAYERS
#include "hardware/hw_main.h" //cv_glshearing


static CV_PossibleValue_t fpscap_cons_t[] = {
#ifdef DEVELOP
	// Lower values are actually pretty useful for debugging interp problems!
	{1, "MIN"},
#else
	{TICRATE, "MIN"},
#endif
	{300, "MAX"},
	{-1, "Unlimited"},
	{0, "Match refresh rate"},
	{0, NULL}
};

consvar_t cv_fpscap = CVAR_INIT ("fpscap", "Match refresh rate",  "Limit framerate to value specified", CV_SAVE, fpscap_cons_t, NULL);

ps_metric_t ps_interp_frac = {0};
ps_metric_t ps_interp_lag = {0};


UINT32 R_GetFramerateCap(void)
{

	if (rendermode == render_none)
	{
		// If we're not rendering (dedicated server),
		// we shouldn't be using any interpolation.
		return TICRATE;
	}

	if (cv_fpscap.value == 0)
	{
		// 0: Match refresh rate
		return I_GetRefreshRate();
	}

	if (cv_fpscap.value < 0)
	{
		// -1: Unlimited
		return 0;
	}

	return cv_fpscap.value;
}


boolean R_UsingFrameInterpolation(void)
{

	if (rendermode == render_none)
	{
		// If we're not rendering (dedicated server),
		// we shouldn't be using any interpolation.
		return TICRATE;
	}

	return (R_GetFramerateCap() != TICRATE || cv_timescale.value < FRACUNIT);
}

static viewvars_t p1view_old;
static viewvars_t p1view_new;
static viewvars_t p2view_old;
static viewvars_t p2view_new;
static viewvars_t sky1view_old;
static viewvars_t sky1view_new;
static viewvars_t sky2view_old;
static viewvars_t sky2view_new;

viewvars_t *oldview = &p1view_old;
viewvars_t *newview = &p1view_new;

enum viewcontext_e viewcontext = VIEWCONTEXT_PLAYER1;

static levelinterpolator_t **levelinterpolators;
static size_t levelinterpolators_len;
static size_t levelinterpolators_size;


static int oldview_invalid[MAXSPLITSCREENPLAYERS] = {0, 0};

static mobj_t **interpolated_mobjs = NULL;
static size_t interpolated_mobjs_len = 0;
static size_t interpolated_mobjs_capacity = 0;



static fixed_t R_LerpFixed(fixed_t from, fixed_t to, fixed_t frac)
{
	return from + FixedMul(frac, to - from);
}

static angle_t R_LerpAngle(angle_t from, angle_t to, fixed_t frac)
{
	return from + FixedMul(frac, to - from);
}

/*static vector2_t *R_LerpVector2(const vector2_t *from, const vector2_t *to, fixed_t frac, vector2_t *out)
{
	FV2_SubEx(to, from, out);
	FV2_MulEx(out, frac, out);
	FV2_AddEx(from, out, out);
	return out;
}

static vector3_t *R_LerpVector3(const vector3_t *from, const vector3_t *to, fixed_t frac, vector3_t *out)
{
	FV3_SubEx(to, from, out);
	FV3_MulEx(out, frac, out);
	FV3_AddEx(from, out, out);
	return out;
}*/

// recalc necessary stuff for mouseaiming
// slopes are already calculated for the full possible view (which is 4*viewheight).
// 18/08/18: (No it's actually 16*viewheight, thanks Jimita for finding this out)
static void R_SetupFreelook(player_t *player, boolean skybox)
{
#ifndef HWRENDER
	(void)player;
	(void)skybox;
#endif

	// clip it in the case we are looking a hardware 90 degrees full aiming
	// (lmps, network and use F12...)
	if (rendermode == render_soft
#ifdef HWRENDER
		|| (rendermode == render_opengl
			&& (cv_glshearing.value == 1
			|| (cv_glshearing.value == 2 && R_IsViewpointThirdPerson(player, skybox))))
#endif
		)
	{
		G_SoftwareClipAimingPitch((INT32 *)&aimingangle);
	}

	centeryfrac = (viewheight/2)<<FRACBITS;

	if (rendermode == render_soft)
		centeryfrac += FixedMul(AIMINGTODY(aimingangle), FixedDiv(viewwidth<<FRACBITS, BASEVIDWIDTH<<FRACBITS));

	centery = FixedInt(FixedRound(centeryfrac));

	if (rendermode == render_soft)
		yslope = &yslopetab[viewheight*8 - centery];
}

#undef AIMINGTODY

void R_InterpolateView(fixed_t frac)
{
	viewvars_t* prevview = oldview;
	boolean skybox = 0;
	UINT8 i;

	if (FIXED_TO_FLOAT(frac) < 0)
		frac = 0;
	if (frac > FRACUNIT)
		frac = FRACUNIT;

	if (viewcontext == VIEWCONTEXT_SKY1 || viewcontext == VIEWCONTEXT_PLAYER1)
	{
		i = 0;
	}
	else
	{
		i = 1;
	}

	if (oldview_invalid[i] != 0)
	{
		// interpolate from newview to newview
		prevview = newview;
	}

	viewx = R_LerpFixed(prevview->x, newview->x, frac);
	viewy = R_LerpFixed(prevview->y, newview->y, frac);
	viewz = R_LerpFixed(prevview->z, newview->z, frac);

	viewangle = R_LerpAngle(prevview->angle, newview->angle, frac);
	aimingangle = R_LerpAngle(prevview->aim, newview->aim, frac);

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	// this is gonna create some interesting visual errors for long distance teleports...
	// might want to recalculate the view sector every frame instead...
	viewplayer = newview->player;
	viewsector = R_PointInSubsectorFast(viewx, viewy)->sector;
	viewsky = newview->sky;

	// well, this ain't pretty
	if (newview == &sky1view_new || newview == &sky2view_new)
	{
		skybox = 1;
	}

	R_SetupFreelook(newview->player, skybox);
}

void R_UpdateViewInterpolation(void)
{
	p1view_old = p1view_new;
	p2view_old = p2view_new;
	sky1view_old = sky1view_new;
	sky2view_old = sky2view_new;
	if (oldview_invalid[0] > 0) oldview_invalid[0]--;
	if (oldview_invalid[1] > 0) oldview_invalid[1]--;
}


void R_ResetViewInterpolation(UINT8 p)
{
	if (p == 0)
	{
		UINT8 i;
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			oldview_invalid[i]++;
		}
	}
	else
	{
		oldview_invalid[p - 1]++;
	}
}


void R_SetViewContext(enum viewcontext_e _viewcontext)
{
	I_Assert(_viewcontext == VIEWCONTEXT_PLAYER1
			|| _viewcontext == VIEWCONTEXT_PLAYER2
			|| _viewcontext == VIEWCONTEXT_SKY1
			|| _viewcontext == VIEWCONTEXT_SKY2);
	viewcontext = _viewcontext;

	switch (viewcontext)
	{
		case VIEWCONTEXT_PLAYER1:
			oldview = &p1view_old;
			newview = &p1view_new;
			break;
		case VIEWCONTEXT_PLAYER2:
			oldview = &p2view_old;
			newview = &p2view_new;
			break;
		case VIEWCONTEXT_SKY1:
			oldview = &sky1view_old;
			newview = &sky1view_new;
			break;
		case VIEWCONTEXT_SKY2:
			oldview = &sky2view_old;
			newview = &sky2view_new;
			break;
		default:
			I_Error("viewcontext value is invalid: we should never get here without an assert!!");
			break;
	}
}

void R_InterpolateMobjState(mobj_t *mobj, fixed_t frac, interpmobjstate_t *out)
{
	if (frac == FRACUNIT)
	{
		out->x = mobj->x;
		out->y = mobj->y;
		out->z = mobj->z;
		out->scale = mobj->scale;
		out->angle = mobj->angle;
		return;
	}
	out->x =  R_LerpFixed(mobj->old_x, mobj->x, frac);
	out->y =  R_LerpFixed(mobj->old_y, mobj->y, frac);
	out->z =  R_LerpFixed(mobj->old_z, mobj->z, frac);
	out->angle = mobj->resetinterp ? mobj->angle : R_LerpAngle(mobj->old_angle, mobj->angle, frac);
	if (mobj->scale == mobj->old_scale) // Tiny optimisation - scale is usually unchanging, so let's skip a lerp
		out->scale = mobj->scale;
	else
		out->scale = R_LerpFixed(mobj->old_scale, mobj->scale, frac);
}

void R_InterpolatePrecipMobjState(precipmobj_t *mobj, fixed_t frac, interpmobjstate_t *out)
{
	(void)frac;

	out->x =  mobj->x;
	out->y =  mobj->y;
	out->z =  mobj->z;
	out->angle = mobj->angle;
}

// NOTE: This will NOT check that the mobj has already been added, for perf
// reasons.
void R_AddMobjInterpolator(mobj_t *mobj)
{
	if (interpolated_mobjs_len >= interpolated_mobjs_capacity)
	{
		if (interpolated_mobjs_capacity == 0)
		{
			interpolated_mobjs_capacity = 256;
		}
		else
		{
			interpolated_mobjs_capacity *= 2;
		}

		interpolated_mobjs = Z_Realloc(
			interpolated_mobjs,
			sizeof(mobj_t *) * interpolated_mobjs_capacity,
			PU_LEVEL,
			NULL
		);
	}

	interpolated_mobjs[interpolated_mobjs_len] = mobj;
	interpolated_mobjs_len += 1;

	R_ResetMobjInterpolationState(mobj);
}

void R_RemoveMobjInterpolator(mobj_t *mobj)
{
	size_t i;

	if (interpolated_mobjs_len == 0) return;

	for (i = 0; i < interpolated_mobjs_len; i++)
	{
		if (interpolated_mobjs[i] == mobj)
		{
			interpolated_mobjs[i] = interpolated_mobjs[
				interpolated_mobjs_len - 1
			];
			interpolated_mobjs_len -= 1;
			return;
		}
	}
}

void R_InitMobjInterpolators(void)
{
	interpolated_mobjs = NULL;
	interpolated_mobjs_len = 0;
	interpolated_mobjs_capacity = 0;
}

void R_UpdateMobjInterpolators(void)
{
	size_t i;
	for (i = 0; i < interpolated_mobjs_len; i++)
	{
		mobj_t *mobj = interpolated_mobjs[i];
		if (!P_MobjWasRemoved(mobj))
			R_ResetMobjInterpolationState(mobj);
	}
}

//
// R_ResetMobjInterpolationState
//
// Reset the rendering interpolation state of the mobj.
//
void R_ResetMobjInterpolationState(mobj_t *mobj)
{
	mobj->old_x2 = mobj->old_x;
	mobj->old_y2 = mobj->old_y;
	mobj->old_z2 = mobj->old_z;
	mobj->old_angle2 = mobj->old_angle;
	mobj->old_scale2 = mobj->old_scale;
	mobj->old_x = mobj->x;
	mobj->old_y = mobj->y;
	mobj->old_z = mobj->z;
	mobj->old_angle = mobj->angle;
	mobj->old_scale = mobj->scale;
	mobj->resetinterp = false;
}

//
// R_ResetPrecipitationMobjInterpolationState
//
// Reset the rendering interpolation state of the precipmobj.
//
void R_ResetPrecipitationMobjInterpolationState(precipmobj_t *mobj)
{
	mobj->old_x2 = mobj->old_x;
	mobj->old_y2 = mobj->old_y;
	mobj->old_z2 = mobj->old_z;
	mobj->old_angle2 = mobj->old_angle;
	mobj->old_x = mobj->x;
	mobj->old_y = mobj->y;
	mobj->old_z = mobj->z;
	mobj->old_angle = mobj->angle;
}

static void AddInterpolator(levelinterpolator_t* interpolator)
{
	if (levelinterpolators_len >= levelinterpolators_size)
	{
		if (levelinterpolators_size == 0)
		{
			levelinterpolators_size = 128;
		}
		else
		{
			levelinterpolators_size *= 2;
		}

		levelinterpolators = Z_Realloc(
			(void*) levelinterpolators,
			sizeof(levelinterpolator_t*) * levelinterpolators_size,
			PU_LEVEL,
			NULL
		);
	}
	levelinterpolators[levelinterpolators_len] = interpolator;
	levelinterpolators_len += 1;
}

static levelinterpolator_t *CreateInterpolator(levelinterpolator_type_e type, thinker_t *thinker)
{
	levelinterpolator_t *ret = (levelinterpolator_t*) Z_Calloc(
		sizeof(levelinterpolator_t), PU_LEVEL, NULL);
	ret->type = type;
	ret->thinker = thinker;
	AddInterpolator(ret);
	return ret;
}

void R_CreateInterpolator_SectorPlane(thinker_t *thinker, sector_t *sector, boolean ceiling)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_SectorPlane, thinker);
	interp->sectorplane.sector = sector;
	interp->sectorplane.ceiling = ceiling;
	if (ceiling)
	{
		interp->sectorplane.oldheight = interp->sectorplane.bakheight = sector->ceilingheight;
	}
	else
	{
		interp->sectorplane.oldheight = interp->sectorplane.bakheight = sector->floorheight;
	}
}

void R_CreateInterpolator_SectorScroll(thinker_t *thinker, sector_t *sector, boolean ceiling)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_SectorScroll, thinker);
	interp->sectorscroll.sector = sector;
	interp->sectorscroll.ceiling = ceiling;
	if (ceiling)
	{
		interp->sectorscroll.oldxoffs = interp->sectorscroll.bakxoffs = sector->ceiling_xoffs;
		interp->sectorscroll.oldyoffs = interp->sectorscroll.bakyoffs = sector->ceiling_yoffs;
	}
	else
	{
		interp->sectorscroll.oldxoffs = interp->sectorscroll.bakxoffs = sector->floor_xoffs;
		interp->sectorscroll.oldyoffs = interp->sectorscroll.bakyoffs = sector->floor_yoffs;
	}
}


void R_CreateInterpolator_SideScroll(thinker_t *thinker, side_t *side)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_SideScroll, thinker);
	interp->sidescroll.side = side;
	interp->sidescroll.oldtextureoffset = interp->sidescroll.baktextureoffset = side->textureoffset;
	interp->sidescroll.oldrowoffset = interp->sidescroll.bakrowoffset = side->rowoffset;
}

void R_CreateInterpolator_Polyobj(thinker_t *thinker, polyobj_t *polyobj)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_Polyobj, thinker);
	interp->polyobj.polyobj = polyobj;
	interp->polyobj.vertices_size = polyobj->numVertices;

	interp->polyobj.oldvertices = Z_CallocAlign(sizeof(fixed_t) * 2 * polyobj->numVertices, PU_LEVEL, NULL, 32);
	interp->polyobj.bakvertices = Z_CallocAlign(sizeof(fixed_t) * 2 * polyobj->numVertices, PU_LEVEL, NULL, 32);
	for (size_t i = 0; i < polyobj->numVertices; i++)
	{
		interp->polyobj.oldvertices[i * 2    ] = interp->polyobj.bakvertices[i * 2    ] = polyobj->vertices[i]->x;
		interp->polyobj.oldvertices[i * 2 + 1] = interp->polyobj.bakvertices[i * 2 + 1] = polyobj->vertices[i]->y;
	}

	interp->polyobj.oldcx = interp->polyobj.bakcx = polyobj->centerPt.x;
	interp->polyobj.oldcy = interp->polyobj.bakcy = polyobj->centerPt.y;
}

/*void R_CreateInterpolator_DynSlope(thinker_t *thinker, pslope_t *slope)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_DynSlope, thinker);
	interp->dynslope.slope = slope;

	FV3_Copy(&interp->dynslope.oldo, &slope->o);
	FV3_Copy(&interp->dynslope.bako, &slope->o);

	FV2_Copy(&interp->dynslope.oldd, &slope->d);
	FV2_Copy(&interp->dynslope.bakd, &slope->d);

	interp->dynslope.oldzdelta = interp->dynslope.bakzdelta = slope->zdelta;
}*/


void R_InitializeLevelInterpolators(void)
{
	levelinterpolators_len = 0;
	levelinterpolators_size = 0;
	levelinterpolators = NULL;
}

static void UpdateLevelInterpolatorState(levelinterpolator_t *interp)
{
	size_t i;

	switch (interp->type)
	{
	case LVLINTERP_SectorPlane:
		interp->sectorplane.oldheight = interp->sectorplane.bakheight;
		interp->sectorplane.bakheight = interp->sectorplane.ceiling ? interp->sectorplane.sector->ceilingheight : interp->sectorplane.sector->floorheight;
		break;

	case LVLINTERP_SectorScroll:
		interp->sectorscroll.oldxoffs = interp->sectorscroll.bakxoffs;
		interp->sectorscroll.bakxoffs = interp->sectorscroll.ceiling ? interp->sectorscroll.sector->ceiling_xoffs : interp->sectorscroll.sector->floor_xoffs;
		interp->sectorscroll.oldyoffs = interp->sectorscroll.bakyoffs;
		interp->sectorscroll.bakyoffs = interp->sectorscroll.ceiling ? interp->sectorscroll.sector->ceiling_yoffs : interp->sectorscroll.sector->floor_yoffs;
		break;
	case LVLINTERP_SideScroll:
		interp->sidescroll.oldtextureoffset = interp->sidescroll.baktextureoffset;
		interp->sidescroll.baktextureoffset = interp->sidescroll.side->textureoffset;
		interp->sidescroll.oldrowoffset = interp->sidescroll.bakrowoffset;
		interp->sidescroll.bakrowoffset = interp->sidescroll.side->rowoffset;
		break;
	case LVLINTERP_Polyobj:
		for (i = 0; i < interp->polyobj.vertices_size; i++)
		{
			interp->polyobj.oldvertices[i * 2    ] = interp->polyobj.bakvertices[i * 2    ];
			interp->polyobj.oldvertices[i * 2 + 1] = interp->polyobj.bakvertices[i * 2 + 1];
			interp->polyobj.bakvertices[i * 2    ] = interp->polyobj.polyobj->vertices[i]->x;
			interp->polyobj.bakvertices[i * 2 + 1] = interp->polyobj.polyobj->vertices[i]->y;
		}
		interp->polyobj.oldcx = interp->polyobj.bakcx;
		interp->polyobj.oldcy = interp->polyobj.bakcy;
		interp->polyobj.bakcx = interp->polyobj.polyobj->centerPt.x;
		interp->polyobj.bakcy = interp->polyobj.polyobj->centerPt.y;
		break;
    /*case LVLINTERP_DynSlope:
		FV3_Copy(&interp->dynslope.oldo, &interp->dynslope.bako);
		FV2_Copy(&interp->dynslope.oldd, &interp->dynslope.bakd);
		interp->dynslope.oldzdelta = interp->dynslope.bakzdelta;

		FV3_Copy(&interp->dynslope.bako, &interp->dynslope.slope->o);
		FV2_Copy(&interp->dynslope.bakd, &interp->dynslope.slope->d);
		interp->dynslope.bakzdelta = interp->dynslope.slope->zdelta;
		break;*/
	}
}

void R_UpdateLevelInterpolators(void)
{
	size_t i;
	for (i = 0; i < levelinterpolators_len; i++)
	{
		levelinterpolator_t *interp = levelinterpolators[i];

		UpdateLevelInterpolatorState(interp);
	}
}

void R_ClearLevelInterpolatorState(thinker_t *thinker)
{

	size_t i;
	for (i = 0; i < levelinterpolators_len; i++)
	{
		levelinterpolator_t *interp = levelinterpolators[i];

		if (interp->thinker == thinker)
		{
			// Do it twice to make the old state match the new
			UpdateLevelInterpolatorState(interp);
			UpdateLevelInterpolatorState(interp);
		}
	}
}

void R_ApplyLevelInterpolators(fixed_t frac)
{
	size_t i, ii;
	for (i = 0; i < levelinterpolators_len; i++)
	{
		levelinterpolator_t *interp = levelinterpolators[i];
		switch (interp->type)
		{
		case LVLINTERP_SectorPlane:
			if (interp->sectorplane.ceiling)
			{
				interp->sectorplane.sector->ceilingheight = R_LerpFixed(interp->sectorplane.oldheight, interp->sectorplane.bakheight, frac);
			}
			else
			{
				interp->sectorplane.sector->floorheight = R_LerpFixed(interp->sectorplane.oldheight, interp->sectorplane.bakheight, frac);
			}
			break;

		case LVLINTERP_SectorScroll:
			if (interp->sectorscroll.ceiling)
			{
				interp->sectorscroll.sector->ceiling_xoffs = R_LerpFixed(interp->sectorscroll.oldxoffs, interp->sectorscroll.bakxoffs, frac);
				interp->sectorscroll.sector->ceiling_yoffs = R_LerpFixed(interp->sectorscroll.oldyoffs, interp->sectorscroll.bakyoffs, frac);
			}
			else
			{
				interp->sectorscroll.sector->floor_xoffs = R_LerpFixed(interp->sectorscroll.oldxoffs, interp->sectorscroll.bakxoffs, frac);
				interp->sectorscroll.sector->floor_yoffs = R_LerpFixed(interp->sectorscroll.oldyoffs, interp->sectorscroll.bakyoffs, frac);
			}
			break;

        case LVLINTERP_SideScroll:
			interp->sidescroll.side->textureoffset = R_LerpFixed(interp->sidescroll.oldtextureoffset, interp->sidescroll.baktextureoffset, frac);
			interp->sidescroll.side->rowoffset = R_LerpFixed(interp->sidescroll.oldrowoffset, interp->sidescroll.bakrowoffset, frac);
			break;

		case LVLINTERP_Polyobj:
			for (ii = 0; ii < interp->polyobj.vertices_size; ii++)
			{
				interp->polyobj.polyobj->vertices[ii]->x = R_LerpFixed(interp->polyobj.oldvertices[ii * 2    ], interp->polyobj.bakvertices[ii * 2    ], frac);
				interp->polyobj.polyobj->vertices[ii]->y = R_LerpFixed(interp->polyobj.oldvertices[ii * 2 + 1], interp->polyobj.bakvertices[ii * 2 + 1], frac);
			}
			interp->polyobj.polyobj->centerPt.x = R_LerpFixed(interp->polyobj.oldcx, interp->polyobj.bakcx, frac);
			interp->polyobj.polyobj->centerPt.y = R_LerpFixed(interp->polyobj.oldcy, interp->polyobj.bakcy, frac);
			break;
        /*case LVLINTERP_DynSlope:
			R_LerpVector3(&interp->dynslope.oldo, &interp->dynslope.bako, frac, &interp->dynslope.slope->o);
			R_LerpVector2(&interp->dynslope.oldd, &interp->dynslope.bakd, frac, &interp->dynslope.slope->d);
			interp->dynslope.slope->zdelta = R_LerpFixed(interp->dynslope.oldzdelta, interp->dynslope.bakzdelta, frac);
			break;*/

		}
	}
}

void R_RestoreLevelInterpolators(void)
{
	size_t i, ii;;
	for (i = 0; i < levelinterpolators_len; i++)
	{
		levelinterpolator_t *interp = levelinterpolators[i];

		switch (interp->type)
		{
		case LVLINTERP_SectorPlane:
			if (interp->sectorplane.ceiling)
			{
				interp->sectorplane.sector->ceilingheight = interp->sectorplane.bakheight;
			}
			else
			{
				interp->sectorplane.sector->floorheight = interp->sectorplane.bakheight;
			}
			break;
		case LVLINTERP_SectorScroll:
			if (interp->sectorscroll.ceiling)
			{
				interp->sectorscroll.sector->ceiling_xoffs = interp->sectorscroll.bakxoffs;
				interp->sectorscroll.sector->ceiling_yoffs = interp->sectorscroll.bakyoffs;
			}
			else
			{
				interp->sectorscroll.sector->floor_xoffs = interp->sectorscroll.bakxoffs;
				interp->sectorscroll.sector->floor_yoffs = interp->sectorscroll.bakyoffs;
			}
			break;
		case LVLINTERP_SideScroll:
			interp->sidescroll.side->textureoffset = interp->sidescroll.baktextureoffset;
			interp->sidescroll.side->rowoffset = interp->sidescroll.bakrowoffset;
			break;
        case LVLINTERP_Polyobj:
			for (ii = 0; ii < interp->polyobj.vertices_size; ii++)
			{
				interp->polyobj.polyobj->vertices[ii]->x = interp->polyobj.bakvertices[ii * 2    ];
				interp->polyobj.polyobj->vertices[ii]->y = interp->polyobj.bakvertices[ii * 2 + 1];
			}
			interp->polyobj.polyobj->centerPt.x = interp->polyobj.bakcx;
			interp->polyobj.polyobj->centerPt.y = interp->polyobj.bakcy;
			break;
        /*case LVLINTERP_DynSlope:
			FV3_Copy(&interp->dynslope.slope->o, &interp->dynslope.bako);
			FV2_Copy(&interp->dynslope.slope->d, &interp->dynslope.bakd);
			interp->dynslope.slope->zdelta = interp->dynslope.bakzdelta;
			break;*/
		}
	}
}


void R_DestroyLevelInterpolators(thinker_t *thinker)
{
	size_t i;

	for (i = 0; i < levelinterpolators_len; i++)
	{
		levelinterpolator_t *interp = levelinterpolators[i];

		if (interp->thinker == thinker)
		{
			// Swap the tail of the level interpolators to this spot
			levelinterpolators[i] = levelinterpolators[levelinterpolators_len - 1];
			levelinterpolators_len -= 1;

			Z_Free(interp);
			i -= 1;
		}
	}
}
