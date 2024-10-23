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
#ifdef POLYOBJECTS
#include "p_polyobj.h"
#endif

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

#define ISA(_THINKNAME_) th->function.acp1 == (actionf_p1)_THINKNAME_
#define CAST(_NAME_,_TYPE_) _TYPE_ *_NAME_ = (_TYPE_ *)th


enum viewcontext_e viewcontext = VIEWCONTEXT_PLAYER1;

// taken from r_main.c
// WARNING: a should be unsigned but to add with 2048, it isn't!
#define AIMINGTODY(a) ((FINETANGENT((2048+(((INT32)a)>>ANGLETOFINESHIFT)) & FINEMASK)*160)>>FRACBITS)

void R_InterpolateView(fixed_t frac)
{
	INT32 dy = 0;

	if (FIXED_TO_FLOAT(frac) < 0)
		frac = 0;

	viewx = oldview->x + R_LerpFixed(oldview->x, newview->x, frac);
	viewy = oldview->y + R_LerpFixed(oldview->y, newview->y, frac);
	viewz = oldview->z + R_LerpFixed(oldview->z, newview->z, frac);

	viewangle = oldview->angle + R_LerpAngle(oldview->angle, newview->angle, frac);
	aimingangle = oldview->aim + R_LerpAngle(oldview->aim, newview->aim, frac);

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	// this is gonna create some interesting visual errors for long distance teleports...
	// might want to recalculate the view sector every frame instead...
	if (frac >= FRACUNIT)
	{
		viewplayer = newview->player;
		viewsector = newview->sector;
		viewsky    = newview->sky;
	}
	else
	{
		viewplayer = oldview->player;
		viewsector = oldview->sector;
		viewsky    = oldview->sky;
	}

	if (rendermode == render_soft)
	{
		// clip it in the case we are looking a hardware 90 degrees full aiming
		// (lmps, network and use F12...)
		G_SoftwareClipAimingPitch((INT32 *)&aimingangle);

		dy = AIMINGTODY(aimingangle) * viewwidth/BASEVIDWIDTH;

		yslope = &yslopetab[viewheight*8 - (viewheight/2 + dy)];
	}
	centery = (viewheight/2) + dy;
	centeryfrac = centery<<FRACBITS;
}

