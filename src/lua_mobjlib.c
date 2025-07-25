// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_mobjlib.c
/// \brief mobj/thing library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "r_things.h"
#include "p_local.h"
#include "g_game.h"
#include "p_setup.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors

static const char *const array_opt[] ={"iterate",NULL};

enum mobj_e {
	mobj_valid = 0,
	mobj_x,
	mobj_y,
	mobj_z,
	mobj_snext,
	mobj_sprev,
	mobj_angle,
	mobj_sprite,
	mobj_frame,
	mobj_anim_duration,
	mobj_touching_sectorlist,
	mobj_subsector,
	mobj_floorz,
	mobj_ceilingz,
	mobj_radius,
	mobj_height,
	mobj_momx,
	mobj_momy,
	mobj_momz,
	mobj_pmomz,
	mobj_tics,
	mobj_state,
	mobj_flags,
	mobj_flags2,
	mobj_eflags,
	mobj_skin,
	mobj_color,
	mobj_bnext,
	mobj_bprev,
	mobj_hnext,
	mobj_hprev,
	mobj_type,
	mobj_info,
	mobj_health,
	mobj_movedir,
	mobj_movecount,
	mobj_target,
	mobj_reactiontime,
	mobj_threshold,
	mobj_player,
	mobj_lastlook,
	mobj_spawnpoint,
	mobj_tracer,
	mobj_friction,
	mobj_movefactor,
	mobj_fuse,
	mobj_watertop,
	mobj_waterbottom,
	mobj_mobjnum,
	mobj_scale,
	mobj_destscale,
	mobj_scalespeed,
	mobj_extravalue1,
	mobj_extravalue2,
	mobj_cusval,
	mobj_cvmem,
	mobj_standingslope
};

static const char *const mobj_opt[] = {
	"valid",
	"x",
	"y",
	"z",
	"snext",
	"sprev",
	"angle",
	"sprite",
	"frame",
	"anim_duration",
	"touching_sectorlist",
	"subsector",
	"floorz",
	"ceilingz",
	"radius",
	"height",
	"momx",
	"momy",
	"momz",
	"pmomz",
	"tics",
	"state",
	"flags",
	"flags2",
	"eflags",
	"skin",
	"color",
	"bnext",
	"bprev",
	"hnext",
	"hprev",
	"type",
	"info",
	"health",
	"movedir",
	"movecount",
	"target",
	"reactiontime",
	"threshold",
	"player",
	"lastlook",
	"spawnpoint",
	"tracer",
	"friction",
	"movefactor",
	"fuse",
	"watertop",
	"waterbottom",
	"mobjnum",
	"scale",
	"destscale",
	"scalespeed",
	"extravalue1",
	"extravalue2",
	"cusval",
	"cvmem",
	"standingslope",
	NULL};

static int mobj_fields_ref = LUA_NOREF;

enum mapthing_e {
	mapthing_valid,
	mapthing_x,
	mapthing_y,
	mapthing_angle,
	mapthing_type,
	mapthing_options,
	mapthing_z,
	mapthing_extrainfo,
	mapthing_mobj,
};

static const char *const mapthing_opt[] = {
	"valid",
	"x",
	"y",
	"angle",
	"type",
	"options",
	"z",
	"extrainfo",
	"mobj",
	NULL,
};

static int mapthing_fields_ref = LUA_NOREF;

#define UNIMPLEMENTED luaL_error(L, LUA_QL("mobj_t") " field " LUA_QS " is not implemented for Lua and cannot be accessed.", mobj_opt[field])

