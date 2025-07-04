// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hudlib.c
/// \brief custom HUD rendering library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "r_defs.h"
#include "r_local.h"
#include "st_stuff.h" // hudinfo[]
#include "g_game.h"
#include "i_video.h" // rendermode
#include "p_local.h" // camera_t
#include "screen.h" // screen width/height
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h"

#define HUDONLY if (!hud_running) return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

boolean hud_running = false;
static UINT8 hud_enabled[(hud_MAX/8)+1];

static UINT8 hudAvailable; // hud hooks field

// must match enum hud in lua_hud.h
static const char *const hud_disable_options[] = {
	"stagetitle",
	"textspectator",

	"score",
	"time",
	"rings",
	"lives",

	"weaponrings",
	"powerstones",

	"nightslink",
	"nightsdrill",
	"nightsrings",
	"nightsscore",
	"nightstime",
	"nightsrecords",

	"rankings",
	"coopemeralds",
	"tokens",
	"tabemblems",
	NULL};

enum hudinfo {
	hudinfo_x = 0,
	hudinfo_y
};

static const char *const hudinfo_opt[] = {
	"x",
	"y",
	NULL};

enum patch {
	patch_valid = 0,
	patch_width,
	patch_height,
	patch_leftoffset,
	patch_topoffset
};
static const char *const patch_opt[] = {
	"valid",
	"width",
	"height",
	"leftoffset",
	"topoffset",
	NULL};

static int patch_fields_ref = LUA_NOREF;

enum hudhook {
	hudhook_game = 0,
	hudhook_scores
};
static const char *const hudhook_opt[] = {
	"game",
	"scores",
	NULL};

// alignment types for v.drawString
enum align {
	align_left = 0,
	align_center,
	align_right,
	align_fixed,
	align_fixedcenter,
	align_fixedright,
	align_small,
	align_smallfixed,
	align_smallfixedcenter,
	align_smallfixedright,
	align_smallcenter,
	align_smallright,
	align_smallthin,
	align_smallthincenter,
	align_smallthinright,
	align_smallthinfixed,
	align_smallthinfixedcenter,
	align_smallthinfixedright,
	align_thin,
	align_thinfixed,
	align_thinfixedcenter,
	align_thinfixedright,
	align_thincenter,
	align_thinright
};
static const char *const align_opt[] = {
	"left",
	"center",
	"right",
	"fixed",
	"fixed-center",
	"fixed-right",
	"small",
	"small-fixed",
	"small-fixed-center",
	"small-fixed-right",
	"small-center",
	"small-right",
	"small-thin",
	"small-thin-center",
	"small-thin-right",
	"small-thin-fixed",
	"small-thin-fixed-center",
	"small-thin-fixed-right",
	"thin",
	"thin-fixed",
	"thin-fixed-center",
	"thin-fixed-right",
	"thin-center",
	"thin-right",
	NULL};

// width types for v.stringWidth
enum widtht {
	widtht_normal = 0,
	widtht_small,
	widtht_thin
};
static const char *const widtht_opt[] = {
	"normal",
	"small",
	"thin",
	NULL};

enum cameraf {
	camera_chase = 0,
	camera_aiming,
	camera_x,
	camera_y,
	camera_z,
	camera_angle,
	camera_subsector,
	camera_floorz,
	camera_ceilingz,
	camera_radius,
	camera_height,
	camera_momx,
	camera_momy,
	camera_momz
};


static const char *const camera_opt[] = {
	"chase",
	"aiming",
	"x",
	"y",
	"z",
	"angle",
	"subsector",
	"floorz",
	"ceilingz",
	"radius",
	"height",
	"momx",
	"momy",
	"momz",
	NULL};

static int camera_fields_ref = LUA_NOREF;

static int lib_getHudInfo(lua_State *L)
{
	UINT32 i;
	lua_remove(L, 1);

	i = luaL_checkinteger(L, 1);
	if (i >= NUMHUDITEMS)
		return luaL_error(L, "hudinfo[] index %d out of range (0 - %d)", i, NUMHUDITEMS-1);
	LUA_PushUserdata(L, &hudinfo[i], META_HUDINFO);
	return 1;
}

static int lib_hudinfolen(lua_State *L)
{
	lua_pushinteger(L, NUMHUDITEMS);
	return 1;
}

