// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_maplib.c
/// \brief game map library for Lua scripting

#include "doomdef.h"
#include "r_state.h"
#include "p_local.h"
#include "p_setup.h"
#include "z_zone.h"
#include "p_slopes.h"
#include "r_main.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors

#include "dehacked.h"
#include "fastcmp.h"
#include "doomstat.h"

enum sector_e {
	sector_valid = 0,
	sector_floorheight,
	sector_ceilingheight,
	sector_floorpic,
	sector_ceilingpic,
	sector_lightlevel,
	sector_special,
	sector_tag,
	sector_thinglist,
	sector_heightsec,
	sector_camsec,
	sector_lines,
	sector_ffloors,
	sector_fslope,
	sector_cslope
};

static const char *const sector_opt[] = {
	"valid",
	"floorheight",
	"ceilingheight",
	"floorpic",
	"ceilingpic",
	"lightlevel",
	"special",
	"tag",
	"thinglist",
	"heightsec",
	"camsec",
	"lines",
	"ffloors",
	"f_slope",
	"c_slope",
	NULL};

static int sector_fields_ref = LUA_NOREF;

enum subsector_e {
	subsector_valid = 0,
	subsector_sector,
	subsector_numlines,
	subsector_firstline,
};

static const char *const subsector_opt[] = {
	"valid",
	"sector",
	"numlines",
	"firstline",
	NULL};

static int subsector_fields_ref = LUA_NOREF;

enum line_e {
	line_valid = 0,
	line_v1,
	line_v2,
	line_dx,
	line_dy,
	line_flags,
	line_special,
	line_tag,
	line_sidenum,
	line_frontside,
	line_backside,
	line_slopetype,
	line_frontsector,
	line_backsector,
	line_firsttag,
	line_nexttag,
	line_text,
	line_callcount
};

static const char *const line_opt[] = {
	"valid",
	"v1",
	"v2",
	"dx",
	"dy",
	"flags",
	"special",
	"tag",
	"sidenum",
	"frontside",
	"backside",
	"slopetype",
	"frontsector",
	"backsector",
	"firsttag",
	"nexttag",
	"text",
	"callcount",
	NULL};

static int line_fields_ref = LUA_NOREF;

enum side_e {
	side_valid = 0,
	side_textureoffset,
	side_rowoffset,
	side_toptexture,
	side_bottomtexture,
	side_midtexture,
	side_sector,
	side_special,
	side_repeatcnt,
	side_text
};

static const char *const side_opt[] = {
	"valid",
	"textureoffset",
	"rowoffset",
	"toptexture",
	"bottomtexture",
	"midtexture",
	"sector",
	"special",
	"repeatcnt",
	"text",
	NULL};

static int side_fields_ref = LUA_NOREF;

enum vertex_e {
	vertex_valid = 0,
	vertex_x,
	vertex_y,
	vertex_z
};

static const char *const vertex_opt[] = {
	"valid",
	"x",
	"y",
	"z",
	NULL};

static int vertex_fields_ref = LUA_NOREF;

enum ffloor_e {
	ffloor_valid = 0,
	ffloor_topheight,
	ffloor_toppic,
	ffloor_toplightlevel,
	ffloor_bottomheight,
	ffloor_bottompic,
	ffloor_tslope,
	ffloor_bslope,
	ffloor_sector,
	ffloor_flags,
	ffloor_master,
	ffloor_target,
	ffloor_next,
	ffloor_prev,
	ffloor_alpha,
};

static const char *const ffloor_opt[] = {
	"valid",
	"topheight",
	"toppic",
	"toplightlevel",
	"bottomheight",
	"bottompic",
	"t_slope",
	"b_slope",

	"sector", // secnum pushed as control sector userdata
	"flags",
	"master", // control linedef
	"target", // target sector
	"next",
	"prev",
	"alpha",
	NULL};

static int ffloor_fields_ref = LUA_NOREF;


enum slope_e {
	slope_valid = 0,
	slope_o,
	slope_d,
	slope_zdelta,
	slope_normal,
	slope_zangle,
	slope_xydirection,
	slope_sourceline,
	slope_refpos,
	slope_flags
};

static const char *const slope_opt[] = {
	"valid",
	"o",
	"d",
	"zdelta",
	"normal",
	"zangle",
	"xydirection",
	"sourceline",
	"refpos",
	"flags",
	NULL};

static int slope_fields_ref = LUA_NOREF;

// shared by both vector2_t and vector3_t
enum vector_e {
	vector_x = 0,
	vector_y,
	vector_z
};

static const char *const vector_opt[] = {
	"x",
	"y",
	"z",
	NULL};

static int vector_fields_ref = LUA_NOREF;