void R_UpdateViewInterpolation(void)
{
	p1view_old = p1view_new;
	p2view_old = p2view_new;
	sky1view_old = sky1view_new;
	sky2view_old = sky2view_new;
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


void R_SetThinkerOldStates(void)
{
	thinker_t *th;

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th == NULL)
		{
			break;
		}
		if (ISA(P_MobjThinker))
		{
			CAST(mo, mobj_t);
			mo->old_x = mo->new_x;
			mo->old_y = mo->new_y;
			mo->old_z = mo->new_z;
		}
		if (ISA(P_RainThinker) || ISA(P_SnowThinker))
		{
			CAST(mo, precipmobj_t);
			mo->old_x = mo->new_x;
			mo->old_y = mo->new_y;
			mo->old_z = mo->new_z;
		}

		// Other thinkers
		if (ISA(T_MoveCeiling) || ISA(T_CrushCeiling))
		{
			CAST(s, ceiling_t);
			s->old_ceilingheight = s->new_ceilingheight;
		}
		if (ISA(T_MoveFloor))
		{
			CAST(s, floormove_t);
			s->old_floorheight = s->new_floorheight;
		}
		if (ISA(T_LightningFlash))
		{
			CAST(l, lightflash_t);
			l->old_lightlevel = l->new_lightlevel;
		}
		if (ISA(T_StrobeFlash))
		{
			CAST(s, strobe_t);
			s->old_lightlevel = s->new_lightlevel;
		}
		if (ISA(T_Glow))
		{
			CAST(g, glow_t);
			g->old_lightlevel = g->new_lightlevel;
		}
		if (ISA(T_FireFlicker))
		{
			CAST(f, fireflicker_t);
			f->old_lightlevel = f->new_lightlevel;
		}
		if (ISA(T_MoveElevator)
			|| ISA(T_CameraScanner)
			|| ISA(T_StartCrumble))
		{
			CAST(e, elevator_t);
			e->old_floorheight = e->new_floorheight;
			e->old_ceilingheight = e->new_ceilingheight;
		}
		if (ISA(T_ContinuousFalling)
			|| ISA(T_ThwompSector)
			|| ISA(T_NoEnemiesSector)
			|| ISA(T_EachTimeThinker)
			|| ISA(T_RaiseSector)
			|| ISA(T_BounceCheese)
			|| ISA(T_MarioBlock)
			|| ISA(T_SpikeSector)
			|| ISA(T_FloatSector)
			|| ISA(T_BridgeThinker))
		{
			CAST(l, levelspecthink_t);
			l->old_floorheight = l->new_floorheight;
			l->old_ceilingheight = l->new_ceilingheight;
		}
		if (ISA(T_LaserFlash))
		{
			//CAST(l, laserthink_t);
		}
		if (ISA(T_LightFade))
		{
			CAST(l, lightlevel_t);
			l->old_lightlevel = l->new_lightlevel;
		}
		if (ISA(T_ExecutorDelay))
		{
			//CAST(e, executor_t);
		}
		if (ISA(T_Disappear))
		{
			//CAST(d, disappear_t);
		}
#ifdef POLYOBJECTS
		if (ISA(T_PolyObjRotate))
		{
			//CAST(p, polyrotate_t);
		}
		if (ISA(T_PolyObjMove)
			|| ISA(T_PolyObjFlag))
		{
			//CAST(p, polymove_t);
		}
		if (ISA(T_PolyObjWaypoint))
		{
			//CAST(p, polywaypoint_t);
		}
		if (ISA(T_PolyDoorSlide))
		{
			//CAST(p, polyslidedoor_t);
		}
		if (ISA(T_PolyDoorSwing))
		{
			//CAST(p, polyswingdoor_t);
		}
#endif
		if (ISA(T_Scroll))
		{
			CAST(s, scroll_t);
			switch (s->type)
			{
				case sc_side:
					s->old_textureoffset = s->new_textureoffset;
					s->old_rowoffset = s->new_rowoffset;
					break;
				case sc_floor:
				case sc_ceiling:
					s->old_xoffs = s->new_xoffs;
					s->old_yoffs = s->new_yoffs;
					break;
				case sc_carry:
				case sc_carry_ceiling:
					break;
			}
		}
		if (ISA(T_Friction))
		{
			//CAST(f, friction_t);
		}
		if (ISA(T_Pusher))
		{
			//CAST(f, pusher_t);
		}
	}
}