static int mobj_get(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	enum mobj_e field = Lua_optoption(L, 2, -1, mobj_fields_ref);
	lua_settop(L, 2);

	if (!mo) {
		if (field == mobj_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return LUA_ErrInvalid(L, "mobj_t");
	}

	switch(field)
	{
	case mobj_valid:
		lua_pushboolean(L, 1);
		break;
	case mobj_x:
		lua_pushfixed(L, mo->x);
		break;
	case mobj_y:
		lua_pushfixed(L, mo->y);
		break;
	case mobj_z:
		lua_pushfixed(L, mo->z);
		break;
	case mobj_snext:
		LUA_PushUserdata(L, mo->snext, META_MOBJ);
		break;
	case mobj_sprev:
		// sprev is actually the previous mobj's snext pointer,
		// or the subsector->sector->thing_list if there is no previous mobj,
		// i.e. it will always ultimately point to THIS mobj -- so that's actually not useful to Lua and won't be included.
		return UNIMPLEMENTED;
	case mobj_angle:
		lua_pushangle(L, mo->angle);
		break;
	case mobj_sprite:
		lua_pushinteger(L, mo->sprite);
		break;
	case mobj_frame:
		lua_pushinteger(L, mo->frame);
		break;
	case mobj_anim_duration:
		lua_pushinteger(L, mo->anim_duration);
		break;
	case mobj_touching_sectorlist:
		return UNIMPLEMENTED;
	case mobj_subsector:
		LUA_PushUserdata(L, mo->subsector, META_SUBSECTOR);
		break;
	case mobj_floorz:
		lua_pushfixed(L, mo->floorz);
		break;
	case mobj_ceilingz:
		lua_pushfixed(L, mo->ceilingz);
		break;
	case mobj_radius:
		lua_pushfixed(L, mo->radius);
		break;
	case mobj_height:
		lua_pushfixed(L, mo->height);
		break;
	case mobj_momx:
		lua_pushfixed(L, mo->momx);
		break;
	case mobj_momy:
		lua_pushfixed(L, mo->momy);
		break;
	case mobj_momz:
		lua_pushfixed(L, mo->momz);
		break;
	case mobj_pmomz:
		lua_pushfixed(L, mo->pmomz);
		break;
	case mobj_tics:
		lua_pushinteger(L, mo->tics);
		break;
	case mobj_state: // state number, not struct
		lua_pushinteger(L, mo->state-states);
		break;
	case mobj_flags:
		lua_pushinteger(L, mo->flags);
		break;
	case mobj_flags2:
		lua_pushinteger(L, mo->flags2);
		break;
	case mobj_eflags:
		lua_pushinteger(L, mo->eflags);
		break;
	case mobj_skin: // skin name or nil, not struct
		if (!mo->skin)
			return 0;
		lua_pushstring(L, ((skin_t *)mo->skin)->name);
		break;
	case mobj_color:
		lua_pushinteger(L, mo->color);
		break;
	case mobj_bnext:
		LUA_PushUserdata(L, mo->bnext, META_MOBJ);
		break;
	case mobj_bprev:
		// bprev -- same deal as sprev above, but for the blockmap.
		return UNIMPLEMENTED;
	case mobj_hnext:
		LUA_PushUserdata(L, mo->hnext, META_MOBJ);
		break;
	case mobj_hprev:
		// implimented differently from sprev and bprev because SSNTails.
		LUA_PushUserdata(L, mo->hprev, META_MOBJ);
		break;
	case mobj_type:
		lua_pushinteger(L, mo->type);
		break;
	case mobj_info:
		LUA_PushUserdata(L, &mobjinfo[mo->type], META_MOBJINFO);
		break;
	case mobj_health:
		lua_pushinteger(L, mo->health);
		break;
	case mobj_movedir:
		lua_pushinteger(L, mo->movedir);
		break;
	case mobj_movecount:
		lua_pushinteger(L, mo->movecount);
		break;
	case mobj_target:
		if (mo->target && P_MobjWasRemoved(mo->target))
		{ // don't put invalid mobj back into Lua.
			P_SetTarget(&mo->target, NULL);
			return 0;
		}
		LUA_PushUserdata(L, mo->target, META_MOBJ);
		break;
	case mobj_reactiontime:
		lua_pushinteger(L, mo->reactiontime);
		break;
	case mobj_threshold:
		lua_pushinteger(L, mo->threshold);
		break;
	case mobj_player:
		LUA_PushUserdata(L, mo->player, META_PLAYER);
		break;
	case mobj_lastlook:
		lua_pushinteger(L, mo->lastlook);
		break;
	case mobj_spawnpoint:
		LUA_PushUserdata(L, mo->spawnpoint, META_MAPTHING);
		break;
	case mobj_tracer:
		if (mo->tracer && P_MobjWasRemoved(mo->tracer))
		{ // don't put invalid mobj back into Lua.
			P_SetTarget(&mo->tracer, NULL);
			return 0;
		}
		LUA_PushUserdata(L, mo->tracer, META_MOBJ);
		break;
	case mobj_friction:
		lua_pushfixed(L, mo->friction);
		break;
	case mobj_movefactor:
		lua_pushfixed(L, mo->movefactor);
		break;
	case mobj_fuse:
		lua_pushinteger(L, mo->fuse);
		break;
	case mobj_watertop:
		lua_pushfixed(L, mo->watertop);
		break;
	case mobj_waterbottom:
		lua_pushfixed(L, mo->waterbottom);
		break;
	case mobj_mobjnum:
		// mobjnum is a networking thing generated for $$$.sav
		// and therefore shouldn't be used by Lua.
		return UNIMPLEMENTED;
	case mobj_scale:
		lua_pushfixed(L, mo->scale);
		break;
	case mobj_destscale:
		lua_pushfixed(L, mo->destscale);
		break;
	case mobj_scalespeed:
		lua_pushfixed(L, mo->scalespeed);
		break;
	case mobj_extravalue1:
		lua_pushinteger(L, mo->extravalue1);
		break;
	case mobj_extravalue2:
		lua_pushinteger(L, mo->extravalue2);
		break;
	case mobj_cusval:
		lua_pushinteger(L, mo->cusval);
		break;
	case mobj_cvmem:
		lua_pushinteger(L, mo->cvmem);
		break;
	case mobj_standingslope:
		LUA_PushUserdata(L, mo->standingslope, META_SLOPE);
		break;
	default: // extra custom variables in Lua memory
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, mo);
		lua_rawget(L, -2);
		if (!lua_istable(L, -1)) { // no extra values table
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no extvars table or field named '%s'; returning nil.\n"), "mobj_t", lua_tostring(L, 2));
			return 0;
		}
		lua_pushvalue(L, 2); // field name
		lua_gettable(L, -2);
		if (lua_isnil(L, -1)) // no value for this field
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "mobj_t", lua_tostring(L, 2));
		break;
	}
	return 1;
}