enum mapheaderinfo_e
{
	mapheaderinfo_lvlttl,
	mapheaderinfo_subttl,
	mapheaderinfo_actnum,
	mapheaderinfo_typeoflevel,
	mapheaderinfo_nextlevel,
	mapheaderinfo_musname,
	mapheaderinfo_mustrack,
	mapheaderinfo_muspos,
	mapheaderinfo_musinterfadeout,
	mapheaderinfo_musintername,
	mapheaderinfo_forcecharacter,
	mapheaderinfo_weather,
	mapheaderinfo_skynum,
	mapheaderinfo_skybox_scalex,
	mapheaderinfo_skybox_scaley,
	mapheaderinfo_skybox_scalez,
	mapheaderinfo_interscreen,
	mapheaderinfo_runsoc,
	mapheaderinfo_scriptname,
	mapheaderinfo_precutscenenum,
	mapheaderinfo_cutscenenum,
	mapheaderinfo_countdown,
	mapheaderinfo_palette,
	mapheaderinfo_numlaps,
	mapheaderinfo_unlockrequired,
	mapheaderinfo_levelselect,
	mapheaderinfo_bonustype,
	mapheaderinfo_saveoverride,
	mapheaderinfo_levelflags,
	mapheaderinfo_menuflags,
};

static const char *const mapheaderinfo_opt[] = {
	"lvlttl",
	"subttl",
	"actnum",
	"typeoflevel",
	"nextlevel",
	"musname",
	"mustrack",
	"muspos",
	"musinterfadeout",
	"musintername",
	"forcecharacter",
	"weather",
	"skynum",
	"skybox_scalex",
	"skybox_scaley",
	"skybox_scalez",
	"interscreen",
	"runsoc",
	"scriptname",
	"precutscenenum",
	"cutscenenum",
	"countdown",
	"palette",
	"numlaps",
	"unlockrequired",
	"levelselect",
	"bonustype",
	"saveoverride",
	"levelflags",
	"menuflags",
	NULL,
};

static int mapheaderinfo_fields_ref = LUA_NOREF;

static const char *const array_opt[] ={"iterate",NULL};

// iterates through a sector's thinglist!
static int lib_iterateSectorThinglist(lua_State *L)
{
	mobj_t *state = NULL;
	mobj_t *thing = NULL;

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sector.thinglist() directly, use it as 'for rover in sector.thinglist do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	else
		return 0; // no thinglist to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (P_MobjWasRemoved(thing))
			return luaL_error(L, "current entry in thinglist was removed; avoid calling P_RemoveMobj on entries!");
		thing = thing->snext;
	}
	else
		thing = state; // state is used as the "start" of the thinglist

	if (thing)
	{
		LUA_PushUserdata(L, thing, META_MOBJ);
		return 1;
	}
	return 0;
}

// iterates through the ffloors list in a sector!
static int lib_iterateSectorFFloors(lua_State *L)
{
	ffloor_t *state = NULL;
	ffloor_t *ffloor = NULL;

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sector.ffloors() directly, use it as 'for rover in sector.ffloors do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	else
		return 0; // no ffloors to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
		ffloor = ffloor->next;
	}
	else
		ffloor = state; // state is used as the "start" of the ffloor list

	if (ffloor)
	{
		LUA_PushUserdata(L, ffloor, META_FFLOOR);
		return 1;
	}
	return 0;
}

static int sector_iterate(lua_State *L)
{
	lua_pushvalue(L, lua_upvalueindex(1)); // iterator function, or the "generator"
	lua_pushvalue(L, lua_upvalueindex(2)); // state (used as the "start" of the list for our purposes
	lua_pushnil(L); // initial value (unused)
	return 3;
}

// sector.lines, i -> sector.lines[i]
// sector.lines.valid, for validity checking
static int sectorlines_get(lua_State *L)
{
	line_t **seclines = *((line_t ***)luaL_checkudata(L, 1, META_SECTORLINES));
	size_t i;
	size_t numoflines = 0;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		enum sector_e field = Lua_optoption(L, 2, sector_valid, sector_fields_ref);
		if (!seclines)
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed sector_t doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	// check first linedef to figure which of its sectors owns this sector->lines pointer
	// then check that sector's linecount to get a maximum index
	//if (!seclines[0])
		//return luaL_error(L, "no lines found!"); // no first linedef?????
	if (seclines[0]->frontsector->lines == seclines)
		numoflines = seclines[0]->frontsector->linecount;
	else if (seclines[0]->backsector && seclines[0]->backsector->lines == seclines) // check backsector exists first
		numoflines = seclines[0]->backsector->linecount;
	//if neither sector has it then ???

	if (!numoflines)
		return luaL_error(L, "no lines found!");

	i = (size_t)lua_tointeger(L, 2);
	if (i >= numoflines)
		return 0;
	LUA_PushUserdata(L, seclines[i], META_LINE);
	return 1;
}

static int sectorlines_num(lua_State *L)
{
	line_t **seclines = *((line_t ***)luaL_checkudata(L, 1, META_SECTORLINES));
	size_t numoflines = 0;
	// check first linedef to figure which of its sectors owns this sector->lines pointer
	// then check that sector's linecount to get a maximum index
	//if (!seclines[0])
		//return luaL_error(L, "no lines found!"); // no first linedef?????
	if (seclines[0]->frontsector->lines == seclines)
		numoflines = seclines[0]->frontsector->linecount;
	else if (seclines[0]->backsector && seclines[0]->backsector->lines == seclines) // check backsector exists first
		numoflines = seclines[0]->backsector->linecount;
	//if neither sector has it then ???
	lua_pushinteger(L, numoflines);
	return 1;
}