void R_SetThinkerNewStates(void)
{
	thinker_t *th;

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th == NULL)
		{
			break;
		}
		if (ISA(P_MobjThinker))
		{
			CAST(mo, mobj_t);
			if (mo->firstlerp == 0)
			{
				mo->firstlerp = 1;
				mo->old_x = mo->x;
				mo->old_y = mo->y;
				mo->old_z = mo->z;
			}
			mo->new_x = mo->x;
			mo->new_y = mo->y;
			mo->new_z = mo->z;
		}
		if (ISA(P_RainThinker) || ISA(P_SnowThinker))
		{
			CAST(mo, precipmobj_t);
			if (mo->firstlerp == 0)
			{
				mo->firstlerp = 1;
				mo->old_x = mo->x;
				mo->old_y = mo->y;
				mo->old_z = mo->z;
			}
			mo->new_x = mo->x;
			mo->new_y = mo->y;
			mo->new_z = mo->z;
		}

		// Other thinkers
		if (ISA(T_MoveCeiling) || ISA(T_CrushCeiling))
		{
			CAST(s, ceiling_t);
			if (s->firstlerp != 1)
			{
				s->firstlerp = 1;
				s->old_ceilingheight = s->sector->ceilingheight;
			}
			s->new_ceilingheight = s->sector->ceilingheight;
		}
		if (ISA(T_MoveFloor))
		{
			CAST(s, floormove_t);
			if (s->firstlerp != 1)
			{
				s->firstlerp = 1;
				s->old_floorheight = s->sector->floorheight;
			}
			s->new_floorheight = s->sector->floorheight;
		}
		if (ISA(T_LightningFlash))
		{
			CAST(l, lightflash_t);
			if (l->firstlerp != 1)
			{
				l->firstlerp = 1;
				l->old_lightlevel = l->sector->lightlevel;
			}
			l->new_lightlevel = l->sector->lightlevel;
		}
		if (ISA(T_StrobeFlash))
		{
			CAST(s, strobe_t);
			if (s->firstlerp != 1)
			{
				s->firstlerp = 1;
				s->old_lightlevel = s->sector->lightlevel;
			}
			s->new_lightlevel = s->sector->lightlevel;
		}
		if (ISA(T_Glow))
		{
			CAST(g, glow_t);
			if (g->firstlerp != 1)
			{
				g->firstlerp = 1;
				g->old_lightlevel = g->sector->lightlevel;
			}
			g->new_lightlevel = g->sector->lightlevel;
		}
		if (ISA(T_FireFlicker))
		{
			CAST(f, fireflicker_t);
			if (f->firstlerp != 1)
			{
				f->firstlerp = 1;
				f->old_lightlevel = f->sector->lightlevel;
			}
			f->new_lightlevel = f->sector->lightlevel;
		}
		if (ISA(T_MoveElevator)
			|| ISA(T_CameraScanner)
			|| ISA(T_StartCrumble))
		{
			CAST(e, elevator_t);
			if (e->firstlerp != 1)
			{
				e->firstlerp = 1;
				e->old_floorheight = e->sector->floorheight;
				e->old_ceilingheight = e->sector->ceilingheight;
			}
			e->new_floorheight = e->sector->floorheight;
			e->new_ceilingheight = e->sector->ceilingheight;
		}
		if (ISA(T_ContinuousFalling)
			|| ISA(T_ThwompSector)
			|| ISA(T_NoEnemiesSector)
			|| ISA(T_EachTimeThinker)
			|| ISA(T_RaiseSector)
			|| ISA(T_BounceCheese)
			|| ISA(T_MarioBlock)
			|| ISA(T_SpikeSector)
			|| ISA(T_FloatSector)
			|| ISA(T_BridgeThinker))
		{
			CAST(l, levelspecthink_t);
			if (l->firstlerp != 1)
			{
				l->firstlerp = 1;
				l->old_floorheight = l->sector->floorheight;
				l->old_ceilingheight = l->sector->ceilingheight;
			}
			l->new_floorheight = l->sector->floorheight;
			l->new_ceilingheight = l->sector->ceilingheight;
		}
		if (ISA(T_LaserFlash))
		{
			//CAST(l, laserthink_t);
		}
		if (ISA(T_LightFade))
		{
			CAST(l, lightlevel_t);
			if (l->firstlerp != 1)
			{
				l->firstlerp = 1;
				l->old_lightlevel = l->sector->lightlevel;
			}
			l->new_lightlevel = l->sector->lightlevel;
		}
		if (ISA(T_ExecutorDelay))
		{
			//CAST(e, executor_t);
		}
		if (ISA(T_Disappear))
		{
			//CAST(d, disappear_t);
		}
#ifdef POLYOBJECTS
		if (ISA(T_PolyObjRotate))
		{
			//CAST(p, polyrotate_t);
		}
		if (ISA(T_PolyObjMove)
			|| ISA(T_PolyObjFlag))
		{
			//CAST(p, polymove_t);
		}
		if (ISA(T_PolyObjWaypoint))
		{
			//CAST(p, polywaypoint_t);
		}
		if (ISA(T_PolyDoorSlide))
		{
			//CAST(p, polyslidedoor_t);
		}
		if (ISA(T_PolyDoorSwing))
		{
			//CAST(p, polyswingdoor_t);
		}
#endif
		if (ISA(T_Scroll))
		{
			CAST(s, scroll_t);
			switch (s->type)
			{
				case sc_side:
				{
					side_t *side;
					side = sides + s->affectee;
					if (s->firstlerp != 1)
					{
						s->firstlerp = 1;
						s->old_textureoffset = side->textureoffset;
						s->old_rowoffset = side->rowoffset;
					}
					s->new_textureoffset = side->textureoffset;
					s->new_rowoffset = side->rowoffset;
					break;
				}
				case sc_floor:
				{
					sector_t *sec;
					sec = sectors + s->affectee;
					if (s->firstlerp != 1)
					{
						s->firstlerp = 1;
						s->old_xoffs = sec->floor_xoffs;
						s->old_yoffs = sec->floor_yoffs;
					}
					s->new_xoffs = sec->floor_xoffs;
					s->new_yoffs = sec->floor_yoffs;
					break;
				}
				case sc_ceiling:
				{
					sector_t *sec;
					sec = sectors + s->affectee;
					if (s->firstlerp != 1)
					{
						s->firstlerp = 1;
						s->old_xoffs = sec->ceiling_xoffs;
						s->old_yoffs = sec->ceiling_yoffs;
					}
					s->new_xoffs = sec->ceiling_xoffs;
					s->new_yoffs = sec->ceiling_yoffs;
					break;
				}
				case sc_carry:
				case sc_carry_ceiling:
					break;
			}
		}
		if (ISA(T_Friction))
		{
			//CAST(f, friction_t);
		}
		if (ISA(T_Pusher))
		{
			//CAST(f, pusher_t);
		}
	}
}

