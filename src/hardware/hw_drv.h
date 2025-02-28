// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief imports/exports for the 3D hardware low-level interface API

#ifndef __HWR_DRV_H__
#define __HWR_DRV_H__

// this must be here 19991024 by Kin
#include "../screen.h"
#include "hw_data.h"
#include "hw_defs.h"
#include "hw_md2.h"

#include "hw_dll.h"

// ==========================================================================
//                                                       STANDARD DLL EXPORTS
// ==========================================================================

EXPORT boolean HWRAPI(Init) (void);
#ifndef HAVE_SDL
EXPORT void HWRAPI(Shutdown) (void);
#endif
#ifdef _WINDOWS
EXPORT void HWRAPI(GetModeList) (vmode_t **pvidmodes, INT32 *numvidmodes);
#endif
#if defined (PURESDL) || defined (macintosh)
EXPORT void HWRAPI(SetPalette) (INT32 *, RGBA_t *gamma);
#else
EXPORT void HWRAPI(SetPalette) (RGBA_t *ppal, RGBA_t *pgamma);
#endif
EXPORT void HWRAPI(FinishUpdate) (INT32 waitvbl);
EXPORT void HWRAPI(Draw2DLine) (F2DCoord *v1, F2DCoord *v2, RGBA_t Color);
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags);
EXPORT void HWRAPI(RenderSkyDome) (INT32 tex, INT32 texture_width, INT32 texture_height, FTransform transform);
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags);
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor);
EXPORT void HWRAPI(SetTexture) (FTextureInfo *TexInfo);
EXPORT void HWRAPI(ReadRect) (INT32 x, INT32 y, INT32 width, INT32 height, INT32 dst_stride, UINT16 *dst_data);
EXPORT void HWRAPI(GClipRect) (INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip);
EXPORT void HWRAPI(ClearMipMapCache) (void);

//Hurdler: added for backward compatibility
EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, INT32 Value);

//Hurdler: added for new development
EXPORT void HWRAPI(DrawModel) (model_t *model, INT32 frameIndex, INT32 duration, INT32 tics, INT32 nextFrameIndex, FTransform *pos, float scale, UINT8 flipped, FSurfaceInfo *Surface);
EXPORT void HWRAPI(CreateModelVBOs) (model_t *model);
EXPORT void HWRAPI(SetTransform) (FTransform *ptransform);
EXPORT INT32 HWRAPI(GetTextureUsed) (void);



EXPORT void HWRAPI(FlushScreenTextures) (void);
EXPORT void HWRAPI(StartScreenWipe) (void);
EXPORT void HWRAPI(EndScreenWipe) (void);
EXPORT void HWRAPI(DoScreenWipe) (void);
EXPORT void HWRAPI(DrawIntermissionBG) (void);
EXPORT void HWRAPI(MakeScreenTexture) (void);
EXPORT void HWRAPI(MakeScreenFinalTexture) (void);
EXPORT void HWRAPI(DrawScreenFinalTexture) (int width, int height);

#define SCREENVERTS 10
EXPORT void HWRAPI(PostImgRedraw) (float points[SCREENVERTS][SCREENVERTS][2]);

// jimita
EXPORT void HWRAPI(LoadShaders) (void);
EXPORT void HWRAPI(KillShaders) (void);
EXPORT void HWRAPI(SetShader) (int shader);
EXPORT void HWRAPI(UnSetShader) (void);

EXPORT void HWRAPI(SetShaderInfo) (hwdshaderinfo_t info, INT32 value);
EXPORT void HWRAPI(LoadCustomShader) (int number, char *shader, size_t size, boolean fragment);
EXPORT void HWRAPI(InitCustomShaders) (void);

// ==========================================================================
//                                      HWR DRIVER OBJECT, FOR CLIENT PROGRAM
// ==========================================================================

#if !defined (_CREATE_DLL_)

struct hwdriver_s
{
	Init                pfnInit;
	SetPalette          pfnSetPalette;
	FinishUpdate        pfnFinishUpdate;
	Draw2DLine          pfnDraw2DLine;
	DrawPolygon         pfnDrawPolygon;
	RenderSkyDome       pfnRenderSkyDome;
	SetBlend            pfnSetBlend;
	ClearBuffer         pfnClearBuffer;
	SetTexture          pfnSetTexture;
	ReadRect            pfnReadRect;
	GClipRect           pfnGClipRect;
	ClearMipMapCache    pfnClearMipMapCache;
	SetSpecialState     pfnSetSpecialState;//Hurdler: added for backward compatibility
	DrawModel           pfnDrawModel;
	CreateModelVBOs     pfnCreateModelVBOs;
	SetTransform        pfnSetTransform;
	GetTextureUsed      pfnGetTextureUsed;
#ifdef _WINDOWS
	GetModeList         pfnGetModeList;
#endif
#ifndef HAVE_SDL
	Shutdown            pfnShutdown;
#endif
	PostImgRedraw       pfnPostImgRedraw;
	FlushScreenTextures pfnFlushScreenTextures;
	StartScreenWipe     pfnStartScreenWipe;
	EndScreenWipe       pfnEndScreenWipe;
	DoScreenWipe        pfnDoScreenWipe;
	DrawIntermissionBG  pfnDrawIntermissionBG;
	MakeScreenTexture   pfnMakeScreenTexture;
	MakeScreenFinalTexture  pfnMakeScreenFinalTexture;
	DrawScreenFinalTexture  pfnDrawScreenFinalTexture;

	LoadShaders         pfnLoadShaders;
	KillShaders         pfnKillShaders;
	SetShader           pfnSetShader;
	UnSetShader         pfnUnSetShader;

	SetShaderInfo       pfnSetShaderInfo;
	LoadCustomShader    pfnLoadCustomShader;
	InitCustomShaders   pfnInitCustomShaders;
};

extern struct hwdriver_s hwdriver;

//Hurdler: 16/10/99: added for OpenGL gamma correction
//extern RGBA_t  gamma_correction;

#define HWD hwdriver

#endif //not defined _CREATE_DLL_

#endif //__HWR_DRV_H__

