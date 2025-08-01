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
/// \file  r_defs.h
/// \brief Refresh/rendering module, shared data struct definitions

#ifndef __R_DEFS__
#define __R_DEFS__

// Some more or less basic data types we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "p_mobj.h"

#include "screen.h" // MAXVIDWIDTH, MAXVIDHEIGHT

#define POLYOBJECTS

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
typedef struct
{
	INT32 first;
	INT32 last;
} cliprange_t;

// Silhouette, needed for clipping segs (mainly) and sprites representing things.
#define SIL_NONE   0
#define SIL_BOTTOM 1
#define SIL_TOP    2
#define SIL_BOTH   3

// This could be wider for >8 bit display.
// Indeed, true color support is possible precalculating 24bpp lightmap/colormap LUT
// from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef UINT8 lighttable_t;

// ExtraColormap type. Use for extra_colormaps from now on.
typedef struct
{
	UINT16 maskcolor, fadecolor;
	double maskamt;
	UINT16 fadestart, fadeend;
	INT32 fog;

	// rgba is used in hw mode for colored sector lighting
	INT32 rgba; // similar to maskcolor in sw mode
	INT32 fadergba; // The colour the colourmaps fade to

	lighttable_t *colormap;

#ifdef HWRENDER
	// The id of the hardware lighttable. Zero means it does not exist yet.
	UINT32 gl_lighttable_id;
#endif

} extracolormap_t;

//
// INTERNAL MAP TYPES used by play and refresh
//

/** Your plain vanilla vertex.
  */
typedef struct
{
	fixed_t x, y, z;
} vertex_t;

// Forward of linedefs, for sectors.
struct line_s;

/** Degenerate version of ::mobj_t, storing only a location.
  * Used for sound origins in sectors, hoop centers, and the like. Does not
  * handle sound from moving objects (doppler), because position is probably
  * just buffered, not updated.
  */
typedef struct
{
	thinker_t thinker; ///< Not used for anything.
	fixed_t x;         ///< X coordinate.
	fixed_t y;         ///< Y coordinate.
	fixed_t z;         ///< Z coordinate.
} degenmobj_t;


#include "p_polyobj.h"

// Store fake planes in a resizable array insted of just by
// heightsec. Allows for multiple fake planes.
/** Flags describing 3Dfloor behavior and appearance.
  */