static int hudinfo_get(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	enum hudinfo field = luaL_checkoption(L, 2, hudinfo_opt[0], hudinfo_opt);
	I_Assert(info != NULL); // huditems are always valid

	switch(field)
	{
	case hudinfo_x:
		lua_pushinteger(L, info->x);
		break;
	case hudinfo_y:
		lua_pushinteger(L, info->y);
		break;
	}
	return 1;
}

static int hudinfo_set(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	enum hudinfo field = luaL_checkoption(L, 2, hudinfo_opt[0], hudinfo_opt);
	I_Assert(info != NULL);

	switch(field)
	{
	case hudinfo_x:
		info->x = (INT32)luaL_checkinteger(L, 3);
		break;
	case hudinfo_y:
		info->y = (INT32)luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

static int hudinfo_num(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	lua_pushinteger(L, info-hudinfo);
	return 1;
}

static int colormap_get(lua_State *L)
{
	return luaL_error(L, "colormap is not a struct.");
}

static int patch_get(lua_State *L)
{
	patch_t *patch = *((patch_t **)luaL_checkudata(L, 1, META_PATCH));
	enum patch field = Lua_optoption(L, 2, -1, patch_fields_ref);

	// patches are CURRENTLY always valid, expected to be cached with PU_STATIC
	// this may change in the future, so patch.valid still exists
	I_Assert(patch != NULL);

	switch (field)
	{
	case patch_valid:
		lua_pushboolean(L, patch != NULL);
		break;
	case patch_width:
		lua_pushinteger(L, SHORT(patch->width));
		break;
	case patch_height:
		lua_pushinteger(L, SHORT(patch->height));
		break;
	case patch_leftoffset:
		lua_pushinteger(L, SHORT(patch->leftoffset));
		break;
	case patch_topoffset:
		lua_pushinteger(L, SHORT(patch->topoffset));
		break;
	}
	return 1;
}

static int patch_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("patch_t") " struct cannot be edited by Lua.");
}

static int camera_get(lua_State *L)
{
	camera_t *cam = *((camera_t **)luaL_checkudata(L, 1, META_CAMERA));
	enum cameraf field = Lua_optoption(L, 2, -1, camera_fields_ref);

	// cameras should always be valid unless I'm a nutter
	I_Assert(cam != NULL);

	switch (field)
	{
	case camera_chase:
		lua_pushboolean(L, cam->chase);
		break;
	case camera_aiming:
		lua_pushinteger(L, cam->aiming);
		break;
	case camera_x:
		lua_pushinteger(L, cam->x);
		break;
	case camera_y:
		lua_pushinteger(L, cam->y);
		break;
	case camera_z:
		lua_pushinteger(L, cam->z);
		break;
	case camera_angle:
		lua_pushinteger(L, cam->angle);
		break;
	case camera_subsector:
		LUA_PushUserdata(L, cam->subsector, META_SUBSECTOR);
		break;
	case camera_floorz:
		lua_pushinteger(L, cam->floorz);
		break;
	case camera_ceilingz:
		lua_pushinteger(L, cam->ceilingz);
		break;
	case camera_radius:
		lua_pushinteger(L, cam->radius);
		break;
	case camera_height:
		lua_pushinteger(L, cam->height);
		break;
	case camera_momx:
		lua_pushinteger(L, cam->momx);
		break;
	case camera_momy:
		lua_pushinteger(L, cam->momy);
		break;
	case camera_momz:
		lua_pushinteger(L, cam->momz);
		break;
	}
	return 1;
}

//
// lib_draw
//

static int libd_patchExists(lua_State *L)
{
	HUDONLY
	lua_pushboolean(L, W_LumpExists(luaL_checkstring(L, 1)));
	return 1;
}

static int libd_cachePatch(lua_State *L)
{
	HUDONLY
	LUA_PushUserdata(L, W_CachePatchName(luaL_checkstring(L, 1), PU_STATIC), META_PATCH);
	return 1;
}