static int sector_get(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	enum sector_e field = Lua_optoption(L, 2, sector_valid, sector_fields_ref);
	INT16 i;

	if (!sector)
	{
		if (field == sector_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case sector_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case sector_floorheight:
		lua_pushfixed(L, sector->floorheight);
		return 1;
	case sector_ceilingheight:
		lua_pushfixed(L, sector->ceilingheight);
		return 1;
	case sector_floorpic: // floorpic
	{
		levelflat_t *levelflat = &levelflats[sector->floorpic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case sector_ceilingpic: // ceilingpic
	{
		levelflat_t *levelflat = &levelflats[sector->ceilingpic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case sector_lightlevel:
		lua_pushinteger(L, sector->lightlevel);
		return 1;
	case sector_special:
		lua_pushinteger(L, sector->special);
		return 1;
	case sector_tag:
		lua_pushinteger(L, sector->tag);
		return 1;
	case sector_thinglist: // thinglist
		lua_pushcfunction(L, lib_iterateSectorThinglist);
		LUA_PushUserdata(L, sector->thinglist, META_MOBJ);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateSectorThinglist and sector->thinglist as upvalues for the function
		return 1;
	case sector_heightsec: // heightsec - fake floor heights
		if (sector->heightsec < 0)
			return 0;
		LUA_PushUserdata(L, &sectors[sector->heightsec], META_SECTOR);
		return 1;
	case sector_camsec: // camsec - camera clipping heights
		if (sector->camsec < 0)
			return 0;
		LUA_PushUserdata(L, &sectors[sector->camsec], META_SECTOR);
		return 1;
	case sector_lines: // lines
		LUA_PushUserdata(L, sector->lines, META_SECTORLINES);
		return 1;
	case sector_ffloors: // ffloors
		lua_pushcfunction(L, lib_iterateSectorFFloors);
		LUA_PushUserdata(L, sector->ffloors, META_FFLOOR);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateFFloors and sector->ffloors as upvalues for the function
		return 1;
	case sector_fslope: // f_slope
		LUA_PushUserdata(L, sector->f_slope, META_SLOPE);
		return 1;
	case sector_cslope: // c_slope
		LUA_PushUserdata(L, sector->c_slope, META_SLOPE);
		return 1;
	}
	return 0;
}

static int sector_set(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	enum sector_e field = Lua_optoption(L, 2, sector_valid, sector_fields_ref);

	if (!sector)
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter sector_t in HUD rendering code!");

	switch(field)
	{
	case sector_valid: // valid
	case sector_thinglist: // thinglist
	case sector_heightsec: // heightsec
	case sector_camsec: // camsec
	case sector_ffloors: // ffloors
	case sector_fslope: // f_slope
	case sector_cslope: // c_slope
	default:
		return luaL_error(L, "sector_t field " LUA_QS " cannot be set.", sector_opt[field]);
	case sector_floorheight: { // floorheight
		boolean flag;
		mobj_t *ptmthing = tmthing;
		fixed_t lastpos = sector->floorheight;
		sector->floorheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->floorheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case sector_ceilingheight: { // ceilingheight
		boolean flag;
		mobj_t *ptmthing = tmthing;
		fixed_t lastpos = sector->ceilingheight;
		sector->ceilingheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->ceilingheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case sector_floorpic:
		sector->floorpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case sector_ceilingpic:
		sector->ceilingpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case sector_lightlevel:
		sector->lightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_special:
		sector->special = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_tag:
		P_ChangeSectorTag((UINT32)(sector - sectors), (INT16)luaL_checkinteger(L, 3));
		break;
	}
	return 0;
}

static int sector_num(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	lua_pushinteger(L, sector-sectors);
	return 1;
}

static int subsector_get(lua_State *L)
{
	subsector_t *subsector = *((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR));
	enum subsector_e field = Lua_optoption(L, 2, subsector_valid, subsector_fields_ref);

	if (!subsector)
	{
		if (field == subsector_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed subsector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case subsector_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case subsector_sector:
		LUA_PushUserdata(L, subsector->sector, META_SECTOR);
		return 1;
	case subsector_numlines:
		lua_pushinteger(L, subsector->numlines);
		return 1;
	case subsector_firstline:
		lua_pushinteger(L, subsector->firstline);
		return 1;
	}
	return 0;
}

static int subsector_num(lua_State *L)
{
	subsector_t *subsector = *((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR));
	lua_pushinteger(L, subsector-subsectors);
	return 1;
}

static int line_get(lua_State *L)
{
	line_t *line = *((line_t **)luaL_checkudata(L, 1, META_LINE));
	enum line_e field = Lua_optoption(L, 2, line_valid, line_fields_ref);

	if (!line)
	{
		if (field == line_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed line_t doesn't exist anymore.");
	}

	switch(field)
	{
	case line_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case line_v1:
		LUA_PushUserdata(L, line->v1, META_VERTEX);
		return 1;
	case line_v2:
		LUA_PushUserdata(L, line->v2, META_VERTEX);
		return 1;
	case line_dx:
		lua_pushfixed(L, line->dx);
		return 1;
	case line_dy:
		lua_pushfixed(L, line->dy);
		return 1;
	case line_flags:
		lua_pushinteger(L, line->flags);
		return 1;
	case line_special:
		lua_pushinteger(L, line->special);
		return 1;
	case line_tag:
		lua_pushinteger(L, line->tag);
		return 1;
	case line_sidenum:
		LUA_PushUserdata(L, line->sidenum, META_SIDENUM);
		return 1;
	case line_frontside: // frontside
		LUA_PushUserdata(L, &sides[line->sidenum[0]], META_SIDE);
		return 1;
	case line_backside: // backside
		if (line->sidenum[1] == 0xffff)
			return 0;
		LUA_PushUserdata(L, &sides[line->sidenum[1]], META_SIDE);
		return 1;
	case line_slopetype:
		switch(line->slopetype)
		{
		case ST_HORIZONTAL:
			lua_pushliteral(L, "horizontal");
			break;
		case ST_VERTICAL:
			lua_pushliteral(L, "vertical");
			break;
		case ST_POSITIVE:
			lua_pushliteral(L, "positive");
			break;
		case ST_NEGATIVE:
			lua_pushliteral(L, "negative");
			break;
		}
		return 1;
	case line_frontsector:
		LUA_PushUserdata(L, line->frontsector, META_SECTOR);
		return 1;
	case line_backsector:
		LUA_PushUserdata(L, line->backsector, META_SECTOR);
		return 1;
	case line_firsttag:
		lua_pushinteger(L, line->firsttag);
		return 1;
	case line_nexttag:
		lua_pushinteger(L, line->nexttag);
		return 1;
	case line_text:
		lua_pushstring(L, line->text);
		return 1;
	case line_callcount:
		lua_pushinteger(L, line->callcount);
		return 1;
	}
	return 0;
}

static int line_num(lua_State *L)
{
	line_t *line = *((line_t **)luaL_checkudata(L, 1, META_LINE));
	lua_pushinteger(L, line-lines);
	return 1;
}

static int sidenum_get(lua_State *L)
{
	UINT16 *sidenum = *((UINT16 **)luaL_checkudata(L, 1, META_SIDENUM));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		enum side_e field = Lua_optoption(L, 2, side_valid, side_fields_ref);
		if (!sidenum)
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed line_t doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	i = lua_tointeger(L, 2);
	if (i < 0 || i > 1)
		return 0;
	lua_pushinteger(L, sidenum[i]);
	return 1;
}

static int side_get(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	enum side_e field = Lua_optoption(L, 2, side_valid, side_fields_ref);

	if (!side)
	{
		if (field == side_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed side_t doesn't exist anymore.");
	}

	switch(field)
	{
	case side_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case side_textureoffset:
		lua_pushfixed(L, side->textureoffset);
		return 1;
	case side_rowoffset:
		lua_pushfixed(L, side->rowoffset);
		return 1;
	case side_toptexture:
		lua_pushinteger(L, side->toptexture);
		return 1;
	case side_bottomtexture:
		lua_pushinteger(L, side->bottomtexture);
		return 1;
	case side_midtexture:
		lua_pushinteger(L, side->midtexture);
		return 1;
	case side_sector:
		LUA_PushUserdata(L, side->sector, META_SECTOR);
		return 1;
	case side_special:
		lua_pushinteger(L, side->special);
		return 1;
	case side_repeatcnt:
		lua_pushinteger(L, side->repeatcnt);
		return 1;
	case side_text:
		lua_pushstring(L, side->text);
		return 1;
	}
	return 0;
}

static int side_set(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	enum side_e field = Lua_optoption(L, 2, side_valid, side_fields_ref);

	if (!side)
	{
		if (field == side_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed side_t doesn't exist anymore.");
	}

	switch(field)
	{
	case side_valid: // valid
	case side_sector:
	case side_special:
	case side_text:
	default:
		return luaL_error(L, "side_t field " LUA_QS " cannot be set.", side_opt[field]);
	case side_textureoffset:
		side->textureoffset = luaL_checkfixed(L, 3);
		break;
	case side_rowoffset:
		side->rowoffset = luaL_checkfixed(L, 3);
		break;
	case side_toptexture:
		side->toptexture = luaL_checkinteger(L, 3);
		break;
	case side_bottomtexture:
		side->bottomtexture = luaL_checkinteger(L, 3);
		break;
	case side_midtexture:
		side->midtexture = luaL_checkinteger(L, 3);
		break;
	case side_repeatcnt:
		side->repeatcnt = luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

static int side_num(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	lua_pushinteger(L, side-sides);
	return 1;
}

static int vertex_get(lua_State *L)
{
	vertex_t *vertex = *((vertex_t **)luaL_checkudata(L, 1, META_VERTEX));
	enum vertex_e field = Lua_optoption(L, 2, vertex_valid, vertex_fields_ref);

	if (!vertex)
	{
		if (field == vertex_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed vertex_t doesn't exist anymore.");
	}

	switch(field)
	{
	case vertex_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case vertex_x:
		lua_pushfixed(L, vertex->x);
		return 1;
	case vertex_y:
		lua_pushfixed(L, vertex->y);
		return 1;
	case vertex_z:
		lua_pushfixed(L, vertex->z);
		return 1;
	}
	return 0;
}

static int vertex_num(lua_State *L)
{
	vertex_t *vertex = *((vertex_t **)luaL_checkudata(L, 1, META_VERTEX));
	lua_pushinteger(L, vertex-vertexes);
	return 1;
}

static int lib_iterateSectors(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sectors.iterate() directly, use it as 'for sector in sectors.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((sector_t **)luaL_checkudata(L, 1, META_SECTOR)) - sectors)+1;
	if (i < numsectors)
	{
		LUA_PushUserdata(L, &sectors[i], META_SECTOR);
		return 1;
	}
	return 0;
}

static int lib_getSector(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsectors)
			return 0;
		LUA_PushUserdata(L, &sectors[i], META_SECTOR);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSectors);
		return 1;
	}
	return 0;
}

static int lib_numsectors(lua_State *L)
{
	lua_pushinteger(L, numsectors);
	return 1;
}

static int lib_iterateSubsectors(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call subsectors.iterate() directly, use it as 'for subsector in subsectors.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR)) - subsectors)+1;
	if (i < numsubsectors)
	{
		LUA_PushUserdata(L, &subsectors[i], META_SUBSECTOR);
		return 1;
	}
	return 0;
}

static int lib_getSubsector(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsubsectors)
			return 0;
		LUA_PushUserdata(L, &subsectors[i], META_SUBSECTOR);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSubsectors);
		return 1;
	}
	return 0;
}

static int lib_numsubsectors(lua_State *L)
{
	lua_pushinteger(L, numsubsectors);
	return 1;
}

static int lib_iterateLines(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call lines.iterate() directly, use it as 'for line in lines.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((line_t **)luaL_checkudata(L, 1, META_LINE)) - lines)+1;
	if (i < numlines)
	{
		LUA_PushUserdata(L, &lines[i], META_LINE);
		return 1;
	}
	return 0;
}

static int lib_getLine(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numlines)
			return 0;
		LUA_PushUserdata(L, &lines[i], META_LINE);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateLines);
		return 1;
	}
	return 0;
}

static int lib_numlines(lua_State *L)
{
	lua_pushinteger(L, numlines);
	return 1;
}

static int lib_iterateSides(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sides.iterate() directly, use it as 'for side in sides.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((side_t **)luaL_checkudata(L, 1, META_SIDE)) - sides)+1;
	if (i < numsides)
	{
		LUA_PushUserdata(L, &sides[i], META_SIDE);
		return 1;
	}
	return 0;
}

static int lib_getSide(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsides)
			return 0;
		LUA_PushUserdata(L, &sides[i], META_SIDE);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSides);
		return 1;
	}
	return 0;
}

static int lib_numsides(lua_State *L)
{
	lua_pushinteger(L, numsides);
	return 1;
}

static int lib_iterateVertexes(lua_State *L)
{
	size_t i = 0;
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call vertexes.iterate() directly, use it as 'for vertex in vertexes.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((vertex_t **)luaL_checkudata(L, 1, META_VERTEX)) - vertexes)+1;
	if (i < numvertexes)
	{
		LUA_PushUserdata(L, &vertexes[i], META_VERTEX);
		return 1;
	}
	return 0;
}

static int lib_getVertex(lua_State *L)
{
	int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numvertexes)
			return 0;
		LUA_PushUserdata(L, &vertexes[i], META_VERTEX);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateVertexes);
		return 1;
	}
	return 0;
}