typedef enum
{
	FF_EXISTS            = 0x1,        ///< Always set, to check for validity.
	FF_BLOCKPLAYER       = 0x2,        ///< Solid to player, but nothing else
	FF_BLOCKOTHERS       = 0x4,        ///< Solid to everything but player
	FF_SOLID             = 0x6,        ///< Clips things.
	FF_RENDERSIDES       = 0x8,        ///< Renders the sides.
	FF_RENDERPLANES      = 0x10,       ///< Renders the floor/ceiling.
	FF_RENDERALL         = 0x18,       ///< Renders everything.
	FF_SWIMMABLE         = 0x20,       ///< Is a water block.
	FF_NOSHADE           = 0x40,       ///< Messes with the lighting?
	FF_CUTSOLIDS         = 0x80,       ///< Cuts out hidden solid pixels.
	FF_CUTEXTRA          = 0x100,      ///< Cuts out hidden translucent pixels.
	FF_CUTLEVEL          = 0x180,      ///< Cuts out all hidden pixels.
	FF_CUTSPRITES        = 0x200,      ///< Final step in making 3D water.
	FF_BOTHPLANES        = 0x400,      ///< Renders both planes all the time.
	FF_EXTRA             = 0x800,      ///< Gets cut by ::FF_CUTEXTRA.
	FF_TRANSLUCENT       = 0x1000,     ///< See through!
	FF_FOG               = 0x2000,     ///< Fog "brush."
	FF_INVERTPLANES      = 0x4000,     ///< Reverse the plane visibility rules.
	FF_ALLSIDES          = 0x8000,     ///< Render inside and outside sides.
	FF_INVERTSIDES       = 0x10000,    ///< Only render inside sides.
	FF_DOUBLESHADOW      = 0x20000,    ///< Make two lightlist entries to reset light?
	FF_FLOATBOB          = 0x40000,    ///< Floats on water and bobs if you step on it.
	FF_NORETURN          = 0x80000,    ///< Used with ::FF_CRUMBLE. Will not return to its original position after falling.
	FF_CRUMBLE           = 0x100000,   ///< Falls 2 seconds after being stepped on, and randomly brings all touching crumbling 3dfloors down with it, providing their master sectors share the same tag (allows crumble platforms above or below, to also exist).
	FF_SHATTERBOTTOM     = 0x200000,   ///< Used with ::FF_BUSTUP. Like FF_SHATTER, but only breaks from the bottom. Good for springing up through rubble.
	FF_MARIO             = 0x400000,   ///< Acts like a question block when hit from underneath. Goodie spawned at top is determined by master sector.
	FF_BUSTUP            = 0x800000,   ///< You can spin through/punch this block and it will crumble!
	FF_QUICKSAND         = 0x1000000,  ///< Quicksand!
	FF_PLATFORM          = 0x2000000,  ///< You can jump up through this to the top.
	FF_REVERSEPLATFORM   = 0x4000000,  ///< A fall-through floor in normal gravity, a platform in reverse gravity.
	FF_INTANGABLEFLATS   = 0x6000000,  ///< Both flats are intangable, but the sides are still solid.
	FF_SHATTER           = 0x8000000,  ///< Used with ::FF_BUSTUP. Thinks everyone's Knuckles.
	FF_SPINBUST          = 0x10000000, ///< Used with ::FF_BUSTUP. Jump or fall onto it while curled in a ball.
	FF_ONLYKNUX          = 0x20000000, ///< Used with ::FF_BUSTUP. Only Knuckles can break this rock.
	FF_RIPPLE            = 0x40000000, ///< Ripple the flats
	FF_COLORMAPONLY      = 0x80000000, ///< Only copy the colormap, not the lightlevel
	FF_GOOWATER          = FF_SHATTERBOTTOM, ///< Used with ::FF_SWIMMABLE. Makes thick bouncey goop.
} ffloortype_e;

typedef struct ffloor_s
{
	fixed_t *topheight;
	INT32 *toppic;
	INT16 *toplightlevel;
	fixed_t *topxoffs;
	fixed_t *topyoffs;
	angle_t *topangle;

	fixed_t *bottomheight;
	INT32 *bottompic;
	fixed_t *bottomxoffs;
	fixed_t *bottomyoffs;
	angle_t *bottomangle;


	// Pointers to pointers. Yup.
	struct pslope_s **t_slope;
	struct pslope_s **b_slope;


	size_t secnum;
	ffloortype_e flags;
	struct line_s *master;

	struct sector_s *target;

	struct ffloor_s *next;
	struct ffloor_s *prev;

	INT32 lastlight;
	INT32 alpha;
	tic_t norender; // for culling

	// these are saved for netgames, so do not let Lua touch these!
	ffloortype_e spawnflags; // flags the 3D floor spawned with
	INT32 spawnalpha; // alpha the 3D floor spawned with
} ffloor_t;


// This struct holds information for shadows casted by 3D floors.
// This information is contained inside the sector_t and is used as the base
// information for casted shadows.
typedef struct lightlist_s
{
	fixed_t height;
	INT16 *lightlevel;
	extracolormap_t *extra_colormap;
	INT32 flags;
	ffloor_t *caster;
	struct pslope_s *slope; // FF_DOUBLESHADOW makes me have to store this pointer here. Bluh bluh.
} lightlist_t;


// This struct is used for rendering walls with shadows casted on them...
typedef struct r_lightlist_s
{
	fixed_t height;
	fixed_t heightstep;
	fixed_t botheight;
	fixed_t botheightstep;
	fixed_t startheight; // for repeating midtextures
	INT16 lightlevel;
	extracolormap_t *extra_colormap;
	lighttable_t *rcolormap;
	ffloortype_e flags;
	INT32 lightnum;
} r_lightlist_t;