static int libd_getSpritePatch(lua_State *L)
{
	UINT32 i; // sprite prefix
	INT32 skn = -1; // skin number
	UINT32 frame = 0; // 'A'
	UINT8 angle = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;

	HUDONLY

	if (!lua_isnoneornil(L, 1)) // ONLY do this if we have a skin
	{
		if (lua_isnumber(L, 1)) // find skin by number
		{
			skn = lua_tonumber(L, 1);
			if (skn < 0 || skn >= MAXSKINS)
				return luaL_error(L, "skin number %d out of range (0 - %d)", skn, MAXSKINS-1);
			if (skn >= numskins)
				return 0;
		}
		else // find skin by name
		{
			const char *name = luaL_checkstring(L, 1);
			for (skn = 0; skn < numskins; skn++)
				if (fastcmp(skins[skn].name, name))
					break;
			if (skn >= numskins)
				return 0;
		}
	}

	lua_remove(L, 1); // remove skin now

	if (lua_isnumber(L, 1)) // sprite number given, e.g. SPR_THOK
	{
		i = lua_tonumber(L, 1);
		if (i >= NUMSPRITES)
			return 0;
	}
	else if (lua_isstring(L, 1)) // sprite prefix name given, e.g. "THOK"
	{
		const char *name = lua_tostring(L, 1);
		for (i = 0; i < NUMSPRITES; i++)
			if (fastcmp(name, sprnames[i]))
				break;
		if (i >= NUMSPRITES)
			return 0;
	}
	else
		return 0;

	// get outta dodge if we're a playersprite and have no skin
	if ((i == SPR_PLAY) && (skn < 0))
		return luaL_error(L, "You must provide a skin for player sprites!");

	if (skn < 0) // standard sprite
	{
		sprdef = &sprites[i];
	}
	else // player skin
	{
		sprdef = &skins[skn].spritedef;
	}

	// set frame number
	frame = luaL_optinteger(L, 2, 0);
	frame &= FF_FRAMEMASK; // ignore any bits that are not the actual frame, just in case
	if (frame >= sprdef->numframes)
		return 0;
	// set angle number
	sprframe = &sprdef->spriteframes[frame];
	angle = luaL_optinteger(L, 3, 1);

	// convert WAD editor angle numbers (1-8) to internal angle numbers (0-7)
	// keep 0 the same since we'll make it default to angle 1 (which is internally 0)
	// in case somebody didn't know that angle 0 really just maps all 8 angles to the same patch
	if (angle != 0)
		angle--;

	if (angle >= 8) // out of range?
		angle = (angle & 7); // modulus angle by 8

	// push both the patch and its "flip" value
	LUA_PushUserdata(L, W_CachePatchNum(sprframe->lumppat[angle], PU_STATIC), META_PATCH);
	lua_pushboolean(L, (sprframe->flip & (1<<angle)) != 0);
	return 2;
}