#define NOSET luaL_error(L, LUA_QL("mobj_t") " field " LUA_QS " should not be set directly.", mobj_opt[field])
#define NOSETPOS luaL_error(L, LUA_QL("mobj_t") " field " LUA_QS " should not be set directly. Use " LUA_QL("P_Move") ", " LUA_QL("P_TryMove") ", or " LUA_QL("P_SetOrigin") ", or " LUA_QL("P_MoveOrigin") " instead.", mobj_opt[field])
static int mobj_set(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	enum mobj_e field = Lua_optoption(L, 2, mobj_valid, mobj_fields_ref);
	lua_settop(L, 3);

	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");

	if (hud_running)
		return luaL_error(L, "Do not alter mobj_t in HUD rendering code!");

	switch(field)
	{
	case mobj_valid:
		return NOSET;
	case mobj_x:
		return NOSETPOS;
	case mobj_y:
		return NOSETPOS;
	case mobj_z:
	{
		// z doesn't cross sector bounds so it's okay.
		mobj_t *ptmthing = tmthing;
		mo->z = luaL_checkfixed(L, 3);
		P_CheckPosition(mo, mo->x, mo->y);
		mo->floorz = tmfloorz;
		mo->ceilingz = tmceilingz;
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case mobj_snext:
		return NOSETPOS;
	case mobj_sprev:
		return UNIMPLEMENTED;
	case mobj_angle:
		mo->angle = luaL_checkangle(L, 3);
		if (mo->player == &players[consoleplayer])
			localangle = mo->angle;
		else if (mo->player == &players[secondarydisplayplayer])
			localangle2 = mo->angle;
		break;
	case mobj_sprite:
		mo->sprite = luaL_checkinteger(L, 3);
		break;
	case mobj_frame:
		mo->frame = (UINT32)luaL_checkinteger(L, 3);
		break;
	case mobj_anim_duration:
		mo->anim_duration = (UINT16)luaL_checkinteger(L, 3);
		break;
	case mobj_touching_sectorlist:
		return UNIMPLEMENTED;
	case mobj_subsector:
		return NOSETPOS;
	case mobj_floorz:
		return NOSETPOS;
	case mobj_ceilingz:
		return NOSETPOS;
	case mobj_radius:
	{
		mobj_t *ptmthing = tmthing;
		mo->radius = luaL_checkfixed(L, 3);
		if (mo->radius < 0)
			mo->radius = 0;
		P_CheckPosition(mo, mo->x, mo->y);
		mo->floorz = tmfloorz;
		mo->ceilingz = tmceilingz;
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case mobj_height:
	{
		mobj_t *ptmthing = tmthing;
		mo->height = luaL_checkfixed(L, 3);
		if (mo->height < 0)
			mo->height = 0;
		P_CheckPosition(mo, mo->x, mo->y);
		mo->floorz = tmfloorz;
		mo->ceilingz = tmceilingz;
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case mobj_momx:
		mo->momx = luaL_checkfixed(L, 3);
		break;
	case mobj_momy:
		mo->momy = luaL_checkfixed(L, 3);
		break;
	case mobj_momz:
		mo->momz = luaL_checkfixed(L, 3);
		break;
	case mobj_pmomz:
		mo->pmomz = luaL_checkfixed(L, 3);
		mo->eflags |= MFE_APPLYPMOMZ;
		break;
	case mobj_tics:
		mo->tics = luaL_checkinteger(L, 3);
		break;
	case mobj_state: // set state by enum
		if (mo->player)
			P_SetPlayerMobjState(mo, luaL_checkinteger(L, 3));
		else
			P_SetMobjState(mo, luaL_checkinteger(L, 3));
		break;
	case mobj_flags: // special handling for MF_NOBLOCKMAP and MF_NOSECTOR
	{
		UINT32 flags = luaL_checkinteger(L, 3);
		if ((flags & (MF_NOBLOCKMAP|MF_NOSECTOR)) != (mo->flags & (MF_NOBLOCKMAP|MF_NOSECTOR)))
		{
			P_UnsetThingPosition(mo);
			mo->flags = flags;
			if (flags & MF_NOSECTOR && sector_list)
			{
				P_DelSeclist(sector_list);
				sector_list = NULL;
			}
			mo->snext = NULL, mo->sprev = NULL;
			mo->bnext = NULL, mo->bprev = NULL;
			P_SetThingPosition(mo);
		}
		else
			mo->flags = flags;
		break;
	}
	case mobj_flags2:
		mo->flags2 = (UINT32)luaL_checkinteger(L, 3);
		break;
	case mobj_eflags:
		mo->eflags = (UINT32)luaL_checkinteger(L, 3);
		break;
	case mobj_skin: // set skin by name
	{
		INT32 i;
		char skin[SKINNAMESIZE+1]; // all skin names are limited to this length
		strlcpy(skin, luaL_checkstring(L, 3), sizeof skin);
		strlwr(skin); // all skin names are lowercase
		for (i = 0; i < numskins; i++)
			if (fastcmp(skins[i].name, skin))
			{
				mo->skin = &skins[i];
				return 0;
			}
		return luaL_error(L, "mobj.skin '%s' not found!", skin);
	}
	case mobj_color:
	{
		UINT16 newcolor = (UINT16)luaL_checkinteger(L,3);
		if (newcolor >= numskincolors)
			return luaL_error(L, "mobj.color %d out of range (0 - %d).", newcolor, numskincolors-1);
		mo->color = newcolor;
		break;
	}
	case mobj_bnext:
		return NOSETPOS;
	case mobj_bprev:
		return UNIMPLEMENTED;
	case mobj_hnext:
		mo->hnext = luaL_checkudata(L, 3, META_MOBJ);
		break;
	case mobj_hprev:
		mo->hprev = luaL_checkudata(L, 3, META_MOBJ);
		break;
	case mobj_type: // yeah sure, we'll let you change the mobj's type.
	{
		mobjtype_t newtype = luaL_checkinteger(L, 3);
		if (newtype >= NUMMOBJTYPES)
			return luaL_error(L, "mobj.type %d out of range (0 - %d).", newtype, NUMMOBJTYPES-1);
		mo->type = newtype;
		mo->info = &mobjinfo[newtype];
		P_SetScale(mo, mo->scale);
		break;
	}
	case mobj_info:
		return NOSET;
	case mobj_health:
		mo->health = luaL_checkinteger(L, 3);
		break;
	case mobj_movedir:
		mo->movedir = (angle_t)luaL_checkinteger(L, 3);
		break;
	case mobj_movecount:
		mo->movecount = luaL_checkinteger(L, 3);
		break;
	case mobj_target:
		if (lua_isnil(L, 3))
			P_SetTarget(&mo->target, NULL);
		else
		{
			mobj_t *target = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
			P_SetTarget(&mo->target, target);
		}
		break;
	case mobj_reactiontime:
		mo->reactiontime = luaL_checkinteger(L, 3);
		break;
	case mobj_threshold:
		mo->threshold = luaL_checkinteger(L, 3);
		break;
	case mobj_player:
		return NOSET;
	case mobj_lastlook:
		mo->lastlook = luaL_checkinteger(L, 3);
		break;
	case mobj_spawnpoint:
		if (lua_isnil(L, 3))
			mo->spawnpoint = NULL;
		else
		{
			mapthing_t *spawnpoint = *((mapthing_t **)luaL_checkudata(L, 3, META_MAPTHING));
			mo->spawnpoint = spawnpoint;
		}
		break;
	case mobj_tracer:
		if (lua_isnil(L, 3))
			P_SetTarget(&mo->tracer, NULL);
		else
		{
			mobj_t *tracer = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
			P_SetTarget(&mo->tracer, tracer);
		}
		break;
	case mobj_friction:
		mo->friction = luaL_checkfixed(L, 3);
		break;
	case mobj_movefactor:
		mo->movefactor = luaL_checkfixed(L, 3);
		break;
	case mobj_fuse:
		mo->fuse = luaL_checkinteger(L, 3);
		break;
	case mobj_watertop:
		mo->watertop = luaL_checkfixed(L, 3);
		break;
	case mobj_waterbottom:
		mo->waterbottom = luaL_checkfixed(L, 3);
		break;
	case mobj_mobjnum:
		return UNIMPLEMENTED;
	case mobj_scale:
	{
		fixed_t scale = luaL_checkfixed(L, 3);
		if (scale < FRACUNIT/100)
			scale = FRACUNIT/100;
		mo->destscale = scale;
		P_SetScale(mo, scale);
		mo->old_scale = scale;
		break;
	}
	case mobj_destscale:
	{
		fixed_t scale = luaL_checkfixed(L, 3);
		if (scale < FRACUNIT/100)
			scale = FRACUNIT/100;
		mo->destscale = scale;
		break;
	}
	case mobj_scalespeed:
		mo->scalespeed = luaL_checkfixed(L, 3);
		break;
	case mobj_extravalue1:
		mo->extravalue1 = luaL_checkinteger(L, 3);
		break;
	case mobj_extravalue2:
		mo->extravalue2 = luaL_checkinteger(L, 3);
		break;
	case mobj_cusval:
		mo->cusval = luaL_checkinteger(L, 3);
		break;
	case mobj_cvmem:
		mo->cvmem = luaL_checkinteger(L, 3);
		break;
	case mobj_standingslope:
		return NOSET;
	default:
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, mo);
		lua_rawget(L, -2);
		if (lua_isnil(L, -1)) {
			// This index doesn't have a table for extra values yet, let's make one.
			lua_pop(L, 1);
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; adding it as Lua data.\n"), "mobj_t", lua_tostring(L, 2));
			lua_newtable(L);
			lua_pushlightuserdata(L, mo);
			lua_pushvalue(L, -2); // ext value table
			lua_rawset(L, -4); // LREG_EXTVARS table
		}
		lua_pushvalue(L, 2); // key
		lua_pushvalue(L, 3); // value to store
		lua_settable(L, -3);
		lua_pop(L, 2);
		break;
	}
	return 0;
}

#undef UNIMPLEMENTED
#undef NOSET
#undef NOSETPOS
#undef NOFIELD

static int mapthing_get(lua_State *L)
{
	mapthing_t *mt = *((mapthing_t **)luaL_checkudata(L, 1, META_MAPTHING));
	enum mapthing_e field = Lua_optoption(L, 2, -1, mapthing_fields_ref);
	INT32 number;
	lua_settop(L, 2);

	if (!mt) {
		if (field == mapthing_valid) {
			lua_pushboolean(L, false);
			return 1;
		}
		if (devparm)
			return luaL_error(L, "accessed mapthing_t doesn't exist anymore.");
		return 0;
	}

	if (field == (enum mapthing_e)-1)
		return LUA_ErrInvalid(L, "fields");

	switch (field)
	{
	case mapthing_valid:
		lua_pushboolean(L, true);
		return 1;
	case mapthing_x:
		number = mt->x;
		break;
	case mapthing_y:
		number = mt->y;
		break;
	case mapthing_angle:
		number = mt->angle;
		break;
	case mapthing_type:
		number = mt->type;
		break;
	case mapthing_options:
		number = mt->options;
		break;
	case mapthing_z:
		number = mt->z;
		break;
	case mapthing_extrainfo:
		number = mt->extrainfo;
		break;
	case mapthing_mobj:
		LUA_PushUserdata(L, mt->mobj, META_MOBJ);
		return 1;
	default:
		if (devparm)
			return luaL_error(L, LUA_QL("mapthing_t") " has no field named " LUA_QS, luaL_checkstring(L, 2));
		return 0;
	}

	lua_pushinteger(L, number);
	return 1;
}

static int mapthing_set(lua_State *L)
{
	mapthing_t *mt = *((mapthing_t **)luaL_checkudata(L, 1, META_MAPTHING));
	enum mapthing_e field = Lua_optoption(L, 2, -1, mapthing_fields_ref);
	lua_settop(L, 3);

	if (!mt)
		return luaL_error(L, "accessed mapthing_t doesn't exist anymore.");

	if (field == (enum mapthing_e)-1)
		return LUA_ErrInvalid(L, "fields");

	if (hud_running)
		return luaL_error(L, "Do not alter mapthing_t in HUD rendering code!");

	switch (field)
	{
	case mapthing_x:
		mt->x = (INT16)luaL_checkinteger(L, 3);
		break;
	case mapthing_y:
		mt->y = (INT16)luaL_checkinteger(L, 3);
		break;
	case mapthing_angle:
		mt->angle = (INT16)luaL_checkinteger(L, 3);
		break;
	case mapthing_type:
		mt->type = (UINT16)luaL_checkinteger(L, 3);
		break;
	case mapthing_options:
		mt->options = (UINT16)luaL_checkinteger(L, 3);
		break;
	case mapthing_z:
		mt->z = (INT16)luaL_checkinteger(L, 3);
		break;
	case mapthing_extrainfo:
		mt->extrainfo = (UINT8)luaL_checkinteger(L, 3);
		break;
	case mapthing_mobj:
		mt->mobj = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		break;
	default:
		return luaL_error(L, "%s %s", LUA_QL("mapthing_t"), va("has no field named: %ui", field));
	}

	return 0;
}

static int lib_iterateMapthings(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call mapthings.iterate() directly, use it as 'for mapthing in mapthings.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((mapthing_t **)luaL_checkudata(L, 1, META_MAPTHING)) - mapthings) + 1;
	if (i < nummapthings)
	{
		LUA_PushUserdata(L, &mapthings[i], META_MAPTHING);
		return 1;
	}
	return 0;
}

static int lib_getMapthing(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= nummapthings)
			return 0;
		LUA_PushUserdata(L, &mapthings[i], META_MAPTHING);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateMapthings);
		return 1;
	}
	return 0;
}

static int lib_nummapthings(lua_State *L)
{
	lua_pushinteger(L, nummapthings);
	return 1;
}

int LUA_MobjLib(lua_State *L)
{
	luaL_newmetatable(L, META_MOBJ);
		lua_pushcfunction(L, mobj_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, mobj_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	mobj_fields_ref = Lua_CreateFieldTable(L, mobj_opt);

	luaL_newmetatable(L, META_MAPTHING);
		lua_pushcfunction(L, mapthing_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, mapthing_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	mapthing_fields_ref = Lua_CreateFieldTable(L, mapthing_opt);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getMapthing);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_nummapthings);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "mapthings");
	return 0;
}