// Slopes
typedef enum {
	SL_NOPHYSICS = 1, // Don't do momentum adjustment with this slope
	SL_NODYNAMIC = 1<<1, // Slope will never need to move during the level, so don't fuss with recalculating it
	SL_ANCHORVERTEX = 1<<2, // Slope is using a Slope Vertex Thing to anchor its position
	SL_VERTEXSLOPE = 1<<3, // Slope is built from three Slope Vertex Things
} slopeflags_t;

typedef struct pslope_s
{
	UINT16 id; // The number of the slope, mostly used for netgame syncing purposes

	// --- Information used in clipping/projection ---
	// Origin vector for the plane
	vector3_t o;

	// 2-Dimentional vector (x, y) normalized. Used to determine distance from
	// the origin in 2d mapspace. (Basically a thrust of FRACUNIT in xydirection angle)
	vector2_t d;

	// The rate at which z changes based on distance from the origin plane.
	fixed_t zdelta;

	// The normal of the slope; will always point upward, and thus be inverted on ceilings. I think it's only needed for physics? -Red
	vector3_t normal;

	// For comparing when a slope should be rendered
	fixed_t lowz;
	fixed_t highz;

	// This values only check and must be updated if the slope itself is modified
	angle_t zangle; // Angle of the plane going up from the ground (not mesured in degrees)
	angle_t xydirection; // The direction the slope is facing (north, west, south, etc.)

	struct line_s *sourceline; // The line that generated the slope
	fixed_t extent; // Distance value used for recalculating zdelta
	UINT8 refpos; // 1=front floor 2=front ceiling 3=back floor 4=back ceiling (used for dynamic sloping)

	UINT8 flags; // Slope options
	mapthing_t **vertices; // List should be three long for slopes made by vertex things, or one long for slopes using one vertex thing to anchor

	struct pslope_s *next; // Make a linked list of dynamic slopes, for easy reference later
} pslope_t;


typedef enum
{
	SF_FLIPSPECIAL_FLOOR    =  1,
	SF_FLIPSPECIAL_CEILING  =  2,
	SF_FLIPSPECIAL_BOTH     =  3,
	SF_TRIGGERSPECIAL_TOUCH =  4,
} sectorflags_t;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
typedef struct sector_s
{
	fixed_t floorheight;
	fixed_t ceilingheight;
	INT32 floorpic;
	INT32 ceilingpic;
	INT16 lightlevel;
	INT16 special;
	UINT16 tag;
	INT32 nexttag, firsttag; // for fast tag searches

	// origin for any sounds played by the sector
	// also considered the center for e.g. Mario blocks
	degenmobj_t soundorg;

	// if == validcount, already checked
	size_t validcount;

	// list of mobjs in sector
	mobj_t *thinglist;

	// thinker_ts for reversable actions
	void *floordata; // floor move thinker
	void *ceilingdata; // ceiling move thinker
	void *lightingdata; // lighting change thinker

	// floor and ceiling texture offsets
	fixed_t floor_xoffs, floor_yoffs;
	fixed_t ceiling_xoffs, ceiling_yoffs;

	// flat angle
	angle_t floorpic_angle;
	angle_t ceilingpic_angle;

	INT32 heightsec; // other sector, or -1 if no other sector
	INT32 camsec; // used for camera clipping

	INT32 floorlightsec, ceilinglightsec;
	INT32 crumblestate; // used for crumbling and bobbing

	INT32 bottommap, midmap, topmap; // dynamic colormaps

	// list of mobjs that are at least partially in the sector
	// thinglist is a subset of touching_thinglist
	struct msecnode_s *touching_thinglist;

	size_t linecount;
	struct line_s **lines; // [linecount] size
	// Hack: store special line tagging to some sectors
	// to efficiently help work around bugs by directly
	// referencing the specific line that the problem happens in.
	// (used in T_MovePlane mobj physics)
	struct line_s *tagline;

	// Improved fake floor hack
	ffloor_t *ffloors;
	size_t *attached;
	boolean *attachedsolid;
	size_t numattached;
	size_t maxattached;
	lightlist_t *lightlist;
	INT32 numlights;
	boolean moved;

	// per-sector colormaps!
	extracolormap_t *extra_colormap;

	// This points to the master's floorheight, so it can be changed in realtime!
	fixed_t *gravity; // per-sector gravity
	boolean verticalflip; // If gravity < 0, then allow flipped physics
	sectorflags_t flags;

	// Sprite culling feature
	struct line_s *cullheight;

	// Current speed of ceiling/floor. For Knuckles to hold onto stuff.
	fixed_t floorspeed, ceilspeed;

	// list of precipitation mobjs in sector
	precipmobj_t *preciplist;
	struct mprecipsecnode_s *touching_preciplist;


	// Eternity engine slope
	pslope_t *f_slope; // floor slope
	pslope_t *c_slope; // ceiling slope
	boolean hasslope; // The sector, or one of its visible FOFs, contains a slope


	// these are saved for netgames, so do not let Lua touch these!
	INT32 spawn_nexttag, spawn_firsttag; // the actual nexttag/firsttag values may differ if the sector's tag was changed

	// offsets sector spawned with (via linedef type 7)
	fixed_t spawn_flr_xoffs, spawn_flr_yoffs;
	fixed_t spawn_ceil_xoffs, spawn_ceil_yoffs;

	// flag angles sector spawned with (via linedef type 7)
	angle_t spawn_flrpic_angle;
	angle_t spawn_ceilpic_angle;
} sector_t;