static int libd_draw(lua_State *L)
{
	INT32 x, y, flags;
	patch_t *patch;
	const UINT8 *colormap = NULL;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	patch = *((patch_t **)luaL_checkudata(L, 3, META_PATCH));
	flags = luaL_optinteger(L, 4, 0);
	if (!lua_isnoneornil(L, 5))
		colormap = *((UINT8 **)luaL_checkudata(L, 5, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, flags, patch, colormap);
	return 0;
}

static int libd_drawScaled(lua_State *L)
{
	fixed_t x, y, scale;
	INT32 flags;
	patch_t *patch;
	const UINT8 *colormap = NULL;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	scale = luaL_checkinteger(L, 3);
	if (scale < 0)
		return luaL_error(L, "negative scale");
	patch = *((patch_t **)luaL_checkudata(L, 4, META_PATCH));
	flags = luaL_optinteger(L, 5, 0);
	if (!lua_isnoneornil(L, 6))
		colormap = *((UINT8 **)luaL_checkudata(L, 6, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawFixedPatch(x, y, scale, flags, patch, colormap);
	return 0;
}

static int libd_drawStretched(lua_State *L)
{
	fixed_t x, y, hscale, vscale;
	INT32 flags;
	patch_t *patch;
	const UINT8 *colormap = NULL;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	hscale = luaL_checkinteger(L, 3);
	if (hscale < 0)
		return luaL_error(L, "negative horizontal scale");
	vscale = luaL_checkinteger(L, 4);
	if (vscale < 0)
		return luaL_error(L, "negative vertical scale");
	patch = *((patch_t **)luaL_checkudata(L, 5, META_PATCH));
	flags = luaL_optinteger(L, 6, 0);
	if (!lua_isnoneornil(L, 7))
		colormap = *((UINT8 **)luaL_checkudata(L, 7, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawStretchyFixedPatch(x, y, hscale, vscale, flags, patch, colormap);
	return 0;
}

static int libd_drawNum(lua_State *L)
{
	INT32 x, y, flags, num;
	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = luaL_checkinteger(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawTallNum(x, y, flags, num);
	return 0;
}

static int libd_drawPaddedNum(lua_State *L)
{
	INT32 x, y, flags, num, digits;
	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = labs(luaL_checkinteger(L, 3));
	digits = luaL_optinteger(L, 4, 2);
	flags = luaL_optinteger(L, 5, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawPaddedTallNum(x, y, flags, num, digits);
	return 0;
}

static int libd_drawFill(lua_State *L)
{
	INT32 x = luaL_optinteger(L, 1, 0);
	INT32 y = luaL_optinteger(L, 2, 0);
	INT32 w = luaL_optinteger(L, 3, BASEVIDWIDTH);
	INT32 h = luaL_optinteger(L, 4, BASEVIDHEIGHT);
	INT32 c = luaL_optinteger(L, 5, 31);

	HUDONLY
	V_DrawFill(x, y, w, h, c);
	return 0;
}

static int libd_drawString(lua_State *L)
{
	fixed_t x = luaL_checkinteger(L, 1);
	fixed_t y = luaL_checkinteger(L, 2);
	const char *str = luaL_checkstring(L, 3);
	INT32 flags = luaL_optinteger(L, 4, V_ALLOWLOWERCASE);
	enum align align = luaL_checkoption(L, 5, "left", align_opt);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	HUDONLY
	switch(align)
	{
	// hu_font
	case align_left:
		V_DrawString(x, y, flags, str);
		break;
	case align_center:
		V_DrawCenteredString(x, y, flags, str);
		break;
	case align_right:
		V_DrawRightAlignedString(x, y, flags, str);
		break;
	case align_fixed:
		V_DrawStringAtFixed(x, y, flags, str);
		break;
	case align_fixedcenter:
		V_DrawCenteredStringAtFixed(x, y, flags, str);
		break;
	case align_fixedright:
		V_DrawRightAlignedStringAtFixed(x, y, flags, str);
		break;
	// hu_font, 0.5x scale
	case align_small:
		V_DrawSmallString(x, y, flags, str);
		break;
	case align_smallfixed:
		V_DrawSmallStringAtFixed(x, y, flags, str);
		break;
	case align_smallfixedcenter:
		V_DrawCenteredSmallStringAtFixed(x, y, flags, str);
		break;
	case align_smallfixedright:
		V_DrawRightAlignedSmallStringAtFixed(x, y, flags, str);
		break;
	case align_smallcenter:
		V_DrawCenteredSmallString(x, y, flags, str);
		break;
	case align_smallright:
		V_DrawRightAlignedSmallString(x, y, flags, str);
		break;
	case align_smallthin:
		V_DrawSmallThinString(x, y, flags, str);
		break;
	case align_smallthincenter:
		V_DrawCenteredSmallThinString(x, y, flags, str);
		break;
	case align_smallthinright:
		V_DrawRightAlignedSmallThinString(x, y, flags, str);
		break;
	case align_smallthinfixed:
		V_DrawSmallThinStringAtFixed(x, y, flags, str);
		break;
	case align_smallthinfixedcenter:
		V_DrawCenteredSmallThinStringAtFixed(x, y, flags, str);
		break;
	case align_smallthinfixedright:
		V_DrawRightAlignedSmallThinStringAtFixed(x, y, flags, str);
		break;
	// tny_font
	case align_thin:
		V_DrawThinString(x, y, flags, str);
		break;
	case align_thincenter:
		V_DrawCenteredThinString(x, y, flags, str);
		break;
	case align_thinright:
		V_DrawRightAlignedThinString(x, y, flags, str);
		break;
	case align_thinfixed:
		V_DrawThinStringAtFixed(x, y, flags, str);
		break;
	case align_thinfixedcenter:
		V_DrawCenteredThinStringAtFixed(x, y, flags, str);
		break;
	case align_thinfixedright:
		V_DrawRightAlignedThinStringAtFixed(x, y, flags, str);
		break;
	}
	return 0;
}

static int libd_stringWidth(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	INT32 flags = luaL_optinteger(L, 2, V_ALLOWLOWERCASE);
	enum widtht widtht = luaL_checkoption(L, 3, "normal", widtht_opt);

	HUDONLY
	switch(widtht)
	{
	case widtht_normal: // hu_font
		lua_pushinteger(L, V_StringWidth(str, flags));
		break;
	case widtht_small: // hu_font, 0.5x scale
		lua_pushinteger(L, V_SmallStringWidth(str, flags));
		break;
	case widtht_thin: // tny_font
		lua_pushinteger(L, V_ThinStringWidth(str, flags));
		break;
	}
	return 1;
}

static int libd_getColormap(lua_State *L)
{
	INT32 skinnum = TC_DEFAULT;
	skincolornum_t color = luaL_optinteger(L, 2, 0);
	UINT8* colormap = NULL;
	HUDONLY
	if (lua_isnoneornil(L, 1))
		; // defaults to TC_DEFAULT
	else if (lua_type(L, 1) == LUA_TNUMBER) // skin number
	{
		skinnum = (INT32)luaL_checkinteger(L, 1);
		if (skinnum < TC_ALLWHITE || skinnum >= MAXSKINS)
			return luaL_error(L, "skin number %d is out of range (%d - %d)", skinnum, TC_ALLWHITE, MAXSKINS-1);
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 1);
		INT32 i = R_SkinAvailable(skinname);
		if (i != -1) // if -1, just default to TC_DEFAULT as above
			skinnum = i;
	}

	// all was successful above, now we generate the colormap at last!

	colormap = R_GetTranslationColormap(skinnum, color, GTC_CACHE);
	LUA_PushUserdata(L, colormap, META_COLORMAP); // push as META_COLORMAP userdata, specifically for patches to use!
	return 1;
}

static int libd_fadeScreen(lua_State *L)
{
	UINT16 color = luaL_checkinteger(L, 1);
	UINT8 strength = luaL_checkinteger(L, 2);
	const UINT8 maxstrength = ((color & 0xFF00) ? 31 : 9);

	HUDONLY
	if (!strength)
		return 0;

	if (strength > maxstrength)
		return luaL_error(L, "%s fade strength %d out of range (0 - %d)", ((color & 0xFF00) ? "COLORMAP" : "TRANSMAP"), strength, maxstrength);

	V_DrawFadeScreen(color, strength);
	return 0;
}

static int libd_width(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.width); // push screen width
	return 1;
}

static int libd_height(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.height); // push screen height
	return 1;
}

static int libd_dupx(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.dupx); // push integral scale (patch scale)
	lua_pushfixed(L, vid.fdupx); // push fixed point scale (position scale)
	return 2;
}

static int libd_dupy(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.dupy); // push integral scale (patch scale)
	lua_pushfixed(L, vid.fdupy); // push fixed point scale (position scale)
	return 2;
}

static int libd_renderer(lua_State *L)
{
	HUDONLY
	switch (rendermode) {
		case render_opengl: lua_pushliteral(L, "opengl");   break; // OpenGL renderer
		case render_soft:   lua_pushliteral(L, "software"); break; // Software renderer
		default:            lua_pushliteral(L, "none");     break; // render_none (for dedicated), in case there's any reason this should be run
	}
	return 1;
}

// 30/10/18 Lat': Get cv_translucenthud's value for HUD rendering as a normal V_xxTRANS int
// Could as well be thrown in global vars for ease of access but I guess it makes sense for it to be a HUD fn
static int libd_getlocaltransflag(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, (10-cv_translucenthud.value)*V_10TRANS);	// A bit weird that it's called "translucenthud" yet 10 is fully opaque :V
	return 1;
}

static luaL_Reg lib_draw[] = {
	{"patchExists", libd_patchExists},
	{"cachePatch", libd_cachePatch},
	{"draw", libd_draw},
	{"drawScaled", libd_drawScaled},
	{"drawStretched", libd_drawStretched},
	{"drawNum", libd_drawNum},
	{"drawPaddedNum", libd_drawPaddedNum},
	{"drawFill", libd_drawFill},
	{"getSpritePatch",libd_getSpritePatch},
	{"drawString", libd_drawString},
	{"stringWidth", libd_stringWidth},
	{"getColormap", libd_getColormap},
	{"fadeScreen", libd_fadeScreen},
	{"width", libd_width},
	{"height", libd_height},
	{"dupx", libd_dupx},
	{"dupy", libd_dupy},
	{"renderer", libd_renderer},
	{"localTransFlag", libd_getlocaltransflag},
	{NULL, NULL}
};

//
// lib_hud
//

// enable vanilla HUD element
static int lib_hudenable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] |= 1<<(option%8);
	return 0;
}