static int lib_numvertexes(lua_State *L)
{
	lua_pushinteger(L, numvertexes);
	return 1;
}

static int ffloor_get(lua_State *L)
{
	ffloor_t *ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	enum ffloor_e field = Lua_optoption(L, 2, ffloor_valid, ffloor_fields_ref);
	INT16 i;

	if (!ffloor)
	{
		if (field == ffloor_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed ffloor_t doesn't exist anymore.");
	}

	switch(field)
	{
	case ffloor_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case ffloor_topheight:
		lua_pushfixed(L, *ffloor->topheight);
		return 1;
	case ffloor_toppic: { // toppic
		levelflat_t *levelflat = &levelflats[*ffloor->toppic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case ffloor_toplightlevel:
		lua_pushinteger(L, *ffloor->toplightlevel);
		return 1;
	case ffloor_bottomheight:
		lua_pushfixed(L, *ffloor->bottomheight);
		return 1;
	case ffloor_bottompic: { // bottompic
		levelflat_t *levelflat = &levelflats[*ffloor->bottompic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case ffloor_tslope:
		LUA_PushUserdata(L, *ffloor->t_slope, META_SLOPE);
		return 1;
	case ffloor_bslope:
		LUA_PushUserdata(L, *ffloor->b_slope, META_SLOPE);
		return 1;
	case ffloor_sector:
		LUA_PushUserdata(L, &sectors[ffloor->secnum], META_SECTOR);
		return 1;
	case ffloor_flags:
		lua_pushinteger(L, ffloor->flags);
		return 1;
	case ffloor_master:
		LUA_PushUserdata(L, ffloor->master, META_LINE);
		return 1;
	case ffloor_target:
		LUA_PushUserdata(L, ffloor->target, META_SECTOR);
		return 1;
	case ffloor_next:
		LUA_PushUserdata(L, ffloor->next, META_FFLOOR);
		return 1;
	case ffloor_prev:
		LUA_PushUserdata(L, ffloor->prev, META_FFLOOR);
		return 1;
	case ffloor_alpha:
		lua_pushinteger(L, ffloor->alpha);
		return 1;
	}
	return 0;
}

static int ffloor_set(lua_State *L)
{
	ffloor_t *ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	enum ffloor_e field = Lua_optoption(L, 2, ffloor_valid, ffloor_fields_ref);

	if (!ffloor)
		return luaL_error(L, "accessed ffloor_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter ffloor_t in HUD rendering code!");

	switch(field)
	{
	case ffloor_valid: // valid 
	case ffloor_tslope: // t_slope
	case ffloor_bslope: // b_slope
	case ffloor_sector: // sector
	case ffloor_master: // master
	case ffloor_target: // target
	case ffloor_next: // next
	case ffloor_prev: // prev
	default:
		return luaL_error(L, "ffloor_t field " LUA_QS " cannot be set.", ffloor_opt[field]);
	case ffloor_topheight: { // topheight
		boolean flag;
		fixed_t lastpos = *ffloor->topheight;
		mobj_t *ptmthing = tmthing;
		sector_t *sector = &sectors[ffloor->secnum];
		sector->ceilingheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			*ffloor->topheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case ffloor_toppic:
		*ffloor->toppic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case ffloor_toplightlevel:
		*ffloor->toplightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case ffloor_bottomheight: { // bottomheight
		boolean flag;
		fixed_t lastpos = *ffloor->bottomheight;
		mobj_t *ptmthing = tmthing;
		sector_t *sector = &sectors[ffloor->secnum];
		sector->floorheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			*ffloor->bottomheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_SetTarget(&tmthing, ptmthing);
		break;
	}
	case ffloor_bottompic:
		*ffloor->bottompic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case ffloor_flags: {
		ffloortype_e oldflags = ffloor->flags; // store FOF's old flags
		ffloor->flags = luaL_checkinteger(L, 3);
		if (ffloor->flags != oldflags)
			ffloor->target->moved = true; // reset target sector's lightlist
		break;
	}
	case ffloor_alpha:
		ffloor->alpha = (INT32)luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}


static int slope_get(lua_State *L)
{
	pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
	enum slope_e field = Lua_optoption(L, 2, slope_valid, slope_fields_ref);

	if (!slope)
	{
		if (field == slope_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed pslope_t doesn't exist anymore.");
	}

	switch(field)
	{
	case slope_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case slope_o: // o
		LUA_PushUserdata(L, &slope->o, META_VECTOR3);
		return 1;
	case slope_d: // d
		LUA_PushUserdata(L, &slope->d, META_VECTOR2);
		return 1;
	case slope_zdelta: // zdelta
		lua_pushfixed(L, slope->zdelta);
		return 1;
	case slope_normal: // normal
		LUA_PushUserdata(L, &slope->normal, META_VECTOR3);
		return 1;
	case slope_zangle: // zangle
		lua_pushangle(L, slope->zangle);
		return 1;
	case slope_xydirection: // xydirection
		lua_pushangle(L, slope->xydirection);
		return 1;
	case slope_sourceline: // source linedef
		LUA_PushUserdata(L, slope->sourceline, META_LINE);
		return 1;
	case slope_refpos: // refpos
		lua_pushinteger(L, slope->refpos);
		return 1;
	case slope_flags: // flags
		lua_pushinteger(L, slope->flags);
		return 1;
	}
	return 0;
}

static int slope_set(lua_State *L)
{
	pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
	enum slope_e field = Lua_optoption(L, 2, slope_valid, slope_fields_ref);

	if (!slope)
		return luaL_error(L, "accessed pslope_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter pslope_t in HUD rendering code!");

	switch(field) // todo: reorganize this shit
	{
	case slope_valid: // valid
	case slope_sourceline: // sourceline
	case slope_d: // d
	case slope_flags: // flags
	case slope_normal: // normal
	case slope_refpos: // refpos
	default:
		return luaL_error(L, "pslope_t field " LUA_QS " cannot be set.", slope_opt[field]);
	case slope_o: { // o
		luaL_checktype(L, 3, LUA_TTABLE);

		lua_getfield(L, 3, "x");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 1);
		}
		if (!lua_isnil(L, -1))
			slope->o.x = luaL_checkfixed(L, -1);
		else
			slope->o.x = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "y");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 2);
		}
		if (!lua_isnil(L, -1))
			slope->o.y = luaL_checkfixed(L, -1);
		else
			slope->o.y = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "z");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 3);
		}
		if (!lua_isnil(L, -1))
			slope->o.z = luaL_checkfixed(L, -1);
		else
			slope->o.z = 0;
		lua_pop(L, 1);
		break;
	}
	case slope_zdelta: { // zdelta, this is temp until i figure out wtf to do
		slope->zdelta = luaL_checkfixed(L, 3);
		slope->zangle = R_PointToAngle2(0, 0, FRACUNIT, -slope->zdelta);
		P_CalculateSlopeNormal(slope);
		break;
	}
	case slope_zangle: { // zangle
		angle_t zangle = luaL_checkangle(L, 3);
		if (zangle == ANGLE_90 || zangle == ANGLE_270)
			return luaL_error(L, "invalid zangle for slope!");
		slope->zangle = zangle;
		slope->zdelta = -FINETANGENT(((slope->zangle+ANGLE_90)>>ANGLETOFINESHIFT) & 4095);
		P_CalculateSlopeNormal(slope);
		break;
	}
	case slope_xydirection: // xydirection
		slope->xydirection = luaL_checkangle(L, 3);
		slope->d.x = -FINECOSINE((slope->xydirection>>ANGLETOFINESHIFT) & FINEMASK);
		slope->d.y = -FINESINE((slope->xydirection>>ANGLETOFINESHIFT) & FINEMASK);
		P_CalculateSlopeNormal(slope);
		break;
	}
	return 0;
}