//
// Move clipping aid for linedefs.
//
typedef enum
{
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
} slopetype_t;

typedef struct line_s
{
	// Vertices, from v1 to v2.
	vertex_t *v1;
	vertex_t *v2;

	fixed_t dx, dy; // Precalculated v2 - v1 for side checking.

	// Animation related.
	INT16 flags;
	INT16 special;
	INT16 tag;

	// Visual appearance: sidedefs.
	UINT16 sidenum[2]; // sidenum[1] will be 0xffff if one-sided

	fixed_t bbox[4]; // bounding box for the extent of the linedef

	// To aid move clipping.
	slopetype_t slopetype;

	// Front and back sector.
	// Note: redundant? Can be retrieved from SideDefs.
	sector_t *frontsector;
	sector_t *backsector;

	size_t validcount; // if == validcount, already checked
#if 1//#ifdef WALLSPLATS
	void *splats; // wallsplat_t list
#endif
	INT32 firsttag, nexttag; // improves searches for tags.
	polyobj_t *polyobj; // Belongs to a polyobject?

	char *text; // a concatination of all front and back texture names, for linedef specials that require a string.
	INT16 callcount; // no. of calls left before triggering, for the "X calls" linedef specials, defaults to 0
} line_t;

//
// The SideDef.
//

typedef struct
{
	// add this to the calculated texture column
	fixed_t textureoffset;

	// add this to the calculated texture top
	fixed_t rowoffset;

	// Texture indices.
	// We do not maintain names here.
	INT32 toptexture, bottomtexture, midtexture;

	// Sector the SideDef is facing.
	sector_t *sector;

	INT16 special; // the special of the linedef this side belongs to
	INT16 repeatcnt; // # of times to repeat midtexture

	char *text; // a concatination of all top, bottom, and mid texture names, for linedef specials that require a string.
} side_t;

//
// A subsector.
// References a sector.
// Basically, this is a list of linesegs, indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
typedef struct subsector_s
{
	sector_t *sector;
	INT16 numlines;
	UINT16 firstline;
	struct polyobj_s *polyList; // haleyjd 02/19/06: list of polyobjects
#if 1//#ifdef FLOORSPLATS
	void *splats; // floorsplat_t list
#endif
	size_t validcount;
} subsector_t;

// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_thinglist_next
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_sectorlist_next links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