// disable vanilla HUD element
static int lib_huddisable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] &= ~(1<<(option%8));
	return 0;
}

// 30/10/18: Lat': How come this wasn't here before?
static int lib_hudenabled(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	if (hud_enabled[option/8] & (1<<(option%8)))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}


// add a HUD element for rendering
static int lib_hudadd(lua_State *L)
{
	enum hudhook field;

	luaL_checktype(L, 1, LUA_TFUNCTION);
	field = luaL_checkoption(L, 2, "game", hudhook_opt);

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(L, -1));
	lua_rawgeti(L, -1, field+2); // HUD[2+]
	I_Assert(lua_istable(L, -1));
	lua_remove(L, -2);

	lua_pushvalue(L, 1);
	lua_rawseti(L, -2, (int)(lua_objlen(L, -2) + 1));

	hudAvailable |= 1<<field;
	return 0;
}

static luaL_Reg lib_hud[] = {
	{"enable", lib_hudenable},
	{"disable", lib_huddisable},
	{"enabled", lib_hudenabled},
	{"add", lib_hudadd},
	{NULL, NULL}
};

//
//
//

int LUA_HudLib(lua_State *L)
{
	memset(hud_enabled, 0xff, (hud_MAX/8)+1);

	lua_newtable(L); // HUD registry table
		lua_newtable(L);
		luaL_register(L, NULL, lib_draw);
		lua_rawseti(L, -2, 1); // HUD[1] = lib_draw

		lua_newtable(L);
		lua_rawseti(L, -2, 2); // HUD[2] = game rendering functions array

		lua_newtable(L);
		lua_rawseti(L, -2, 3); // HUD[2] = scores rendering functions array
	lua_setfield(L, LUA_REGISTRYINDEX, "HUD");

	luaL_newmetatable(L, META_HUDINFO);
		lua_pushcfunction(L, hudinfo_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, hudinfo_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, hudinfo_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getHudInfo);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_hudinfolen);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "hudinfo");

	luaL_newmetatable(L, META_COLORMAP);
		lua_pushcfunction(L, colormap_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_PATCH);
		lua_pushcfunction(L, patch_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, patch_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	patch_fields_ref = Lua_CreateFieldTable(L, patch_opt);

	luaL_newmetatable(L, META_CAMERA);
		lua_pushcfunction(L, camera_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	camera_fields_ref = Lua_CreateFieldTable(L, camera_opt);

	luaL_register(L, "hud", lib_hud);
	return 0;
}

boolean LUA_HudEnabled(enum hud option)
{
	if (!gL || hud_enabled[option/8] & (1<<(option%8)))
		return true;
	return false;
}

// Hook for HUD rendering
void LUAh_GameHUD(player_t *stplayr)
{
	if (!gL || !(hudAvailable & (1<<hudhook_game)))
		return;

	hud_running = true;
	lua_pop(gL, -1);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 2); // HUD[2] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	LUA_PushUserdata(gL, stplayr, META_PLAYER);

	if (splitscreen && stplayr == &players[secondarydisplayplayer])
		LUA_PushUserdata(gL, &camera2, META_CAMERA);
	else
		LUA_PushUserdata(gL, &camera, META_CAMERA);

	lua_pushnil(gL);
	while (lua_next(gL, -5) != 0) {
		lua_pushvalue(gL, -5); // graphics library (HUD[1])
		lua_pushvalue(gL, -5); // stplayr
		lua_pushvalue(gL, -5); // camera
		LUA_Call(gL, 3);
	}
	lua_pop(gL, -1);
	hud_running = false;
}

void LUAh_ScoresHUD(void)
{
	if (!gL || !(hudAvailable & (1<<hudhook_scores)))
		return;

	hud_running = true;
	lua_pop(gL, -1);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 3); // HUD[3] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // graphics library (HUD[1])
		LUA_Call(gL, 1);
	}
	lua_pop(gL, -1);
	hud_running = false;
}
