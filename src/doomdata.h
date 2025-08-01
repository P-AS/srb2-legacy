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
/// \file  doomdata.h
/// \brief all external data is defined here
///
///        most of the data is loaded into different structures at run time
///        some internal structures shared by many modules are here

#ifndef __DOOMDATA__
#define __DOOMDATA__

// The most basic types we use, portability.
#include "doomtype.h"

// Some global defines, that configure the game.
#include "doomdef.h"

//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
	ML_LABEL,    // A separator, name, MAPxx
	ML_THINGS,   // Enemies, rings, monitors, scenery, etc.
	ML_LINEDEFS, // Linedefs, from editing
	ML_SIDEDEFS, // Sidedefs, from editing
	ML_VERTEXES, // Vertices, edited and BSP splits generated
	ML_SEGS,     // Linesegs, from linedefs split by BSP
	ML_SSECTORS, // Subsectors, list of linesegs
	ML_NODES,    // BSP nodes
	ML_SECTORS,  // Sectors, from editing
	ML_REJECT,    // LUT, sector-sector visibility
	ML_BLOCKMAP,  // LUT, motion clipping, walls/grid element
};

// Reverse gravity flag for objects.
#define MTF_OBJECTFLIP 2

// Special flag used with certain objects.
#define MTF_OBJECTSPECIAL 4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH 8

// Do not use bit five or after, as they are used for object z-offsets.

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// A single Vertex.
typedef struct
{
	INT16 x, y;
}ATTRPACK  mapvertex_t;

// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef struct
{
	INT16 textureoffset, rowoffset;
	char toptexture[8], bottomtexture[8], midtexture[8];
	// Front sector, towards viewer.
	INT16 sector;
} ATTRPACK mapsidedef_t;

// A LineDef, as used for editing, and as input
// to the BSP builder.
typedef struct
{
	INT16 v1, v2;
	INT16 flags;
	INT16 special;
	INT16 tag;
	// sidenum[1] will be 0xffff if one sided
	UINT16 sidenum[2];
} ATTRPACK maplinedef_t;

//
// LineDef attributes.
//

// Solid, is an obstacle.
#define ML_IMPASSIBLE           1

// Blocks monsters only.
#define ML_BLOCKMONSTERS        2

// Backside will not be present at all if not two sided.
#define ML_TWOSIDED             4

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures allways have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

// upper texture unpegged
#define ML_DONTPEGTOP           8

// lower texture unpegged
#define ML_DONTPEGBOTTOM       16

#define ML_EFFECT1             32

// Don't let Knuckles climb on this line
#define ML_NOCLIMB             64

#define ML_EFFECT2             128
#define ML_EFFECT3             256
#define ML_EFFECT4             512
#define ML_EFFECT5            1024

// New ones to disable lines for characters
#define ML_NOSONIC           2048
#define ML_NOTAILS           4096
#define ML_NOKNUX            8192
#define ML_NETONLY          14336 // all of the above

// Bounce off walls!
#define ML_BOUNCY           16384

#define ML_TFERLINE         32768

// Sector definition, from editing.
typedef struct
{
	INT16 floorheight;
	INT16 ceilingheight;
	char floorpic[8];
	char ceilingpic[8];
	INT16 lightlevel;
	INT16 special;
	INT16 tag;
} ATTRPACK mapsector_t;

// SubSector, as generated by BSP.
typedef struct
{
	UINT16 numsegs;
	// Index of first one, segs are stored sequentially.
	UINT16 firstseg;
} ATTRPACK mapsubsector_t;


// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
typedef struct
{
	INT16 v1, v2;
	INT16 angle;
	INT16 linedef;
	INT16 side;
	INT16 offset;
} ATTRPACK mapseg_t;

// BSP node structure.

// Indicate a leaf.
#define NF_SUBSECTOR 0x8000

typedef struct
{
	// Partition line from (x,y) to x+dx,y+dy)
	INT16 x, y;
	INT16 dx, dy;

	// Bounding box for each child, clip against view frustum.
	INT16 bbox[2][4];

	// If NF_SUBSECTOR it's a subsector, else it's a node of another subtree.
	UINT16 children[2];
} ATTRPACK mapnode_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

// Thing definition, position, orientation and type,
// plus visibility flags and attributes.
typedef struct
{
	INT16 x, y;
	INT16 angle;
	UINT16 type;
	UINT16 options;
	INT16 z;
	UINT8 extrainfo;
	struct mobj_s *mobj;
} mapthing_t;

#define ZSHIFT 4

#define NUMMAPS 1035

#endif // __DOOMDATA__