static int vector2_get(lua_State *L)
{
	vector2_t *vec = *((vector2_t **)luaL_checkudata(L, 1, META_VECTOR2));
	enum vector_e field = Lua_optoption(L, 2, -1, vector_fields_ref);

	if (!vec)
		return luaL_error(L, "accessed vector2_t doesn't exist anymore.");

	switch(field)
	{
		case vector_x: lua_pushfixed(L, vec->x); return 1;
		case vector_y: lua_pushfixed(L, vec->y); return 1;
		default: break;
	}

	return 0;
}

static int vector3_get(lua_State *L)
{
	vector3_t *vec = *((vector3_t **)luaL_checkudata(L, 1, META_VECTOR3));
	enum vector_e field = Lua_optoption(L, 2, -1, vector_fields_ref);

	if (!vec)
		return luaL_error(L, "accessed vector3_t doesn't exist anymore.");

	switch(field)
	{
		case vector_x: lua_pushfixed(L, vec->x); return 1;
		case vector_y: lua_pushfixed(L, vec->y); return 1;
		case vector_z: lua_pushfixed(L, vec->z); return 1;
		default: break;
	}

	return 0;
}

static int lib_getMapheaderinfo(lua_State *L)
{
	// i -> mapheaderinfo[i-1]

	//int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1)-1;
		if (i >= NUMMAPS)
			return 0;
		LUA_PushUserdata(L, mapheaderinfo[i], META_MAPHEADER);
		//CONS_Printf(mapheaderinfo[i]->lvlttl);
		return 1;
	}/*
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSubsectors);
		return 1;
	}*/
	return 0;
}