void R_DoThinkerLerp(fixed_t frac)
{
	thinker_t *th;

	if (cv_capframerate.value != 0)
	{
		return;
	}

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th == NULL)
		{
			break;
		}
		if (ISA(P_MobjThinker))
		{
			CAST(mo, mobj_t);
			if (mo->firstlerp < 1) continue;
			mo->x = mo->old_x + R_LerpFixed(mo->old_x, mo->new_x, frac);
			mo->y = mo->old_y + R_LerpFixed(mo->old_y, mo->new_y, frac);
			mo->z = mo->old_z + R_LerpFixed(mo->old_z, mo->new_z, frac);
		}
		if (ISA(P_RainThinker) || ISA(P_SnowThinker))
		{
			CAST(mo, precipmobj_t);
			if (mo->firstlerp < 1) continue;
			mo->x = R_LerpFixed(mo->old_x, mo->new_x, frac);
			mo->y = R_LerpFixed(mo->old_y, mo->new_y, frac);
			mo->z = R_LerpFixed(mo->old_z, mo->new_z, frac);
		}

		// Other thinkers
		if (ISA(T_MoveCeiling) || ISA(T_CrushCeiling))
		{
			CAST(s, ceiling_t);
			if (s->firstlerp != 1) continue;
			s->sector->ceilingheight = s->old_ceilingheight + R_LerpFixed(s->old_ceilingheight, s->new_ceilingheight, frac);
		}
		if (ISA(T_MoveFloor))
		{
			CAST(s, floormove_t);
			if (s->firstlerp != 1) continue;
			s->sector->floorheight = s->old_floorheight + R_LerpFixed(s->old_floorheight, s->new_floorheight, frac);
		}
		if (ISA(T_LightningFlash))
		{
			CAST(l, lightflash_t);
			if (l->firstlerp != 1) continue;
			l->sector->lightlevel = l->old_lightlevel + (INT16) R_LerpInt32(l->old_lightlevel, l->new_lightlevel, frac);
		}
		if (ISA(T_StrobeFlash))
		{
			CAST(s, strobe_t);
			if (s->firstlerp != 1) continue;
			s->sector->lightlevel = s->old_lightlevel + (INT16) R_LerpInt32(s->old_lightlevel, s->new_lightlevel, frac);
		}
		if (ISA(T_Glow))
		{
			CAST(g, glow_t);
			if (g->firstlerp != 1) continue;
			g->sector->lightlevel = g->old_lightlevel + (INT16) R_LerpInt32(g->old_lightlevel, g->new_lightlevel, frac);
		}
		if (ISA(T_FireFlicker))
		{
			CAST(f, fireflicker_t);
			if (f->firstlerp != 1) continue;
			f->sector->lightlevel = f->old_lightlevel + (INT16) R_LerpInt32(f->old_lightlevel, f->new_lightlevel, frac);
		}
		if (ISA(T_MoveElevator)
			|| ISA(T_CameraScanner)
			|| ISA(T_StartCrumble))
		{
			CAST(e, elevator_t);
			if (e->firstlerp != 1) continue;
			e->sector->ceilingheight = e->old_ceilingheight + R_LerpFixed(e->old_ceilingheight, e->new_ceilingheight, frac);
			e->sector->floorheight = e->old_floorheight + R_LerpFixed(e->old_floorheight, e->new_floorheight, frac);
		}
		if (ISA(T_ContinuousFalling)
			|| ISA(T_ThwompSector)
			|| ISA(T_NoEnemiesSector)
			|| ISA(T_EachTimeThinker)
			|| ISA(T_RaiseSector)
			|| ISA(T_BounceCheese)
			|| ISA(T_MarioBlock)
			|| ISA(T_SpikeSector)
			|| ISA(T_FloatSector)
			|| ISA(T_BridgeThinker))
		{
			CAST(l, levelspecthink_t);
			if (l->firstlerp != 1) continue;
			l->sector->ceilingheight = l->old_ceilingheight + R_LerpFixed(l->old_ceilingheight, l->new_ceilingheight, frac);
			l->sector->floorheight = l->old_floorheight + R_LerpFixed(l->old_floorheight, l->new_floorheight, frac);
		}
		if (ISA(T_LaserFlash))
		{
			//CAST(l, laserthink_t);
		}
		if (ISA(T_LightFade))
		{
			CAST(l, lightlevel_t);
			if (l->firstlerp != 1) continue;
			l->sector->lightlevel = l->old_lightlevel + (INT16) R_LerpInt32(l->old_lightlevel, l->new_lightlevel, frac);
		}
		if (ISA(T_ExecutorDelay))
		{
			//CAST(e, executor_t);
		}
		if (ISA(T_Disappear))
		{
			//CAST(d, disappear_t);
		}