typedef struct msecnode_s
{
	sector_t *m_sector; // a sector containing this object
	struct mobj_s *m_thing;  // this object
	struct msecnode_s *m_sectorlist_prev;  // prev msecnode_t for this thing
	struct msecnode_s *m_sectorlist_next;  // next msecnode_t for this thing
	struct msecnode_s *m_thinglist_prev;  // prev msecnode_t for this sector
	struct msecnode_s *m_thinglist_next;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
} msecnode_t;

typedef struct mprecipsecnode_s
{
	sector_t *m_sector; // a sector containing this object
	struct precipmobj_s *m_thing;  // this object
	struct mprecipsecnode_s *m_sectorlist_prev;  // prev msecnode_t for this thing
	struct mprecipsecnode_s *m_sectorlist_next;  // next msecnode_t for this thing
	struct mprecipsecnode_s *m_thinglist_prev;  // prev msecnode_t for this sector
	struct mprecipsecnode_s *m_thinglist_next;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
} mprecipsecnode_t;

// for now, only used in hardware mode
// maybe later for software as well?
// that's why it's moved here
typedef struct light_s
{
	UINT16 type;          // light,... (cfr #define in hwr_light.c)

	float light_xoffset;
	float light_yoffset;  // y offset to adjust corona's height

	UINT32 corona_color;   // color of the light for static lighting
	float corona_radius;  // radius of the coronas

	UINT32 dynamic_color;  // color of the light for dynamic lighting
	float dynamic_radius; // radius of the light ball
	float dynamic_sqrradius; // radius^2 of the light ball
} light_t;

typedef struct lightmap_s
{
	float s[2], t[2];
	light_t *light;
	struct lightmap_s *next;
} lightmap_t;

//
// The lineseg.
//
typedef struct seg_s
{
	vertex_t *v1;
	vertex_t *v2;

	INT32 side;

	fixed_t offset;

	angle_t angle;

	side_t *sidedef;
	line_t *linedef;

	// Sector references.
	// Could be retrieved from linedef, too. backsector is NULL for one sided lines
	sector_t *frontsector;
	sector_t *backsector;

	fixed_t length;	// precalculated seg length
#ifdef HWRENDER
	// new pointers so that AdjustSegs doesn't mess with v1/v2
	void *pv1; // polyvertex_t
	void *pv2; // polyvertex_t
	float flength; // length of the seg, used by hardware renderer

	lightmap_t *lightmaps; // for static lightmap
#endif

	// Why slow things down by calculating lightlists for every thick side?
	size_t numlights;
	r_lightlist_t *rlights;
	polyobj_t *polyseg;
	boolean dontrenderme;
} seg_t;

//
// BSP node.
//
typedef struct
{
	// Partition line.
	fixed_t x, y;
	fixed_t dx, dy;

	// Bounding box for each child.
	fixed_t bbox[2][4];

	// If NF_SUBSECTOR its a subsector.
	UINT16 children[2];
} node_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// posts are runs of non masked source pixels
typedef struct
{
	UINT8 topdelta; // -1 is the last post in a column
	UINT8 length;   // length data bytes follows
} post_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

// column_t is a list of 0 or more post_t, (UINT8)-1 terminated
typedef post_t column_t;

//
// OTHER TYPES
//

#ifndef MAXFFLOORS
#define MAXFFLOORS 40
#endif

//
// ?
//
typedef struct drawseg_s
{
	seg_t *curline;
	INT32 x1;
	INT32 x2;

	fixed_t scale1;
	fixed_t scale2;
	fixed_t scalestep;

	INT32 silhouette; // 0 = none, 1 = bottom, 2 = top, 3 = both

	fixed_t bsilheight; // do not clip sprites above this
	fixed_t tsilheight; // do not clip sprites below this

	// Pointers to lists for sprite clipping, all three adjusted so [x1] is first value.
	INT16 *sprtopclip;
	INT16 *sprbottomclip;
	INT16 *maskedtexturecol;

	struct visplane_s *ffloorplanes[MAXFFLOORS];
	INT32 numffloorplanes;
	struct ffloor_s *thicksides[MAXFFLOORS];
	INT16 *thicksidecol;
	INT32 numthicksides;
	fixed_t frontscale[MAXVIDWIDTH];

	UINT8 portalpass; // if > 0 and <= portalrender, do not affect sprite clipping


	fixed_t maskedtextureheight[MAXVIDWIDTH]; // For handling sloped midtextures

	vertex_t leftpos, rightpos; // Used for rendering FOF walls with slopes

} drawseg_t;