static int lib_nummapheaders(lua_State *L)
{
	lua_pushinteger(L, NUMMAPS);
	return 1;
}

static int mapheaderinfo_get(lua_State *L)
{
	mapheader_t *header = *((mapheader_t **)luaL_checkudata(L, 1, META_MAPHEADER));
	enum mapheaderinfo_e field = Lua_optoption(L, 2, -1, mapheaderinfo_fields_ref);
	INT16 i;
	UINT8 j;
	switch (field)
	{
	case mapheaderinfo_lvlttl:
		lua_pushstring(L, header->lvlttl);
		break;
	case mapheaderinfo_subttl:
		lua_pushstring(L, header->subttl);
		break;
	case mapheaderinfo_actnum:
		lua_pushinteger(L, header->actnum);
		break;
	case mapheaderinfo_typeoflevel:
		lua_pushinteger(L, header->typeoflevel);
		break;
	case mapheaderinfo_nextlevel:
		lua_pushinteger(L, header->nextlevel);
		break;
	case mapheaderinfo_musname:
		lua_pushstring(L, header->musname);
		break;
	case mapheaderinfo_mustrack:
		lua_pushinteger(L, header->mustrack);
		break;
	case mapheaderinfo_muspos:
		lua_pushinteger(L, header->muspos);
		break;
	case mapheaderinfo_musinterfadeout:
		lua_pushinteger(L, header->musinterfadeout);
		break;
	case mapheaderinfo_musintername:
		lua_pushstring(L, header->musintername);
		break;
	case mapheaderinfo_forcecharacter:
		lua_pushstring(L, header->forcecharacter);
		break;
	case mapheaderinfo_weather:
		lua_pushinteger(L, header->weather);
		break;
	case mapheaderinfo_skynum:
		lua_pushinteger(L, header->skynum);
		break;
	case mapheaderinfo_skybox_scalex:
		lua_pushinteger(L, header->skybox_scalex);
		break;
	case mapheaderinfo_skybox_scaley:
		lua_pushinteger(L, header->skybox_scaley);
		break;
	case mapheaderinfo_skybox_scalez:
		lua_pushinteger(L, header->skybox_scalez);
		break;
	case mapheaderinfo_interscreen:
		for (i = 0; i < 8; i++)
			if (!header->interscreen[i])
				break;
		lua_pushlstring(L, header->interscreen, i);
		break;
	case mapheaderinfo_runsoc:
		lua_pushstring(L, header->runsoc);
		break;
	case mapheaderinfo_scriptname:
		lua_pushstring(L, header->scriptname);
		break;
	case mapheaderinfo_precutscenenum:
		lua_pushinteger(L, header->precutscenenum);
		break;
	case mapheaderinfo_cutscenenum:
		lua_pushinteger(L, header->cutscenenum);
		break;
	case mapheaderinfo_countdown:
		lua_pushinteger(L, header->countdown);
		break;
	case mapheaderinfo_palette:
		lua_pushinteger(L, header->palette);
		break;
	case mapheaderinfo_numlaps:
		lua_pushinteger(L, header->numlaps);
		break;
	case mapheaderinfo_unlockrequired:
		lua_pushinteger(L, header->unlockrequired);
		break;
	case mapheaderinfo_levelselect:
		lua_pushinteger(L, header->levelselect);
		break;
	case mapheaderinfo_bonustype:
		lua_pushinteger(L, header->bonustype);
		break;
	case mapheaderinfo_saveoverride:
		lua_pushinteger(L, header->saveoverride);
		break;
	case mapheaderinfo_levelflags:
		lua_pushinteger(L, header->levelflags);
		break;
	case mapheaderinfo_menuflags:
		lua_pushinteger(L, header->menuflags);
		break;
	// TODO add support for reading numGradedMares and grades
	default:
		// Read custom vars now
		// (note: don't include the "LUA." in your lua scripts!)
		for (j = 0; j < header->numCustomOptions && !fastcmp(lua_tostring(L, 2), header->customopts[j].option); ++j);

		if(j < header->numCustomOptions)
			lua_pushstring(L, header->customopts[j].value);
		else
			lua_pushnil(L);
	}
	return 1;
}