#ifdef POLYOBJECTS
		if (ISA(T_PolyObjRotate))
		{
			//CAST(p, polyrotate_t);
		}
		if (ISA(T_PolyObjMove)
			|| ISA(T_PolyObjFlag))
		{
			//CAST(p, polymove_t);
		}
		if (ISA(T_PolyObjWaypoint))
		{
			//CAST(p, polywaypoint_t);
		}
		if (ISA(T_PolyDoorSlide))
		{
			//CAST(p, polyslidedoor_t);
		}
		if (ISA(T_PolyDoorSwing))
		{
			//CAST(p, polyswingdoor_t);
		}
#endif
		if (ISA(T_Scroll))
		{
			CAST(s, scroll_t);
			switch (s->type)
			{
				case sc_side:
				{
					side_t *side;
					side = sides + s->affectee;
					if (s->firstlerp != 1) break;
					side->textureoffset = s->old_textureoffset + R_LerpFixed(s->old_textureoffset, s->new_textureoffset, frac);
					side->rowoffset = s->old_rowoffset + R_LerpFixed(s->old_rowoffset, s->new_rowoffset, frac);
					break;
				}
				case sc_floor:
				{
					sector_t *sec;
					sec = sectors + s->affectee;
					if (s->firstlerp != 1) break;
					sec->floor_xoffs = s->old_xoffs + R_LerpFixed(s->old_xoffs, s->new_xoffs, frac);
					sec->floor_yoffs = s->old_yoffs + R_LerpFixed(s->old_yoffs, s->new_yoffs, frac);
					break;
				}
				case sc_ceiling:
				{
					sector_t *sec;
					sec = sectors + s->affectee;
					if (s->firstlerp != 1) break;
					sec->ceiling_xoffs = s->old_xoffs + R_LerpFixed(s->old_xoffs, s->new_xoffs, frac);
					sec->ceiling_yoffs = s->old_yoffs + R_LerpFixed(s->old_yoffs, s->new_yoffs, frac);
					break;
				}
				case sc_carry:
				case sc_carry_ceiling:
					break;
			}
		}
		if (ISA(T_Friction))
		{
			//CAST(f, friction_t);
		}
		if (ISA(T_Pusher))
		{
			//CAST(f, pusher_t);
		}
	}
}

