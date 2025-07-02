// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_glob.h
/// \brief globals (shared data & code) for hw_ modules

#ifndef _HWR_GLOB_H_
#define _HWR_GLOB_H_

#include "hw_defs.h"
#include "hw_main.h"
#include "../m_misc.h"

// the original aspect ratio of Doom graphics isn't square
#define ORIGINAL_ASPECT (320.0f/200.0f)


#define HWR_LOADING_SCREEN

// -----------
// structures
// -----------

// a vertex of a Doom 'plane' polygon
typedef struct
{
	float x;
	float y;
	float z;
} polyvertex_t;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// a convex 'plane' polygon, clockwise order
typedef struct
{
	INT32 numpts;
	polyvertex_t pts[0];
} poly_t;

#ifdef _MSC_VER
#pragma warning(default :  4200)
#endif

// holds extra info for 3D render, for each subsector in subsectors[]
typedef struct
{
	poly_t *planepoly;  // the generated convex polygon
} extrasubsector_t;

// needed for sprite rendering
// equivalent of the software renderer's vissprites
typedef struct gr_vissprite_s
{
	// Doubly linked list
	struct gr_vissprite_s *prev;
	struct gr_vissprite_s *next;
	float x1, x2;
	float tz, ty;
	lumpnum_t patchlumpnum;
	boolean flip;
	UINT8 translucency;       //alpha level 0-255
	mobj_t *mobj;
	float scale;
	boolean precip; // Tails 08-25-2002
	boolean vflip;
   //Hurdler: 25/04/2000: now support colormap in hardware mode
	UINT8 *colormap;
	INT32 dispoffset; // copy of info->dispoffset, affects ordering but not drawing
	float z1, z2;
} gr_vissprite_t;

// --------
// hw_bsp.c
// --------
extern extrasubsector_t *extrasubsectors;
extern size_t addsubsector;

void HWR_InitPolyPool(void);
void HWR_FreePolyPool(void);

// --------
// hw_cache.c
// --------
void HWR_InitTextureCache(void);
void HWR_FreeTextureCache(void);
void HWR_FreeExtraSubsectors(void);

void HWR_GetFlat(lumpnum_t flatlumpnum);
GLTexture_t *HWR_GetTexture(INT32 tex);
void HWR_GetPatch(GLPatch_t *gpatch);
void HWR_GetMappedPatch(GLPatch_t *gpatch, const UINT8 *colormap);
void HWR_MakePatch(patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, boolean makebitmap);
void HWR_UnlockCachedPatch(GLPatch_t *gpatch);
void HWR_SetPalette(RGBA_t *palette);
GLPatch_t *HWR_GetCachedGLPatchPwad(UINT16 wad, UINT16 lump);
GLPatch_t *HWR_GetCachedGLPatch(lumpnum_t lumpnum);
void HWR_GetFadeMask(lumpnum_t fademasklumpnum);



// --------
// hw_draw.c
// --------
extern consvar_t cv_grrounddown; // on/off

extern INT32 patchformat;
extern INT32 textureformat;

#endif //_HW_GLOB_