int LUA_MapLib(lua_State *L)
{
	luaL_newmetatable(L, META_SECTORLINES);
		lua_pushcfunction(L, sectorlines_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, sectorlines_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SECTOR);
		lua_pushcfunction(L, sector_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, sector_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, sector_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	sector_fields_ref = Lua_CreateFieldTable(L, sector_opt);

	luaL_newmetatable(L, META_SUBSECTOR);
		lua_pushcfunction(L, subsector_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, subsector_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	subsector_fields_ref = Lua_CreateFieldTable(L, subsector_opt);

	luaL_newmetatable(L, META_LINE);
		lua_pushcfunction(L, line_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, line_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	line_fields_ref = Lua_CreateFieldTable(L, line_opt);

	luaL_newmetatable(L, META_SIDENUM);
		lua_pushcfunction(L, sidenum_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SIDE);
		lua_pushcfunction(L, side_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, side_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, side_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	side_fields_ref = Lua_CreateFieldTable(L, side_opt);

	luaL_newmetatable(L, META_VERTEX);
		lua_pushcfunction(L, vertex_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, vertex_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	vertex_fields_ref = Lua_CreateFieldTable(L, vertex_opt);

	luaL_newmetatable(L, META_FFLOOR);
		lua_pushcfunction(L, ffloor_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, ffloor_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L, 1);

	ffloor_fields_ref = Lua_CreateFieldTable(L, ffloor_opt);


	luaL_newmetatable(L, META_SLOPE);
		lua_pushcfunction(L, slope_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, slope_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L, 1);

	slope_fields_ref = Lua_CreateFieldTable(L, slope_opt);

	luaL_newmetatable(L, META_VECTOR2);
		lua_pushcfunction(L, vector2_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_VECTOR3);
		lua_pushcfunction(L, vector3_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	vector_fields_ref = Lua_CreateFieldTable(L, vector_opt);

	luaL_newmetatable(L, META_MAPHEADER);
		lua_pushcfunction(L, mapheaderinfo_get);
		lua_setfield(L, -2, "__index");

		//lua_pushcfunction(L, mapheaderinfo_num);
		//lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	mapheaderinfo_fields_ref = Lua_CreateFieldTable(L, mapheaderinfo_opt);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSector);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsectors);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "sectors");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSubsector);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsubsectors);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "subsectors");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getLine);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numlines);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "lines");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSide);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsides);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "sides");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getVertex);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numvertexes);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "vertexes");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getMapheaderinfo);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_nummapheaders);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "mapheaderinfo");
	return 0;
}