void R_ResetThinkerLerp(void)
{
	thinker_t *th;

	if (cv_capframerate.value != 0)
	{
		return;
	}

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th == NULL)
		{
			break;
		}
		if (ISA(P_MobjThinker))
		{
			CAST(mo, mobj_t);
			if (mo->firstlerp < 1) continue;
			mo->x = mo->new_x;
			mo->y = mo->new_y;
			mo->z = mo->new_z;
		}
		if (ISA(P_RainThinker) || ISA(P_SnowThinker))
		{
			CAST(mo, precipmobj_t);
			if (mo->firstlerp < 1) continue;
			mo->x = mo->new_x;
			mo->y = mo->new_y;
			mo->z = mo->new_z;
		}

		// Other thinkers
		if (ISA(T_MoveCeiling) || ISA(T_CrushCeiling))
		{
			CAST(s, ceiling_t);
			if (s->firstlerp != 1) continue;
			s->sector->ceilingheight = s->new_ceilingheight;
		}
		if (ISA(T_MoveFloor))
		{
			CAST(s, floormove_t);
			if (s->firstlerp != 1) continue;
			s->sector->floorheight = s->new_floorheight;
		}
		if (ISA(T_LightningFlash))
		{
			CAST(l, lightflash_t);
			if (l->firstlerp != 1) continue;
			l->sector->lightlevel = l->new_lightlevel;
		}
		if (ISA(T_StrobeFlash))
		{
			CAST(s, strobe_t);
			if (s->firstlerp != 1) continue;
			s->sector->lightlevel = s->new_lightlevel;
		}
		if (ISA(T_Glow))
		{
			CAST(g, glow_t);
			if (g->firstlerp != 1) continue;
			g->sector->lightlevel = g->new_lightlevel;
		}
		if (ISA(T_FireFlicker))
		{
			CAST(f, fireflicker_t);
			if (f->firstlerp != 1) continue;
			f->sector->lightlevel = f->new_lightlevel;
		}
		if (ISA(T_MoveElevator)
			|| ISA(T_CameraScanner)
			|| ISA(T_StartCrumble))
		{
			CAST(e, elevator_t);
			if (e->firstlerp != 1) continue;
			e->sector->ceilingheight = e->new_ceilingheight;
			e->sector->floorheight = e->new_floorheight;
		}
		if (ISA(T_ContinuousFalling)
			|| ISA(T_ThwompSector)
			|| ISA(T_NoEnemiesSector)
			|| ISA(T_EachTimeThinker)
			|| ISA(T_RaiseSector)
			|| ISA(T_BounceCheese)
			|| ISA(T_MarioBlock)
			|| ISA(T_SpikeSector)
			|| ISA(T_FloatSector)
			|| ISA(T_BridgeThinker))
		{
			CAST(l, levelspecthink_t);
			if (l->firstlerp != 1) continue;
			l->sector->ceilingheight = l->new_ceilingheight;
			l->sector->floorheight = l->new_floorheight;
		}
		if (ISA(T_LaserFlash))
		{
			//CAST(l, laserthink_t);
		}
		if (ISA(T_LightFade))
		{
			CAST(l, lightlevel_t);
			if (l->firstlerp != 1) continue;
			l->sector->lightlevel = l->new_lightlevel;
		}
		if (ISA(T_ExecutorDelay))
		{
			//CAST(e, executor_t);
		}
		if (ISA(T_Disappear))
		{
			//CAST(d, disappear_t);
		}
#ifdef POLYOBJECTS
		if (ISA(T_PolyObjRotate))
		{
			//CAST(p, polyrotate_t);
		}
		if (ISA(T_PolyObjMove)
			|| ISA(T_PolyObjFlag))
		{
			//CAST(p, polymove_t);
		}
		if (ISA(T_PolyObjWaypoint))
		{
			//CAST(p, polywaypoint_t);
		}
		if (ISA(T_PolyDoorSlide))
		{
			//CAST(p, polyslidedoor_t);
		}
		if (ISA(T_PolyDoorSwing))
		{
			//CAST(p, polyswingdoor_t);
		}
#endif
		if (ISA(T_Scroll))
		{
			CAST(s, scroll_t);
			switch (s->type)
			{
				case sc_side:
				{
					side_t *side;
					side = sides + s->affectee;
					if (s->firstlerp != 1) break;
					side->textureoffset = s->new_textureoffset;
					side->rowoffset = s->new_rowoffset;
					break;
				}
				case sc_floor:
				{
					sector_t *sec;
					sec = sectors + s->affectee;
					if (s->firstlerp != 1) break;
					sec->floor_xoffs = s->new_xoffs;
					sec->floor_yoffs = s->new_yoffs;
					break;
				}
				case sc_ceiling:
				{
					sector_t *sec;
					sec = sectors + s->affectee;
					if (s->firstlerp != 1) break;
					sec->ceiling_xoffs = s->new_xoffs;
					sec->ceiling_yoffs = s->new_yoffs;
					break;
				}
				case sc_carry:
				case sc_carry_ceiling:
					break;
			}
		}
		if (ISA(T_Friction))
		{
			//CAST(f, friction_t);
		}
		if (ISA(T_Pusher))
		{
			//CAST(f, pusher_t);
		}
	}
}

fixed_t R_LerpFixed(fixed_t from, fixed_t to, fixed_t frac)
{
	return FixedMul(frac, to - from);
}

INT32 R_LerpInt32(INT32 from, INT32 to, fixed_t frac)
{
	return FixedInt(FixedMul(frac, (to*FRACUNIT) - (from*FRACUNIT)));
}

angle_t R_LerpAngle(angle_t from, angle_t to, fixed_t frac)
{
	return FixedMul(frac, to - from);
}