typedef enum
{
	PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
	INTENSITY       = 1,  // 1 byte intensity
	INTENSITY_ALPHA = 2,  // 2 byte: alpha then intensity
	RGB24           = 3,  // 24 bit rgb
	RGBA32          = 4,  // 32 bit rgba
} pic_mode_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures, and we compose
// textures from the TEXTURE1 list of patches.
//
// WARNING: this structure is cloned in GLPatch_t
typedef struct
{
	INT16 width;          // bounding box size
	INT16 height;
	INT16 leftoffset;     // pixels to the left of origin
	INT16 topoffset;      // pixels below the origin
	INT32 columnofs[8];     // only [width] used
	// the [0] is &columnofs[width]
} patch_t;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// a pic is an unmasked block of pixels, stored in horizontal way
typedef struct
{
	INT16 width;
	UINT8 zero;       // set to 0 allow autodetection of pic_t
	                 // mode instead of patch or raw
	UINT8 mode;       // see pic_mode_t above
	INT16 height;
	INT16 reserved1; // set to 0
	UINT8 data[0];
} pic_t;

#ifdef _MSC_VER
#pragma warning(default : 4200)
#endif

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef enum
{
	SRF_SINGLE      = 0,   // 0-angle for all rotations
	SRF_3D          = 1,   // Angles 1-8
	SRF_LEFT        = 2,   // Left side has single patch
	SRF_RIGHT       = 4,   // Right side has single patch
	SRF_2D          = 6,   // SRF_LEFT|SRF_RIGHT
	SRF_NONE        = 0xff // Initial value
} spriterotateflags_t;     // SRF's up!

// Same as a patch_t, except just the header
// and the wadnum/lumpnum combination that points
// to wherever the patch is in memory.
struct patchinfo_s
{
	INT16 width;          // bounding box size
	INT16 height;
	INT16 leftoffset;     // pixels to the left of origin
	INT16 topoffset;      // pixels below the origin

	UINT16 wadnum;        // the software patch lump num for when the patch
	UINT16 lumpnum;       // was flushed, and we need to re-create it

	// next patchinfo_t in memory
	struct patchinfo_s *next;
};
typedef struct patchinfo_s patchinfo_t;

//
// Sprites are patches with a special naming convention so they can be
//  recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with x indicating the rotation,
//  x = 0, 1-8, L/R
// The sprite and frame specified by a thing_t is range checked at run time.
// A sprite is a patch_t that is assumed to represent a three dimensional
//  object and may have multiple rotations predrawn.
// Horizontal flipping is used to save space, thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used for all views: NNNNF0
// Some sprites will take the entirety of the left side: NNNNFL
// Or the right side: NNNNFR
// Or both, mirrored: NNNNFLFR
//
typedef struct
{
	// Note: as eight entries are available, we might as well insert the same
	//  name eight times.
	UINT8 rotate; // see spriterotateflags_t above

	// Lump to use for view angles 0-7.
	lumpnum_t lumppat[8]; // lump number 16 : 16 wad : lump
	size_t lumpid[8]; // id in the spriteoffset, spritewidth, etc. tables

	// Flip bits (1 = flip) to use for view angles 0-7.
	UINT8 flip;
} spriteframe_t;

//
// A sprite definition:  a number of animation frames.
//
typedef struct
{
	size_t numframes;
	spriteframe_t *spriteframes;
} spritedef_t;

#endif
