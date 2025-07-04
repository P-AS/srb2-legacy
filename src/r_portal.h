// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_portal.h
/// \brief Software renderer portal struct, functions, linked list extern.

#include "r_data.h"


/** Portal structure.
 */
typedef struct portal_s
{
	struct portal_s *next;

	// Viewport.
	fixed_t viewx;
	fixed_t viewy;
	fixed_t viewz;
	angle_t viewangle;

	UINT8 pass;			/**< Keeps track of the portal's recursion depth. */
	INT32 clipline;		/**< Optional clipline for line-based portals. */

	// Clipping information.
	INT32 start;		/**< First horizontal pixel coordinate to draw at. */
	INT32 end;			/**< Last horizontal pixel coordinate to draw at. */
	INT16 *ceilingclip; /**< Temporary screen top clipping array. */
	INT16 *floorclip;	/**< Temporary screen bottom clipping array. */
	fixed_t *frontscale;/**< Temporary screen bottom clipping array. */
} portal_t;

extern portal_t* portal_base;
extern portal_t* portal_cap;
extern UINT8 portalrender;

void Portal_InitList	(void);
void Portal_Remove		(portal_t* portal);
void Portal_Add2Lines	(const INT32 line1, const INT32 line2, const INT32 x1, const INT32 x2);

void Portal_ClipStoreFromRange (portal_t* portal);
void Portal_ClipApply (const portal_t* portal);
