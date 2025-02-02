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

#ifndef __R_FPS_H__
#define __R_FPS_H__

#include "m_fixed.h"
#include "p_local.h"
#include "r_state.h"

extern consvar_t cv_fpscap;

UINT32 R_GetFramerateCap(void);
boolean R_UsingFrameInterpolation(void);


enum viewcontext_e
{
	VIEWCONTEXT_PLAYER1 = 0,
	VIEWCONTEXT_PLAYER2,
	VIEWCONTEXT_SKY1,
	VIEWCONTEXT_SKY2
};

typedef struct {
	fixed_t x;
	fixed_t y;
	fixed_t z;
	boolean sky;
	sector_t *sector;
	player_t *player;

	angle_t angle;
	angle_t aim;
} viewvars_t;



typedef struct {
	fixed_t x;
	fixed_t y;
	fixed_t z;
	angle_t angle;
	fixed_t scale;
} interpmobjstate_t;

// Level interpolators
// The union tag for levelinterpolator_t
typedef enum {
	LVLINTERP_SectorPlane,
	LVLINTERP_SectorScroll,
	LVLINTERP_SideScroll,
	LVLINTERP_Polyobj,
	LVLINTERP_DynSlope,
} levelinterpolator_type_e;
// Tagged union of a level interpolator
typedef struct levelinterpolator_s {
	levelinterpolator_type_e type;
	thinker_t *thinker;
	union {
		struct {
			sector_t *sector;
			fixed_t oldheight;
			fixed_t bakheight;
			boolean ceiling;
		} sectorplane;
		struct {
			sector_t *sector;
			fixed_t oldxoffs, oldyoffs, bakxoffs, bakyoffs;
			boolean ceiling;
		} sectorscroll;

		struct {
			side_t *side;
			fixed_t oldtextureoffset, oldrowoffset, baktextureoffset, bakrowoffset;
		} sidescroll;
        struct {
			polyobj_t *polyobj;
			fixed_t *oldvertices;
			fixed_t *bakvertices;
			size_t vertices_size;
			fixed_t oldcx, oldcy, bakcx, bakcy;
		} polyobj;
        struct {
			pslope_t *slope;
			vector3_t oldo, bako;
			vector2_t oldd, bakd;
			fixed_t oldzdelta, bakzdelta;
		} dynslope;
	};
} levelinterpolator_t;


//extern viewvars_t oldview;
extern viewvars_t *newview;

void R_InterpolateView(player_t *player, boolean skybox, fixed_t frac);
void R_UpdateViewInterpolation(void);
void R_SetViewContext(enum viewcontext_e _viewcontext);
void R_ResetViewInterpolation(UINT8 p);
void R_InterpolateMobjState(mobj_t *mobj, fixed_t frac, interpmobjstate_t *out);
void R_InterpolatePrecipMobjState(precipmobj_t *mobj, fixed_t frac, interpmobjstate_t *out);

void R_CreateInterpolator_SectorPlane(thinker_t *thinker, sector_t *sector, boolean ceiling);

void R_CreateInterpolator_SectorScroll(thinker_t *thinker, sector_t *sector, boolean ceiling);

void R_CreateInterpolator_SideScroll(thinker_t *thinker, side_t *side);

void R_CreateInterpolator_Polyobj(thinker_t *thinker, polyobj_t *polyobj);



// Initialize level interpolators after a level change
void R_InitializeLevelInterpolators(void);
// Update level interpolators, storing the previous and current states.
void R_UpdateLevelInterpolators(void);
// Clear states for all level interpolators for the thinker
void R_ClearLevelInterpolatorState(thinker_t *thinker);
// Apply level interpolators to the actual game state
void R_ApplyLevelInterpolators(fixed_t frac);
// Restore level interpolators to the real game state
void R_RestoreLevelInterpolators(void);
// Destroy interpolators associated with a thinker
void R_DestroyLevelInterpolators(thinker_t *thinker);


// Initialize internal mobj interpolator list (e.g. during level loading)
void R_InitMobjInterpolators(void);
// Add interpolation state for the given mobj
void R_AddMobjInterpolator(mobj_t *mobj);
// Remove the interpolation state for the given mobj
void R_RemoveMobjInterpolator(mobj_t *mobj);
void R_UpdateMobjInterpolators(void);
void R_ResetMobjInterpolationState(mobj_t *mobj);
void R_ResetPrecipitationMobjInterpolationState(precipmobj_t *mobj);


fixed_t R_LerpFixed(fixed_t from, fixed_t to, fixed_t frac);
angle_t R_LerpAngle(angle_t from, angle_t to, fixed_t frac);

#endif
