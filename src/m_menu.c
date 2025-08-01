// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2011-2016 by Matthew "Inuyasha" Walsh.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_menu.c
/// \brief XMOD's extremely revamped menu system.

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "m_menu.h"

#include "doomdef.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "d_clisrv.h"
#include "i_net.h"
#include "console.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"
#include "i_time.h"
#include "m_argv.h"

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"

// Addfile
#include "filesrch.h"

#include "v_video.h"
#include "i_video.h"
#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"
#include "p_setup.h"
#include "f_finale.h"
#include "lua_hook.h"
#include "qs22k.h" // qsort

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "d_net.h"
#include "mserv.h"
#include "m_misc.h"
#include "m_anigif.h"
#include "byteptr.h"
#include "st_stuff.h"
#include "i_sound.h"

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

#if defined(HAVE_SDL)
#include "SDL.h"
#if SDL_VERSION_ATLEAST(2,0,0)
#include "sdl/sdlmain.h" // JOYSTICK_HOTPLUG
#endif
#endif

#define SKULLXOFF -32
#define LINEHEIGHT 16
#define STRINGHEIGHT 8
#define FONTBHEIGHT 20
#define SMALLLINEHEIGHT 8
#define SLIDER_RANGE 10
#define SLIDER_WIDTH (8*SLIDER_RANGE+6)
#define SERVERS_PER_PAGE 11

typedef enum
{
	QUITMSG = 0,
	QUITMSG1,
	QUITMSG2,
	QUITMSG3,
	QUITMSG4,
	QUITMSG5,
	QUITMSG6,
	QUITMSG7,

	QUIT2MSG,
	QUIT2MSG1,
	QUIT2MSG2,
	QUIT2MSG3,
	QUIT2MSG4,
	QUIT2MSG5,
	QUIT2MSG6,

	QUIT3MSG,
	QUIT3MSG1,
	QUIT3MSG2,
	QUIT3MSG3,
	QUIT3MSG4,
	QUIT3MSG5,
	QUIT3MSG6,
	NUM_QUITMESSAGES
} text_enum;

const char *quitmsg[NUM_QUITMESSAGES];

// Stuff for customizing the player select screen Tails 09-22-2003
description_t description[MAXSKINS];

static char *char_notes = NULL;
static fixed_t char_scroll = 0;

boolean menuactive = false;
boolean fromlevelselect = false;

static INT32 coolalphatimer = 9;
typedef enum
{
	LLM_CREATESERVER,
	LLM_LEVELSELECT,
	LLM_RECORDATTACK,
	LLM_NIGHTSATTACK
} levellist_mode_t;

levellist_mode_t levellistmode = LLM_CREATESERVER;
UINT8 maplistoption = 0;

static char joystickInfo[8][29];
#ifndef NONET
static UINT32 serverlistpage;
#endif

static saveinfo_t savegameinfo[MAXSAVEGAMES]; // Extra info about the save games.

INT16 startmap; // Mario, NiGHTS, or just a plain old normal game?

static INT16 itemOn = 1; // menu item skull is on, Hack by Tails 09-18-2002
static INT16 skullAnimCounter = 10; // skull animation counter

static  boolean setupcontrols_secondaryplayer;
static  INT32   (*setupcontrols)[2];  // pointer to the gamecontrols of the player being edited

// shhh... what am I doing... nooooo!
static INT32 vidm_testingmode = 0;
static INT32 vidm_previousmode;
static INT32 vidm_selected = 0;
static INT32 vidm_nummodes;
static INT32 vidm_column_size;

//
// PROTOTYPES
//

static void M_StopMessage(INT32 choice);

#ifndef NONET
static void M_HandleServerPage(INT32 choice);
static void M_RoomMenu(INT32 choice);
#endif

// Prototyping is fun, innit?
// ==========================================================================
// NEEDED FUNCTION PROTOTYPES GO HERE
// ==========================================================================

// the haxor message menu
menu_t MessageDef;

menu_t SPauseDef;

#define lsheadingheight 16

// Sky Room
static void M_CustomLevelSelect(INT32 choice);
static void M_CustomWarp(INT32 choice);
FUNCNORETURN static ATTRNORETURN void M_UltimateCheat(INT32 choice);
static void M_LoadGameLevelSelect(INT32 choice);
static void M_GetAllEmeralds(INT32 choice);
static void M_DestroyRobots(INT32 choice);
static void M_LevelSelectWarp(INT32 choice);
static void M_Credits(INT32 choice);
static void M_PandorasBox(INT32 choice);
static void M_EmblemHints(INT32 choice);
menu_t SR_MainDef, SR_UnlockChecklistDef;

// Misc. Main Menu
static void M_SinglePlayerMenu(INT32 choice);
static void M_Options(INT32 choice);
static void M_SelectableClearMenus(INT32 choice);
static void M_Retry(INT32 choice);
static void M_EndGame(INT32 choice);
static void M_MapChange(INT32 choice);
static void M_ChangeLevel(INT32 choice);
static void M_ConfirmSpectate(INT32 choice);
static void M_ConfirmEnterGame(INT32 choice);
static void M_ConfirmTeamScramble(INT32 choice);
static void M_ConfirmTeamChange(INT32 choice);
static void M_SecretsMenu(INT32 choice);
static void M_SetupChoosePlayer(INT32 choice);
static void M_QuitSRB2(INT32 choice);
menu_t SP_MainDef, MP_MainDef, OP_MainDef;
menu_t MISC_ScrambleTeamDef, MISC_ChangeTeamDef;

// Single Player
static void M_LoadGame(INT32 choice);
static void M_TimeAttack(INT32 choice);
static void M_NightsAttack(INT32 choice);
static void M_Statistics(INT32 choice);
static void M_ReplayTimeAttack(INT32 choice);
static void M_ChooseTimeAttack(INT32 choice);
static void M_ChooseNightsAttack(INT32 choice);
static void M_ModeAttackRetry(INT32 choice);
static void M_ModeAttackEndGame(INT32 choice);
static void M_SetGuestReplay(INT32 choice);
static void M_ChoosePlayer(INT32 choice);
menu_t SP_LevelStatsDef;
static menu_t SP_TimeAttackDef, SP_ReplayDef, SP_GuestReplayDef, SP_GhostDef;
static menu_t SP_NightsAttackDef, SP_NightsReplayDef, SP_NightsGuestReplayDef, SP_NightsGhostDef;

// Multiplayer
#ifndef NONET
static void M_StartServerMenu(INT32 choice);
static void M_ConnectMenu(INT32 choice);
#endif
static void M_StartSplitServerMenu(INT32 choice);
static void M_StartServer(INT32 choice);
#ifndef NONET
static void M_Refresh(INT32 choice);
static void M_Connect(INT32 choice);
static void M_ChooseRoom(INT32 choice);
#endif
static void M_SetupMultiPlayer(INT32 choice);
static void M_SetupMultiPlayer2(INT32 choice);

// Options
// Split into multiple parts due to size
// Controls
menu_t OP_AllControlsDef;
menu_t OP_AllControls2Def; // for 2P
menu_t OP_ControlsDef;
menu_t OP_P1ControlsDef, OP_P2ControlsDef, OP_MouseOptionsDef;
menu_t OP_Mouse2OptionsDef, OP_Joystick1Def, OP_Joystick2Def;
static void M_VideoModeMenu(INT32 choice);
static void M_Setup1PControlsMenu(INT32 choice);
static void M_Setup2PControlsMenu(INT32 choice);
static void M_Setup1PJoystickMenu(INT32 choice);
static void M_Setup2PJoystickMenu(INT32 choice);
static void M_AssignJoystick(INT32 choice);
static void M_ChangeControl(INT32 choice);

// Video & Sound
static void M_VideoOptions(INT32 choice);
menu_t OP_VideoOptionsDef, OP_VideoModeDef, OP_ColorOptionsDef;
#ifdef HWRENDER
static void M_OpenGLOptionsMenu(void);
menu_t OP_OpenGLOptionsDef;
#endif
menu_t OP_SoundOptionsDef;
menu_t OP_SoundAdvancedDef;

//Misc
menu_t OP_DataOptionsDef, OP_ScreenshotOptionsDef, OP_EraseDataDef;
menu_t OP_GameOptionsDef, OP_ChatOptionsDef, OP_ServerOptionsDef;
menu_t OP_NetgameOptionsDef, OP_GametypeOptionsDef;
menu_t OP_MonitorToggleDef;
static void M_ScreenshotOptions(INT32 choice);
static void M_EraseData(INT32 choice);

static void M_Addons(INT32 choice);
static void M_AddonsOptions(INT32 choice);
static patch_t *addonsp[NUM_EXT+5];

menu_t OP_LegacyOptionsDef;
menu_t OP_LegacyCreditsDef;

#define numaddonsshown 4

// Drawing functions
static void M_DrawGenericMenu(void);
static void M_DrawGenericScrollMenu(void);
static void M_DrawCenteredMenu(void);
static void M_DrawAddons(void);
static void M_DrawSkyRoom(void);
static void M_DrawChecklist(void);
static void M_DrawEmblemHints(void);
static void M_DrawPauseMenu(void);
static void M_DrawServerMenu(void);
static void M_DrawLevelSelectMenu(void);
static void M_DrawImageDef(void);
static void M_DrawLoad(void);
static void M_DrawLevelStats(void);
static void M_DrawTimeAttackMenu(void);
static void M_DrawNightsAttackMenu(void);
static void M_DrawSetupChoosePlayerMenu(void);
static void M_DrawControl(void);
static void M_DrawVideoMode(void);
static void M_DrawColorMenu(void);
static void M_DrawMonitorToggles(void);
#ifndef NONET
static void M_DrawConnectMenu(void);
static void M_DrawMPMainMenu(void);
static void M_DrawRoomMenu(void);
#endif
static void M_DrawJoystick(void);
static void M_DrawSetupMultiPlayerMenu(void);

// Handling functions
#ifndef NONET
static boolean M_CancelConnect(void);
#endif
static boolean M_ExitPandorasBox(void);
static boolean M_QuitMultiPlayerMenu(void);
static void M_HandleAddons(INT32 choice);
static void M_HandleSoundTest(INT32 choice);
static void M_HandleImageDef(INT32 choice);
static void M_HandleLoadSave(INT32 choice);
static void M_HandleLevelStats(INT32 choice);
#ifndef NONET
static void M_HandleConnectIP(INT32 choice);
static void M_ConnectLastServer(INT32 choice);
#endif
static void M_HandleSetupMultiPlayer(INT32 choice);
static void M_HandleVideoMode(INT32 choice);

static void M_ResetCvars(void);

// Consvar onchange functions
static void Nextmap_OnChange(void);
static void Newgametype_OnChange(void);
static void Dummymares_OnChange(void);

// IP Validity function
static boolean M_CheckIfValidIPv4(const char *str);

// ==========================================================================
// CONSOLE VARIABLES AND THEIR POSSIBLE VALUES GO HERE.
// ==========================================================================

consvar_t cv_showfocuslost = CVAR_INIT ("showfocuslost", "Yes", "Whether or not to show a \"FOCUS LOST\" when the game window is unfocused", CV_SAVE, CV_YesNo, NULL);

static CV_PossibleValue_t map_cons_t[] = {
	{1,"MIN"},
	{NUMMAPS, "MAX"},
	{0,NULL}
};
consvar_t cv_nextmap = CVAR_INIT ("nextmap", "1", NULL, CV_HIDEN|CV_CALL, map_cons_t, Nextmap_OnChange);

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = CVAR_INIT ("chooseskin", DEFAULTSKIN, NULL, CV_HIDEN|CV_CALL, skins_cons_t, Nextmap_OnChange);

// This gametype list is integral for many different reasons.
// When you add gametypes here, don't forget to update them in dehacked.c and doomstat.h!
CV_PossibleValue_t gametype_cons_t[NUMGAMETYPES+1];

consvar_t cv_newgametype = CVAR_INIT ("newgametype", "Co-op", NULL, CV_HIDEN|CV_CALL, gametype_cons_t, Newgametype_OnChange);



static CV_PossibleValue_t serversort_cons_t[] = {
	{0,"Ping"},
	{1,"Modified State"},
	{2,"Most Players"},
	{3,"Least Players"},
	{4,"Max Players"},
	{5,"Gametype"},
	{0,NULL}
};
consvar_t cv_serversort = CVAR_INIT ("serversort", "Ping", NULL, CV_HIDEN | CV_CALL, serversort_cons_t, M_SortServerList);

// autorecord demos for time attack
static consvar_t cv_autorecord = CVAR_INIT ("autorecord", "Yes", NULL, 0, CV_YesNo, NULL);

CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show"}, {2, "Show All"}, {0, NULL}};
CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};

consvar_t cv_ghost_bestscore = CVAR_INIT ("ghost_bestscore", "Show", NULL, CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_besttime  = CVAR_INIT ("ghost_besttime",  "Show", NULL, CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_bestrings = CVAR_INIT ("ghost_bestrings", "Show", NULL, CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_last      = CVAR_INIT ("ghost_last",      "Show", NULL, CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_guest     = CVAR_INIT ("ghost_guest",     "Show", NULL, CV_SAVE, ghost2_cons_t, NULL);

//Console variables used solely in the menu system.
//todo: add a way to use non-console variables in the menu
//      or make these consvars legitimate like color or skin.
static CV_PossibleValue_t dummyteam_cons_t[] = {{0, "Spectator"}, {1, "Red"}, {2, "Blue"}, {0, NULL}};
static CV_PossibleValue_t dummyscramble_cons_t[] = {{0, "Random"}, {1, "Points"}, {0, NULL}};
static CV_PossibleValue_t ringlimit_cons_t[] = {{0, "MIN"}, {9999, "MAX"}, {0, NULL}};
static CV_PossibleValue_t liveslimit_cons_t[] = {{0, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t dummymares_cons_t[] = {
	{-1, "END"}, {0,"Overall"}, {1,"Mare 1"}, {2,"Mare 2"}, {3,"Mare 3"}, {4,"Mare 4"}, {5,"Mare 5"}, {6,"Mare 6"}, {7,"Mare 7"}, {8,"Mare 8"}, {0,NULL}
};

static consvar_t cv_dummyteam = CVAR_INIT ("dummyteam", "Spectator", NULL, CV_HIDEN, dummyteam_cons_t, NULL);
static consvar_t cv_dummyscramble = CVAR_INIT ("dummyscramble", "Random", NULL, CV_HIDEN, dummyscramble_cons_t, NULL);
static consvar_t cv_dummyrings = CVAR_INIT ("dummyrings", "0", NULL, CV_HIDEN, ringlimit_cons_t,	NULL);
static consvar_t cv_dummylives = CVAR_INIT ("dummylives", "0", NULL, CV_HIDEN, liveslimit_cons_t, NULL);
static consvar_t cv_dummycontinues = CVAR_INIT ("dummycontinues", "0", NULL, CV_HIDEN, liveslimit_cons_t, NULL);
static consvar_t cv_dummymares = CVAR_INIT ("dummymares", "Overall", NULL, CV_HIDEN|CV_CALL, dummymares_cons_t, Dummymares_OnChange);

// ==========================================================================
// ORGANIZATION START.
// ==========================================================================
// Note: Never should we be jumping from one category of menu options to another
//       without first going to the Main Menu.
// Note: Ignore the above if you're working with the Pause menu.
// Note: (Prefix)_MainMenu should be the target of all Main Menu options that
//       point to submenus.

// ---------
// Main Menu
// ---------
static menuitem_t MainMenu[] =
{
	{IT_CALL   |IT_STRING, NULL, "1  player", NULL,   M_SinglePlayerMenu, 76},
	{IT_SUBMENU|IT_STRING, NULL, "multiplayer", NULL,  &MP_MainDef,        84},
	{IT_STRING|IT_CALL,    NULL, "Secrets", NULL,      M_SecretsMenu,    	92},
	{IT_CALL   |IT_STRING, NULL, "options", NULL,      M_Options,         100},
	{IT_CALL   |IT_STRING, NULL, "Addons",  NULL,      M_Addons,          108},
	{IT_CALL   |IT_STRING, NULL, "quit  game", NULL,   M_QuitSRB2,        116},
};

typedef enum
{
	singleplr = 0,
	multiplr,
	secrets,
	options,
	addons,
	quitdoom
} main_e;

static menuitem_t MISC_AddonsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", NULL,  M_HandleAddons, 0},     // dummy menuitem for the control func
};

// ---------------------------------
// Pause Menu Mode Attacking Edition
// ---------------------------------
static menuitem_t MAPauseMenu[] =
{
	{IT_CALL | IT_STRING,    NULL, "Continue", NULL,             M_SelectableClearMenus,48},
	{IT_CALL | IT_STRING,    NULL, "Retry",    NULL,             M_ModeAttackRetry,     56},
	{IT_CALL | IT_STRING,    NULL, "Abort",    NULL,             M_ModeAttackEndGame,   64},
};

typedef enum
{
	mapause_continue,
	mapause_retry,
	mapause_abort
} mapause_e;

// ---------------------
// Pause Menu MP Edition
// ---------------------
static menuitem_t MPauseMenu[] =
{
	{IT_STRING | IT_CALL,     NULL, "Add-ons...", NULL,         M_Addons,               8},
	{IT_STRING  | IT_SUBMENU, NULL, "Scramble Teams...", NULL,  &MISC_ScrambleTeamDef, 16},
	{IT_STRING  | IT_CALL,    NULL, "Switch Map..."    , NULL,  M_MapChange,           24},

	{IT_CALL | IT_STRING,    NULL, "Continue",     NULL,         M_SelectableClearMenus,40},
	{IT_CALL | IT_STRING,    NULL, "Player 1 Setup",  NULL,       M_SetupMultiPlayer,    48}, // splitscreen
	{IT_CALL | IT_STRING,    NULL, "Player 2 Setup",  NULL,       M_SetupMultiPlayer2,   56}, // splitscreen

	{IT_STRING | IT_CALL,    NULL, "Spectate",     NULL,         M_ConfirmSpectate,     48},
	{IT_STRING | IT_CALL,    NULL, "Enter Game",     NULL,       M_ConfirmEnterGame,    48},
	{IT_STRING | IT_SUBMENU, NULL, "Switch Team...",  NULL,      &MISC_ChangeTeamDef,   48},
	{IT_CALL | IT_STRING,    NULL, "Player Setup",  NULL,        M_SetupMultiPlayer,    56}, // alone
	{IT_CALL | IT_STRING,    NULL, "Options",       NULL,        M_Options,             64},

	{IT_CALL | IT_STRING,    NULL, "Return to Title",   NULL,    M_EndGame,            80},
	{IT_CALL | IT_STRING,    NULL, "Quit Game",   NULL,          M_QuitSRB2,           88},
};

typedef enum
{
	mpause_addons = 0,
	mpause_scramble,
	mpause_switchmap,

	mpause_continue,
	mpause_psetupsplit,
	mpause_psetupsplit2,
	mpause_spectate,
	mpause_entergame,
	mpause_switchteam,
	mpause_psetup,
	mpause_options,

	mpause_title,
	mpause_quit
} mpause_e;

// ---------------------
// Pause Menu SP Edition
// ---------------------
static menuitem_t SPauseMenu[] =
{
	// Pandora's Box will be shifted up if both options are available
	{IT_CALL | IT_STRING,    NULL, "Pandora's Box...",  NULL,    M_PandorasBox,         16},
	{IT_CALL | IT_STRING,    NULL, "Emblem Hints...",   NULL,    M_EmblemHints,         24},
	{IT_CALL | IT_STRING,    NULL, "Level Select...",  NULL,     M_LoadGameLevelSelect, 32},

	{IT_CALL | IT_STRING,    NULL, "Continue",        NULL,      M_SelectableClearMenus,48},
	{IT_CALL | IT_STRING,    NULL, "Retry",           NULL,      M_Retry,               56},
	{IT_CALL | IT_STRING,    NULL, "Options",         NULL,      M_Options,             64},

	{IT_CALL | IT_STRING,    NULL, "Return to Title",   NULL,    M_EndGame,             80},
	{IT_CALL | IT_STRING,    NULL, "Quit Game",        NULL,     M_QuitSRB2,            88},
};

typedef enum
{
	spause_pandora = 0,
	spause_hints,
	spause_levelselect,

	spause_continue,
	spause_retry,
	spause_options,
	spause_title,
	spause_quit
} spause_e;

// -----------------
// Misc menu options
// -----------------
// Prefix: MISC_
static menuitem_t MISC_ScrambleTeamMenu[] =
{
	{IT_STRING|IT_CVAR,      NULL, "Scramble Method", NULL,  &cv_dummyscramble,     30},
	{IT_WHITESTRING|IT_CALL, NULL, "Confirm",     NULL,     M_ConfirmTeamScramble, 90},
};

static menuitem_t MISC_ChangeTeamMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Select Team",   NULL,           &cv_dummyteam,    30},
	{IT_WHITESTRING|IT_CALL,         NULL, "Confirm",      NULL,      M_ConfirmTeamChange,    90},
};

static menuitem_t MISC_ChangeLevelMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",     NULL,         &cv_newgametype,    30},
	{IT_STRING|IT_CVAR,              NULL, "Level",          NULL,        &cv_nextmap,        60},
	{IT_WHITESTRING|IT_CALL,         NULL, "Change Level",     NULL,      M_ChangeLevel,     120},
};

static menuitem_t MISC_HelpMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPN01", NULL,  M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPN02", NULL,  M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPN03", NULL,  M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPM01", NULL,  M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPM02", NULL,  M_HandleImageDef, 0},
};

// --------------------------------
// Sky Room and all of its submenus
// --------------------------------
// Prefix: SR_

// Pause Menu Pandora's Box Options
static menuitem_t SR_PandorasBox[] =
{
	{IT_STRING | IT_CVAR, NULL, "Rings",        NULL,       &cv_dummyrings,      20},
	{IT_STRING | IT_CVAR, NULL, "Lives",        NULL,       &cv_dummylives,      30},
	{IT_STRING | IT_CVAR, NULL, "Continues",     NULL,      &cv_dummycontinues,  40},

	{IT_STRING | IT_CVAR, NULL, "Gravity",      NULL,       &cv_gravity,         60},
	{IT_STRING | IT_CVAR, NULL, "Throw Rings",    NULL,     &cv_ringslinger,     70},

	{IT_STRING | IT_CALL, NULL, "Get All Emeralds", NULL,   M_GetAllEmeralds,    90},
	{IT_STRING | IT_CALL, NULL, "Destroy All Robots", NULL,  M_DestroyRobots,    100},

	{IT_STRING | IT_CALL, NULL, "Ultimate Cheat",  NULL,    M_UltimateCheat,    130},
};

// Sky Room Custom Unlocks
static menuitem_t SR_MainMenu[] =
{
	{IT_STRING|IT_SUBMENU,NULL, "Secrets Checklist", NULL,  &SR_UnlockChecklistDef, 0},
	{IT_DISABLED,         NULL, "", NULL,    NULL,                 0}, // Custom1
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom2
	{IT_DISABLED,         NULL, "",  NULL,  NULL,                 0}, // Custom3
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom4
	{IT_DISABLED,         NULL, "",  NULL,  NULL,                 0}, // Custom5
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom6
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom7
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom8
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom9
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom10
	{IT_DISABLED,         NULL, "",  NULL,  NULL,                 0}, // Custom11
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom12
	{IT_DISABLED,         NULL, "",  NULL,  NULL,                 0}, // Custom13
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom14
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom15
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom16
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom17
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom18
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom19
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom20
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom21
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom22
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom23
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom24
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom25
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom26
	{IT_DISABLED,         NULL, "",  NULL,  NULL,                 0}, // Custom27
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom28
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom29
	{IT_DISABLED,         NULL, "",  NULL,  NULL,                 0}, // Custom30
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom31
	{IT_DISABLED,         NULL, "", NULL,   NULL,                 0}, // Custom32

};

static menuitem_t SR_LevelSelectMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Level",      NULL,            &cv_nextmap,        60},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",     NULL,             M_LevelSelectWarp,     120},
};

static menuitem_t SR_UnlockChecklistMenu[] =
{
	{IT_SUBMENU | IT_STRING,         NULL, "NEXT", NULL,  &SR_MainDef, 192},
};

static menuitem_t SR_EmblemHintMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Emblem Radar", NULL,  &cv_itemfinder, 10},
	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",   NULL,       &SPauseDef,     20}
};

// --------------------------------
// 1 Player and all of its submenus
// --------------------------------
// Prefix: SP_

// Single Player Main
static menuitem_t SP_MainMenu[] =
{
	{IT_CALL | IT_STRING,                       NULL, "Start Game", NULL,    M_LoadGame,        92},
	{IT_SECRET,                                 NULL, "Record Attack", NULL,  M_TimeAttack,     100},
	{IT_SECRET,                                 NULL, "NiGHTS Mode", NULL,    M_NightsAttack,   108},
	{IT_CALL | IT_STRING | IT_CALL_NOTMODIFIED, NULL, "Statistics",   NULL,   M_Statistics,     116},
};

enum
{
	sploadgame,
	sprecordattack,
	spnightsmode,
	spstatistics
};

// Single Player Load Game
static menuitem_t SP_LoadGameMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", NULL,  M_HandleLoadSave, '\0'},     // dummy menuitem for the control func
};

// Single Player Level Select
static menuitem_t SP_LevelSelectMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Level", NULL,                 &cv_nextmap,        60},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",    NULL,              M_LevelSelectWarp,     120},
};

// Single Player Time Attack
static menuitem_t SP_TimeAttackMenu[] =
{
	{IT_STRING|IT_CVAR,        NULL, "Level", NULL,       &cv_nextmap,          52},
	{IT_STRING|IT_CVAR,        NULL, "Player", NULL,    &cv_chooseskin,       62},

	{IT_DISABLED,              NULL, "Guest Option...", NULL,  &SP_GuestReplayDef, 100},
	{IT_DISABLED,              NULL, "Replay...", NULL,     &SP_ReplayDef,        110},
	{IT_DISABLED,              NULL, "Ghosts...",  NULL,    &SP_GhostDef,         120},
	{IT_WHITESTRING|IT_CALL|IT_CALL_NOTMODIFIED,   NULL, "Start", NULL,          M_ChooseTimeAttack,   130},
};

enum
{
	talevel,
	taplayer,

	taguest,
	tareplay,
	taghost,
	tastart
};

static menuitem_t SP_ReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Score", NULL,  M_ReplayTimeAttack, 0},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Time",  NULL,  M_ReplayTimeAttack, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Rings", NULL,  M_ReplayTimeAttack,16},

	{IT_WHITESTRING|IT_CALL, NULL, "Replay Last",    NULL,    M_ReplayTimeAttack,29},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Guest",  NULL,     M_ReplayTimeAttack,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",     NULL,       &SP_TimeAttackDef, 50}
};

static menuitem_t SP_NightsReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Score", NULL,  M_ReplayTimeAttack, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Time", NULL,   M_ReplayTimeAttack,16},

	{IT_WHITESTRING|IT_CALL, NULL, "Replay Last",   NULL,     M_ReplayTimeAttack,29},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Guest",  NULL,     M_ReplayTimeAttack,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",     NULL,       &SP_NightsAttackDef, 50}
};

static menuitem_t SP_GuestReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Score as Guest", NULL,  M_SetGuestReplay, 0},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Time as Guest", NULL,   M_SetGuestReplay, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Rings as Guest", NULL,  M_SetGuestReplay,16},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Last as Guest",   NULL,     M_SetGuestReplay,24},

	{IT_WHITESTRING|IT_CALL, NULL, "Delete Guest Replay", NULL,      M_SetGuestReplay,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",       NULL,          &SP_TimeAttackDef, 50}
};

static menuitem_t SP_NightsGuestReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Score as Guest", NULL,  M_SetGuestReplay, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Time as Guest",  NULL,  M_SetGuestReplay,16},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Last as Guest",    NULL,    M_SetGuestReplay,24},

	{IT_WHITESTRING|IT_CALL, NULL, "Delete Guest Replay",  NULL,     M_SetGuestReplay,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",    NULL,             &SP_NightsAttackDef, 50}
};

static menuitem_t SP_GhostMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Best Score", NULL,  &cv_ghost_bestscore, 0},
	{IT_STRING|IT_CVAR,         NULL, "Best Time", NULL,  &cv_ghost_besttime,  8},
	{IT_STRING|IT_CVAR,         NULL, "Best Rings", NULL,  &cv_ghost_bestrings,16},
	{IT_STRING|IT_CVAR,         NULL, "Last",   NULL,     &cv_ghost_last,     24},

	{IT_STRING|IT_CVAR,         NULL, "Guest", NULL,      &cv_ghost_guest,    37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",  NULL,     &SP_TimeAttackDef,  50}
};

static menuitem_t SP_NightsGhostMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Best Score", NULL,  &cv_ghost_bestscore, 8},
	{IT_STRING|IT_CVAR,         NULL, "Best Time", NULL,   &cv_ghost_besttime, 16},
	{IT_STRING|IT_CVAR,         NULL, "Last",   NULL,     &cv_ghost_last,     24},

	{IT_STRING|IT_CVAR,         NULL, "Guest",  NULL,     &cv_ghost_guest,    37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",  NULL,      &SP_NightsAttackDef,  50}
};

// Single Player Nights Attack
static menuitem_t SP_NightsAttackMenu[] =
{
	{IT_STRING|IT_CVAR,        NULL, "Level",      NULL,       &cv_nextmap,          44},
	{IT_STRING|IT_CVAR,        NULL, "Show Records For",  NULL, &cv_dummymares,       54},

	{IT_DISABLED,              NULL, "Guest Option...", NULL,   &SP_NightsGuestReplayDef,   108},
	{IT_DISABLED,              NULL, "Replay...",   NULL,      &SP_NightsReplayDef,        118},
	{IT_DISABLED,              NULL, "Ghosts...",    NULL,     &SP_NightsGhostDef,         128},
	{IT_WHITESTRING|IT_CALL|IT_CALL_NOTMODIFIED,   NULL, "Start", NULL,            M_ChooseNightsAttack, 138},
};

enum
{
	nalevel,
	narecords,

	naguest,
	nareplay,
	naghost,
	nastart
};

// Statistics
static menuitem_t SP_LevelStatsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", NULL,  M_HandleLevelStats, '\0'},     // dummy menuitem for the control func
};

// A rare case.
// External files modify this menu, so we can't call it static.
// And I'm too lazy to go through and rename it everywhere. ARRGH!
menuitem_t PlayerMenu[MAXSKINS];

// -----------------------------------
// Multiplayer and all of its submenus
// -----------------------------------
// Prefix: MP_
static menuitem_t MP_MainMenu[] =
{
#ifndef NONET
	{IT_CALL | IT_STRING, NULL, "HOST GAME",      NULL,         M_StartServerMenu,      10},
	{IT_CALL | IT_STRING, NULL, "JOIN GAME (Search)", NULL, 	  M_ConnectMenu,		  30},
	{IT_KEYHANDLER | IT_STRING, NULL, "JOIN GAME (Specify IP)", NULL,  M_HandleConnectIP,        40},
	{IT_STRING|IT_CALL, NULL, "JOIN LAST SERVER", NULL,     M_ConnectLastServer,        65},
#endif
	{IT_CALL | IT_STRING, NULL, "TWO PLAYER GAME",  NULL,       M_StartSplitServerMenu, 95},

	{IT_CALL | IT_STRING, NULL, "SETUP PLAYER 1",   NULL,       M_SetupMultiPlayer,     115},
	{IT_CALL | IT_STRING, NULL, "SETUP PLAYER 2",  NULL,        M_SetupMultiPlayer2,    125},
};

static menuitem_t MP_ServerMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",  NULL,            &cv_newgametype,    10},
#ifndef NONET
	{IT_STRING|IT_CALL,              NULL, "Room...",     NULL,           M_RoomMenu,         20},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Server Name",    NULL,        &cv_servername,     30},
#endif

	{IT_STRING|IT_CVAR,              NULL, "Level",   NULL,               &cv_nextmap,        80},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",    NULL,              M_StartServer,     130},
};

enum
{
	mp_server_gametype = 0,
#ifndef NONET
	mp_server_room,
	mp_server_name,
#endif
	mp_server_level,
	mp_server_start
};

#ifndef NONET
static menuitem_t MP_ConnectMenu[] =
{
	{IT_STRING | IT_CALL,       NULL, "Room...", NULL,   M_RoomMenu,         4},
	{IT_STRING | IT_CVAR,       NULL, "Sort By", NULL,   &cv_serversort,     12},
	{IT_STRING | IT_KEYHANDLER, NULL, "Page",  NULL,     M_HandleServerPage, 20},
	{IT_STRING | IT_CALL,       NULL, "Refresh", NULL,   M_Refresh,          28},

	{IT_STRING | IT_SPACE, NULL, "",   NULL,           M_Connect,          48-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,          M_Connect,          60-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,          M_Connect,          72-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,         M_Connect,          84-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,          M_Connect,          96-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,          M_Connect,         108-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,           M_Connect,         120-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,            M_Connect,         132-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,            M_Connect,         144-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,           M_Connect,         156-4},
	{IT_STRING | IT_SPACE, NULL, "",   NULL,            M_Connect,         168-4},
};

enum
{
	mp_connect_room,
	mp_connect_sort,
	mp_connect_page,
	mp_connect_refresh,
	FIRSTSERVERLINE
};

static menuitem_t MP_RoomMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "<Offline Mode>", NULL,  M_ChooseRoom,   9},
	{IT_DISABLED,         NULL, "",      NULL,          M_ChooseRoom,  18},
	{IT_DISABLED,         NULL, "",      NULL,          M_ChooseRoom,  27},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom,  36},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom,  45},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom,  54},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom,  63},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom,  72},
	{IT_DISABLED,         NULL, "",        NULL,        M_ChooseRoom,  81},
	{IT_DISABLED,         NULL, "",        NULL,        M_ChooseRoom,  90},
	{IT_DISABLED,         NULL, "",        NULL,        M_ChooseRoom,  99},
	{IT_DISABLED,         NULL, "",        NULL,        M_ChooseRoom, 108},
	{IT_DISABLED,         NULL, "",        NULL,        M_ChooseRoom, 117},
	{IT_DISABLED,         NULL, "",        NULL,        M_ChooseRoom, 126},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom, 135},
	{IT_DISABLED,         NULL, "",        NULL,        M_ChooseRoom, 144},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom, 153},
	{IT_DISABLED,         NULL, "",       NULL,         M_ChooseRoom, 162},
};
#endif

// Separated splitscreen and normal servers.
static menuitem_t MP_SplitServerMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",     NULL,         &cv_newgametype,    10},
	{IT_STRING|IT_CVAR,              NULL, "Level",        NULL,          &cv_nextmap,        80},
	{IT_WHITESTRING|IT_CALL,         NULL, "Start",       NULL,           M_StartServer,     130},
};

static menuitem_t MP_PlayerSetupMenu[] =
{
	{IT_KEYHANDLER | IT_STRING, NULL, "Name", NULL,  M_HandleSetupMultiPlayer, 0}, // name
	{IT_KEYHANDLER | IT_STRING, NULL, "Character", NULL,  M_HandleSetupMultiPlayer, 16}, // skin
	{IT_KEYHANDLER, NULL, "Color", NULL,  M_HandleSetupMultiPlayer, 96}, // colour
	{IT_KEYHANDLER, NULL, "", NULL,  M_HandleSetupMultiPlayer, 0}, // default
};

// ------------------------------------
// Options and most (?) of its submenus
// ------------------------------------
// Prefix: OP_
static menuitem_t OP_MainMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Setup Controls...", NULL,      &OP_ControlsDef,      10},

	{IT_CALL | IT_STRING, NULL, "Video Options...", NULL,      M_VideoOptions,  30},
	{IT_SUBMENU | IT_STRING, NULL, "Sound Options...", NULL,      &OP_SoundOptionsDef,  40},
	{IT_SUBMENU | IT_STRING, NULL, "Data Options...",  NULL,      &OP_DataOptionsDef,   50},

	{IT_SUBMENU | IT_STRING, NULL, "Legacy Options...", NULL,     &OP_LegacyOptionsDef,  70},

	{IT_SUBMENU | IT_STRING, NULL, "Game Options...",  NULL,      &OP_GameOptionsDef,   90},
	{IT_SUBMENU | IT_STRING, NULL, "Server Options...", NULL,     &OP_ServerOptionsDef, 100},
	{IT_STRING  | IT_CALL,   NULL, "Add-on Options...", NULL,     M_AddonsOptions,      110},
};

static menuitem_t OP_ControlsMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Player 1 Controls...", NULL,  &OP_P1ControlsDef,  10},
	{IT_SUBMENU | IT_STRING, NULL, "Player 2 Controls...", NULL,  &OP_P2ControlsDef,  20},

	{IT_STRING  | IT_CVAR, NULL, "Controls per key", NULL,  &cv_controlperkey, 40},
};

static menuitem_t OP_P1ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", NULL,  M_Setup1PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Mouse Options...", NULL,  &OP_MouseOptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Joystick Options...", NULL,  &OP_Joystick1Def  ,  30},

	{IT_STRING  | IT_CVAR, NULL, "Camera"  , NULL,  &cv_chasecam  ,  50},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", NULL,  &cv_crosshair , 60},

	{IT_STRING  | IT_CVAR, NULL, "Analog Control", NULL,  &cv_useranalog,  80},
};

static menuitem_t OP_P2ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", NULL,  M_Setup2PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Second Mouse Options...", NULL,  &OP_Mouse2OptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Second Joystick Options...", NULL,  &OP_Joystick2Def  ,  30},

	{IT_CVAR | IT_STRING, NULL, "Camera", NULL, &cv_chasecam2,	50},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", NULL, &cv_crosshair2, 60},

	{IT_STRING  | IT_CVAR, NULL, "Analog Control", NULL,  &cv_useranalog2,  80},
};

static menuitem_t OP_AllControlsMenu[] =
{
	{IT_HEADER, NULL, "  Movement", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Move Forward",  NULL,     M_ChangeControl, gc_forward     },
	{IT_CALL | IT_STRING2, NULL, "Move Backward",  NULL,   M_ChangeControl, gc_backward    },
	{IT_CALL | IT_STRING2, NULL, "Move Left",  NULL,       M_ChangeControl, gc_strafeleft  },
	{IT_CALL | IT_STRING2, NULL, "Move Right",  NULL,      M_ChangeControl, gc_straferight },
	{IT_CALL | IT_STRING2, NULL, "Jump",        NULL,      M_ChangeControl, gc_jump      },
	{IT_CALL | IT_STRING2, NULL, "Spin",      NULL,        M_ChangeControl, gc_use     },
	{IT_HEADER, NULL, "  Camera", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Look Up",  NULL,       M_ChangeControl, gc_lookup      },
	{IT_CALL | IT_STRING2, NULL, "Look Down",  NULL,     M_ChangeControl, gc_lookdown    },
	{IT_CALL | IT_STRING2, NULL, "Turn Left",  NULL,     M_ChangeControl, gc_turnleft    },
	{IT_CALL | IT_STRING2, NULL, "Turn Right",  NULL,    M_ChangeControl, gc_turnright   },
	{IT_CALL | IT_STRING2, NULL, "Center View", NULL,      M_ChangeControl, gc_centerview  },
	{IT_CALL | IT_STRING2, NULL, "Toggle Mouselook", NULL,  M_ChangeControl, gc_mouseaiming },
	{IT_CALL | IT_STRING2, NULL, "Toggle Third-Person", NULL,  M_ChangeControl, gc_camtoggle},
	{IT_CALL | IT_STRING2, NULL, "Reset Camera", NULL,    M_ChangeControl, gc_camreset    },
	{IT_HEADER, NULL, "  Advanced", NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera L", NULL,   M_ChangeControl, gc_camleft      },
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera R", NULL,   M_ChangeControl, gc_camright     },
	{IT_HEADER, NULL, "  Multiplayer", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Talk key",  NULL,        M_ChangeControl, gc_talkkey      },
	{IT_CALL | IT_STRING2, NULL, "Team-Talk key", NULL,    M_ChangeControl, gc_teamkey      },
	{IT_CALL | IT_STRING2, NULL, "Rankings/Scores", NULL,  M_ChangeControl, gc_scores       },
	{IT_CALL | IT_STRING2, NULL, "Toss Flag",    NULL,     M_ChangeControl, gc_tossflag     },
	{IT_CALL | IT_STRING2, NULL, "Next Weapon", NULL,      M_ChangeControl, gc_weaponnext   },
	{IT_CALL | IT_STRING2, NULL, "Prev Weapon",  NULL,     M_ChangeControl, gc_weaponprev   },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 1", NULL,    M_ChangeControl, gc_wepslot1     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 2", NULL,    M_ChangeControl, gc_wepslot2     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 3",  NULL,   M_ChangeControl, gc_wepslot3     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 4",  NULL,   M_ChangeControl, gc_wepslot4     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 5",  NULL,   M_ChangeControl, gc_wepslot5     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 6",  NULL,   M_ChangeControl, gc_wepslot6     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 7", NULL,    M_ChangeControl, gc_wepslot7     },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss",     NULL,    M_ChangeControl, gc_fire         },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss Normal", NULL,  M_ChangeControl, gc_firenormal   },
	{IT_HEADER, NULL, "  Miscellaneous", NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Custom Action 1", NULL,   M_ChangeControl, gc_custom1      },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 2", NULL,  M_ChangeControl, gc_custom2      },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 3",NULL,   M_ChangeControl, gc_custom3      },

	{IT_CALL | IT_STRING2, NULL, "Pause",       NULL,      M_ChangeControl, gc_pause        },
	{IT_CALL | IT_STRING2, NULL, "Screenshot",   NULL,          M_ChangeControl, gc_screenshot },
	{IT_CALL | IT_STRING2, NULL, "Toggle GIF Recording", NULL,  M_ChangeControl, gc_recordgif  },
	{IT_CALL | IT_STRING2, NULL, "Open/Close Menu (ESC)", NULL,  M_ChangeControl, gc_systemmenu },
	{IT_CALL | IT_STRING2, NULL, "Change Viewpoint", NULL,      M_ChangeControl, gc_viewpoint  },
	{IT_CALL | IT_STRING2, NULL, "Console",  NULL,         M_ChangeControl, gc_console      },
};

static menuitem_t OP_AllControls2Menu[] =
{
	{IT_HEADER, NULL, "  Movement", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Move Forward", NULL,     M_ChangeControl, gc_forward     },
	{IT_CALL | IT_STRING2, NULL, "Move Backward",  NULL,   M_ChangeControl, gc_backward    },
	{IT_CALL | IT_STRING2, NULL, "Move Left",    NULL,     M_ChangeControl, gc_strafeleft  },
	{IT_CALL | IT_STRING2, NULL, "Move Right",   NULL,     M_ChangeControl, gc_straferight },
	{IT_CALL | IT_STRING2, NULL, "Jump",       NULL,       M_ChangeControl, gc_jump      },
	{IT_CALL | IT_STRING2, NULL, "Spin",     NULL,         M_ChangeControl, gc_use     },
	{IT_HEADER, NULL, "  Camera", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Look Up",  NULL,       M_ChangeControl, gc_lookup      },
	{IT_CALL | IT_STRING2, NULL, "Look Down", NULL,      M_ChangeControl, gc_lookdown    },
	{IT_CALL | IT_STRING2, NULL, "Turn Left",  NULL,     M_ChangeControl, gc_turnleft    },
	{IT_CALL | IT_STRING2, NULL, "Turn Right",  NULL,    M_ChangeControl, gc_turnright   },
	{IT_CALL | IT_STRING2, NULL, "Center View", NULL,      M_ChangeControl, gc_centerview  },
	{IT_CALL | IT_STRING2, NULL, "Toggle Mouselook", NULL,  M_ChangeControl, gc_mouseaiming },
	{IT_CALL | IT_STRING2, NULL, "Toggle Third-Person", NULL,  M_ChangeControl, gc_camtoggle},
	{IT_CALL | IT_STRING2, NULL, "Reset Camera",  NULL,    M_ChangeControl, gc_camreset    },
	{IT_HEADER, NULL, "  Advanced", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera L", NULL,   M_ChangeControl, gc_camleft      },
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera R", NULL,   M_ChangeControl, gc_camright     },
	{IT_HEADER, NULL, "  Multiplayer", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Toss Flag",   NULL,      M_ChangeControl, gc_tossflag     },
	{IT_CALL | IT_STRING2, NULL, "Next Weapon",  NULL,     M_ChangeControl, gc_weaponnext   },
	{IT_CALL | IT_STRING2, NULL, "Prev Weapon",  NULL,     M_ChangeControl, gc_weaponprev   },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 1", NULL,    M_ChangeControl, gc_wepslot1     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 2", NULL,    M_ChangeControl, gc_wepslot2     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 3", NULL,    M_ChangeControl, gc_wepslot3     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 4", NULL,    M_ChangeControl, gc_wepslot4     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 5", NULL,     M_ChangeControl, gc_wepslot5     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 6",  NULL,   M_ChangeControl, gc_wepslot6     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 7", NULL,    M_ChangeControl, gc_wepslot7     },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss",     NULL,    M_ChangeControl, gc_fire         },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss Normal", NULL,  M_ChangeControl, gc_firenormal   },
	{IT_HEADER, NULL, "  Miscellaneous", NULL,  NULL, 0},
	{IT_CALL | IT_STRING2, NULL, "Custom Action 1", NULL,   M_ChangeControl, gc_custom1      },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 2", NULL,  M_ChangeControl, gc_custom2      },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 3", NULL,  M_ChangeControl, gc_custom3      },

	{IT_CALL | IT_STRING2, NULL, "Screenshot",  NULL,           M_ChangeControl, gc_screenshot },
	{IT_CALL | IT_STRING2, NULL, "Toggle GIF Recording", NULL,   M_ChangeControl, gc_recordgif  },
	{IT_CALL | IT_STRING2, NULL, "Open/Close Menu (ESC)", NULL,  M_ChangeControl, gc_systemmenu },
	{IT_CALL | IT_STRING2, NULL, "Change Viewpoint", NULL,       M_ChangeControl, gc_viewpoint  },
};

static menuitem_t OP_Joystick1Menu[] =
{
	{IT_STRING | IT_CALL,  NULL, "Select Joystick...", NULL,  M_Setup1PJoystickMenu,  10},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Turning"  , NULL,  &cv_turnaxis         ,  30},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Moving"   , NULL,  &cv_moveaxis         ,  40},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Strafe"   , NULL,  &cv_sideaxis         ,  50},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Looking"  , NULL,  &cv_lookaxis         ,  60},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Jumping"  , NULL,  &cv_jumpaxis         ,  70},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Spinning" , NULL,  &cv_spinaxis         ,  80},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Firing"   , NULL,  &cv_fireaxis         ,  90},
	{IT_STRING | IT_CVAR,  NULL, "Axis For NFiring"  , NULL,  &cv_firenaxis        , 100},

	{IT_STRING | IT_CVAR, NULL, "First-Person Vert-Look", NULL,  &cv_alwaysfreelook, 120},
	{IT_STRING | IT_CVAR, NULL, "Third-Person Vert-Look", NULL,  &cv_chasefreelook,  130},
};

static menuitem_t OP_Joystick2Menu[] =
{
	{IT_STRING | IT_CALL,  NULL, "Select Joystick...", NULL,  M_Setup2PJoystickMenu,  10},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Turning"  , NULL,  &cv_turnaxis2        ,  30},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Moving"   , NULL,  &cv_moveaxis2        ,  40},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Strafe"   , NULL,  &cv_sideaxis2        ,  50},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Looking"  , NULL,  &cv_lookaxis2        ,  60},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Jumping"  , NULL,  &cv_jumpaxis2        ,  70},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Spinning" , NULL,  &cv_spinaxis2        ,  80},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Firing"   , NULL,  &cv_fireaxis2        ,  90},
	{IT_STRING | IT_CVAR,  NULL, "Axis For NFiring"  , NULL,  &cv_firenaxis2       , 100},

	{IT_STRING | IT_CVAR, NULL, "First-Person Vert-Look", NULL, &cv_alwaysfreelook2,120},
	{IT_STRING | IT_CVAR, NULL, "Third-Person Vert-Look", NULL, &cv_chasefreelook2, 130},
};

static menuitem_t OP_JoystickSetMenu[] =
{
	{IT_CALL | IT_NOTHING, "None", NULL,  NULL, M_AssignJoystick, '0'},
	{IT_CALL | IT_NOTHING, "", NULL,  NULL, M_AssignJoystick, '1'},
	{IT_CALL | IT_NOTHING, "", NULL,  NULL, M_AssignJoystick, '2'},
	{IT_CALL | IT_NOTHING, "",  NULL, NULL, M_AssignJoystick, '3'},
	{IT_CALL | IT_NOTHING, "", NULL,  NULL, M_AssignJoystick, '4'},
	{IT_CALL | IT_NOTHING, "", NULL,  NULL, M_AssignJoystick, '5'},
	{IT_CALL | IT_NOTHING, "", NULL,  NULL, M_AssignJoystick, '6'},
};

static menuitem_t OP_MouseOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse",  NULL,        &cv_usemouse,         10},


	{IT_STRING | IT_CVAR, NULL, "First-Person MouseLook", NULL,  &cv_alwaysfreelook,   30},
	{IT_STRING | IT_CVAR, NULL, "Third-Person MouseLook", NULL,  &cv_chasefreelook,   40},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move", NULL,       &cv_mousemove,        50},
	{IT_STRING | IT_CVAR, NULL, "Invert Mouse", NULL,      &cv_invertmouse,      60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Speed", NULL,    &cv_mousesens,        70},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Speed",  NULL,   &cv_mouseysens,        80},
};

static menuitem_t OP_Mouse2OptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse 2", NULL,       &cv_usemouse2,        10},
	{IT_STRING | IT_CVAR, NULL, "Second Mouse Serial Port", NULL,
	                                                &cv_mouse2port,       20},
	{IT_STRING | IT_CVAR, NULL, "First-Person MouseLook", NULL,  &cv_alwaysfreelook2,  30},
	{IT_STRING | IT_CVAR, NULL, "Third-Person MouseLook", NULL,  &cv_chasefreelook2,  40},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",   NULL,     &cv_mousemove2,       50},
	{IT_STRING | IT_CVAR, NULL, "Invert Mouse", NULL,      &cv_invertmouse2,     60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Speed", NULL,    &cv_mousesens2,       70},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Speed", NULL,    &cv_mouseysens2,      80},
};

enum
{
	op_video_resolution = 0,
	op_video_renderer,
};

static menuitem_t OP_VideoOptionsMenu[] =
{
	{IT_STRING | IT_CALL,  NULL,   "Video Modes...", "Change game resolution",      M_VideoModeMenu,          5},

#ifdef HWRENDER
	{IT_STRING | IT_CVAR, NULL, "Renderer (F10)",      NULL,         &cv_renderer,        10},
#else
	{IT_TRANSTEXT | IT_PAIR, "Renderer", "Software",   NULL,         &cv_renderer,        10},
#endif


#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	{IT_STRING|IT_CVAR,    NULL,   "Fullscreen (F11)",  NULL,        &cv_fullscreen,      15},
#endif

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness", NULL,  &cv_globalgamma,      25},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", NULL,  &cv_globalsaturation, 30},
	{IT_SUBMENU|IT_STRING, NULL, "Advanced Color Settings...", "Adjust color mapping and per-color brightness",  &OP_ColorOptionsDef, 35},

	{IT_STRING | IT_CVAR,  NULL, "Draw Distance",   NULL,       &cv_drawdist,             45},
	{IT_STRING | IT_CVAR,  NULL, "NiGHTS Hoop Draw Dist", NULL, &cv_drawdist_nights,      50},
	{IT_STRING | IT_CVAR,  NULL, "Precip Draw Dist",   NULL,    &cv_drawdist_precip,      55},
	{IT_STRING | IT_CVAR,  NULL, "Precip Density",    NULL,     &cv_precipdensity,        60},
	{IT_STRING | IT_CVAR,  NULL, "Show FPS",       NULL,        &cv_ticrate,              65},
	{IT_STRING | IT_CVAR,  NULL, "Show TPS",        NULL,       &cv_tpscounter,           70},
	{IT_STRING | IT_CVAR,  NULL, "FPS/TPS Counter Size", NULL,  &cv_fpssize,              75},
	{IT_STRING | IT_CVAR,  NULL, "Clear Before Redraw",  NULL,  &cv_homremoval,           80},
	{IT_STRING | IT_CVAR,  NULL, "Vertical Sync",    NULL,      &cv_vidwait,              85},
	{IT_STRING | IT_CVAR,  NULL, "FPS Cap",         NULL,       &cv_fpscap,               90},
#ifdef HWRENDER
	{IT_CALL | IT_STRING, NULL, "OpenGL Options...",  "Change OpenGL-specific options",        M_OpenGLOptionsMenu, 100},
#endif
};

static menuitem_t OP_ColorOptionsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Reset all", NULL,  M_ResetCvars, 0},

	{IT_HEADER, NULL, "Red", NULL,  NULL, 9},
	{IT_DISABLED, NULL, NULL, NULL,  NULL, 35},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue", NULL,          &cv_rhue,         15},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation",  NULL,  &cv_rsaturation,  20},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness",NULL,    &cv_rgamma,       25},

	{IT_HEADER, NULL, "Yellow", NULL,  NULL, 34},
	{IT_DISABLED, NULL, NULL, NULL,  NULL, 73},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",  NULL,         &cv_yhue,         40},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", NULL,   &cv_ysaturation,  45},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness", NULL,  &cv_ygamma,       50},

	{IT_HEADER, NULL, "Green", NULL,  NULL, 59},
	{IT_DISABLED, NULL, NULL, NULL,  NULL, 112},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",  NULL,         &cv_ghue,         65},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", NULL,   &cv_gsaturation,  70},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness", NULL,   &cv_ggamma,       75},

	{IT_HEADER, NULL, "Cyan", NULL, NULL,  84},
	{IT_DISABLED, NULL, NULL, NULL, NULL, 255},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",    NULL,       &cv_chue,         90},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", NULL,   &cv_csaturation,  95},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness", NULL,   &cv_cgamma,      100},

	{IT_HEADER, NULL, "Blue", NULL, NULL,  109},
	{IT_DISABLED, NULL, NULL, NULL, NULL, 152},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",   NULL,        &cv_bhue,        115},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", NULL,   &cv_bsaturation, 120},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness", NULL,   &cv_bgamma,      125},

	{IT_HEADER, NULL, "Magenta", NULL,  NULL, 134},
	{IT_DISABLED, NULL, NULL, NULL, NULL,  181},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",    NULL,       &cv_mhue,        140},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", NULL,   &cv_msaturation, 145},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness", NULL,   &cv_mgamma,      150},

#ifdef HWRENDER
	{IT_STRING | IT_CALL, NULL,   "OpenGL Options...", NULL,    M_OpenGLOptionsMenu,    160},
#endif
};

static void M_VideoOptions(INT32 choice)
{
	(void)choice;

	OP_VideoOptionsMenu[op_video_renderer].status = (IT_TRANSTEXT | IT_PAIR);
	OP_VideoOptionsMenu[op_video_renderer].patch = "Renderer";
	OP_VideoOptionsMenu[op_video_renderer].text = "Software";

#ifdef HWRENDER
	if (vid.glstate != VID_GL_LIBRARY_ERROR)
	{
		OP_VideoOptionsMenu[op_video_renderer].status = (IT_STRING | IT_CVAR);
		OP_VideoOptionsMenu[op_video_renderer].patch = NULL;
		OP_VideoOptionsMenu[op_video_renderer].text = "Renderer (F10)";
	}

#endif
	M_SetupNextMenu(&OP_VideoOptionsDef);
}

static menuitem_t OP_VideoModeMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", NULL,  M_HandleVideoMode, '\0'},     // dummy menuitem for the control func
};

#ifdef HWRENDER
static menuitem_t OP_OpenGLOptionsMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "3D Models", NULL,     &cv_glmd2,      10},
	{IT_STRING|IT_CVAR,         NULL, "Model Interpolation",  NULL,    &cv_glmodelinterpolation,      20},
	{IT_STRING|IT_CVAR,         NULL, "Ambient lighting", NULL,     &cv_glmodellighting,      30},

	{IT_STRING|IT_CVAR,         NULL, "Shaders", NULL, 	     &cv_glshaders,        50},
	{IT_STRING|IT_CVAR,         NULL, "Lack of Perspective", NULL,  &cv_glshearing,   60},
	{IT_STRING|IT_CVAR,         NULL, "Palette Rendering", NULL,  &cv_glpaletterendering,   70},
	{IT_STRING|IT_CVAR|IT_CV_SLIDER,  NULL, "Field of view", NULL,   &cv_fov,            90},
	{IT_STRING|IT_CVAR,         NULL, "Quality",     NULL,     &cv_scr_depth,        100},
	{IT_STRING|IT_CVAR,         NULL, "Texture Filter", NULL,   &cv_glfiltermode,     110},
	{IT_STRING|IT_CVAR,         NULL, "Anisotropic",  NULL,    &cv_glanisotropicmode,120},
	{IT_STRING|IT_CVAR,         NULL, "OpenGL Loading Screen", NULL,  &cv_glloadingscreen, 130},
};

#endif

static menuitem_t OP_SoundOptionsMenu[] =
{
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Sound Volume" , NULL,  &cv_soundvolume,     10},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Music Volume" , NULL,  &cv_digmusicvolume,  20},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "MIDI Volume"  , NULL,  &cv_midimusicvolume, 30},
#ifdef PC_DOS
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "CD Volume"    , NULL,  &cd_volume,          40},
#endif

	{IT_STRING | IT_CVAR,  NULL,  "SFX"   , NULL,  &cv_gamesounds,        50},
	{IT_STRING | IT_CVAR,  NULL,  "Digital Music", NULL,  &cv_gamedigimusic,     60},
	{IT_STRING | IT_CVAR,  NULL,  "MIDI Music", NULL,  &cv_gamemidimusic,        70},

	{IT_STRING | IT_CVAR,  NULL,  "Music Preference", NULL,  &cv_musicpref,      90},

	{IT_STRING 	  | IT_SUBMENU, NULL, "Advanced Settings...", "Extra game sound settings",  &OP_SoundAdvancedDef, 110},
};

#ifdef HAVE_OPENMPT
#define OPENMPT_MENUOFFSET 40
#else
#define OPENMPT_MENUOFFSET 0
#endif

static menuitem_t OP_SoundAdvancedMenu[] =
{
#ifdef HAVE_OPENMPT
	{IT_HEADER, NULL, "OpenMPT Settings", NULL, NULL,  10},
	{IT_STRING | IT_CVAR, NULL, "Instrument Filter", NULL,  &cv_modfilter, 20},
#endif

	{IT_HEADER, NULL, "Miscellaneous", NULL,  NULL, OPENMPT_MENUOFFSET},
	{IT_STRING | IT_CVAR,  NULL,  "Play SFX if Unfocused", NULL,  &cv_playsoundsifunfocused,  OPENMPT_MENUOFFSET+10},
	{IT_STRING | IT_CVAR,  NULL,  "Play Music if Unfocused", NULL,  &cv_playmusicifunfocused, OPENMPT_MENUOFFSET+20},
};

#undef OPENMPT_MENUOFFSET

static menuitem_t OP_DataOptionsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Screenshot Options...", NULL,  M_ScreenshotOptions, 10},

	{IT_STRING | IT_SUBMENU, NULL, "Erase Data...", NULL,  &OP_EraseDataDef, 30},
};

static menuitem_t OP_ScreenshotOptionsMenu[] =
{
	{IT_HEADER, NULL, "Screenshots (F8)", NULL,  NULL, 0},
	{IT_STRING|IT_CVAR, NULL, "Storage Location", NULL,   &cv_screenshot_option,          5},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder", NULL,  &cv_screenshot_folder, 10},
	{IT_STRING|IT_CVAR, NULL, "Memory Level", NULL,       &cv_zlib_memory,                25},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", NULL,  &cv_zlib_level,                 30},
	{IT_STRING|IT_CVAR, NULL, "Strategy", NULL,          &cv_zlib_strategy,              35},
	{IT_STRING|IT_CVAR, NULL, "Window Size", NULL,       &cv_zlib_window_bits,           40},
	{IT_HEADER, NULL, "Movie Mode (F9)", NULL,  NULL, 50},
	{IT_STRING|IT_CVAR, NULL, "Storage Location", NULL,   &cv_movie_option,              55},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder", NULL,  &cv_movie_folder,     60},
	{IT_STRING|IT_CVAR, NULL, "Capture Mode", NULL,       &cv_moviemode,                 75},
	// Shows when GIF is selected
	{IT_STRING|IT_CVAR, NULL, "Region Optimizing", NULL,  &cv_gif_optimize,              80},
	{IT_STRING|IT_CVAR, NULL, "Downscaling", NULL,       &cv_gif_downscale,             85},
	// Shows when APNG is selected
	{IT_STRING|IT_CVAR, NULL, "Memory Level", NULL,       &cv_zlib_memorya,              80},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", NULL,  &cv_zlib_levela,               85},
	{IT_STRING|IT_CVAR, NULL, "Strategy",   NULL,        &cv_zlib_strategya,            90},
	{IT_STRING|IT_CVAR, NULL, "Window Size",  NULL,      &cv_zlib_window_bitsa,         95},
};

enum
{
	op_screenshot_folder = 2,
	op_movie_folder = 9,
	op_screenshot_capture = 10,
	op_screenshot_gif_start = 11,
	op_screenshot_gif_end = 12,
	op_screenshot_apng_start = 13,
	op_screenshot_apng_end = 16,
};

static menuitem_t OP_EraseDataMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Erase Record Data", NULL,  M_EraseData, 10},
	{IT_STRING | IT_CALL, NULL, "Erase Secrets Data", NULL,  M_EraseData, 20},

	{IT_STRING | IT_CALL, NULL, "\x85" "Erase ALL Data", NULL,  M_EraseData, 40},
};

static menuitem_t OP_AddonsOptionsMenu[] =
{
	{IT_HEADER,                      NULL, "Menu",       NULL,                  NULL,                    0},
	{IT_STRING|IT_CVAR,              NULL, "Location",   NULL,                  &cv_addons_option,      10},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder",   NULL,             &cv_addons_folder,      20},
	{IT_STRING|IT_CVAR,              NULL, "Identify add-ons via", NULL,        &cv_addons_md5,         48},
	{IT_STRING|IT_CVAR,              NULL, "Show unsupported file types", NULL,  &cv_addons_showall,     58},

	{IT_HEADER,                      NULL, "Search",    NULL,                   NULL,                   76},
	{IT_STRING|IT_CVAR,              NULL, "Matching",   NULL,                  &cv_addons_search_type, 86},
	{IT_STRING|IT_CVAR,              NULL, "Case-sensitive",   NULL,            &cv_addons_search_case, 96},
};

enum
{
	op_addons_folder = 2,
};

static menuitem_t OP_GameOptionsMenu[] =
{
#ifndef NONET
	{IT_STRING | IT_CVAR | IT_CV_STRING,
	                      NULL, "Master server",     NULL,       &cv_masterserver,       5},
	{IT_STRING | IT_SUBMENU, NULL, "Chat Options...", "Change how the chat display looks",      &OP_ChatOptionsDef,     20},
#endif
	{IT_STRING | IT_CVAR, NULL, "Show HUD",       NULL,          &cv_showhud,            25},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "HUD Visibility",    NULL,       &cv_translucenthud,     30},
	{IT_STRING | IT_CVAR, NULL, "Timer/Rings Display", NULL,     &cv_timetic,            35},
	{IT_STRING | IT_CVAR, NULL, "Score Display",     NULL,       &cv_scorepos,           40},
	{IT_STRING | IT_CVAR, NULL, "Always Compact Rankings", NULL,  &cv_compactscoreboard,  45},
	{IT_STRING | IT_CVAR, NULL, "Local ping display", NULL,      &cv_showping,           50}, // shows ping next to framerate if we want to.
#ifdef SEENAMES
	{IT_STRING | IT_CVAR, NULL, "HUD Player Names",   NULL,      &cv_seenames,           55},
#endif
	{IT_STRING | IT_CVAR, NULL, "Log Hazard Damage",  NULL,      &cv_hazardlog,          60},
	{IT_STRING | IT_CVAR, NULL, "Console Back Color", NULL,      &cons_backcolor,        65},
	{IT_STRING | IT_CVAR, NULL, "Console Text Size", NULL,       &cv_constextsize,       70},
	{IT_STRING | IT_CVAR, NULL, "Uppercase Console",  NULL,      &cv_allcaps,            75},
	{IT_STRING | IT_CVAR, NULL, "Show \"FOCUS LOST\"", NULL,     &cv_showfocuslost,      80},
	{IT_STRING | IT_CVAR, NULL, "Modern Pause Screen", NULL,    &cv_modernpause,        85},
	{IT_STRING | IT_CVAR, NULL, "Title Screen Demos",  NULL,     &cv_rollingdemos,       90},
};

static menuitem_t OP_ChatOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Chat Mode",       NULL,      		 	 &cv_consolechat,  10},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Chat Box Width", NULL,    &cv_chatwidth,     30},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Chat Box Height", NULL,   &cv_chatheight,    40},
	{IT_STRING | IT_CVAR, NULL, "Message Fadeout Time",   NULL,            &cv_chattime,    50},
	{IT_STRING | IT_CVAR, NULL, "Chat Notifications",     NULL,       	 &cv_chatnotifications,  60},
	{IT_STRING | IT_CVAR, NULL, "Spam Protection",        NULL,    		 &cv_chatspamprotection,  70},
	{IT_STRING | IT_CVAR, NULL, "Chat background tint",   NULL,         	 &cv_chatbacktint,  80},
};

static menuitem_t OP_ServerOptionsMenu[] =
{
	{IT_STRING | IT_SUBMENU, NULL, "General netgame options...", NULL,   &OP_NetgameOptionsDef,  10},
	{IT_STRING | IT_SUBMENU, NULL, "Gametype options...",      NULL,    &OP_GametypeOptionsDef, 20},
	{IT_STRING | IT_SUBMENU, NULL, "Random Monitor Toggles...", NULL,   &OP_MonitorToggleDef,   30},

#ifndef NONET
	{IT_STRING | IT_CVAR | IT_CV_STRING,
	                         NULL, "Server name",       NULL,           &cv_servername,         50},
#endif

	{IT_STRING | IT_CVAR,    NULL, "Intermission Timer",   NULL,        &cv_inttime,            80},
	{IT_STRING | IT_CVAR,    NULL, "Advance to next map",  NULL,       &cv_advancemap,         90},

#ifndef NONET
	{IT_STRING | IT_CVAR,    NULL, "Max Players",         NULL,         &cv_maxplayers,        110},
	{IT_STRING | IT_CVAR,    NULL, "Allow players to join",  NULL,      &cv_allownewplayer,    120},
	{IT_STRING | IT_CVAR,    NULL, "Allow WAD Downloading",  NULL,      &cv_downloading,       130},
	{IT_STRING | IT_CVAR,    NULL, "Attempt to Resynch",   NULL,      &cv_allowgamestateresend,   140},
#endif
};

static menuitem_t OP_NetgameOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Time Limit",  NULL,           &cv_timelimit,        10},
	{IT_STRING | IT_CVAR, NULL, "Point Limit", NULL,           &cv_pointlimit,       18},
	{IT_STRING | IT_CVAR, NULL, "Overtime Tie-Breaker", NULL,   &cv_overtime,         26},

	{IT_STRING | IT_CVAR, NULL, "Special Ring Weapons", NULL,   &cv_specialrings,     42},
	{IT_STRING | IT_CVAR, NULL, "Emeralds",  NULL,             &cv_powerstones,      50},
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",  NULL,           &cv_matchboxes,       58},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn", NULL,          &cv_itemrespawn,      66},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn time", NULL,     &cv_itemrespawntime,  74},

	{IT_STRING | IT_CVAR, NULL, "Sudden Death",  NULL,         &cv_suddendeath,      90},
	{IT_STRING | IT_CVAR, NULL, "Player respawn delay", NULL,   &cv_respawntime,      98},

	{IT_STRING | IT_CVAR, NULL, "Force Skin #",  NULL,         &cv_forceskin,          114},
	{IT_STRING | IT_CVAR, NULL, "Restrict skin changes", NULL,  &cv_restrictskinchange, 122},

	{IT_STRING | IT_CVAR, NULL, "Autobalance Teams",  NULL,           &cv_autobalance,      138},
	{IT_STRING | IT_CVAR, NULL, "Scramble Teams on Map Change", NULL,  &cv_scrambleonchange, 146},
};

static menuitem_t OP_GametypeOptionsMenu[] =
{
	{IT_HEADER,           NULL, "CO-OP",      NULL,            NULL,                  2},
	{IT_STRING | IT_CVAR, NULL, "Players for exit",  NULL,     &cv_playersforexit,   10},
	{IT_STRING | IT_CVAR, NULL, "Starting Lives",  NULL,       &cv_startinglives,    18},

	{IT_HEADER,           NULL, "COMPETITION",   NULL,         NULL,                 34},
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",     NULL,        &cv_competitionboxes, 42},
	{IT_STRING | IT_CVAR, NULL, "Countdown Time", NULL,        &cv_countdowntime,    50},

	{IT_HEADER,           NULL, "RACE",   NULL,                NULL,                 66},
	{IT_STRING | IT_CVAR, NULL, "Number of Laps", NULL,        &cv_numlaps,          74},
	{IT_STRING | IT_CVAR, NULL, "Use Map Lap Counts",   NULL,  &cv_usemapnumlaps,    82},

	{IT_HEADER,           NULL, "MATCH", NULL,                 NULL,                 98},
	{IT_STRING | IT_CVAR, NULL, "Scoring Type", NULL,          &cv_match_scoring,   106},

	{IT_HEADER,           NULL, "TAG",       NULL,             NULL,                122},
	{IT_STRING | IT_CVAR, NULL, "Hide Time",  NULL,            &cv_hidetime,        130},

	{IT_HEADER,           NULL, "CTF",        NULL,            NULL,                146},
	{IT_STRING | IT_CVAR, NULL, "Flag Respawn Time", NULL,     &cv_flagtime,        154},
};

static menuitem_t OP_MonitorToggleMenu[] =
{
	// Printing handled by drawing function
	{IT_STRING|IT_CALL, NULL, "Reset all", NULL,  M_ResetCvars, 15},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Recycler", NULL,           &cv_recycler,      30},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Teleporters", NULL,       &cv_teleporters,   40},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Super Ring",  NULL,       &cv_superring,     50},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Super Sneakers", NULL,     &cv_supersneakers, 60},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Invincibility",  NULL,     &cv_invincibility, 70},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Whirlwind Shield", NULL,   &cv_jumpshield,    80},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Elemental Shield", NULL,   &cv_watershield,   90},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Attraction Shield", NULL,  &cv_ringshield,   100},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Force Shield", NULL,      &cv_forceshield,  110},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Armageddon Shield", NULL,  &cv_bombshield,   120},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "1 Up",  NULL,             &cv_1up,          130},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Eggman Box", NULL,         &cv_eggmanbox,    140},
};

static menuitem_t OP_LegacyOptionsMenu[] =
{
	{IT_HEADER|IT_STRING, NULL, "Screen Tilting", NULL,  NULL, 15},
	{IT_CVAR|IT_STRING, NULL, "Screen Tilting", NULL,  &cv_viewroll, 	  20},
	{IT_CVAR|IT_STRING, NULL, "Earthquake Screen Shaking", NULL,  &cv_quakeiiiarena,  25},
	{IT_CVAR|IT_STRING, NULL, "Quake Viewroll", NULL,    &cv_quakeiv, 30},
	{IT_HEADER|IT_STRING, NULL, "System", NULL,  NULL, 35},
	{IT_CVAR|IT_STRING, NULL, "Window Shaking", NULL,    &cv_quakelive, 40},

	{IT_SUBMENU|IT_STRING, NULL, 	"Credits",  NULL, 		&OP_LegacyCreditsDef, 50},
};

static menuitem_t OP_LegacyCreditsMenu[] = // This barely fits on green resolution
{
	{IT_HEADER|IT_STRING, NULL, "Contributors:", NULL,  NULL, 7},
	{IT_STRING, NULL, "PAS", NULL,  NULL, 22},
	{IT_STRING, NULL, "chromaticpipe", NULL,  NULL, 32},
	{IT_STRING, NULL, "Hanicef", NULL,  NULL, 42},
	{IT_STRING, NULL, "Lugent",  NULL, NULL, 52},
	{IT_STRING, NULL, "tempowad", NULL,  NULL, 62},
	{IT_STRING, NULL, "tatokis", NULL,  NULL, 72},
	{IT_STRING, NULL, "luigi budd", NULL,  NULL, 82}, // Enhanced server info screen
	{IT_STRING, NULL, "Lamibe", NULL,  NULL, 92},
	{IT_STRING, NULL, "Clusterguy!!", NULL,  NULL, 102}, // Software sky barreling
	{IT_STRING, NULL, "Bewer", NULL,  NULL, 112}, // SRB2Kart text colormaps
	{IT_STRING, NULL, "alufolie91", NULL,  NULL, 122},
	{IT_HEADER|IT_STRING, NULL, "Special Thanks:", NULL,  NULL, 132},
	{IT_STRING, NULL, "Upstream SRB2 Contributors", NULL, NULL, 142},
	{IT_STRING, NULL, "SRB2 Classic", NULL, NULL, 152},
	{IT_STRING, NULL, "SRB2Kart: Saturn", NULL, NULL, 162},
	{IT_STRING, NULL, "The Gaming Den", NULL, NULL, 172},  // srb2-legacy Co-op server
	{IT_STRING, NULL, "SRB2EventZ", NULL, NULL,  182}, // Netgame testing and feature ideas
};

static void M_LegacyCreditsToolTips(void)
{
	if (currentMenu == &OP_LegacyCreditsDef)
	{
		INT32 i;
		for (i = 0; i < currentMenu->numitems; i++)
		{
			OP_LegacyCreditsMenu[i].desc = "Thanks! :D";
		}
	}
}

// ==========================================================================
// ALL MENU DEFINITIONS GO HERE
// ==========================================================================

// Main Menu and related
menu_t MainDef = CENTERMENUSTYLE(NULL, MainMenu, NULL, 72);

menu_t MISC_AddonsDef =
{
	NULL,
	sizeof (MISC_AddonsMenu)/sizeof (menuitem_t),
	&MainDef,
	MISC_AddonsMenu,
	M_DrawAddons,
	50, 28,
	0,
	NULL
};

menu_t MAPauseDef = PAUSEMENUSTYLE(MAPauseMenu, 40, 72);
menu_t SPauseDef = PAUSEMENUSTYLE(SPauseMenu, 40, 72);
menu_t MPauseDef = PAUSEMENUSTYLE(MPauseMenu, 40, 72);

// Misc Main Menu
menu_t MISC_ScrambleTeamDef = DEFAULTMENUSTYLE(NULL, MISC_ScrambleTeamMenu, &MPauseDef, 27, 40);
menu_t MISC_ChangeTeamDef = DEFAULTMENUSTYLE(NULL, MISC_ChangeTeamMenu, &MPauseDef, 27, 40);
menu_t MISC_ChangeLevelDef = MAPICONMENUSTYLE(NULL, MISC_ChangeLevelMenu, &MPauseDef);
menu_t MISC_HelpDef = IMAGEDEF(MISC_HelpMenu);

static INT32 highlightflags, recommendedflags, warningflags;


// Sky Room
menu_t SR_PandoraDef =
{
	"M_PANDRA",
	sizeof (SR_PandorasBox)/sizeof (menuitem_t),
	&SPauseDef,
	SR_PandorasBox,
	M_DrawGenericMenu,
	60, 40,
	0,
	M_ExitPandorasBox
};
menu_t SR_MainDef =
{
	"M_SECRET",
	sizeof (SR_MainMenu)/sizeof (menuitem_t),
	&MainDef,
	SR_MainMenu,
	M_DrawSkyRoom,
	60, 40,
	0,
	NULL
};
menu_t SR_LevelSelectDef =
{
	0,
	sizeof (SR_LevelSelectMenu)/sizeof (menuitem_t),
	&SR_MainDef,
	SR_LevelSelectMenu,
	M_DrawLevelSelectMenu,
	40, 40,
	0,
	NULL
};
menu_t SR_UnlockChecklistDef =
{
	NULL,
	1,
	&SR_MainDef,
	SR_UnlockChecklistMenu,
	M_DrawChecklist,
	280, 185,
	0,
	NULL
};
menu_t SR_EmblemHintDef =
{
	NULL,
	sizeof (SR_EmblemHintMenu)/sizeof (menuitem_t),
	&SPauseDef,
	SR_EmblemHintMenu,
	M_DrawEmblemHints,
	60, 150,
	0,
	NULL
};

// Single Player
menu_t SP_MainDef = CENTERMENUSTYLE(NULL, SP_MainMenu, &MainDef, 72);
menu_t SP_LoadDef =
{
	"M_PICKG",
	1,
	&SP_MainDef,
	SP_LoadGameMenu,
	M_DrawLoad,
	68, 46,
	0,
	NULL
};
menu_t SP_LevelSelectDef = MAPICONMENUSTYLE(NULL, SP_LevelSelectMenu, &SP_LoadDef);
menu_t SP_LevelStatsDef =
{
	"M_STATS",
	1,
	&SP_MainDef,
	SP_LevelStatsMenu,
	M_DrawLevelStats,
	280, 185,
	0,
	NULL
};

static menu_t SP_TimeAttackDef =
{
	"M_ATTACK",
	sizeof (SP_TimeAttackMenu)/sizeof (menuitem_t),
	&MainDef,  // Doesn't matter.
	SP_TimeAttackMenu,
	M_DrawTimeAttackMenu,
	32, 40,
	0,
	NULL
};
static menu_t SP_ReplayDef =
{
	"M_ATTACK",
	sizeof(SP_ReplayMenu)/sizeof(menuitem_t),
	&SP_TimeAttackDef,
	SP_ReplayMenu,
	M_DrawTimeAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_GuestReplayDef =
{
	"M_ATTACK",
	sizeof(SP_GuestReplayMenu)/sizeof(menuitem_t),
	&SP_TimeAttackDef,
	SP_GuestReplayMenu,
	M_DrawTimeAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_GhostDef =
{
	"M_ATTACK",
	sizeof(SP_GhostMenu)/sizeof(menuitem_t),
	&SP_TimeAttackDef,
	SP_GhostMenu,
	M_DrawTimeAttackMenu,
	32, 120,
	0,
	NULL
};

static menu_t SP_NightsAttackDef =
{
	"M_NIGHTS",
	sizeof (SP_NightsAttackMenu)/sizeof (menuitem_t),
	&MainDef,  // Doesn't matter.
	SP_NightsAttackMenu,
	M_DrawNightsAttackMenu,
	32, 40,
	0,
	NULL
};
static menu_t SP_NightsReplayDef =
{
	"M_NIGHTS",
	sizeof(SP_NightsReplayMenu)/sizeof(menuitem_t),
	&SP_NightsAttackDef,
	SP_NightsReplayMenu,
	M_DrawNightsAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_NightsGuestReplayDef =
{
	"M_NIGHTS",
	sizeof(SP_NightsGuestReplayMenu)/sizeof(menuitem_t),
	&SP_NightsAttackDef,
	SP_NightsGuestReplayMenu,
	M_DrawNightsAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_NightsGhostDef =
{
	"M_NIGHTS",
	sizeof(SP_NightsGhostMenu)/sizeof(menuitem_t),
	&SP_NightsAttackDef,
	SP_NightsGhostMenu,
	M_DrawNightsAttackMenu,
	32, 120,
	0,
	NULL
};


menu_t SP_PlayerDef =
{
	"M_PICKP",
	sizeof (PlayerMenu)/sizeof (menuitem_t),//player_end,
	&SP_MainDef,
	PlayerMenu,
	M_DrawSetupChoosePlayerMenu,
	24, 32,
	0,
	NULL
};

// Multiplayer
menu_t MP_MainDef =
{
	"M_MULTI",
	sizeof (MP_MainMenu)/sizeof (menuitem_t),
	&MainDef,
	MP_MainMenu,
	M_DrawMPMainMenu,
	60, 45,
	0,
	M_CancelConnect
};
menu_t MP_ServerDef = MAPICONMENUSTYLE("M_MULTI", MP_ServerMenu, &MP_MainDef);
#ifndef NONET
menu_t MP_ConnectDef =
{
	"M_MULTI",
	sizeof (MP_ConnectMenu)/sizeof (menuitem_t),
	&MP_MainDef,
	MP_ConnectMenu,
	M_DrawConnectMenu,
	27,24,
	0,
	M_CancelConnect
};
menu_t MP_RoomDef =
{
	"M_MULTI",
	sizeof (MP_RoomMenu)/sizeof (menuitem_t),
	&MP_ConnectDef,
	MP_RoomMenu,
	M_DrawRoomMenu,
	27, 32,
	0,
	NULL
};
#endif
menu_t MP_SplitServerDef = MAPICONMENUSTYLE("M_MULTI", MP_SplitServerMenu, &MP_MainDef);
menu_t MP_PlayerSetupDef =
{
	"M_SPLAYR",
	sizeof (MP_PlayerSetupMenu)/sizeof (menuitem_t),
	&MP_MainDef,
	MP_PlayerSetupMenu,
	M_DrawSetupMultiPlayerMenu,
	27, 40,
	0,
	M_QuitMultiPlayerMenu
};

// Options
menu_t OP_MainDef = DEFAULTMENUSTYLE("M_OPTTTL", OP_MainMenu, &MainDef, 60, 30);
menu_t OP_ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_ControlsMenu, &OP_MainDef, 60, 30);
menu_t OP_AllControlsDef = CONTROLMENUSTYLE(OP_AllControlsMenu, &OP_P1ControlsDef);
menu_t OP_AllControls2Def = CONTROLMENUSTYLE(OP_AllControls2Menu, &OP_P2ControlsDef);
menu_t OP_P1ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_P1ControlsMenu, &OP_ControlsDef, 60, 30);
menu_t OP_P2ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_P2ControlsMenu, &OP_ControlsDef, 60, 30);
menu_t OP_MouseOptionsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_MouseOptionsMenu, &OP_P1ControlsDef, 60, 30);
menu_t OP_Mouse2OptionsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_Mouse2OptionsMenu, &OP_P2ControlsDef, 60, 30);
menu_t OP_Joystick1Def = DEFAULTMENUSTYLE("M_CONTRO", OP_Joystick1Menu, &OP_P1ControlsDef, 60, 30);
menu_t OP_Joystick2Def = DEFAULTMENUSTYLE("M_CONTRO", OP_Joystick2Menu, &OP_P2ControlsDef, 60, 30);
menu_t OP_JoystickSetDef =
{
	"M_CONTRO",
	sizeof (OP_JoystickSetMenu)/sizeof (menuitem_t),
	&OP_Joystick1Def,
	OP_JoystickSetMenu,
	M_DrawJoystick,
	50, 40,
	0,
	NULL
};

menu_t OP_VideoOptionsDef = DEFAULTSCROLLMENUSTYLE("M_VIDEO", OP_VideoOptionsMenu, &OP_MainDef, 30, 30);
menu_t OP_VideoModeDef =
{
	"M_VIDEO",
	1,
	&OP_VideoOptionsDef,
	OP_VideoModeMenu,
	M_DrawVideoMode,
	48, 26,
	0,
	NULL
};
menu_t OP_ColorOptionsDef =
{
	"M_VIDEO",
	sizeof (OP_ColorOptionsMenu)/sizeof (menuitem_t),
	&OP_VideoOptionsDef,
	OP_ColorOptionsMenu,
	M_DrawColorMenu,
	30, 30,
	0,
	NULL
};
menu_t OP_SoundOptionsDef = DEFAULTMENUSTYLE("M_SOUND", OP_SoundOptionsMenu, &OP_MainDef, 60, 30);
menu_t OP_SoundAdvancedDef = DEFAULTMENUSTYLE("M_SOUND", OP_SoundAdvancedMenu, &OP_SoundOptionsDef, 30, 30);
menu_t OP_GameOptionsDef = DEFAULTSCROLLMENUSTYLE("M_GAME", OP_GameOptionsMenu, &OP_MainDef, 30, 30);
menu_t OP_ServerOptionsDef = DEFAULTMENUSTYLE("M_SERVER", OP_ServerOptionsMenu, &OP_MainDef, 30, 30);

menu_t OP_NetgameOptionsDef = DEFAULTMENUSTYLE("M_SERVER", OP_NetgameOptionsMenu, &OP_ServerOptionsDef, 30, 30);
menu_t OP_GametypeOptionsDef = DEFAULTMENUSTYLE("M_SERVER", OP_GametypeOptionsMenu, &OP_ServerOptionsDef, 30, 30);
menu_t OP_ChatOptionsDef = DEFAULTMENUSTYLE("M_GAME", OP_ChatOptionsMenu, &OP_GameOptionsDef, 30, 30);
menu_t OP_MonitorToggleDef =
{
	"M_SERVER",
	sizeof (OP_MonitorToggleMenu)/sizeof (menuitem_t),
	&OP_ServerOptionsDef,
	OP_MonitorToggleMenu,
	M_DrawMonitorToggles,
	30, 30,
	0,
	NULL
};

#ifdef HWRENDER
static void M_OpenGLOptionsMenu(void)
{
	if (rendermode == render_opengl)
		M_SetupNextMenu(&OP_OpenGLOptionsDef);
	else
		M_StartMessage(M_GetText("You must be in OpenGL mode\nto access this menu.\n\n(Press a key)\n"), NULL, MM_NOTHING);
}

menu_t OP_OpenGLOptionsDef = DEFAULTMENUSTYLE("M_VIDEO", OP_OpenGLOptionsMenu, &OP_VideoOptionsDef, 30, 30);
#endif
menu_t OP_DataOptionsDef = DEFAULTMENUSTYLE("M_DATA", OP_DataOptionsMenu, &OP_MainDef, 60, 30);
menu_t OP_ScreenshotOptionsDef = DEFAULTSCROLLMENUSTYLE("M_DATA", OP_ScreenshotOptionsMenu, &OP_DataOptionsDef, 30, 30);
menu_t OP_AddonsOptionsDef = DEFAULTMENUSTYLE("M_ADDONS", OP_AddonsOptionsMenu, &OP_MainDef, 30, 30);
menu_t OP_EraseDataDef = DEFAULTMENUSTYLE("M_DATA", OP_EraseDataMenu, &OP_DataOptionsDef, 60, 30);
menu_t OP_LegacyOptionsDef = DEFAULTSCROLLMENUSTYLE(NULL, OP_LegacyOptionsMenu, &OP_MainDef, 30, 30);
menu_t OP_LegacyCreditsDef = DEFAULTMENUSTYLE(NULL, OP_LegacyCreditsMenu, &OP_LegacyOptionsDef, 30, 0);

// ==========================================================================
// CVAR ONCHANGE EVENTS GO HERE
// ==========================================================================
// (there's only a couple anyway)

// Prototypes
static INT32 M_FindFirstMap(INT32 gtype);
static INT32 M_GetFirstLevelInList(void);

// Nextmap.  Used for Time Attack.
static void Nextmap_OnChange(void)
{
	char *leveltitle;
	char *tabase = Z_Malloc(512, PU_STATIC, NULL);
	short i;
	boolean active;

	// Update the string in the consvar.
	Z_Free(cv_nextmap.zstring);
	leveltitle = G_BuildMapTitle(cv_nextmap.value);
	cv_nextmap.string = cv_nextmap.zstring = leveltitle ? leveltitle : Z_StrDup(G_BuildMapName(cv_nextmap.value));

	if (currentMenu == &SP_NightsAttackDef)
	{
		CV_StealthSetValue(&cv_dummymares, 0);
		// Hide the record changing CVAR if only one mare is available.
		if (!nightsrecords[cv_nextmap.value-1] || nightsrecords[cv_nextmap.value-1]->nummares < 2)
			SP_NightsAttackMenu[narecords].status = IT_DISABLED;
		else
			SP_NightsAttackMenu[narecords].status = IT_STRING|IT_CVAR;

		// Do the replay things.
		active = false;
		SP_NightsAttackMenu[naguest].status = IT_DISABLED;
		SP_NightsAttackMenu[nareplay].status = IT_DISABLED;
		SP_NightsAttackMenu[naghost].status = IT_DISABLED;

		// Check if file exists, if not, disable REPLAY option
		snprintf(tabase, 512, "%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s",srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));
		for (i = 0; i < 4; i++) {
			SP_NightsReplayMenu[i].status = IT_DISABLED;
			SP_NightsGuestReplayMenu[i].status = IT_DISABLED;
		}
		if (FIL_FileExists(va("%s-score-best.lmp", tabase))) {
			SP_NightsReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-time-best.lmp", tabase))) {
			SP_NightsReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-last.lmp", tabase))) {
			SP_NightsReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-guest.lmp", tabase))) {
			SP_NightsReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (active) {
			SP_NightsAttackMenu[naguest].status = IT_WHITESTRING|IT_SUBMENU;
			SP_NightsAttackMenu[nareplay].status = IT_WHITESTRING|IT_SUBMENU;
			SP_NightsAttackMenu[naghost].status = IT_WHITESTRING|IT_SUBMENU;
		}
		else if(itemOn == nareplay) // Reset lastOn so replay isn't still selected when not available.
		{
			currentMenu->lastOn = itemOn;
			itemOn = nastart;
		}
	}
	else if (currentMenu == &SP_TimeAttackDef)
	{
		active = false;
		SP_TimeAttackMenu[taguest].status = IT_DISABLED;
		SP_TimeAttackMenu[tareplay].status = IT_DISABLED;
		SP_TimeAttackMenu[taghost].status = IT_DISABLED;

		// Check if file exists, if not, disable REPLAY option
		snprintf(tabase, 512, "%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s",srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.string);
		for (i = 0; i < 5; i++) {
			SP_ReplayMenu[i].status = IT_DISABLED;
			SP_GuestReplayMenu[i].status = IT_DISABLED;
		}
		if (FIL_FileExists(va("%s-time-best.lmp", tabase))) {
			SP_ReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-score-best.lmp", tabase))) {
			SP_ReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-rings-best.lmp", tabase))) {
			SP_ReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-last.lmp", tabase))) {
			SP_ReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)))) {
			SP_ReplayMenu[4].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[4].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (active) {
			SP_TimeAttackMenu[taguest].status = IT_WHITESTRING|IT_SUBMENU;
			SP_TimeAttackMenu[tareplay].status = IT_WHITESTRING|IT_SUBMENU;
			SP_TimeAttackMenu[taghost].status = IT_WHITESTRING|IT_SUBMENU;
		}
		else if(itemOn == tareplay) // Reset lastOn so replay isn't still selected when not available.
		{
			currentMenu->lastOn = itemOn;
			itemOn = tastart;
		}

		if (mapheaderinfo[cv_nextmap.value-1] && mapheaderinfo[cv_nextmap.value-1]->forcecharacter[0] != '\0')
			CV_Set(&cv_chooseskin, mapheaderinfo[cv_nextmap.value-1]->forcecharacter);
	}
	Z_Free(tabase);
}

static void Dummymares_OnChange(void)
{
	if (!nightsrecords[cv_nextmap.value-1])
	{
		CV_StealthSetValue(&cv_dummymares, 0);
		return;
	}
	else
	{
		UINT8 mares = nightsrecords[cv_nextmap.value-1]->nummares;

		if (cv_dummymares.value < 0)
			CV_StealthSetValue(&cv_dummymares, mares);
		else if (cv_dummymares.value > mares)
			CV_StealthSetValue(&cv_dummymares, 0);
	}
}

// Newgametype.  Used for gametype changes.
static void Newgametype_OnChange(void)
{
	if (menuactive)
	{
		if(!mapheaderinfo[cv_nextmap.value-1])
			P_AllocMapHeader((INT16)(cv_nextmap.value-1));

		if ((cv_newgametype.value == GT_COOP && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_COOP)) ||
			(cv_newgametype.value == GT_COMPETITION && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_COMPETITION)) ||
			(cv_newgametype.value == GT_RACE && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_RACE)) ||
			((cv_newgametype.value == GT_MATCH || cv_newgametype.value == GT_TEAMMATCH) && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_MATCH)) ||
			((cv_newgametype.value == GT_TAG || cv_newgametype.value == GT_HIDEANDSEEK) && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_TAG)) ||
			(cv_newgametype.value == GT_CTF && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_CTF)))
		{
			INT32 value = 0;

			switch (cv_newgametype.value)
			{
				case GT_COOP:
					value = TOL_COOP;
					break;
				case GT_COMPETITION:
					value = TOL_COMPETITION;
					break;
				case GT_RACE:
					value = TOL_RACE;
					break;
				case GT_MATCH:
				case GT_TEAMMATCH:
					value = TOL_MATCH;
					break;
				case GT_TAG:
				case GT_HIDEANDSEEK:
					value = TOL_TAG;
					break;
				case GT_CTF:
					value = TOL_CTF;
					break;
			}

			CV_SetValue(&cv_nextmap, M_FindFirstMap(value));
			CV_AddValue(&cv_nextmap, -1);
			CV_AddValue(&cv_nextmap, 1);
		}
	}
}

void Screenshot_option_Onchange(void)
{
	OP_ScreenshotOptionsMenu[op_screenshot_folder].status =
		(cv_screenshot_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

void Moviemode_mode_Onchange(void)
{
	INT32 i, cstart, cend;
	for (i = op_screenshot_gif_start; i <= op_screenshot_apng_end; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_DISABLED;

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			cstart = op_screenshot_gif_start;
			cend = op_screenshot_gif_end;
			break;
		case MM_APNG:
			cstart = op_screenshot_apng_start;
			cend = op_screenshot_apng_end;
			break;
		default:
			return;
	}
	for (i = cstart; i <= cend; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_STRING|IT_CVAR;
}

void Addons_option_Onchange(void)
{
	OP_AddonsOptionsMenu[op_addons_folder].status =
		(cv_addons_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

void Moviemode_option_Onchange(void)
{
	OP_ScreenshotOptionsMenu[op_movie_folder].status =
		(cv_movie_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

// ==========================================================================
// END ORGANIZATION STUFF.
// ==========================================================================

// current menudef
menu_t *currentMenu = &MainDef;

// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

static void M_UpdateItemOn(void)
{
	I_SetTextInputMode((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING ||
		(currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER);
}

static void M_ChangeCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
	char s[20];

	// yea
	choice = (choice*2-1);

	if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
	    ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_INVISSLIDER)
	    ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD))
	{
		if ((cv->flags & CV_FLOAT) && (currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
		{
			float adjust = choice * ((cv == &cv_fov) ? 0.5f : (1.0f/16.0f)); // :)
			sprintf(s,"%f",FIXED_TO_FLOAT(cv->value)+adjust);
			CV_Set(cv,s);
		}
		else
			CV_SetValue(cv,cv->value+choice);
	}
	else if (cv->flags & CV_FLOAT)
	{
		sprintf(s,"%f",FIXED_TO_FLOAT(cv->value)+choice*(1.0f/16.0f));
		CV_Set(cv,s);
	}
	else
		CV_AddValue(cv,choice);
}


static boolean M_ChangeStringCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
	char buf[255];
	size_t len;

	switch (choice)
	{
		case KEY_BACKSPACE:
			len = strlen(cv->string);
			if (len > 0)
			{
				M_Memcpy(buf, cv->string, len);
				buf[len-1] = 0;
				CV_Set(cv, buf);
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(cv->string);
				if (len < MAXSTRINGLENGTH - 1)
				{
					M_Memcpy(buf, cv->string, len);
					buf[len++] = (char)choice;
					buf[len] = 0;
					CV_Set(cv, buf);
				}
				return true;
			}
			break;
	}
	return false;
}

// resets all cvars on a menu - assumes that all that have itemactions are cvars
static void M_ResetCvars(void)
{
	INT32 i;
	consvar_t *cv;
	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (!(currentMenu->menuitems[i].status & IT_CVAR) || !(cv = (consvar_t *)currentMenu->menuitems[i].itemaction))
			continue;
		CV_SetValue(cv, atoi(cv->defaultvalue));
	}
}

static void M_NextOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	do
	{
		if (itemOn + 1 > currentMenu->numitems - 1)
			itemOn = 0;
		else
			itemOn++;
	} while (oldItemOn != itemOn && ( (currentMenu->menuitems[itemOn].status & IT_TYPE) & IT_SPACE ));
	M_UpdateItemOn();
}

static void M_PrevOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	do
	{
		if (!itemOn)
			itemOn = currentMenu->numitems - 1;
		else
			itemOn--;
	} while (oldItemOn != itemOn && ( (currentMenu->menuitems[itemOn].status & IT_TYPE) & IT_SPACE ));
	M_UpdateItemOn();
}

// lock out further input in a tic when important buttons are pressed
// (in other words -- stop bullshit happening by mashing buttons in fades)
static boolean noFurtherInput = false;

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	INT32 ch = -1;
//	INT32 i;
	static tic_t joywait = 0, mousewait = 0;
	static INT32 pjoyx = 0, pjoyy = 0;
	static INT32 pmousex = 0, pmousey = 0;
	static INT32 lastx = 0, lasty = 0;
	void (*routine)(INT32 choice); // for some casting problem

	if (dedicated || (demoplayback && titledemo)
	|| gamestate == GS_INTRO || gamestate == GS_CUTSCENE || gamestate == GS_GAMEEND
	|| gamestate == GS_CREDITS || gamestate == GS_EVALUATION)
		return false;

	if (CON_Ready())
		return false;

	if (noFurtherInput)
	{
		// Ignore input after enter/escape/other buttons
		// (but still allow shift keyup so caps doesn't get stuck)
		return false;
	}
	else if (menuactive)
	{
		if (ev->type == ev_keydown || ev->type == ev_text)
		{
			ch = ev->data1;

			if (ev->type == ev_keydown)
			{
				// added 5-2-98 remap virtual keys (mouse & joystick buttons)
				switch (ch)
				{
					case KEY_MOUSE1:
					case KEY_JOY1:
						ch = KEY_ENTER;
						break;
					case KEY_JOY1 + 3:
						ch = 'n';
						break;
					case KEY_MOUSE1 + 1:
					case KEY_JOY1 + 1:
						ch = KEY_ESCAPE;
						break;
					case KEY_JOY1 + 2:
						ch = KEY_BACKSPACE;
						break;
					case KEY_HAT1:
						ch = KEY_UPARROW;
						break;
					case KEY_HAT1 + 1:
						ch = KEY_DOWNARROW;
						break;
					case KEY_HAT1 + 2:
						ch = KEY_LEFTARROW;
						break;
					case KEY_HAT1 + 3:
						ch = KEY_RIGHTARROW;
						break;
				}
			}
		}
		else if (ev->type == ev_joystick  && ev->data1 == 0 && joywait < I_GetTime())
		{
			const INT32 jdeadzone = JOYAXISRANGE/4;
			if (ev->data3 != INT32_MAX)
			{
				if (Joystick.bGamepadStyle || abs(ev->data3) > jdeadzone)
				{
					if (ev->data3 < 0 && pjoyy >= 0)
					{
						ch = KEY_UPARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					else if (ev->data3 > 0 && pjoyy <= 0)
					{
						ch = KEY_DOWNARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					pjoyy = ev->data3;
				}
				else
					pjoyy = 0;
			}

			if (ev->data2 != INT32_MAX)
			{
				if (Joystick.bGamepadStyle || abs(ev->data2) > jdeadzone)
				{
					if (ev->data2 < 0 && pjoyx >= 0)
					{
						ch = KEY_LEFTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					else if (ev->data2 > 0 && pjoyx <= 0)
					{
						ch = KEY_RIGHTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					pjoyx = ev->data2;
				}
				else
					pjoyx = 0;
			}
		}
		else if (ev->type == ev_mouse && mousewait < I_GetTime())
		{
			pmousey += ev->data3;
			if (pmousey < lasty-30)
			{
				ch = KEY_DOWNARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty -= 30;
			}
			else if (pmousey > lasty + 30)
			{
				ch = KEY_UPARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty += 30;
			}

			pmousex += ev->data2;
			if (pmousex < lastx - 30)
			{
				ch = KEY_LEFTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx -= 30;
			}
			else if (pmousex > lastx+30)
			{
				ch = KEY_RIGHTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx += 30;
			}
		}
	}
	else if (ev->type == ev_keydown) // Preserve event for other responders
		ch = ev->data1;

	if (ch == -1)
		return false;
	else if (ch == gamecontrol[gc_systemmenu][0] || ch == gamecontrol[gc_systemmenu][1]) // allow remappable ESC key
		ch = KEY_ESCAPE;

	// F-Keys
	if (!menuactive)
	{
		noFurtherInput = true;
		switch (ch)
		{
			case KEY_F1: // Help key
				if (modeattacking)
					return true;
				M_StartControlPanel();
				currentMenu = &MISC_HelpDef;
				itemOn = 0;
				M_UpdateItemOn();
				return true;

			case KEY_F2: // Empty
				return true;

			case KEY_F3: // Toggle HUD
				CV_SetValue(&cv_showhud, !cv_showhud.value);
				return true;

			case KEY_F4: // Sound Volume
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				currentMenu = &OP_SoundOptionsDef;
				itemOn = 0;
				M_UpdateItemOn();
				return true;

#ifndef DC
			case KEY_F5: // Video Mode
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_VideoModeMenu(0);
				return true;
#endif

			case KEY_F6: // Empty
				return true;

			case KEY_F7: // Options
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_SetupNextMenu(&OP_MainDef);
				return true;

			// Screenshots on F8 now handled elsewhere
			// Same with Moviemode on F9

			case KEY_F10: // Renderer toggle, also processed inside menus
				CV_AddValue(&cv_renderer, 1);
				return true;

			case KEY_F11: // Fullscreen toggle, also processed inside
				CV_SetValue(&cv_fullscreen, !cv_fullscreen.value);
				return true;

			// Spymode on F12 handled in game logic

			case KEY_ESCAPE: // Pop up menu
				if (chat_on)
					HU_clearChatChars();
				else
					M_StartControlPanel();
				return true;
		}
		noFurtherInput = false; // turns out we didn't care
		return false;
	}

	routine = currentMenu->menuitems[itemOn].itemaction;

	// Handle menuitems which need a specific key handling
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER)
	{
		// block text input if ctrl is held, to allow using ctrl+c ctrl+v and ctrl+x
		if (ctrldown)
		{
			routine(ch);
			return true;
		}
		// ignore ev_keydown events if the key maps to a character, since
		// the ev_text event will follow immediately after in that case.
		if (ev->type == ev_keydown && ch >= 32 && ch <= 127)
			return true;
		routine(ch);
		return true;
	}

	if (currentMenu->menuitems[itemOn].status == IT_MSGHANDLER)
	{
		if (currentMenu->menuitems[itemOn].alphaKey != MM_EVENTHANDLER)
		{
			if (ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE || ch == KEY_ENTER)
			{
				if (routine)
					routine(ch);
				M_StopMessage(0);
				noFurtherInput = true;
				return true;
			}
			return true;
		}
		else
		{
			// dirty hack: for customising controls, I want only buttons/keys, not moves
			if (ev->type == ev_mouse || ev->type == ev_mouse2 || ev->type == ev_joystick
				|| ev->type == ev_joystick2)
				return true;
			if (routine)
			{
				void (*otherroutine)(event_t *sev) = currentMenu->menuitems[itemOn].itemaction;
				otherroutine(ev); //Alam: what a hack
			}
			return true;
		}
	}

	// BP: one of the more big hack i have never made
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	{
		if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING)
		{
			// ignore ev_keydown events if the key maps to a character, since
			// the ev_text event will follow immediately after in that case.
			if (ev->type == ev_keydown && ch >= 32 && ch <= 127)
				return true;
			if (M_ChangeStringCvar(ch))
				return true;
			else
				routine = NULL;
		}
		else
			routine = M_ChangeCvar;
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL, sfx_menu1);
			if (currentMenu == &SP_PlayerDef)
			{
				Z_Free(char_notes);
				char_notes = NULL;
			}
			coolalphatimer = 9;
			return true;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_menu1);
			if (currentMenu == &SP_PlayerDef)
			{
				Z_Free(char_notes);
				char_notes = NULL;
			}
			coolalphatimer = 9;
			return true;

		case KEY_LEFTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				if (currentMenu != &OP_SoundOptionsDef)
					S_StartSound(NULL, sfx_menu1);
				routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				if (currentMenu != &OP_SoundOptionsDef)
					S_StartSound(NULL, sfx_menu1);
				routine(1);
			}
			return true;

		case KEY_ENTER:
			noFurtherInput = true;
			currentMenu->lastOn = itemOn;
			if (routine)
			{
				if (((currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_CALL
				 || (currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_SUBMENU)
                 && (currentMenu->menuitems[itemOn].status & IT_CALLTYPE))
				{
					if (((currentMenu->menuitems[itemOn].status & IT_CALLTYPE) & IT_CALL_NOTMODIFIED) && modifiedgame && !savemoddata)
					{
						S_StartSound(NULL, sfx_menu1);
						M_StartMessage(M_GetText("This cannot be done in a modified game.\n\n(Press a key)\n"), NULL, MM_NOTHING);
						return true;
					}
				}
				S_StartSound(NULL, sfx_menu1);
				switch (currentMenu->menuitems[itemOn].status & IT_TYPE)
				{
					case IT_CVAR:
					case IT_ARROWS:
						routine(1); // right arrow
						break;
					case IT_CALL:
						routine(itemOn);
						break;
					case IT_SUBMENU:
						currentMenu->lastOn = itemOn;
						M_SetupNextMenu((menu_t *)currentMenu->menuitems[itemOn].itemaction);
						break;
				}
			}
			return true;

		case KEY_ESCAPE:
			noFurtherInput = true;
			currentMenu->lastOn = itemOn;
			if (currentMenu->prevMenu)
			{
				//If we entered the game search menu, but didn't enter a game,
				//make sure the game doesn't still think we're in a netgame.
				if (!Playing() && netgame && multiplayer)
				{
					netgame = false;
					multiplayer = false;
				}

				if (currentMenu == &SP_TimeAttackDef || currentMenu == &SP_NightsAttackDef)
				{
					// D_StartTitle does its own wipe, since GS_TIMEATTACK is now a complete gamestate.
					menuactive = false;
					I_UpdateMouseGrab();
					D_StartTitle();
				}
				else if (currentMenu == &SP_PlayerDef)
				{
					if (!Playing())
					{
						S_StopMusic();
						S_ChangeMusicInternal("titles", looptitle);
					}
					M_SetupNextMenu(currentMenu->prevMenu);
				}
				else
					M_SetupNextMenu(currentMenu->prevMenu);
			}
			else
				M_ClearMenus(true);
			return true;

		case KEY_BACKSPACE:
			if ((currentMenu->menuitems[itemOn].status) == IT_CONTROL)
			{
				// detach any keys associated with the game control
				G_ClearControlKeys(setupcontrols, currentMenu->menuitems[itemOn].alphaKey);
				return true;
			}
			else if ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
			{
 				consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
				if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
					CV_Set(cv, cv->defaultvalue);
			}
			// Why _does_ backspace go back anyway?
			//currentMenu->lastOn = itemOn;
			//if (currentMenu->prevMenu)
			//	M_SetupNextMenu(currentMenu->prevMenu);
			return false;

		case KEY_F10: // Renderer toggle, also processed outside menus
			CV_AddValue(&cv_renderer, 1);
			return true;

		case KEY_F11: // Fullscreen toggle, also processed outside menus
			CV_SetValue(&cv_fullscreen, !cv_fullscreen.value);
			return true;

		default:
			CON_Responder(ev);
			break;
	}

	return true;
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
	if (currentMenu == &MessageDef)
		menuactive = true;

	if (menuactive)
	{
		// now that's more readable with a faded background (yeah like Quake...)
		if (!WipeInAction)
			V_DrawFadeScreen(0xFF00, 16);

		if (currentMenu->drawroutine)
			currentMenu->drawroutine(); // call current menu Draw routine

		// Draw version down in corner
		// ... but only in the MAIN MENU.  I'm a picky bastard.
		if (currentMenu == &MainDef)
		{
			if (customversionstring[0] != '\0')
			{
				V_DrawThinString(vid.dupx, vid.height - 17*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT, "Mod version:");
				V_DrawThinString(vid.dupx, vid.height - 9*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, customversionstring);
			}
			else
			{
				char *menucompnote = Z_StrDup(compnote);
				if(strlen(compnote) > 15) // Don't want it to potentially cross over into the menu, somewhat of a magic number but ehh it works on most resolutions
				{
				   strncpy(menucompnote, compnote, 15);
				   menucompnote[15] = '\0';
				   strcat(menucompnote, "...");
				}
#ifdef DEVELOP // Development -- show revision / branch info
				V_DrawThinString(vid.dupx, vid.height - 17*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, compbranch);
				V_DrawThinString(vid.dupx, vid.height - 9*vid.dupy,  V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, comprevision);
#else // Regular build
#ifdef BETAVERSION
				V_DrawThinString(vid.dupx, vid.height - 20*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, va("%s %s", comprevision, menucompnote));
#endif
				V_DrawThinString(vid.dupx, vid.height - 9*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, va("%s", VERSIONSTRING));
#endif
				Z_Free(menucompnote);
			}
		}
	}

	// focus lost notification goes on top of everything, even the former everything
	if (window_notinfocus && cv_showfocuslost.value)
	{
		M_DrawTextBox((BASEVIDWIDTH/2) - (60), (BASEVIDHEIGHT/2) - (16), 13, 2);
		if (gamestate == GS_LEVEL && (P_AutoPause() || paused))
			V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2) - (4), V_YELLOWMAP, "Game Paused");
		else
			V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2) - (4), V_YELLOWMAP, "Focus Lost");
	}
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	// time attack HACK
	if (modeattacking && demoplayback)
	{
		G_CheckDemoStatus();
		return;
	}

	// intro might call this repeatedly
	if (menuactive)
	{
		CON_ToggleOff(); // move away console
		return;
	}

	menuactive = true;

	if (!Playing())
	{
		// Secret menu!
		MainMenu[singleplr].alphaKey = (M_AnySecretUnlocked()) ? 76 : 84;
		MainMenu[multiplr].alphaKey = (M_AnySecretUnlocked()) ? 84 : 92;
		MainMenu[secrets].status = (M_AnySecretUnlocked()) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		currentMenu = &MainDef;
		itemOn = singleplr;
		M_UpdateItemOn();
	}
	else if (modeattacking)
	{
		currentMenu = &MAPauseDef;
		itemOn = mapause_continue;
		M_UpdateItemOn();
	}
	else if (!(netgame || multiplayer)) // Single Player
	{
		if (gamestate != GS_LEVEL || ultimatemode) // intermission, so gray out stuff.
		{
			SPauseMenu[spause_pandora].status = (M_SecretUnlocked(SECRET_PANDORA)) ? (IT_GRAYEDOUT) : (IT_DISABLED);
			SPauseMenu[spause_retry].status = IT_GRAYEDOUT;
		}
		else
		{
			INT32 numlives = 2;

			SPauseMenu[spause_pandora].status = (M_SecretUnlocked(SECRET_PANDORA)) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

			numlives = players[consoleplayer].lives;
			if (players[consoleplayer].playerstate != PST_LIVE)
				++numlives;

			// The list of things that can disable retrying is (was?) a little too complex
			// for me to want to use the short if statement syntax
			if (numlives <= 1 || G_IsSpecialStage(gamemap))
				SPauseMenu[spause_retry].status = (IT_GRAYEDOUT);
			else
				SPauseMenu[spause_retry].status = (IT_STRING | IT_CALL);
		}

		// We can always use level select though. :33
		SPauseMenu[spause_levelselect].status = (gamecomplete) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		// And emblem hints.
		SPauseMenu[spause_hints].status = (M_SecretUnlocked(SECRET_EMBLEMHINTS)) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		// Shift up Pandora's Box if both pandora and levelselect are active
		/*if (SPauseMenu[spause_pandora].status != (IT_DISABLED)
		 && SPauseMenu[spause_levelselect].status != (IT_DISABLED))
			SPauseMenu[spause_pandora].alphaKey = 24;
		else
			SPauseMenu[spause_pandora].alphaKey = 32;*/

		currentMenu = &SPauseDef;
		itemOn = spause_continue;
		M_UpdateItemOn();
	}
	else // multiplayer
	{
		MPauseMenu[mpause_switchmap].status = IT_DISABLED;
		MPauseMenu[mpause_addons].status = IT_DISABLED;
		MPauseMenu[mpause_scramble].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit2].status = IT_DISABLED;
		MPauseMenu[mpause_spectate].status = IT_DISABLED;
		MPauseMenu[mpause_entergame].status = IT_DISABLED;
		MPauseMenu[mpause_switchteam].status = IT_DISABLED;
		MPauseMenu[mpause_psetup].status = IT_DISABLED;

		if ((server || IsPlayerAdmin(consoleplayer)))
		{
			MPauseMenu[mpause_switchmap].status = IT_STRING | IT_CALL;
			MPauseMenu[mpause_addons].status = IT_STRING | IT_CALL;
			if (G_GametypeHasTeams())
				MPauseMenu[mpause_scramble].status = IT_STRING | IT_SUBMENU;
		}

		if (splitscreen)
			MPauseMenu[mpause_psetupsplit].status = MPauseMenu[mpause_psetupsplit2].status = IT_STRING | IT_CALL;
		else
		{
			MPauseMenu[mpause_psetup].status = IT_STRING | IT_CALL;

			if (G_GametypeHasTeams())
				MPauseMenu[mpause_switchteam].status = IT_STRING | IT_SUBMENU;
			else if (G_GametypeHasSpectators())
				MPauseMenu[(players[consoleplayer].spectator ? mpause_entergame : mpause_spectate)].status = IT_STRING | IT_CALL;
			else // in this odd case, we still want something to be on the menu even if it's useless
				MPauseMenu[mpause_spectate].status = IT_GRAYEDOUT;
		}

		currentMenu = &MPauseDef;
		itemOn = mpause_continue;
		M_UpdateItemOn();
	}

	//CON_ToggleOff(); // move away console
}

void M_EndModeAttackRun(void)
{
	M_ModeAttackEndGame(0);
}

//
// M_ClearMenus
//
void M_ClearMenus(boolean callexitmenufunc)
{
	if (!menuactive)
		return;

	if (currentMenu->quitroutine && callexitmenufunc && !currentMenu->quitroutine())
		return; // we can't quit this menu (also used to set parameter from the menu)

#ifndef DC // Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));
#endif //Alam: But not on the Dreamcast's VMUs

	if (currentMenu == &MessageDef) // Oh sod off!
		currentMenu = &MainDef; // Not like it matters
	menuactive = false;

	I_UpdateMouseGrab();
	I_SetTextInputMode(false);
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
	INT16 i;

	if (currentMenu->quitroutine)
	{
		// If you're going from a menu to itself, why are you running the quitroutine? You're not quitting it! -SH
		if (currentMenu != menudef && !currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
	}
	currentMenu = menudef;
	itemOn = currentMenu->lastOn;

	// in case of...
	if (itemOn >= currentMenu->numitems)
		itemOn = currentMenu->numitems - 1;
	M_UpdateItemOn();

	// the curent item can be disabled,
	// this code go up until an enabled item found
	if (( (currentMenu->menuitems[itemOn].status & IT_TYPE) & IT_SPACE ))
	{
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if (!( (currentMenu->menuitems[i].status & IT_TYPE) & IT_SPACE ))
			{
				itemOn = i;
				M_UpdateItemOn();
				break;
			}
		}
	}
}

// Guess I'll put this here, idk
boolean M_MouseNeeded(void)
{
	return (currentMenu == &MessageDef && currentMenu->prevMenu == &OP_AllControlsDef);
}

//
// M_Ticker
//
void M_Ticker(void)
{
	// reset input trigger
	noFurtherInput = false;

	if (dedicated)
		return;

	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8;

	//added : 30-01-98 : test mode for five seconds
	if (vidm_testingmode > 0)
	{
		// restore the previous video mode
		if (--vidm_testingmode == 0)
			setmodeneeded = vidm_previousmode + 1;
	}
}

//
// M_Init
//
void M_Init(void)
{
	CV_RegisterVar(&cv_nextmap);
	CV_RegisterVar(&cv_newgametype);
	CV_RegisterVar(&cv_chooseskin);
	CV_RegisterVar(&cv_autorecord);

	if (dedicated)
		return;

	// Menu hacks
	CV_RegisterVar(&cv_dummyteam);
	CV_RegisterVar(&cv_dummyscramble);
	CV_RegisterVar(&cv_dummyrings);
	CV_RegisterVar(&cv_dummylives);
	CV_RegisterVar(&cv_dummycontinues);
	CV_RegisterVar(&cv_dummymares);

	quitmsg[QUITMSG] = M_GetText("Eggman's tied explosives\nto your girlfriend, and\nwill activate them if\nyou press the 'Y' key!\nPress 'N' to save her!\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG1] = M_GetText("What would Tails say if\nhe saw you quitting the game?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG2] = M_GetText("Hey!\nWhere do ya think you're goin'?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG3] = M_GetText("Forget your studies!\nPlay some more!\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG4] = M_GetText("You're trying to say you\nlike Sonic 2K6 better than\nthis, right?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG5] = M_GetText("Don't leave yet -- there's a\nsuper emerald around that corner!\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG6] = M_GetText("You'd rather work than play?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG7] = M_GetText("Go ahead and leave. See if I care...\n*sniffle*\n\n(Press 'Y' to quit)");

	quitmsg[QUIT2MSG] = M_GetText("If you leave now,\nEggman will take over the world!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG1] = M_GetText("Don't quit!\nThere are animals\nto save!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG2] = M_GetText("Aw c'mon, just bop\na few more robots!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG3] = M_GetText("Did you get all those Chaos Emeralds?\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG4] = M_GetText("If you leave, I'll use\nmy spin attack on you!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG5] = M_GetText("Don't go!\nYou might find the hidden\nlevels!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG6] = M_GetText("Hit the 'N' key, Sonic!\nThe 'N' key!\n\n(Press 'Y' to quit)");

	quitmsg[QUIT3MSG] = M_GetText("Are you really going to give up?\nWe certainly would never give you up.\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG1] = M_GetText("Come on, just ONE more netgame!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG2] = M_GetText("Press 'N' to unlock\nthe Ultimate Cheat!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG3] = M_GetText("Why don't you go back and try\njumping on that house to\nsee what happens?\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG4] = M_GetText("Every time you press 'Y', an\nSRB2 Developer cries...\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG5] = M_GetText("You'll be back to play soon, though...\n......right?\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG6] = M_GetText("Aww, is Egg Rock Zone too\ndifficult for you?\n\n(Press 'Y' to quit)");

/*#ifdef HWRENDER
	// Permanently hide some options based on render mode
	if (rendermode == render_soft)
		OP_VideoOptionsMenu[1].status = IT_DISABLED;
#endif*/

#ifndef NONET
	CV_RegisterVar(&cv_serversort);
#endif

	//todo put this somewhere better...
	CV_RegisterVar(&cv_allcaps);
}

void M_InitCharacterTables(void)
{
	UINT8 i;

	// Setup PlayerMenu table
	for (i = 0; i < MAXSKINS; i++)
	{
		PlayerMenu[i].status = (i < 4 ? IT_CALL : IT_DISABLED);
		PlayerMenu[i].patch = PlayerMenu[i].text = NULL;
		PlayerMenu[i].itemaction = M_ChoosePlayer;
		PlayerMenu[i].alphaKey = 0;
	}

	// Setup description table
	for (i = 0; i < MAXSKINS; i++)
	{
		if (i == 0)
		{
			strcpy(description[i].notes, "\x82Sonic\x80 is the fastest of the three, but also the hardest to control. Beginners beware, but experts will find Sonic very powerful.\n\n\x82""Ability:\x80 Speed Thok\nDouble jump to zoom forward with a huge burst of speed.\n\n\x82Tip:\x80 Simply letting go of forward does not slow down in SRB2. To slow down, hold the opposite direction.");
			strcpy(description[i].picname, "");
			strcpy(description[i].skinname, "sonic");
		}
		else if (i == 1)
		{
			strcpy(description[i].notes, "\x82Tails\x80 is the most mobile of the three, but has the slowest speed. Because of his mobility, he's well-\nsuited to beginners.\n\n\x82""Ability:\x80 Fly\nDouble jump to start flying for a limited time. Repetitively hit the jump button to ascend.\n\n\x82Tip:\x80 To quickly descend while flying, hit the spin button.");
			strcpy(description[i].picname, "");
			strcpy(description[i].skinname, "tails");
		}
		else if (i == 2)
		{
			strcpy(description[i].notes, "\x82Knuckles\x80 is well-\nrounded and can destroy breakable walls simply by touching them, but he can't jump as high as the other two.\n\n\x82""Ability:\x80 Glide & Climb\nDouble jump to glide in the air as long as jump is held. Glide into a wall to climb it.\n\n\x82Tip:\x80 Press spin while climbing to jump off the wall; press jump instead to jump off\nand face away from\nthe wall.");
			strcpy(description[i].picname, "");
			strcpy(description[i].skinname, "knuckles");
		}
		else if (i == 3)
		{
			strcpy(description[i].notes, "\x82Sonic & Tails\x80 team up to take on Dr. Eggman!\nControl Sonic while Tails desperately struggles to keep up.\n\nPlayer 2 can control Tails directly by setting the controls in the options menu.\nTails's directional controls are relative to Player 1's camera.\n\nTails can pick up Sonic while flying and carry him around.");
			strcpy(description[i].picname, "CHRS&T");
			strcpy(description[i].skinname, "sonic&tails");
		}
		else
		{
			strcpy(description[i].notes, "???");
			strcpy(description[i].picname, "");
			strcpy(description[i].skinname, "");
		}
	}
}

// ==========================================================================
// SPECIAL MENU OPTION DRAW ROUTINES GO HERE
// ==========================================================================

// Converts a string into question marks.
// Used for the secrets menu, to hide yet-to-be-unlocked stuff.
static const char *M_CreateSecretMenuOption(const char *str)
{
	static char qbuf[32];
	int i;

	for (i = 0; i < 31; ++i)
	{
		if (!str[i])
		{
			qbuf[i] = '\0';
			return qbuf;
		}
		else if (str[i] != ' ')
			qbuf[i] = '?';
		else
			qbuf[i] = ' ';
	}

	qbuf[31] = '\0';
	return qbuf;
}

static void M_DrawThermo(INT32 x, INT32 y, consvar_t *cv)
{
	INT32 xx = x, i;
	lumpnum_t leftlump, rightlump, centerlump[2], cursorlump;
	patch_t *p;

	leftlump = W_GetNumForName("M_THERML");
	rightlump = W_GetNumForName("M_THERMR");
	centerlump[0] = W_GetNumForName("M_THERMM");
	centerlump[1] = W_GetNumForName("M_THERMM");
	cursorlump = W_GetNumForName("M_THERMO");

	V_DrawScaledPatch(xx, y, 0, p = W_CachePatchNum(leftlump,PU_PATCH));
	xx += SHORT(p->width) - SHORT(p->leftoffset);
	for (i = 0; i < 16; i++)
	{
		V_DrawScaledPatch(xx, y, V_WRAPX, W_CachePatchNum(centerlump[i & 1], PU_PATCH));
		xx += 8;
	}
	V_DrawScaledPatch(xx, y, 0, W_CachePatchNum(rightlump, PU_PATCH));

	xx = (cv->value - cv->PossibleValue[0].value) * (15*8) /
		(cv->PossibleValue[1].value - cv->PossibleValue[0].value);

	V_DrawScaledPatch((x + 8) + xx, y, 0, W_CachePatchNum(cursorlump, PU_PATCH));
}

//  A smaller 'Thermo', with range given as percents (0-100)
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv)
{
	INT32 i;
	INT32 range;
	INT32 range_default;
	patch_t *p;

	for (i = 0; cv->PossibleValue[i+1].strvalue; i++);

	range = ((cv->value - cv->PossibleValue[0].value) * 100 /
	 (cv->PossibleValue[i].value - cv->PossibleValue[0].value));

	if (range < 0)
		range = 0;
	else if (range > 100)
		range = 100;

	range_default = ((atoi(cv->defaultvalue) - cv->PossibleValue[0].value) * 100 /
	 (cv->PossibleValue[i].value - cv->PossibleValue[0].value));

	if (range_default < 0)
		range_default = 0;
	else if (range_default > 100)
		range_default = 100;

	x = BASEVIDWIDTH - x - SLIDER_WIDTH;

	p =  W_CachePatchName("M_SLIDEM", PU_PATCH);
	for (i = 0; i < SLIDER_RANGE; i++)
		V_DrawScaledPatch (x+i*8, y, 0,p);

	V_DrawScaledPatch(x - 8, y, 0, W_CachePatchName("M_SLIDEL", PU_PATCH));

	p = W_CachePatchName("M_SLIDER", PU_PATCH);
	V_DrawScaledPatch(x+SLIDER_RANGE*8, y, 0, p);

	// draw the slider cursor
	p = W_CachePatchName("M_SLIDEC", PU_PATCH);
	V_DrawMappedPatch(x + ((SLIDER_RANGE-1)*8*range)/100, y, 0, p, yellowmap);

	if (range != range_default)
	{
		// draw the default
		V_DrawMappedPatch(x + ((SLIDER_RANGE-1)*8*range_default)/100, y, V_TRANSLUCENT, p, yellowmap);
	}

	// draw current value
	V_DrawCenteredString(x + 40, y, V_30TRANS,
			(cv->flags & CV_FLOAT) ? va("%.2f", FIXED_TO_FLOAT(cv->value)) : va("%d", cv->value));
}

//
//  Draw a textbox, like Quake does, because sometimes it's difficult
//  to read the text with all the stuff in the background...
//
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines)
{
	// Solid color textbox.
	V_DrawFill(x+5, y+5, width*8+6, boxlines*8+6, 239);
	//V_DrawFill(x+8, y+8, width*8, boxlines*8, 31);
/*
	patch_t *p;
	INT32 cx, cy, n;
	INT32 step, boff;

	step = 8;
	boff = 8;

	// draw left side
	cx = x;
	cy = y;
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TL], PU_PATCH));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_L], PU_PATCH);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BL], PU_PATCH));

	// draw middle
	V_DrawFlatFill(x + boff, y + boff, width*step, boxlines*step, st_borderpatchnum);

	cx += boff;
	cy = y;
	while (width > 0)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_T], PU_PATCH));
		V_DrawScaledPatch(cx, y + boff + boxlines*step, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_B], PU_PATCH));
		width--;
		cx += step;
	}

	// draw right side
	cy = y;
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TR], PU_PATCH));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_R], PU_PATCH);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BR], PU_PATCH));
*/
}

//
// Draw border for the savegame description
//
static void M_DrawSaveLoadBorder(INT32 x,INT32 y)
{
	INT32 i;

	V_DrawScaledPatch (x-8,y+7,0,W_CachePatchName("M_LSLEFT",PU_PATCH));

	for (i = 0;i < 24;i++)
	{
		V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSCNTR",PU_PATCH));
		x += 8;
	}

	V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSRGHT",PU_PATCH));
}

//
// M_DrawMapEmblems
//
// used by pause & statistics to draw a row of emblems for a map
//
static void M_DrawMapEmblems(INT32 mapnum, INT32 x, INT32 y)
{
	UINT8 lasttype = UINT8_MAX, curtype;
	emblem_t *emblem = M_GetLevelEmblems(mapnum);

	while (emblem)
	{
		switch (emblem->type)
		{
			case ET_SCORE: case ET_TIME: case ET_RINGS:
				curtype = 1; break;
			case ET_NGRADE: case ET_NTIME:
				curtype = 2; break;
			default:
				curtype = 0; break;
		}

		// Shift over if emblem is of a different discipline
		if (lasttype != UINT8_MAX && lasttype != curtype)
			x -= 4;
		lasttype = curtype;

		if (emblem->collected)
			V_DrawSmallMappedPatch(x, y, 0, W_CachePatchName(M_GetEmblemPatch(emblem), PU_PATCH),
			                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
		else
			V_DrawSmallScaledPatch(x, y, 0, W_CachePatchName("NEEDIT", PU_PATCH));

		emblem = M_GetLevelEmblems(-1);
		x -= 12;
	}
}

static void M_DrawMenuTitle(void)
{
	if (currentMenu->menutitlepic)
	{
		patch_t *p = W_CachePatchName(currentMenu->menutitlepic, PU_PATCH);

		if (p->height > 24) // title is larger than normal
		{
			INT32 xtitle = (BASEVIDWIDTH - (SHORT(p->width)/2))/2;
			INT32 ytitle = (30 - (SHORT(p->height)/2))/2;

			if (xtitle < 0)
				xtitle = 0;
			if (ytitle < 0)
				ytitle = 0;

			V_DrawSmallScaledPatch(xtitle, ytitle, 0, p);
		}
		else
		{
			INT32 xtitle = (BASEVIDWIDTH - SHORT(p->width))/2;
			INT32 ytitle = (30 - SHORT(p->height))/2;

			if (xtitle < 0)
				xtitle = 0;
			if (ytitle < 0)
				ytitle = 0;

			V_DrawScaledPatch(xtitle, ytitle, 0, p);
		}
	}
}

static void M_DrawSplitText(INT32 x, INT32 y, INT32 option, const char* str, INT32 alpha)
{
	char* icopy = strdup(str);
	char** clines = NULL;
	INT16 num_lines = 0;

	if (icopy == NULL) return;

	char* tok = strtok(icopy, "\n");

	while (tok != NULL)
	{
		char* line = strdup(tok);

		if (line == NULL) return;

		clines = realloc(clines, (num_lines + 1) * sizeof(char*));
		clines[num_lines] = line;
		num_lines++;

		tok = strtok(NULL, "\n");
	}

	free(icopy);

	INT16 yoffset;
	yoffset = (((5*10 - num_lines*10)));

	// Draw BG first,,,
	for (int i = 0; i < num_lines; i++)
	{
		V_DrawFill(0, (y + yoffset - 6)+5, vid.width, 11, 239|V_SNAPTOBOTTOM|V_SNAPTOLEFT);
		yoffset += 11;
	}

	yoffset = (((5*10 - num_lines*10)));

	// THEN the text
	for (int i = 0; i < num_lines; i++)
	{
        V_DrawCenteredThinString(x, y + yoffset, option, clines[i]);
		V_DrawCenteredThinString(x, y + yoffset, option|V_YELLOWMAP|((9 - alpha) << V_ALPHASHIFT), clines[i]);
		yoffset += 10;
        // Remember to free the memory for each line when you're done with it.
        free(clines[i]);
    }

	free(clines);
}

static void M_DoToolTips(menu_t *menu)
{
	char *desc = NULL;
	if (menu->menuitems[itemOn].desc != NULL)
	{
		desc = V_WordWrap(0, BASEVIDWIDTH, V_ALLOWLOWERCASE, menu->menuitems[itemOn].desc);
	}
	else if (menu->menuitems[itemOn].status & IT_CVAR)
	{
		consvar_t *cvar = menu->menuitems[itemOn].itemaction;
		if (cvar != NULL && cvar->desc != NULL)
		{
			desc = V_WordWrap(0, BASEVIDWIDTH, V_ALLOWLOWERCASE, cvar->desc);
		}
	}

	if (desc != NULL)
	{
		M_DrawSplitText(BASEVIDWIDTH / 2, BASEVIDHEIGHT-50, V_ALLOWLOWERCASE|V_SNAPTOBOTTOM, desc, coolalphatimer);
		Z_Free(desc);
		if (coolalphatimer > 0)
			coolalphatimer--;
	}
}

static void M_DrawGenericMenu(void)
{
	INT32 x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH));
					}
				}
				/* FALLTHRU */
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - SLIDER_WIDTH - 6 - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0) - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0), y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				/* FALLTHRU */
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_PATCH), graymap);
				y += LINEHEIGHT;
				break;
			case IT_TRANSTEXT:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				/* FALLTHRU */
			case IT_TRANSTEXT2:
				V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;

				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_HEADERTEXT: // draws 16 pixels to the left, in yellow text
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;

				V_DrawString(x-16, y, V_YELLOWMAP, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(currentMenu->x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
		V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
	M_DoToolTips(currentMenu);
	M_LegacyCreditsToolTips();
}

static void M_DrawPauseMenu(void)
{
	if (!netgame && !multiplayer && (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		emblem_t *emblem_detail[3] = {NULL, NULL, NULL};
		char emblem_text[3][20];
		INT32 i;

		M_DrawTextBox(27, 16, 32, 6);

		// Draw any and all emblems at the top.
		M_DrawMapEmblems(gamemap, 272, 28);

		if (mapheaderinfo[gamemap-1]->actnum != 0)
			V_DrawString(40, 28, V_YELLOWMAP, va("%s %d", mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->actnum));
		else
			V_DrawString(40, 28, V_YELLOWMAP, mapheaderinfo[gamemap-1]->lvlttl);

		// Set up the detail boxes.
		{
			emblem_t *emblem = M_GetLevelEmblems(gamemap);
			while (emblem)
			{
				INT32 emblemslot;
				char targettext[9], currenttext[9];

				switch (emblem->type)
				{
					case ET_SCORE:
						snprintf(targettext, 9, "%d", emblem->var);
						snprintf(currenttext, 9, "%u", G_GetBestScore(gamemap));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 0;
						break;
					case ET_TIME:
						emblemslot = emblem->var; // dumb hack
						snprintf(targettext, 9, "%i:%02i.%02i",
							G_TicsToMinutes((tic_t)emblemslot, false),
							G_TicsToSeconds((tic_t)emblemslot),
							G_TicsToCentiseconds((tic_t)emblemslot));

						emblemslot = (INT32)G_GetBestTime(gamemap); // dumb hack pt ii
						if ((tic_t)emblemslot == UINT32_MAX)
							snprintf(currenttext, 9, "-:--.--");
						else
							snprintf(currenttext, 9, "%i:%02i.%02i",
								G_TicsToMinutes((tic_t)emblemslot, false),
								G_TicsToSeconds((tic_t)emblemslot),
								G_TicsToCentiseconds((tic_t)emblemslot));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 1;
						break;
					case ET_RINGS:
						snprintf(targettext, 9, "%d", emblem->var);
						snprintf(currenttext, 9, "%u", G_GetBestRings(gamemap));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 2;
						break;
					case ET_NGRADE:
						snprintf(targettext, 9, "%u", P_GetScoreForGrade(gamemap, 0, emblem->var));
						snprintf(currenttext, 9, "%u", G_GetBestNightsScore(gamemap, 0));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 1;
						break;
					case ET_NTIME:
						emblemslot = emblem->var; // dumb hack pt iii
						snprintf(targettext, 9, "%i:%02i.%02i",
							G_TicsToMinutes((tic_t)emblemslot, false),
							G_TicsToSeconds((tic_t)emblemslot),
							G_TicsToCentiseconds((tic_t)emblemslot));

						emblemslot = (INT32)G_GetBestNightsTime(gamemap, 0); // dumb hack pt iv
						if ((tic_t)emblemslot == UINT32_MAX)
							snprintf(currenttext, 9, "-:--.--");
						else
							snprintf(currenttext, 9, "%i:%02i.%02i",
								G_TicsToMinutes((tic_t)emblemslot, false),
								G_TicsToSeconds((tic_t)emblemslot),
								G_TicsToCentiseconds((tic_t)emblemslot));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 2;
						break;
					default:
						goto bademblem;
				}
				if (emblem_detail[emblemslot])
					goto bademblem;

				emblem_detail[emblemslot] = emblem;
				snprintf(emblem_text[emblemslot], 20, "%8s /%8s", currenttext, targettext);
				emblem_text[emblemslot][19] = 0;

				bademblem:
				emblem = M_GetLevelEmblems(-1);
			}
		}
		for (i = 0; i < 3; ++i)
		{
			emblem_t *emblem = emblem_detail[i];
			if (!emblem)
				continue;

			if (emblem->collected)
				V_DrawSmallMappedPatch(40, 44 + (i*8), 0, W_CachePatchName(M_GetEmblemPatch(emblem), PU_PATCH),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(40, 44 + (i*8), 0, W_CachePatchName("NEEDIT", PU_PATCH));

			switch (emblem->type)
			{
				case ET_SCORE:
				case ET_NGRADE:
					V_DrawString(56, 44 + (i*8), V_YELLOWMAP, "SCORE:");
					break;
				case ET_TIME:
				case ET_NTIME:
					V_DrawString(56, 44 + (i*8), V_YELLOWMAP, "TIME:");
					break;
				case ET_RINGS:
					V_DrawString(56, 44 + (i*8), V_YELLOWMAP, "RINGS:");
					break;
			}
			V_DrawRightAlignedString(284, 44 + (i*8), V_MONOSPACE, emblem_text[i]);
		}
	}

	M_DrawGenericMenu();
}

static void M_DrawCenteredMenu(void)
{
	INT32 x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH));
					}
				}
				/* FALLTHRU */
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawCenteredString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch(currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch(currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - SLIDER_WIDTH - 6 - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0) - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0), y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
				/* FALLTHRU */
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;

				V_DrawCenteredString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_PATCH), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
	}
	else
	{
		V_DrawScaledPatch(x - V_StringWidth(currentMenu->menuitems[itemOn].text, 0)/2 - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
		V_DrawCenteredString(x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 8, i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] == '\n')
			h += 8;

	return h;
}

// ==========================================================================
// Extraneous menu patching functions
// ==========================================================================

//
// M_PatchSkinNameTable
//
// Like M_PatchLevelNameTable, but for cv_chooseskin
//
static void M_PatchSkinNameTable(void)
{
	INT32 j;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	for (j = 0; j < MAXSKINS; j++)
	{
		if (skins[j].name[0] != '\0')
		{
			skins_cons_t[j].strvalue = skins[j].name;
			skins_cons_t[j].value = j+1;
		}
		else
		{
			skins_cons_t[j].strvalue = NULL;
			skins_cons_t[j].value = 0;
		}
	}

	CV_SetValue(&cv_chooseskin, cv_chooseskin.value); // This causes crash sometimes?!

	CV_SetValue(&cv_chooseskin, 1);
	CV_AddValue(&cv_chooseskin, -1);
	CV_AddValue(&cv_chooseskin, 1);

	return;
}

// Call before showing any level-select menus
static void M_PrepareLevelSelect(void)
{
	if (levellistmode != LLM_CREATESERVER)
		CV_SetValue(&cv_nextmap, M_GetFirstLevelInList());
	else
		Newgametype_OnChange(); // Make sure to start on an appropriate map if wads have been added
}

//
// M_CanShowLevelInList
//
// Determines whether to show a given map in the various level-select lists.
// Set gt = -1 to ignore gametype.
//
boolean M_CanShowLevelInList(INT32 mapnum, INT32 gt)
{
	// Does the map exist?
	if (!mapheaderinfo[mapnum])
		return false;

	// Does the map have a name?
	if (!mapheaderinfo[mapnum]->lvlttl[0])
		return false;

	switch (levellistmode)
	{
		case LLM_CREATESERVER:
			// Should the map be hidden?
			if (mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU)
				return false;

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

			if (gt == GT_COOP && (mapheaderinfo[mapnum]->typeoflevel & TOL_COOP))
				return true;

			if (gt == GT_COMPETITION && (mapheaderinfo[mapnum]->typeoflevel & TOL_COMPETITION))
				return true;

			if (gt == GT_CTF && (mapheaderinfo[mapnum]->typeoflevel & TOL_CTF))
				return true;

			if ((gt == GT_MATCH || gt == GT_TEAMMATCH) && (mapheaderinfo[mapnum]->typeoflevel & TOL_MATCH))
				return true;

			if ((gt == GT_TAG || gt == GT_HIDEANDSEEK) && (mapheaderinfo[mapnum]->typeoflevel & TOL_TAG))
				return true;

			if (gt == GT_RACE && (mapheaderinfo[mapnum]->typeoflevel & TOL_RACE))
				return true;

			return false;

		case LLM_LEVELSELECT:
			if (mapheaderinfo[mapnum]->levelselect != maplistoption)
				return false;

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

			return true;
		case LLM_RECORDATTACK:
			if (!(mapheaderinfo[mapnum]->menuflags & LF2_RECORDATTACK))
				return false;

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

			if (mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
				return true;

			if (!mapvisited[mapnum])
				return false;

			return true;
		case LLM_NIGHTSATTACK:
			if (!(mapheaderinfo[mapnum]->menuflags & LF2_NIGHTSATTACK))
				return false;

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

			if (mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
				return true;

			if (!mapvisited[mapnum])
				return false;

			return true;
	}

	// Hmm? Couldn't decide?
	return false;
}

static INT32 M_CountLevelsToShowInList(void)
{
	INT32 mapnum, count = 0;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelInList(mapnum, -1))
			count++;

	return count;
}

static INT32 M_GetFirstLevelInList(void)
{
	INT32 mapnum;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelInList(mapnum, -1))
			return mapnum + 1;

	return 1;
}

// ==================================================
// MESSAGE BOX (aka: a hacked, cobbled together menu)
// ==================================================
static void M_DrawMessageMenu(void);

// Because this is just a hack-ish 'menu', I'm not putting this with the others
static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0,NULL, NULL, NULL, NULL, 0}
};

menu_t MessageDef =
{
	NULL,               // title
	1,                  // # of menu items
	NULL,               // previous menu       (TO HACK)
	MessageMenu,        // menuitem_t ->
	M_DrawMessageMenu,  // drawing routine ->
	0, 0,               // x, y                (TO HACK)
	0,                  // lastOn, flags       (TO HACK)
	NULL
};


void M_StartMessage(const char *string, void *routine,
	menumessagetype_t itemtype)
{
	size_t max = 0, start = 0, i, strlines;
	static char *message = NULL;
	Z_Free(message);
	message = Z_StrDup(string);
	DEBFILE(message);

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, colors, etc. but who cares.
	strlines = 0;
	for (i = 0; message[i]; i++)
	{
		if (message[i] == ' ')
		{
			start = i;
			max += 4;
		}
		else if (message[i] == '\n')
		{
			strlines = i;
			start = 0;
			max = 0;
			continue;
		}
		else
			max += 8;

		// Start trying to wrap if presumed length exceeds the screen width.
		if (max >= BASEVIDWIDTH && start > 0)
		{
			message[start] = '\n';
			max -= (start-strlines)*8;
			strlines = start;
			start = 0;
		}
	}

	start = 0;
	max = 0;

	M_StartControlPanel(); // can't put menuactive to true

	if (currentMenu == &MessageDef) // Prevent recursion
		MessageDef.prevMenu = &MainDef;
	else
		MessageDef.prevMenu = currentMenu;

	MessageDef.menuitems[0].text     = message;
	MessageDef.menuitems[0].alphaKey = (UINT8)itemtype;
	if (!routine && itemtype != MM_NOTHING) itemtype = MM_NOTHING;
	switch (itemtype)
	{
		case MM_NOTHING:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = M_StopMessage;
			break;
		case MM_YESNO:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
		case MM_EVENTHANDLER:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
	}
	//added : 06-02-98: now draw a textbox around the message
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0;i < strlen(message+start);i++)
		{
			if (*(message+start+i) == '\n')
			{
				if (i > max)
					max = i;
				start += i;
				i = (size_t)-1; //added : 07-02-98 : damned!
				start++;
				break;
			}
		}

		if (i == strlen(message+start))
			start += i;
	}

	MessageDef.x = (INT16)((BASEVIDWIDTH  - 8*max-16)/2);
	MessageDef.y = (INT16)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	MessageDef.lastOn = (INT16)((strlines<<8)+max);

	//M_SetupNextMenu();
	currentMenu = &MessageDef;
	itemOn = 0;
	M_UpdateItemOn();
}

#define MAXMSGLINELEN 256

static void M_DrawMessageMenu(void)
{
	INT32 y = currentMenu->y;
	size_t i, start = 0;
	INT16 max;
	char string[MAXMSGLINELEN];
	INT32 mlines;
	const char *msg = currentMenu->menuitems[0].text;

	mlines = currentMenu->lastOn>>8;
	max = (INT16)((UINT8)(currentMenu->lastOn & 0xFF)*8);

	// hack: draw RA background in RA menus
	if (gamestate == GS_TIMEATTACK)
		V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_PATCH));

	M_DrawTextBox(currentMenu->x, y - 8, (max+7)>>3, mlines);

	while (*(msg+start))
	{
		size_t len = strlen(msg+start);

		for (i = 0; i < len; i++)
		{
			if (*(msg+start+i) == '\n')
			{
				memset(string, 0, MAXMSGLINELEN);
				if (i >= MAXMSGLINELEN)
				{
					CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
					return;
				}
				else
				{
					strncpy(string,msg+start, i);
					string[i] = '\0';
					start += i;
					i = (size_t)-1; //added : 07-02-98 : damned!
					start++;
				}
				break;
			}
		}

		if (i == strlen(msg+start))
		{
			if (i >= MAXMSGLINELEN)
			{
				CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
				return;
			}
			else
			{
				strcpy(string, msg + start);
				start += i;
			}
		}

		V_DrawString((BASEVIDWIDTH - V_StringWidth(string, 0))/2,y,V_ALLOWLOWERCASE,string);
		y += 8; //SHORT(hu_font[0]->height);
	}
}

// default message handler
static void M_StopMessage(INT32 choice)
{
	(void)choice;
	if (menuactive)
		M_SetupNextMenu(MessageDef.prevMenu);
}

// =========
// IMAGEDEFS
// =========

// Draw an Image Def.  Aka, Help images.
// Defines what image is used in (menuitem_t)->text.
// You can even put multiple images in one menu!
static void M_DrawImageDef(void)
{
	patch_t *patch = W_CachePatchName(currentMenu->menuitems[itemOn].text, PU_CACHE);
	if (patch->width <= BASEVIDWIDTH)
		V_DrawScaledPatch(0,0,0,patch);
	else
		V_DrawSmallScaledPatch(0,0,0,patch);

	if (currentMenu->numitems > 1)
		V_DrawString(0,192,V_TRANSLUCENT, va("PAGE %d of %hd", itemOn+1, currentMenu->numitems));
}

// Handles the ImageDefs.  Just a specialized function that
// uses left and right movement.
static void M_HandleImageDef(INT32 choice)
{
	switch (choice)
	{
		case KEY_RIGHTARROW:
			if (currentMenu->numitems == 1)
				break;

			S_StartSound(NULL, sfx_menu1);
			if (itemOn >= (INT16)(currentMenu->numitems-1))
				itemOn = 0;
            else itemOn++;
			M_UpdateItemOn();
			break;

		case KEY_LEFTARROW:
			if (currentMenu->numitems == 1)
				break;

			S_StartSound(NULL, sfx_menu1);
			if (!itemOn)
				itemOn = currentMenu->numitems - 1;
			else itemOn--;
			M_UpdateItemOn();
			break;

		case KEY_ESCAPE:
		case KEY_ENTER:
			M_ClearMenus(true);
			break;
	}
}

// ======================
// MISC MAIN MENU OPTIONS
// ======================

static void M_AddonsOptions(INT32 choice)
{
	(void)choice;
	Addons_option_Onchange();

	M_SetupNextMenu(&OP_AddonsOptionsDef);
}

#define LOCATIONSTRING1 "Visit \x83SRB2.ORG/MODS\x80 to get & make add-ons!"
//#define LOCATIONSTRING2 "Visit \x88SRB2.ORG/MODS\x80 to get & make add-ons!"

static void M_LoadAddonsPatches(void)
{
	addonsp[EXT_FOLDER] = W_CachePatchName("M_FFLDR", PU_PATCH);
	addonsp[EXT_UP] = W_CachePatchName("M_FBACK", PU_PATCH);
	addonsp[EXT_NORESULTS] = W_CachePatchName("M_FNOPE", PU_PATCH);
	addonsp[EXT_TXT] = W_CachePatchName("M_FTXT", PU_PATCH);
	addonsp[EXT_CFG] = W_CachePatchName("M_FCFG", PU_PATCH);
	addonsp[EXT_WAD] = W_CachePatchName("M_FWAD", PU_PATCH);
#ifdef USE_KART
	addonsp[EXT_KART] = W_CachePatchName("M_FKART", PU_PATCH);
#endif
	addonsp[EXT_PK3] = W_CachePatchName("M_FPK3", PU_PATCH);
	addonsp[EXT_SOC] = W_CachePatchName("M_FSOC", PU_PATCH);
	addonsp[EXT_LUA] = W_CachePatchName("M_FLUA", PU_PATCH);
	addonsp[NUM_EXT] = W_CachePatchName("M_FUNKN", PU_PATCH);
	addonsp[NUM_EXT+1] = W_CachePatchName("M_FSEL", PU_PATCH);
	addonsp[NUM_EXT+2] = W_CachePatchName("M_FLOAD", PU_PATCH);
	addonsp[NUM_EXT+3] = W_CachePatchName("M_FSRCH", PU_PATCH);
	addonsp[NUM_EXT+4] = W_CachePatchName("M_FSAVE", PU_PATCH);
}

static void M_Addons(INT32 choice)
{
	const char *pathname = ".";

	(void)choice;

	// If M_GetGameypeColor() is ever ported from Kart, then remove this.
	highlightflags = V_YELLOWMAP;
	recommendedflags = V_GREENMAP;
	warningflags = V_REDMAP;

	if (cv_addons_option.value == 0)
		pathname = usehome ? srb2home : srb2path;
	else if (cv_addons_option.value == 1)
		pathname = srb2home;
	else if (cv_addons_option.value == 2)
		pathname = srb2path;
	else if (cv_addons_option.value == 3 && *cv_addons_folder.string != '\0')
		pathname = cv_addons_folder.string;

	strlcpy(menupath, pathname, 1024);
	menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath) + 1;

	if (menupath[menupathindex[menudepthleft]-2] != PATHSEP[0])
	{
		menupath[menupathindex[menudepthleft]-1] = PATHSEP[0];
		menupath[menupathindex[menudepthleft]] = 0;
	}
	else
		--menupathindex[menudepthleft];

	if (!preparefilemenu(false))
	{
		M_StartMessage(va("No files/folders found.\n\n%s\n\n(Press a key)\n",LOCATIONSTRING1),NULL,MM_NOTHING);
			// (recommendedflags == V_SKYMAP ? LOCATIONSTRING2 : LOCATIONSTRING1))
		return;
	}
	else
		dir_on[menudepthleft] = 0;

	M_LoadAddonsPatches();

	MISC_AddonsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_AddonsDef);
}

#define width 4
#define vpadding 27
#define h (BASEVIDHEIGHT-(2*vpadding))
#define NUMCOLOURS 8 // when toast's coding it's british english hacker fucker
static void M_DrawTemperature(INT32 x, fixed_t t)
{
	INT32 y;

	// bounds check
	if (t > FRACUNIT)
		t = FRACUNIT;
	/*else if (t < 0) -- not needed
		t = 0;*/

	// scale
	if (t > 1)
		t = (FixedMul(h<<FRACBITS, t)>>FRACBITS);

	// border
	V_DrawFill(x - 1, vpadding, 1, h, 120);
	V_DrawFill(x + width, vpadding, 1, h, 120);
	V_DrawFill(x - 1, vpadding-1, width+2, 1, 120);
	V_DrawFill(x - 1, vpadding+h, width+2, 1, 120);

	// bar itself
	y = h;
	if (t)
		for (t = h - t; y > 0; y--)
		{
			UINT8 colours[NUMCOLOURS] = {135, 133, 92, 77, 114, 178, 161, 162};
			UINT8 c;
			if (y <= t) break;
			if (y+vpadding >= BASEVIDHEIGHT/2)
				c = 185;
			else
				c = colours[(NUMCOLOURS*(y-1))/(h/2)];
			V_DrawFill(x, y-1 + vpadding, width, 1, c);
		}

	// fill the rest of the backing
	if (y)
		V_DrawFill(x, vpadding, width, y, 30);
}
#undef width
#undef vpadding
#undef h
#undef NUMCOLOURS

static char *M_AddonsHeaderPath(void)
{
	UINT32 len;
	static char header[1024];

	strlcpy(header, va("%s folder%s", cv_addons_option.string, menupath+menupathindex[menudepth-1]-1), 1024);
	len = strlen(header);
	if (len > 34)
	{
		len = len-34;
		header[len] = header[len+1] = header[len+2] = '.';
	}
	else
		len = 0;

	return header+len;
}

#define UNEXIST S_StartSound(NULL, sfx_lose);\
		M_SetupNextMenu(MISC_AddonsDef.prevMenu);\
		M_StartMessage(va("\x82%s\x80\nThis folder no longer exists!\nAborting to main menu.\n\n(Press a key)\n", M_AddonsHeaderPath()),NULL,MM_NOTHING)

#define CLEARNAME Z_Free(refreshdirname);\
					refreshdirname = NULL

static void M_AddonsClearName(INT32 choice)
{
	CLEARNAME;
	M_StopMessage(choice);
}

// returns whether to do message draw
static boolean M_AddonsRefresh(void)
{
	if ((refreshdirmenu & REFRESHDIR_NORMAL) && !preparefilemenu(true))
	{
		UNEXIST;
		return true;
	}

	if (refreshdirmenu & REFRESHDIR_ADDFILE)
	{
		char *message = NULL;

		if (refreshdirmenu & REFRESHDIR_NOTLOADED)
		{
			S_StartSound(NULL, sfx_lose);
			if (refreshdirmenu & REFRESHDIR_MAX)
				message = va("%c%s\x80\nMaximum number of add-ons reached.\nA file could not be loaded.\nIf you want to play with this add-on, restart the game to clear existing ones.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			else
				message = va("%c%s\x80\nA file was not loaded.\nCheck the console log for more information.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
		}
		else if (refreshdirmenu & (REFRESHDIR_WARNING|REFRESHDIR_ERROR))
		{
			S_StartSound(NULL, sfx_skid);
			message = va("%c%s\x80\nA file was loaded with %s.\nCheck the console log for more information.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname, ((refreshdirmenu & REFRESHDIR_ERROR) ? "errors" : "warnings"));
		}

		if (message)
		{
			M_StartMessage(message,M_AddonsClearName,MM_EVENTHANDLER);
			return true;
		}

		S_StartSound(NULL, sfx_strpst);
		CLEARNAME;
	}

	return false;
}

static void M_DrawAddons(void)
{
	INT32 x, y;
	ssize_t i, m;
	const UINT8 *flashcol = NULL;
	UINT8 hilicol;

	// hack - need to refresh at end of frame to handle addfile...
	if (refreshdirmenu & M_AddonsRefresh())
	{
		M_DrawMessageMenu();
		return;
	}

	// Jimita: Load addons menu patches.
	if (needpatchrecache)
		M_LoadAddonsPatches();

	if (Playing())
		V_DrawCenteredString(BASEVIDWIDTH/2, 5, warningflags, "Adding files mid-game may cause problems.");
	else
		V_DrawCenteredString(BASEVIDWIDTH/2, 5, 0, LOCATIONSTRING1);
			// (recommendedflags == V_SKYMAP ? LOCATIONSTRING2 : LOCATIONSTRING1)

	if (numwadfiles <= mainwads+1)
		y = 0;
	else if (numwadfiles >= MAX_WADFILES)
		y = FRACUNIT;
	else
	{
		x = FixedDiv(((ssize_t)(numwadfiles) - (ssize_t)(mainwads+1))<<FRACBITS, ((ssize_t)MAX_WADFILES - (ssize_t)(mainwads+1))<<FRACBITS);
		y = FixedDiv((((ssize_t)packetsizetally-(ssize_t)mainwadstally)<<FRACBITS), ((((ssize_t)MAXFILENEEDED*sizeof(UINT8)-(ssize_t)mainwadstally)-(5+22))<<FRACBITS)); // 5+22 = (a.ext + checksum length) is minimum addition to packet size tally
		if (x > y)
			y = x;
		if (y > FRACUNIT) // happens because of how we're shrinkin' it a little
			y = FRACUNIT;
	}

	M_DrawTemperature(BASEVIDWIDTH - 19 - 5, y);

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y + 1;

	hilicol = V_GetStringColormap(highlightflags)[120];

	V_DrawString(x-21, (y - 16) + (lsheadingheight - 12), highlightflags|V_ALLOWLOWERCASE, M_AddonsHeaderPath());
	V_DrawFill(x-21, (y - 16) + (lsheadingheight - 3), MAXSTRINGLENGTH*8+6, 1, hilicol);
	V_DrawFill(x-21, (y - 16) + (lsheadingheight - 2), MAXSTRINGLENGTH*8+6, 1, 30);

	m = (BASEVIDHEIGHT - currentMenu->y + 2) - (y - 1);
	V_DrawFill(x - 21, y - 1, MAXSTRINGLENGTH*8+6, m, 239);

	// scrollbar!
	if (sizedirmenu <= (2*numaddonsshown + 1))
		i = 0;
	else
	{
		ssize_t q = m;
		m = ((2*numaddonsshown + 1) * m)/sizedirmenu;
		if (dir_on[menudepthleft] <= numaddonsshown) // all the way up
			i = 0;
		else if (sizedirmenu <= (dir_on[menudepthleft] + numaddonsshown + 1)) // all the way down
			i = q-m;
		else
			i = ((dir_on[menudepthleft] - numaddonsshown) * (q-m))/(sizedirmenu - (2*numaddonsshown + 1));
	}

	V_DrawFill(x + MAXSTRINGLENGTH*8+5 - 21, (y - 1) + i, 1, m, hilicol);

	// get bottom...
	m = dir_on[menudepthleft] + numaddonsshown + 1;
	if (m > (ssize_t)sizedirmenu)
		m = sizedirmenu;

	// then compute top and adjust bottom if needed!
	if (m < (2*numaddonsshown + 1))
	{
		m = min(sizedirmenu, 2*numaddonsshown + 1);
		i = 0;
	}
	else
		i = m - (2*numaddonsshown + 1);

	if (i != 0)
		V_DrawString(19, y+4 - (skullAnimCounter/5), highlightflags, "\x1A");

	if (skullAnimCounter < 4)
		flashcol = V_GetStringColormap(highlightflags);

	for (; i < m; i++)
	{
		UINT32 flags = V_ALLOWLOWERCASE;
		if (y > BASEVIDHEIGHT) break;
		if (dirmenu[i])
#define type (UINT8)(dirmenu[i][DIR_TYPE])
		{
			if (type & EXT_LOADED)
			{
				flags |= V_TRANSLUCENT;
				V_DrawSmallScaledPatch(x-(16+4), y, V_TRANSLUCENT, addonsp[(type & ~EXT_LOADED)]);
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[NUM_EXT+2]);
			}
			else
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[(type & ~EXT_LOADED)]);

			if ((size_t)i == dir_on[menudepthleft])
			{
				V_DrawFixedPatch((x-(16+4))<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, 0, addonsp[NUM_EXT+1], flashcol);
				flags = V_ALLOWLOWERCASE|highlightflags;
			}

#define charsonside 14
			if (dirmenu[i][DIR_LEN] > (charsonside*2 + 3))
				V_DrawString(x, y+4, flags, va("%.*s...%s", charsonside, dirmenu[i]+DIR_STRING, dirmenu[i]+DIR_STRING+dirmenu[i][DIR_LEN]-(charsonside+1)));
#undef charsonside
			else
				V_DrawString(x, y+4, flags, dirmenu[i]+DIR_STRING);
		}
#undef type
		y += 16;
	}

	if (m != (ssize_t)sizedirmenu)
		V_DrawString(19, y-12 + (skullAnimCounter/5), highlightflags, "\x1B");

	y = BASEVIDHEIGHT - currentMenu->y + 1;

	M_DrawTextBox(x - (21 + 5), y, MAXSTRINGLENGTH, 1);
	if (menusearch[0])
		V_DrawString(x - 18, y + 8, V_ALLOWLOWERCASE, menusearch+1);
	else
		V_DrawString(x - 18, y + 8, V_ALLOWLOWERCASE|V_TRANSLUCENT, "Type to search...");
	if (skullAnimCounter < 4)
		V_DrawCharacter(x - 18 + V_StringWidth(menusearch+1, 0), y + 8,
			'_' | 0x80, false);

	x -= (21 + 5 + 16);
	V_DrawSmallScaledPatch(x, y + 4, (menusearch[0] ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+3]);

	x = BASEVIDWIDTH - x - 16;
	V_DrawSmallScaledPatch(x, y + 4, ((!modifiedgame || savemoddata) ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+4]);

	if (modifiedgame)
		V_DrawSmallScaledPatch(x, y + 4, 0, addonsp[NUM_EXT+2]);
}

static void M_AddonExec(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	S_StartSound(NULL, sfx_zoom);
	COM_BufAddText(va("exec \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
}

#define len menusearch[0]
static boolean M_ChangeStringAddons(INT32 choice)
{
	if (shiftdown && choice >= 32 && choice <= 127)
		choice = shiftxform[choice];

	switch (choice)
	{
		case KEY_DEL:
			if (len)
			{
				len = menusearch[1] = 0;
				return true;
			}
			break;
		case KEY_BACKSPACE:
			if (len)
			{
				menusearch[1+--len] = 0;
				return true;
			}
			break;
		default:
			if (choice >= 32 && choice <= 127)
			{
				if (len < MAXSTRINGLENGTH - 1)
				{
					menusearch[1+len++] = (char)choice;
					menusearch[1+len] = 0;
					return true;
				}
			}
			break;
	}
	return false;
}
#undef len

static void M_HandleAddons(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	if (M_ChangeStringAddons(choice))
	{
		char *tempname = NULL;
		if (dirmenu && dirmenu[dir_on[menudepthleft]])
			tempname = Z_StrDup(dirmenu[dir_on[menudepthleft]]+DIR_STRING); // don't need to I_Error if can't make - not important, just QoL
#if 0 // much slower
		if (!preparefilemenu(true))
		{
			UNEXIST;
			return;
		}
#else // streamlined
		searchfilemenu(tempname);
#endif
	}

	switch (choice)
	{
		case KEY_DOWNARROW:
			if (dir_on[menudepthleft] < sizedirmenu-1)
				dir_on[menudepthleft]++;
			else if (dir_on[menudepthleft] == sizedirmenu-1)
				dir_on[menudepthleft] = 0;
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_UPARROW:
			if (dir_on[menudepthleft])
				dir_on[menudepthleft]--;
			else if (!dir_on[menudepthleft])
				dir_on[menudepthleft] = sizedirmenu-1;
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_PGDN:
			{
				UINT8 i;
				for (i = numaddonsshown; i && (dir_on[menudepthleft] < sizedirmenu-1); i--)
					dir_on[menudepthleft]++;
			}
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_PGUP:
			{
				UINT8 i;
				for (i = numaddonsshown; i && (dir_on[menudepthleft]); i--)
					dir_on[menudepthleft]--;
			}
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_ENTER:
			{
				boolean refresh = true;
				if (!dirmenu[dir_on[menudepthleft]])
					S_StartSound(NULL, sfx_lose);
				else
				{
					switch (dirmenu[dir_on[menudepthleft]][DIR_TYPE])
					{
						case EXT_FOLDER:
							strcpy(&menupath[menupathindex[menudepthleft]],dirmenu[dir_on[menudepthleft]]+DIR_STRING);
							if (menudepthleft)
							{
								menupathindex[--menudepthleft] = strlen(menupath);
								menupath[menupathindex[menudepthleft]] = 0;

								if (!preparefilemenu(false))
								{
									S_StartSound(NULL, sfx_skid);
									M_StartMessage(va("%c%s\x80\nThis folder is empty.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
									menupath[menupathindex[++menudepthleft]] = 0;

									if (!preparefilemenu(true))
									{
										UNEXIST;
										return;
									}
								}
								else
								{
									S_StartSound(NULL, sfx_menu1);
									dir_on[menudepthleft] = 1;
								}
								refresh = false;
							}
							else
							{
								S_StartSound(NULL, sfx_lose);
								M_StartMessage(va("%c%s\x80\nThis folder is too deep to navigate to!\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
								menupath[menupathindex[menudepthleft]] = 0;
							}
							break;
						case EXT_UP:
							S_StartSound(NULL, sfx_menu1);
							menupath[menupathindex[++menudepthleft]] = 0;
							if (!preparefilemenu(false))
							{
								UNEXIST;
								return;
							}
							break;
						case EXT_TXT:
							M_StartMessage(va("%c%s\x80\nThis file may not be a console script.\nAttempt to run anyways? \n\n(Press 'Y' to confirm)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),M_AddonExec,MM_YESNO);
							break;
						case EXT_CFG:
							M_AddonExec(KEY_ENTER);
							break;
						case EXT_LUA:
						case EXT_SOC:
						case EXT_WAD:
#ifdef USE_KART
						case EXT_KART:
#endif
						case EXT_PK3:
							COM_BufAddText(va("addfile \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
							break;
						default:
							S_StartSound(NULL, sfx_lose);
					}
				}
				if (refresh)
					refreshdirmenu |= REFRESHDIR_NORMAL;
			}
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		default:
			break;
	}
	if (exitmenu)
	{
		closefilemenu(true);

		// Secret menu!
		MainMenu[secrets].status = (M_AnySecretUnlocked()) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

static void M_PandorasBox(INT32 choice)
{
	(void)choice;
	CV_StealthSetValue(&cv_dummyrings, max(players[consoleplayer].health - 1, 0));
	CV_StealthSetValue(&cv_dummylives, players[consoleplayer].lives);
	CV_StealthSetValue(&cv_dummycontinues, players[consoleplayer].continues);
	M_SetupNextMenu(&SR_PandoraDef);
}

static boolean M_ExitPandorasBox(void)
{
	if (cv_dummyrings.value != max(players[consoleplayer].health - 1, 0))
		COM_ImmedExecute(va("setrings %d", cv_dummyrings.value));
	if (cv_dummylives.value != players[consoleplayer].lives)
		COM_ImmedExecute(va("setlives %d", cv_dummylives.value));
	if (cv_dummycontinues.value != players[consoleplayer].continues)
		COM_ImmedExecute(va("setcontinues %d", cv_dummycontinues.value));
	return true;
}

static void M_ChangeLevel(INT32 choice)
{
	char mapname[6];
	(void)choice;

	strlcpy(mapname, G_BuildMapName(cv_nextmap.value), sizeof (mapname));
	strlwr(mapname);
	mapname[5] = '\0';

	M_ClearMenus(true);
	COM_BufAddText(va("map %s -gametype \"%s\"\n", mapname, cv_newgametype.string));
}

static void M_ConfirmSpectate(INT32 choice)
{
	(void)choice;
	// We allow switching to spectator even if team changing is not allowed
	M_ClearMenus(true);
	COM_ImmedExecute("changeteam spectator");
}

static void M_ConfirmEnterGame(INT32 choice)
{
	(void)choice;
	if (!cv_allowteamchange.value)
	{
		M_StartMessage(M_GetText("The server is not allowing\nteam changes at this time.\nPress a key.\n"), NULL, MM_NOTHING);
		return;
	}
	M_ClearMenus(true);
	COM_ImmedExecute("changeteam playing");
}

static void M_ConfirmTeamScramble(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);

	switch (cv_dummyscramble.value)
	{
		case 0:
			COM_ImmedExecute("teamscramble 1");
			break;
		case 1:
			COM_ImmedExecute("teamscramble 2");
			break;
	}
}

static void M_ConfirmTeamChange(INT32 choice)
{
	(void)choice;
	if (!cv_allowteamchange.value && cv_dummyteam.value)
	{
		M_StartMessage(M_GetText("The server is not allowing\nteam changes at this time.\nPress a key.\n"), NULL, MM_NOTHING);
		return;
	}

	M_ClearMenus(true);

	switch (cv_dummyteam.value)
	{
		case 0:
			COM_ImmedExecute("changeteam spectator");
			break;
		case 1:
			COM_ImmedExecute("changeteam red");
			break;
		case 2:
			COM_ImmedExecute("changeteam blue");
			break;
	}
}

static void M_Options(INT32 choice)
{
	(void)choice;

	// if the player is not admin or server, disable server options
	OP_MainMenu[6].status = (Playing() && !(server || IsPlayerAdmin(consoleplayer))) ? (IT_GRAYEDOUT) : (IT_STRING|IT_SUBMENU);

	// if the player is playing _at all_, disable the erase data options
	OP_DataOptionsMenu[1].status = (Playing()) ? (IT_GRAYEDOUT) : (IT_STRING|IT_SUBMENU);

	OP_MainDef.prevMenu = currentMenu;
	M_SetupNextMenu(&OP_MainDef);
}

static void M_RetryResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	if (netgame || multiplayer) // Should never happen!
		return;

	M_ClearMenus(true);
	G_SetRetryFlag();
}

static void M_Retry(INT32 choice)
{
	(void)choice;
	M_StartMessage(M_GetText("Retry this act from the last starpost?\n\n(Press 'Y' to confirm)\n"),M_RetryResponse,MM_YESNO);
}

static void M_SelectableClearMenus(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);
}

// ======
// CHEATS
// ======

static void M_UltimateCheat(INT32 choice)
{
	(void)choice;
	if (Playing())
		LUAh_GameQuit();
	I_Quit();
}

static void M_GetAllEmeralds(INT32 choice)
{
	(void)choice;

	emeralds = ((EMERALD7)*2)-1;
	M_StartMessage(M_GetText("You now have all 7 emeralds.\nUse them wisely.\nWith great power comes great ring drain.\n"),NULL,MM_NOTHING);

	G_SetGameModified(multiplayer);
}

static void M_DestroyRobotsResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Destroy all robots
	P_DestroyRobots();

	G_SetGameModified(multiplayer);
}

static void M_DestroyRobots(INT32 choice)
{
	(void)choice;

	M_StartMessage(M_GetText("Do you want to destroy all\nrobots in the current level?\n\n(Press 'Y' to confirm)\n"),M_DestroyRobotsResponse,MM_YESNO);
}

static void M_LevelSelectWarp(INT32 choice)
{
	boolean fromloadgame = (currentMenu == &SP_LevelSelectDef);

	(void)choice;

	if (W_CheckNumForName(G_BuildMapName(cv_nextmap.value)) == LUMPERROR)
	{
//		CONS_Alert(CONS_WARNING, "Internal game map '%s' not found\n", G_BuildMapName(cv_nextmap.value));
		return;
	}

	startmap = (INT16)(cv_nextmap.value);

	fromlevelselect = true;

	if (fromloadgame)
		G_LoadGame((UINT32)cursaveslot, startmap);
	else
	{
		cursaveslot = -1;
		M_SetupChoosePlayer(0);
	}
}

// ========
// SKY ROOM
// ========

UINT8 skyRoomMenuTranslations[MAXUNLOCKABLES];

#define NUMCHECKLIST 8
static void M_DrawChecklist(void)
{
	INT32 i, j = 0;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		char *s;
		if (unlockables[i].name[0] == 0 || unlockables[i].nochecklist
		|| !unlockables[i].conditionset || unlockables[i].conditionset > MAXCONDITIONSETS)
			continue;

		s = V_WordWrap(160, 292, 0, unlockables[i].objective);
		V_DrawString(8, 8+(24*j), V_RETURN8, unlockables[i].name);
		V_DrawString(160, 8+(24*j), V_RETURN8, s);
		Z_Free(s);

		if (unlockables[i].unlocked)
			V_DrawString(308, 8+(24*j), V_YELLOWMAP, "Y");
		else
			V_DrawString(308, 8+(24*j), V_YELLOWMAP, "N");

		if (++j >= NUMCHECKLIST)
			break;
	}
}

#define NUMHINTS 5
static void M_EmblemHints(INT32 choice)
{
	(void)choice;
	SR_EmblemHintMenu[0].status = (M_SecretUnlocked(SECRET_ITEMFINDER)) ? (IT_CVAR|IT_STRING) : (IT_SECRET);
	M_SetupNextMenu(&SR_EmblemHintDef);
	itemOn = 1; // always start on back.
	M_UpdateItemOn();
}

static void M_DrawEmblemHints(void)
{
	INT32 i, j = 0;
	UINT32 collected = 0;
	emblem_t *emblem;
	char *hint;

	for (i = 0; i < numemblems; i++)
	{
		emblem = &emblemlocations[i];
		if (emblem->level != gamemap || emblem->type > ET_SKIN)
			continue;

		if (emblem->collected)
		{
			collected = V_GREENMAP;
			V_DrawMappedPatch(12, 12+(28*j), 0, W_CachePatchName(M_GetEmblemPatch(emblem), PU_PATCH),
				R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
		}
		else
		{
			collected = 0;
			V_DrawScaledPatch(12, 12+(28*j), 0, W_CachePatchName("NEEDIT", PU_PATCH));
		}

		if (emblem->hint[0])
			hint = emblem->hint;
		else
			hint = Z_StrDup(M_GetText("No hints available."));
		hint = V_WordWrap(40, BASEVIDWIDTH-12, 0, hint);
		V_DrawString(40, 8+(28*j), V_RETURN8|V_ALLOWLOWERCASE|collected, hint);
		Z_Free(hint);

		if (++j >= NUMHINTS)
			break;
	}
	if (!j)
		V_DrawCenteredString(160, 48, V_YELLOWMAP, "No hidden emblems on this map.");

	M_DrawGenericMenu();
}

static void M_DrawLevelSelectMenu(void)
{
	M_DrawGenericMenu();

	if (cv_nextmap.value)
	{
		lumpnum_t lumpnum;
		patch_t *PictureOfLevel;

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_PATCH);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_PATCH);

		V_DrawSmallScaledPatch(200, 110, 0, PictureOfLevel);
	}
}

static void M_DrawSkyRoom(void)
{
	INT32 i, y = 0;

	M_DrawGenericMenu();

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		if (currentMenu->menuitems[i].status == (IT_STRING|IT_KEYHANDLER))
		{
			y = currentMenu->menuitems[i].alphaKey;
			break;
		}
	}
	if (!y)
		return;

	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + y, V_YELLOWMAP, cv_soundtest.string);
	if (cv_soundtest.value)
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + y + 8, V_YELLOWMAP, S_sfx[cv_soundtest.value].name);
}

static void M_HandleSoundTest(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_BACKSPACE:
		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_RIGHTARROW:
			CV_AddValue(&cv_soundtest, 1);
			break;
		case KEY_LEFTARROW:
			CV_AddValue(&cv_soundtest, -1);
			break;
		case KEY_ENTER:
			S_StopSounds();
			S_StartSound(NULL, cv_soundtest.value);
			break;

		default:
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// Entering secrets menu
static void M_SecretsMenu(INT32 choice)
{
	INT32 i, j, ul;
	UINT8 done[MAXUNLOCKABLES];
	UINT16 curheight;

	(void)choice;

	// Clear all before starting
	for (i = 1; i < MAXUNLOCKABLES+1; ++i)
		SR_MainMenu[i].status = IT_DISABLED;

	memset(skyRoomMenuTranslations, 0, sizeof(skyRoomMenuTranslations));
	memset(done, 0, sizeof(done));

	for (i = 1; i < MAXUNLOCKABLES+1; ++i)
	{
		curheight = UINT16_MAX;
		ul = -1;

		// Autosort unlockables
		for (j = 0; j < MAXUNLOCKABLES; ++j)
		{
			if (!unlockables[j].height || done[j] || unlockables[j].type < 0)
				continue;

			if (unlockables[j].height < curheight)
			{
				curheight = unlockables[j].height;
				ul = j;
			}
		}
		if (ul < 0)
			break;

		done[ul] = true;

		skyRoomMenuTranslations[i-1] = (UINT8)ul;
		SR_MainMenu[i].text = unlockables[ul].name;
		SR_MainMenu[i].alphaKey = (UINT8)unlockables[ul].height;

		if (unlockables[ul].type == SECRET_HEADER)
		{
			SR_MainMenu[i].status = IT_HEADER;
			continue;
		}

		SR_MainMenu[i].status = IT_SECRET;

		if (unlockables[ul].unlocked)
		{
			switch (unlockables[ul].type)
			{
				case SECRET_LEVELSELECT:
					SR_MainMenu[i].status = IT_STRING|IT_CALL;
					SR_MainMenu[i].itemaction = M_CustomLevelSelect;
					break;
				case SECRET_WARP:
					SR_MainMenu[i].status = IT_STRING|IT_CALL;
					SR_MainMenu[i].itemaction = M_CustomWarp;
					break;
				case SECRET_CREDITS:
					SR_MainMenu[i].status = IT_STRING|IT_CALL;
					SR_MainMenu[i].itemaction = M_Credits;
					break;
				case SECRET_SOUNDTEST:
					SR_MainMenu[i].status = IT_STRING|IT_KEYHANDLER;
					SR_MainMenu[i].itemaction = M_HandleSoundTest;
				default:
					break;
			}
		}
	}

	M_SetupNextMenu(&SR_MainDef);
}

// ==================
// NEW GAME FUNCTIONS
// ==================

INT32 ultimate_selectable = false;

static void M_NewGame(void)
{
	fromlevelselect = false;

	startmap = spstage_start;
	CV_SetValue(&cv_newgametype, GT_COOP); // Graue 09-08-2004

	M_SetupChoosePlayer(0);
}

static void M_CustomWarp(INT32 choice)
{
	INT32 ul = skyRoomMenuTranslations[choice-1];

	startmap = (INT16)(unlockables[ul].variable);

	M_SetupChoosePlayer(0);
}

static void M_Credits(INT32 choice)
{
	(void)choice;
	cursaveslot = -2;
	M_ClearMenus(true);
	F_StartCredits();
}

static void M_CustomLevelSelect(INT32 choice)
{
	INT32 ul = skyRoomMenuTranslations[choice-1];

	SR_LevelSelectDef.prevMenu = currentMenu;
	levellistmode = LLM_LEVELSELECT;
	maplistoption = (UINT8)(unlockables[ul].variable);
	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_PrepareLevelSelect();
	M_SetupNextMenu(&SR_LevelSelectDef);
}

// ==================
// SINGLE PLAYER MENU
// ==================

static void M_SinglePlayerMenu(INT32 choice)
{
	(void)choice;
	SP_MainMenu[sprecordattack].status =
		(M_SecretUnlocked(SECRET_RECORDATTACK)) ? IT_CALL|IT_STRING : IT_SECRET;
	SP_MainMenu[spnightsmode].status =
		(M_SecretUnlocked(SECRET_NIGHTSMODE)) ? IT_CALL|IT_STRING : IT_SECRET;

	M_SetupNextMenu(&SP_MainDef);
}

static void M_LoadGameLevelSelect(INT32 choice)
{
	(void)choice;
	levellistmode = LLM_LEVELSELECT;
	maplistoption = 1;
	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	SP_LevelSelectDef.prevMenu = currentMenu;

	M_PrepareLevelSelect();
	M_SetupNextMenu(&SP_LevelSelectDef);
}

// ==============
// LOAD GAME MENU
// ==============

static INT32 saveSlotSelected = 0;
static INT32 menumovedir = 0;

static void M_DrawLoadGameData(void)
{
	INT32 ecks;
	INT32 i;

	ecks = SP_LoadDef.x + 24;
	M_DrawTextBox(SP_LoadDef.x-12,144, 24, 4);

	if (saveSlotSelected == NOSAVESLOT) // last slot is play without saving
	{
		if (ultimate_selectable)
		{
			V_DrawCenteredString(ecks + 68, 144, V_ORANGEMAP, "ULTIMATE MODE");
			V_DrawCenteredString(ecks + 68, 156, 0, "NO RINGS, NO ONE-UPS,");
			V_DrawCenteredString(ecks + 68, 164, 0, "NO CONTINUES, ONE LIFE,");
			V_DrawCenteredString(ecks + 68, 172, 0, "FINAL DESTINATION.");
		}
		else
		{
			V_DrawCenteredString(ecks + 68, 144, V_ORANGEMAP, "PLAY WITHOUT SAVING");
			V_DrawCenteredString(ecks + 68, 156, 0, "THIS GAME WILL NOT BE");
			V_DrawCenteredString(ecks + 68, 164, 0, "SAVED, BUT YOU CAN STILL");
			V_DrawCenteredString(ecks + 68, 172, 0, "GET EMBLEMS AND SECRETS.");
		}
		return;
	}

	if (savegameinfo[saveSlotSelected].lives == -42) // Empty
	{
		V_DrawCenteredString(ecks + 68, 160, 0, "NO DATA");
		return;
	}

	if (savegameinfo[saveSlotSelected].lives == -666) // savegame is bad
	{
		V_DrawCenteredString(ecks + 68, 144, V_REDMAP, "CORRUPT SAVE FILE");
		V_DrawCenteredString(ecks + 68, 156, 0, "THIS SAVE FILE");
		V_DrawCenteredString(ecks + 68, 164, 0, "CAN NOT BE LOADED.");
		V_DrawCenteredString(ecks + 68, 172, 0, "DELETE USING BACKSPACE.");
		return;
	}

	// Draw the back sprite, it looks ugly if we don't
	V_DrawScaledPatch(SP_LoadDef.x, 144+8, 0, livesback);
	if (savegameinfo[saveSlotSelected].skincolor == 0)
		V_DrawScaledPatch(SP_LoadDef.x,144+8,0,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].face, PU_PATCH));
	else
	{
		UINT8 *colormap = R_GetTranslationColormap(savegameinfo[saveSlotSelected].skinnum, savegameinfo[saveSlotSelected].skincolor, GTC_CACHE);
		V_DrawMappedPatch(SP_LoadDef.x,144+8,0,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].face, PU_PATCH), colormap);
	}

	V_DrawString(ecks + 12, 152, 0, savegameinfo[saveSlotSelected].playername);

#ifdef SAVEGAMES_OTHERVERSIONS
	if (savegameinfo[saveSlotSelected].gamemap & 16384)
		V_DrawCenteredString(ecks + 68, 144, V_REDMAP, "OUTDATED SAVE FILE!");
#endif

	if (savegameinfo[saveSlotSelected].gamemap & 8192)
		V_DrawString(ecks + 12, 160, V_GREENMAP, "CLEAR!");
	else
		V_DrawString(ecks + 12, 160, 0, va("%s", savegameinfo[saveSlotSelected].levelname));

	// Use the big face pic for lives, duh. :3
	V_DrawScaledPatch(ecks + 12, 175, 0, W_CachePatchName("STLIVEX", PU_HUDGFX));
	V_DrawTallNum(ecks + 40, 172, 0, savegameinfo[saveSlotSelected].lives);

	// Absolute ridiculousness, condensed into another function.
	V_DrawContinueIcon(ecks + 58, 182, 0, savegameinfo[saveSlotSelected].skinnum, savegameinfo[saveSlotSelected].skincolor);
	V_DrawScaledPatch(ecks + 68, 175, 0, W_CachePatchName("STLIVEX", PU_HUDGFX));
	V_DrawTallNum(ecks + 96, 172, 0, savegameinfo[saveSlotSelected].continues);

	for (i = 0; i < 7; ++i)
	{
		if (savegameinfo[saveSlotSelected].numemeralds & (1 << i))
			V_DrawScaledPatch(ecks + 104 + (i * 8), 172, 0, tinyemeraldpics[i]);
	}
}

#define LOADBARHEIGHT SP_LoadDef.y + (LINEHEIGHT * (j+1)) + ymod
#define CURSORHEIGHT  SP_LoadDef.y + (LINEHEIGHT*3) - 1

static void M_DrawLoad(void)
{
	INT32 i, j;
	INT32 ymod = 0, offset = 0;

	M_DrawMenuTitle();

	if (menumovedir != 0) //movement illusion
	{
		ymod = (-(LINEHEIGHT/4))*(menumovedir/(FRACUNIT-1));
		offset = ((menumovedir > 0) ? -1 : 1);
	}

	V_DrawCenteredString(BASEVIDWIDTH/2, 40, 0, "Press backspace to delete a save.");

	for (i = MAXSAVEGAMES + saveSlotSelected - 2 + offset, j = 0;i <= MAXSAVEGAMES + saveSlotSelected + 2 + offset; i++, j++)
	{
		if ((menumovedir < 0 && j == 4) || (menumovedir > 0 && j == 0))
			continue; //this helps give the illusion of movement

		M_DrawSaveLoadBorder(SP_LoadDef.x, LOADBARHEIGHT);

		if ((i%MAXSAVEGAMES) == NOSAVESLOT) // play without saving
		{
			if (ultimate_selectable)
				V_DrawCenteredString(SP_LoadDef.x+92, LOADBARHEIGHT - 1, V_ORANGEMAP, "ULTIMATE MODE");
			else
				V_DrawCenteredString(SP_LoadDef.x+92, LOADBARHEIGHT - 1, V_ORANGEMAP, "PLAY WITHOUT SAVING");
			continue;
		}

		if (savegameinfo[i%MAXSAVEGAMES].lives == -42)
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, V_TRANSLUCENT, "NO DATA");
		else if (savegameinfo[i%MAXSAVEGAMES].lives == -666)
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, V_REDMAP, "CORRUPT SAVE FILE");
		else if (savegameinfo[i%MAXSAVEGAMES].gamemap & 8192)
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, V_GREENMAP, "CLEAR!");
		else
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, 0, va("%s", savegameinfo[i%MAXSAVEGAMES].levelname));

		//Draw the save slot number on the right side
		V_DrawRightAlignedString(SP_LoadDef.x+192, LOADBARHEIGHT - 1, 0, va("%d",(i%MAXSAVEGAMES) + 1));
	}

	//Draw cursors on both sides.
	V_DrawScaledPatch( 32, CURSORHEIGHT, 0, W_CachePatchName("M_CURSOR", PU_PATCH));
	V_DrawScaledPatch(274, CURSORHEIGHT, 0, W_CachePatchName("M_CURSOR", PU_PATCH));

	M_DrawLoadGameData();

	//finishing the movement illusion
	if (menumovedir)
		menumovedir += FixedMul((menumovedir > 0) ? FRACUNIT : -FRACUNIT, renderdeltatics);
	if (abs(menumovedir) >= 4*FRACUNIT)
		menumovedir = 0;
}
#undef LOADBARHEIGHT
#undef CURSORHEIGHT

//
// User wants to load this game
//
static void M_LoadSelect(INT32 choice)
{
	(void)choice;

	if (saveSlotSelected == NOSAVESLOT) //last slot is play without saving
	{
		M_NewGame();
		cursaveslot = -1;
		return;
	}

	if (!FIL_ReadFileOK(va(savegamename, saveSlotSelected)))
	{
		// This slot is empty, so start a new game here.
		M_NewGame();
	}
	else if (savegameinfo[saveSlotSelected].gamemap & 8192) // Completed
		M_LoadGameLevelSelect(saveSlotSelected + 1);
	else
		G_LoadGame((UINT32)saveSlotSelected, 0);

	cursaveslot = saveSlotSelected;
}

#define VERSIONSIZE 16
#define BADSAVE { savegameinfo[slot].lives = -666; Z_Free(savebuffer); return; }
#define CHECKPOS if (save_p >= end_p) BADSAVE
// Reads the save file to list lives, level, player, etc.
// Tails 05-29-2003
static void M_ReadSavegameInfo(UINT32 slot)
{
	size_t length;
	char savename[255];
	UINT8 *savebuffer;
	UINT8 *end_p; // buffer end point, don't read past here
	UINT8 *save_p;
	INT32 fake; // Dummy variable
	char temp[sizeof(timeattackfolder)];
	char vcheck[VERSIONSIZE];
#ifdef SAVEGAMES_OTHERVERSIONS
	boolean oldversion = false;
#endif

	sprintf(savename, savegamename, slot);

	length = FIL_ReadFile(savename, &savebuffer);
	if (length == 0)
	{
		savegameinfo[slot].lives = -42;
		return;
	}

	end_p = savebuffer + length;

	// skip the description field
	save_p = savebuffer;

	// Version check
	memset(vcheck, 0, sizeof (vcheck));
	sprintf(vcheck, "version %d", VERSION);
	if (strcmp((const char *)save_p, (const char *)vcheck))
	{
#ifdef SAVEGAMES_OTHERVERSIONS
		oldversion = true;
#else
		BADSAVE // Incompatible versions?
#endif
	}
	save_p += VERSIONSIZE;

	// dearchive all the modifications
	// P_UnArchiveMisc()

	CHECKPOS
	fake = READINT16(save_p);

	if (((fake-1) & 8191) >= NUMMAPS) BADSAVE

	if(!mapheaderinfo[(fake-1) & 8191])
	{
		savegameinfo[slot].levelname[0] = '\0';
		savegameinfo[slot].actnum = 0;
	}
	else
	{
		strcpy(savegameinfo[slot].levelname, mapheaderinfo[(fake-1) & 8191]->lvlttl);
		savegameinfo[slot].actnum = mapheaderinfo[(fake-1) & 8191]->actnum;
	}

#ifdef SAVEGAMES_OTHERVERSIONS
	if (oldversion)
	{
		if (fake == 24) //meh, let's count old Clear! saves too
			fake |= 8192;
		fake |= 16384; // marker for outdated version
	}
#endif
	savegameinfo[slot].gamemap = fake;

	CHECKPOS
	fake = READUINT16(save_p)-357; // emeralds

	savegameinfo[slot].numemeralds = (UINT8)fake;

	CHECKPOS
	READSTRINGN(save_p, temp, sizeof(temp)); // mod it belongs to

	if (strcmp(temp, timeattackfolder)) BADSAVE

	// P_UnArchivePlayer()
	CHECKPOS
	// Skincolor is set by skin prefcolor.
	(void)READUINT8(save_p);
	CHECKPOS
	savegameinfo[slot].skinnum = READUINT8(save_p);
	savegameinfo[slot].skincolor = skins[savegameinfo[slot].skinnum].prefcolor;

	CHECKPOS
	(void)READINT32(save_p); // Score

	CHECKPOS
	savegameinfo[slot].lives = READINT32(save_p); // lives
	CHECKPOS
	savegameinfo[slot].continues = READINT32(save_p); // continues

	if (fake & (1<<10))
	{
		CHECKPOS
		savegameinfo[slot].botskin = READUINT8(save_p);
		if (savegameinfo[slot].botskin-1 >= numskins)
			savegameinfo[slot].botskin = 0;
		CHECKPOS
		(void)READUINT8(save_p);
		savegameinfo[slot].botcolor = skins[savegameinfo[slot].skinnum].prefcolor; // bot color
	}
	else
		savegameinfo[slot].botskin = 0;

	if (savegameinfo[slot].botskin)
		snprintf(savegameinfo[slot].playername, 36, "%s & %s",
			skins[savegameinfo[slot].skinnum].realname,
			skins[savegameinfo[slot].botskin-1].realname);
	else
		strcpy(savegameinfo[slot].playername, skins[savegameinfo[slot].skinnum].realname);

	savegameinfo[slot].playername[31] = 0;

	// File end marker check
	CHECKPOS
	if (READUINT8(save_p) != 0x1d) BADSAVE;

	// done
	Z_Free(savebuffer);
}
#undef CHECKPOS
#undef BADSAVE

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//  and put it in savegamestrings global variable
//
static void M_ReadSaveStrings(void)
{
	FILE *handle;
	UINT32 i;
	char name[256];

	for (i = 0; i < MAXSAVEGAMES; i++)
	{
		snprintf(name, sizeof name, savegamename, i);
		name[sizeof name - 1] = '\0';

		handle = fopen(name, "rb");
		if (handle == NULL)
		{
			savegameinfo[i].lives = -42;
			continue;
		}
		fclose(handle);
		M_ReadSavegameInfo(i);
	}
}

//
// User wants to delete this game
//
static void M_SaveGameDeleteResponse(INT32 ch)
{
	char name[256];

	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// delete savegame
	snprintf(name, sizeof name, savegamename, saveSlotSelected);
	name[sizeof name - 1] = '\0';
	remove(name);

	// Refresh savegame menu info
	M_ReadSaveStrings();
}

static void M_HandleLoadSave(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			++saveSlotSelected;
			if (saveSlotSelected >= MAXSAVEGAMES)
				saveSlotSelected -= MAXSAVEGAMES;
			menumovedir = FixedMul(FRACUNIT, renderdeltatics);
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			--saveSlotSelected;
			if (saveSlotSelected < 0)
				saveSlotSelected += MAXSAVEGAMES;
			menumovedir = -FixedMul(FRACUNIT, renderdeltatics);
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			if (savegameinfo[saveSlotSelected].lives != -666) // don't allow loading of "bad saves"
				M_LoadSelect(saveSlotSelected);
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			S_StartSound(NULL, sfx_menu1);
			// Don't allow people to 'delete' "Play without Saving."
			// Nor allow people to 'delete' slots with no saves in them.
			if (saveSlotSelected != NOSAVESLOT && savegameinfo[saveSlotSelected].lives != -42)
				M_StartMessage(M_GetText("Are you sure you want to delete\nthis save game?\n\n(Press 'Y' to confirm)\n"),M_SaveGameDeleteResponse,MM_YESNO);
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

//
// Selected from SRB2 menu
//
static void M_LoadGame(INT32 choice)
{
	(void)choice;

	M_ReadSaveStrings();
	M_SetupNextMenu(&SP_LoadDef);
}

//
// Used by cheats to force the save menu to a specific spot.
//
void M_ForceSaveSlotSelected(INT32 sslot)
{
	// Already there? Out of bounds? Whatever, then!
	if (sslot == saveSlotSelected || sslot >= MAXSAVEGAMES)
		return;

	// Figure out whether to display up movement or down movement
	menumovedir = (saveSlotSelected - sslot) > 0 ? -1 : 1;
	if (abs(saveSlotSelected - sslot) > (MAXSAVEGAMES>>1))
		menumovedir *= -1;

	saveSlotSelected = sslot;
}

// ================
// CHARACTER SELECT
// ================

static void M_SetupChoosePlayer(INT32 choice)
{
	(void)choice;

	if (mapheaderinfo[startmap-1] && mapheaderinfo[startmap-1]->forcecharacter[0] != '\0')
	{
		M_ChoosePlayer(0); //oh for crying out loud just get STARTED, it doesn't matter!
		return;
	}

	if (Playing() == false)
	{
		S_StopMusic();
		S_ChangeMusicInternal("chrsel", true);
	}

	SP_PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&SP_PlayerDef);
	char_scroll = itemOn*128*FRACUNIT; // finish scrolling the menu
	Z_Free(char_notes);
	char_notes = NULL;
}

// Draw the choose player setup menu, had some fun with player anim
static void M_DrawSetupChoosePlayerMenu(void)
{
	const INT32 my = 24;
	patch_t *patch;
	INT32 i, o, j;
	char *picname;

	// Black BG
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	//V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_PATCH));

	// Character select profile images!1
	M_DrawTextBox(0, my, 16, 20);

	if (abs(itemOn*128*FRACUNIT - char_scroll) > 256*FRACUNIT)
		char_scroll = itemOn*128*FRACUNIT;
	else if (itemOn*128*FRACUNIT - char_scroll > 128*FRACUNIT)
		char_scroll += FixedMul(48*FRACUNIT, renderdeltatics);
	else if (itemOn*128*FRACUNIT - char_scroll < -128*FRACUNIT)
		char_scroll -= FixedMul(48*FRACUNIT, renderdeltatics);
	else if (itemOn*128*FRACUNIT > char_scroll+FixedMul(16*FRACUNIT, renderdeltatics))
		char_scroll += FixedMul(16*FRACUNIT, renderdeltatics);
	else if (itemOn*128*FRACUNIT < char_scroll-FixedMul(16*FRACUNIT, renderdeltatics))
		char_scroll -= FixedMul(16*FRACUNIT, renderdeltatics);
	else // close enough.
		char_scroll = itemOn*128*FRACUNIT; // just be exact now.
	i = (char_scroll+16*FRACUNIT)/(128*FRACUNIT);
	o = ((char_scroll/FRACUNIT)+16)%128;

	// prev character
	if (i-1 >= 0 && PlayerMenu[i-1].status != IT_DISABLED
	&& o < 32)
	{
		picname = description[i-1].picname;
		if (picname[0] == '\0')
		{
			picname = strtok(Z_StrDup(description[i-1].skinname), "&");
			for (j = 0; j < numskins; j++)
				if (stricmp(skins[j].name, picname) == 0)
				{
					Z_Free(picname);
					picname = skins[j].charsel;
					break;
				}
			if (j == numskins) // AAAAAAAAAA
				picname = skins[0].charsel;
		}
		patch = W_CachePatchName(picname, PU_PATCH);
		if (SHORT(patch->width) >= 256)
			V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT/2, 0, patch, 0, SHORT(patch->height) - 64 + o*2, SHORT(patch->width), SHORT(patch->height));
		else
			V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT, 0, patch, 0, SHORT(patch->height) - 32 + o, SHORT(patch->width), SHORT(patch->height));
		W_UnlockCachedPatch(patch);
	}

	// next character
	if (i+1 < currentMenu->numitems && PlayerMenu[i+1].status != IT_DISABLED
	&& o < 128)
	{
		picname = description[i+1].picname;
		if (picname[0] == '\0')
		{
			picname = strtok(Z_StrDup(description[i+1].skinname), "&");
			for (j = 0; j < numskins; j++)
				if (stricmp(skins[j].name, picname) == 0)
				{
					Z_Free(picname);
					picname = skins[j].charsel;
					break;
				}
			if (j == numskins) // AAAAAAAAAA
				picname = skins[0].charsel;
		}
		patch = W_CachePatchName(picname, PU_PATCH);
		if (SHORT(patch->width) >= 256)
			V_DrawCroppedPatch(8<<FRACBITS, (my + 168 - o)<<FRACBITS, FRACUNIT/2, 0, patch, 0, 0, SHORT(patch->width), o*2);
		else
			V_DrawCroppedPatch(8<<FRACBITS, (my + 168 - o)<<FRACBITS, FRACUNIT, 0, patch, 0, 0, SHORT(patch->width), o);
		W_UnlockCachedPatch(patch);
	}

	// current character
	if (i < currentMenu->numitems && PlayerMenu[i].status != IT_DISABLED)
	{
		picname = description[i].picname;
		if (picname[0] == '\0')
		{
			picname = strtok(Z_StrDup(description[i].skinname), "&");
			for (j = 0; j < numskins; j++)
				if (stricmp(skins[j].name, picname) == 0)
				{
					Z_Free(picname);
					picname = skins[j].charsel;
					break;
				}
			if (j == numskins) // AAAAAAAAAA
				picname = skins[0].charsel;
		}
		patch = W_CachePatchName(picname, PU_PATCH);
		if (o >= 0 && o <= 32)
		{
			if (SHORT(patch->width) >= 256)
				V_DrawSmallScaledPatch(8, my + 40 - o, 0, patch);
			else
				V_DrawScaledPatch(8, my + 40 - o, 0, patch);
		}
		else
		{
			if (SHORT(patch->width) >= 256)
				V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT/2, 0, patch, 0, (o - 32)*2, SHORT(patch->width), SHORT(patch->height));
			else
				V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT, 0, patch, 0, o - 32, SHORT(patch->width), SHORT(patch->height));
		}
		W_UnlockCachedPatch(patch);
	}

	// draw title (or big pic)
	M_DrawMenuTitle();

	// Character description
	M_DrawTextBox(136, my, 21, 20);
	if (!char_notes)
		char_notes = V_WordWrap(0, 21*8, V_ALLOWLOWERCASE, description[itemOn].notes);
	V_DrawString(146, my + 9, V_RETURN8|V_ALLOWLOWERCASE, char_notes);
}

// Chose the player you want to use Tails 03-02-2002
static void M_ChoosePlayer(INT32 choice)
{
	char *skin1,*skin2;
	INT32 skinnum;
	boolean ultmode = (ultimate_selectable && SP_PlayerDef.prevMenu == &SP_LoadDef && saveSlotSelected == NOSAVESLOT);

	// skip this if forcecharacter
	if (mapheaderinfo[startmap-1] && mapheaderinfo[startmap-1]->forcecharacter[0] == '\0')
	{
		// M_SetupChoosePlayer didn't call us directly, that means we've been properly set up.
		char_scroll = itemOn*128*FRACUNIT; // finish scrolling the menu
		M_DrawSetupChoosePlayerMenu(); // draw the finally selected character one last time for the fadeout
	}
	M_ClearMenus(true);

	skin1 = strtok(description[choice].skinname, "&");
	skin2 = strtok(NULL, "&");

	if (skin2) {
		// this character has a second skin
		skinnum = R_SkinAvailable(skin1);
		botskin = (UINT8)(R_SkinAvailable(skin2)+1);
		botingame = true;

		botcolor = skins[botskin-1].prefcolor;

		// undo the strtok
		description[choice].skinname[strlen(skin1)] = '&';
	} else {
		skinnum = R_SkinAvailable(description[choice].skinname);
		botingame = false;
		botskin = 0;
		botcolor = 0;
	}

	if (startmap != spstage_start)
		cursaveslot = -1;

	lastmapsaved = 0;
	gamecomplete = false;

	G_DeferedInitNew(ultmode, G_BuildMapName(startmap), (UINT8)skinnum, false, fromlevelselect);
	COM_BufAddText("dummyconsvar 1\n"); // G_DeferedInitNew doesn't do this
}

// ===============
// STATISTICS MENU
// ===============

static INT32 statsLocation;
static INT32 statsMax;
static INT16 statsMapList[NUMMAPS+1];

static void M_Statistics(INT32 choice)
{
	INT16 i, j = 0;

	(void)choice;

	memset(statsMapList, 0, sizeof(statsMapList));

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!mapheaderinfo[i] || mapheaderinfo[i]->lvlttl[0] == '\0')
			continue;

		if (!(mapheaderinfo[i]->typeoflevel & TOL_SP) || (mapheaderinfo[i]->menuflags & LF2_HIDEINSTATS))
			continue;

		if (!mapvisited[i])
			continue;

		statsMapList[j++] = i;
	}
	statsMapList[j] = -1;
	statsMax = j - 11 + numextraemblems;
	statsLocation = 0;

	if (statsMax < 0)
		statsMax = 0;

	M_SetupNextMenu(&SP_LevelStatsDef);
}

static void M_DrawStatsMaps(int location)
{
	INT32 y = 76, i = -1;
	INT16 mnum;
	extraemblem_t *exemblem;
	boolean dotopname = true, dobottomarrow = (location < statsMax);


	while (statsMapList[++i] != -1)
	{
		if (location)
		{
			--location;
			continue;
		}
		else if (dotopname)
		{
			V_DrawString(20,  y, V_GREENMAP, "LEVEL NAME");
			V_DrawString(248, y, V_GREENMAP, "EMBLEMS");
			y += 8;
			dotopname = false;
		}

		mnum = statsMapList[i];
		M_DrawMapEmblems(mnum+1, 292, y);

		if (mapheaderinfo[mnum]->actnum != 0)
			V_DrawString(20, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[mnum]->lvlttl, mapheaderinfo[mnum]->actnum));
		else
			V_DrawString(20, y, V_YELLOWMAP, mapheaderinfo[mnum]->lvlttl);

		y += 8;

		if (y >= BASEVIDHEIGHT-8)
			goto bottomarrow;
	}
	if (dotopname && !location)
	{
		V_DrawString(20,  y, V_GREENMAP, "LEVEL NAME");
		V_DrawString(248, y, V_GREENMAP, "EMBLEMS");
		y += 8;
	}
	else if (location)
		--location;

	// Extra Emblems
	for (i = -2; i < numextraemblems; ++i)
	{
		if (i == -1)
		{
			V_DrawString(20, y, V_GREENMAP, "EXTRA EMBLEMS");
			if (location)
			{
				y += 8;
				location++;
			}
		}
		if (location)
		{
			--location;
			continue;
		}

		if (i >= 0)
		{
			exemblem = &extraemblems[i];

			if (exemblem->collected)
				V_DrawSmallMappedPatch(292, y, 0, W_CachePatchName(M_GetExtraEmblemPatch(exemblem), PU_PATCH),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetExtraEmblemColor(exemblem), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(292, y, 0, W_CachePatchName("NEEDIT", PU_PATCH));

			V_DrawString(20, y, V_YELLOWMAP, va("%s", exemblem->description));
		}

		y += 8;

		if (y >= BASEVIDHEIGHT-8)
			goto bottomarrow;
	}
bottomarrow:
	if (dobottomarrow)
		V_DrawString(10, y-8 + (skullAnimCounter/5), V_YELLOWMAP, "\x1B");
}

static void M_DrawLevelStats(void)
{
	char beststr[40];

	tic_t besttime = 0;
	UINT32 bestscore = 0;
	UINT32 bestrings = 0;

	INT32 i;
	INT32 mapsunfinished = 0;
	boolean bestunfinished[3] = {false, false, false};

	M_DrawMenuTitle();

	V_DrawString(20, 24, V_YELLOWMAP, "Total Play Time:");
	V_DrawCenteredString(BASEVIDWIDTH/2, 32, 0, va("%i hours, %i minutes, %i seconds",
	                         G_TicsToHours(totalplaytime),
	                         G_TicsToMinutes(totalplaytime, false),
	                         G_TicsToSeconds(totalplaytime)));

	for (i = 0; i < NUMMAPS; i++)
	{
		boolean mapunfinished = false;

		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;

		if (!mainrecords[i])
		{
			mapsunfinished++;
			bestunfinished[0] = bestunfinished[1] = bestunfinished[2] = true;
			continue;
		}

		if (mainrecords[i]->score > 0)
			bestscore += mainrecords[i]->score;
		else
			mapunfinished = bestunfinished[0] = true;

		if (mainrecords[i]->time > 0)
			besttime += mainrecords[i]->time;
		else
			mapunfinished = bestunfinished[1] = true;

		if (mainrecords[i]->rings > 0)
			bestrings += mainrecords[i]->rings;
		else
			mapunfinished = bestunfinished[2] = true;

		if (mapunfinished)
			mapsunfinished++;
	}

	V_DrawString(20, 48, 0, "Combined records:");

	if (mapsunfinished)
		V_DrawString(20, 56, V_REDMAP, va("(%d unfinished)", mapsunfinished));
	else
		V_DrawString(20, 56, V_GREENMAP, "(complete)");

	V_DrawString(36, 64, 0, va("x %d/%d", M_CountEmblems(), numemblems+numextraemblems));
	V_DrawSmallScaledPatch(20, 64, 0, W_CachePatchName("EMBLICON", PU_STATIC));

	sprintf(beststr, "%u", bestscore);
	V_DrawString(BASEVIDWIDTH/2, 48, V_YELLOWMAP, "SCORE:");
	V_DrawRightAlignedString(BASEVIDWIDTH-16, 48, (bestunfinished[0] ? V_REDMAP : 0), beststr);

	sprintf(beststr, "%i:%02i:%02i.%02i", G_TicsToHours(besttime), G_TicsToMinutes(besttime, false), G_TicsToSeconds(besttime), G_TicsToCentiseconds(besttime));
	V_DrawString(BASEVIDWIDTH/2, 56, V_YELLOWMAP, "TIME:");
	V_DrawRightAlignedString(BASEVIDWIDTH-16, 56, (bestunfinished[1] ? V_REDMAP : 0), beststr);

	sprintf(beststr, "%u", bestrings);
	V_DrawString(BASEVIDWIDTH/2, 64, V_YELLOWMAP, "RINGS:");
	V_DrawRightAlignedString(BASEVIDWIDTH-16, 64, (bestunfinished[2] ? V_REDMAP : 0), beststr);

	M_DrawStatsMaps(statsLocation);
}

// Handle statistics.
static void M_HandleLevelStats(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			if (statsLocation < statsMax)
				++statsLocation;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			if (statsLocation)
				--statsLocation;
			break;

		case KEY_PGDN:
			S_StartSound(NULL, sfx_menu1);
			statsLocation += (statsLocation+13 >= statsMax) ? statsMax-statsLocation : 13;
			break;

		case KEY_PGUP:
			S_StartSound(NULL, sfx_menu1);
			statsLocation -= (statsLocation < 13) ? statsLocation : 13;
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// ===========
// MODE ATTACK
// ===========

// Drawing function for Time Attack
void M_DrawTimeAttackMenu(void)
{
	INT32 i, x, y, cursory = 0;
	UINT16 dispstatus;
	patch_t *PictureOfLevel, *PictureOfUrFace;
	lumpnum_t lumpnum;
	char beststr[40];

	S_ChangeMusicInternal("racent", true); // Eww, but needed for when user hits escape during demo playback

	V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_PATCH));

	M_DrawMenuTitle();

	// draw menu (everything else goes on top of it)
	// Sadly we can't just use generic mode menus because we need some extra hacks
	x = currentMenu->x;
	y = currentMenu->y;

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		dispstatus = (currentMenu->menuitems[i].status & IT_DISPLAY);
		if (dispstatus != IT_STRING && dispstatus != IT_WHITESTRING)
			continue;

		y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
		if (i == itemOn)
			cursory = y;

		V_DrawString(x, y, (dispstatus == IT_WHITESTRING) ? V_YELLOWMAP : 0 , currentMenu->menuitems[i].text);

		// Cvar specific handling
		if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_CVAR)
		{
			consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
			INT32 soffset = 0;

			// hack to keep the menu from overlapping the player icon
			if (currentMenu != &SP_TimeAttackDef)
				soffset = 80;

			// Should see nothing but strings
			V_DrawString(BASEVIDWIDTH - x - soffset - V_StringWidth(cv->string, 0), y, V_YELLOWMAP, cv->string);
			if (i == itemOn)
			{
				V_DrawCharacter(BASEVIDWIDTH - x - soffset - 10 - V_StringWidth(cv->string, 0) - (skullAnimCounter/5), y,
					'\x1C' | V_YELLOWMAP, false);
				V_DrawCharacter(BASEVIDWIDTH - x - soffset + 2 + (skullAnimCounter/5), y,
					'\x1D' | V_YELLOWMAP, false);
			}
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_PATCH));
	V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_PATCH);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_PATCH);

	V_DrawSmallScaledPatch(208, 32, 0, PictureOfLevel);

	// Character face!
	if (W_CheckNumForName(skins[cv_chooseskin.value-1].charsel) != LUMPERROR)
	{
		PictureOfUrFace = W_CachePatchName(skins[cv_chooseskin.value-1].charsel, PU_PATCH);
		if (PictureOfUrFace->width >= 256)
			V_DrawTinyScaledPatch(224, 120, 0, PictureOfUrFace);
		else
			V_DrawSmallScaledPatch(224, 120, 0, PictureOfUrFace);
	}

	// Level record list
	if (cv_nextmap.value)
	{
		emblem_t *em;
		INT32 yHeight;

		V_DrawCenteredString(104, 32, 0, "* LEVEL RECORDS *");

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->score)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%u", mainrecords[cv_nextmap.value-1]->score);

		V_DrawString(104-72, 48, V_YELLOWMAP, "SCORE:");
		V_DrawRightAlignedString(104+72, 48, V_ALLOWLOWERCASE, beststr);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->time)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%i:%02i.%02i", G_TicsToMinutes(mainrecords[cv_nextmap.value-1]->time, true),
			                                 G_TicsToSeconds(mainrecords[cv_nextmap.value-1]->time),
			                                 G_TicsToCentiseconds(mainrecords[cv_nextmap.value-1]->time));

		V_DrawString(104-72, 58, V_YELLOWMAP, "TIME:");
		V_DrawRightAlignedString(104+72, 58, V_ALLOWLOWERCASE, beststr);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->rings)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%hu", mainrecords[cv_nextmap.value-1]->rings);

		V_DrawString(104-72, 68, V_YELLOWMAP, "RINGS:");
		V_DrawRightAlignedString(104+72, 68, V_ALLOWLOWERCASE, beststr);

		// Draw record emblems.
		em = M_GetLevelEmblems(cv_nextmap.value);
		while (em)
		{
			switch (em->type)
			{
				case ET_SCORE: yHeight = 48; break;
				case ET_TIME:  yHeight = 58; break;
				case ET_RINGS: yHeight = 68; break;
				default:
					goto skipThisOne;
			}

			if (em->collected)
				V_DrawSmallMappedPatch(104+76, yHeight, 0, W_CachePatchName(M_GetEmblemPatch(em), PU_PATCH),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(104+76, yHeight, 0, W_CachePatchName("NEEDIT", PU_PATCH));

			skipThisOne:
			em = M_GetLevelEmblems(-1);
		}
	}

	// ALWAYS DRAW level name and skin even when not on this menu!
	if (currentMenu != &SP_TimeAttackDef)
	{
		consvar_t *ncv;

		x = SP_TimeAttackDef.x;
		y = SP_TimeAttackDef.y;

		for (i = 0; i < 2; ++i)
		{
			ncv = (consvar_t *)SP_TimeAttackMenu[i].itemaction;

			V_DrawString(x, y + SP_TimeAttackMenu[i].alphaKey, V_TRANSLUCENT, SP_TimeAttackMenu[i].text);
			V_DrawString(BASEVIDWIDTH - x - V_StringWidth(ncv->string, 0),
			             y + SP_TimeAttackMenu[i].alphaKey, V_YELLOWMAP|V_TRANSLUCENT, ncv->string);
		}
	}
}

// Going to Time Attack menu...
static void M_TimeAttack(INT32 choice)
{
	(void)choice;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	levellistmode = LLM_RECORDATTACK; // Don't be dependent on cv_newgametype

	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No record-attackable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_PatchSkinNameTable();

	M_PrepareLevelSelect();
	M_SetupNextMenu(&SP_TimeAttackDef);
	Nextmap_OnChange();

	itemOn = tastart; // "Start" is selected.
	M_UpdateItemOn();

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusicInternal("racent", true);
}

// Drawing function for Nights Attack
void M_DrawNightsAttackMenu(void)
{
	patch_t *PictureOfLevel;
	lumpnum_t lumpnum;
	char beststr[40];

	S_ChangeMusicInternal("racent", true); // Eww, but needed for when user hits escape during demo playback

	V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_PATCH));

	// draw menu (everything else goes on top of it)
	M_DrawGenericMenu();

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_PATCH);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_PATCH);

	V_DrawSmallScaledPatch(90, 28, 0, PictureOfLevel);

	// Level record list
	if (cv_nextmap.value)
	{
		emblem_t *em;
		INT32 yHeight;

		UINT8 bestoverall	= G_GetBestNightsGrade(cv_nextmap.value, 0);
		UINT8 bestgrade		= G_GetBestNightsGrade(cv_nextmap.value, cv_dummymares.value);
		UINT32 bestscore	= G_GetBestNightsScore(cv_nextmap.value, cv_dummymares.value);
		tic_t besttime		= G_GetBestNightsTime(cv_nextmap.value, cv_dummymares.value);

		if (P_HasGrades(cv_nextmap.value, 0))
			V_DrawScaledPatch(200, 28 + 8, 0, ngradeletters[bestoverall]);

		if (currentMenu == &SP_NightsAttackDef)
		{
			if (P_HasGrades(cv_nextmap.value, cv_dummymares.value))
			{
				V_DrawString(160-88, 112, V_YELLOWMAP, "BEST GRADE:");
				V_DrawSmallScaledPatch(160 + 86 - (ngradeletters[bestgrade]->width/2),
					112 + 8 - (ngradeletters[bestgrade]->height/2),
					0, ngradeletters[bestgrade]);
			}

			if (!bestscore)
				sprintf(beststr, "(none)");
			else
				sprintf(beststr, "%u", bestscore);

			V_DrawString(160 - 88, 122, V_YELLOWMAP, "BEST SCORE:");
			V_DrawRightAlignedString(160 + 88, 122, V_ALLOWLOWERCASE, beststr);

			if (besttime == UINT32_MAX)
				sprintf(beststr, "(none)");
			else
				sprintf(beststr, "%i:%02i.%02i", G_TicsToMinutes(besttime, true),
																				 G_TicsToSeconds(besttime),
																				 G_TicsToCentiseconds(besttime));

			V_DrawString(160-88, 132, V_YELLOWMAP, "BEST TIME:");
			V_DrawRightAlignedString(160+88, 132, V_ALLOWLOWERCASE, beststr);

			if (cv_dummymares.value == 0) {
				// Draw record emblems.
				em = M_GetLevelEmblems(cv_nextmap.value);
				while (em)
				{
					switch (em->type)
					{
						case ET_NGRADE: yHeight = 112; break;
						case ET_NTIME:  yHeight = 132; break;
						default:
							goto skipThisOne;
					}

					if (em->collected)
						V_DrawSmallMappedPatch(160+88, yHeight, 0, W_CachePatchName(M_GetEmblemPatch(em), PU_PATCH),
																	 R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
					else
						V_DrawSmallScaledPatch(160+88, yHeight, 0, W_CachePatchName("NEEDIT", PU_PATCH));

					skipThisOne:
					em = M_GetLevelEmblems(-1);
				}
			}
		}
		// ALWAYS DRAW level name even when not on this menu!
		else
		{
			consvar_t *ncv;
			INT32 x = SP_NightsAttackDef.x;
			INT32 y = SP_NightsAttackDef.y;

			ncv = (consvar_t *)SP_NightsAttackMenu[0].itemaction;
			V_DrawString(x, y + SP_NightsAttackMenu[0].alphaKey, V_TRANSLUCENT, SP_NightsAttackMenu[0].text);
			V_DrawString(BASEVIDWIDTH - x - V_StringWidth(ncv->string, 0),
									 y + SP_NightsAttackMenu[0].alphaKey, V_YELLOWMAP|V_TRANSLUCENT, ncv->string);
		}
	}
}

// Going to Nights Attack menu...
static void M_NightsAttack(INT32 choice)
{
	(void)choice;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	levellistmode = LLM_NIGHTSATTACK; // Don't be dependent on cv_newgametype

	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No NiGHTS-attackable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	// This is really just to make sure Sonic is the played character, just in case
	M_PatchSkinNameTable();

	M_PrepareLevelSelect();
	M_SetupNextMenu(&SP_NightsAttackDef);
	Nextmap_OnChange();

	itemOn = nastart; // "Start" is selected.
	M_UpdateItemOn();

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusicInternal("racent", true);
}

// Player has selected the "START" from the nights attack screen
static void M_ChooseNightsAttack(INT32 choice)
{
	char nameofdemo[256];
	(void)choice;
	emeralds = 0;
	M_ClearMenus(true);
	modeattacking = ATTACKING_NIGHTS;

	I_mkdir(va("%s"PATHSEP"replay", srb2home), 0755);
	I_mkdir(va("%s"PATHSEP"replay"PATHSEP"%s", srb2home, timeattackfolder), 0755);

	snprintf(nameofdemo, sizeof nameofdemo, "replay"PATHSEP"%s"PATHSEP"%s-last", timeattackfolder, G_BuildMapName(cv_nextmap.value));

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	G_DeferedInitNew(false, G_BuildMapName(cv_nextmap.value), 0, false, false);
}

// Player has selected the "START" from the time attack screen
static void M_ChooseTimeAttack(INT32 choice)
{
	char *gpath;
	const size_t glen = strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char nameofdemo[256];
	(void)choice;
	emeralds = 0;
	M_ClearMenus(true);
	modeattacking = ATTACKING_RECORD;

	I_mkdir(va("%s"PATHSEP"replay", srb2home), 0755);
	I_mkdir(va("%s"PATHSEP"replay"PATHSEP"%s", srb2home, timeattackfolder), 0755);

	if ((gpath = malloc(glen)) == NULL)
		I_Error("Out of memory for replay filepath\n");

	sprintf(gpath,"replay"PATHSEP"%s"PATHSEP"%s", timeattackfolder, G_BuildMapName(cv_nextmap.value));
	snprintf(nameofdemo, sizeof nameofdemo, "%s-%s-last", gpath, cv_chooseskin.string);

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	G_DeferedInitNew(false, G_BuildMapName(cv_nextmap.value), (UINT8)(cv_chooseskin.value-1), false, false);
}

// Player has selected the "REPLAY" from the time attack screen
static void M_ReplayTimeAttack(INT32 choice)
{
	const char *which;
	M_ClearMenus(true);
	modeattacking = ATTACKING_RECORD; // set modeattacking before G_DoPlayDemo so the map loader knows

	if (currentMenu == &SP_ReplayDef)
	{
		switch(choice) {
		default:
		case 0: // best score
			which = "score-best";
			break;
		case 1: // best time
			which = "time-best";
			break;
		case 2: // best rings
			which = "rings-best";
			break;
		case 3: // last
			which = "last";
			break;
		case 4: // guest
			// srb2/replay/main/map01-guest.lmp
			G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)));
			return;
		}
		// srb2/replay/main/map01-sonic-time-best.lmp
		G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.string, which));
	}
	else if (currentMenu == &SP_NightsReplayDef)
	{
		switch(choice) {
		default:
		case 0: // best score
			which = "score-best";
			break;
		case 1: // best time
			which = "time-best";
			break;
		case 2: // last
			which = "last";
			break;
		case 3: // guest
			which = "guest";
			break;
		}
		// srb2/replay/main/map01-score-best.lmp
		G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), which));
	}
}

static void M_EraseGuest(INT32 choice)
{
	const char *rguest = va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));
	(void)choice;
	if (FIL_FileExists(rguest))
		remove(rguest);
	if (currentMenu == &SP_NightsGuestReplayDef)
		M_SetupNextMenu(&SP_NightsAttackDef);
	else
		M_SetupNextMenu(&SP_TimeAttackDef);
	CV_AddValue(&cv_nextmap, -1);
	CV_AddValue(&cv_nextmap, 1);
	M_StartMessage(M_GetText("Guest replay data erased.\n"),NULL,MM_NOTHING);
}

static void M_OverwriteGuest(const char *which, boolean nights)
{
	char *rguest = Z_StrDup(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)));
	UINT8 *buf;
	size_t len;
	if (!nights)
		len = FIL_ReadFile(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.string, which), &buf);
	else
		len = FIL_ReadFile(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), which), &buf);
	if (!len) {
		return;
	}
	if (FIL_FileExists(rguest)) {
		M_StopMessage(0);
		remove(rguest);
	}
	FIL_WriteFile(rguest, buf, len);
	Z_Free(rguest);
	if (currentMenu == &SP_NightsGuestReplayDef)
		M_SetupNextMenu(&SP_NightsAttackDef);
	else
		M_SetupNextMenu(&SP_TimeAttackDef);
	CV_AddValue(&cv_nextmap, -1);
	CV_AddValue(&cv_nextmap, 1);
	M_StartMessage(M_GetText("Guest replay data saved.\n"),NULL,MM_NOTHING);
}

static void M_OverwriteGuest_Time(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("time-best", currentMenu == &SP_NightsGuestReplayDef);
}

static void M_OverwriteGuest_Score(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("score-best", currentMenu == &SP_NightsGuestReplayDef);
}

static void M_OverwriteGuest_Rings(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("rings-best", false);
}

static void M_OverwriteGuest_Last(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("last", currentMenu == &SP_NightsGuestReplayDef);
}

static void M_SetGuestReplay(INT32 choice)
{
	void (*which)(INT32);
	if (currentMenu == &SP_NightsGuestReplayDef && choice >= 2)
		choice++; // skip best rings
	switch(choice)
	{
	case 0: // best score
		which = M_OverwriteGuest_Score;
		break;
	case 1: // best time
		which = M_OverwriteGuest_Time;
		break;
	case 2: // best rings
		which = M_OverwriteGuest_Rings;
		break;
	case 3: // last
		which = M_OverwriteGuest_Last;
		break;
	case 4: // guest
	default:
		M_StartMessage(M_GetText("Are you sure you want to\ndelete the guest replay data?\n\n(Press 'Y' to confirm)\n"),M_EraseGuest,MM_YESNO);
		return;
	}
	if (FIL_FileExists(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value))))
		M_StartMessage(M_GetText("Are you sure you want to\noverwrite the guest replay data?\n\n(Press 'Y' to confirm)\n"),which,MM_YESNO);
	else
		which(0);
}

static void M_ModeAttackRetry(INT32 choice)
{
	(void)choice;
	G_CheckDemoStatus(); // Cancel recording
	if (modeattacking == ATTACKING_RECORD)
		M_ChooseTimeAttack(0);
	else if (modeattacking == ATTACKING_NIGHTS)
		M_ChooseNightsAttack(0);
}

static void M_ModeAttackEndGame(INT32 choice)
{
	(void)choice;
	G_CheckDemoStatus(); // Cancel recording

	if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
		Command_ExitGame_f();

	M_StartControlPanel();
	switch(modeattacking)
	{
	default:
	case ATTACKING_RECORD:
		currentMenu = &SP_TimeAttackDef;
		break;
	case ATTACKING_NIGHTS:
		currentMenu = &SP_NightsAttackDef;
		break;
	}
	itemOn = currentMenu->lastOn;
	M_UpdateItemOn();
	G_SetGamestate(GS_TIMEATTACK);
	modeattacking = ATTACKING_NONE;
	S_ChangeMusicInternal("racent", true);
	// Update replay availability.
	CV_AddValue(&cv_nextmap, 1);
	CV_AddValue(&cv_nextmap, -1);
}

// ========
// END GAME
// ========

static void M_ExitGameResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	//Command_ExitGame_f();
	G_SetExitGameFlag();
	M_ClearMenus(true);
}

static void M_EndGame(INT32 choice)
{
	(void)choice;
	if (demoplayback || demorecording)
		return;

	if (!Playing())
		return;

	M_StartMessage(M_GetText("Are you sure you want to end the game?\n\n(Press 'Y' to confirm)\n"), M_ExitGameResponse, MM_YESNO);
}

//===========================================================================
// Connect Menu
//===========================================================================

#define SERVERHEADERHEIGHT 44
#define SERVERLINEHEIGHT 12

#define S_LINEY(n) currentMenu->y + SERVERHEADERHEIGHT + (n * SERVERLINEHEIGHT)

#ifndef NONET
static UINT32 localservercount;

static void M_HandleServerPage(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_BACKSPACE:
		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_ENTER:
		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_menu1);
			if ((serverlistpage + 1) * SERVERS_PER_PAGE < serverlistcount)
				serverlistpage++;
			break;
		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_menu1);
			if (serverlistpage > 0)
				serverlistpage--;
			break;

		default:
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

static void M_Connect(INT32 choice)
{
	// do not call menuexitfunc
	M_ClearMenus(false);

	COM_BufAddText(va("connect node %d\n", serverlist[choice-FIRSTSERVERLINE + serverlistpage * SERVERS_PER_PAGE].node));
}

static void M_Refresh(INT32 choice)
{
	(void)choice;

	// Display a little "please wait" message.
	M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Searching for servers...");
	V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer

	// note: this is the one case where 0 is a valid room number
	// because it corresponds to "All"
	CL_UpdateServerList(!(ms_RoomId < 0), ms_RoomId);

	// first page of servers
	serverlistpage = 0;
}

static INT32 menuRoomIndex = 0;

static void M_DrawRoomMenu(void)
{
	char *rmotd;

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	V_DrawString(currentMenu->x - 16, currentMenu->y, V_YELLOWMAP, M_GetText("Select a room"));

	M_DrawTextBox(144, 24, 20, 20);

	if (itemOn == 0)
		rmotd = Z_StrDup(M_GetText("Don't connect to the Master Server."));
	else
		rmotd = room_list[itemOn-1].motd;

	rmotd = V_WordWrap(0, 20*8, 0, rmotd);
	V_DrawString(144+8, 32, V_ALLOWLOWERCASE|V_RETURN8, rmotd);
	Z_Free(rmotd);
}

static void M_DrawConnectMenu(void)
{
	UINT16 i;
	const char *gt = "Unknown";
	INT32 numPages = (serverlistcount+(SERVERS_PER_PAGE-1))/SERVERS_PER_PAGE;

	for (i = FIRSTSERVERLINE; i < min(localservercount, SERVERS_PER_PAGE)+FIRSTSERVERLINE; i++)
		MP_ConnectMenu[i].status = IT_STRING | IT_SPACE;

	if (!numPages)
		numPages = 1;

	// Room name
	if (ms_RoomId < 0)
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ConnectMenu[mp_connect_room].alphaKey,
		                         V_YELLOWMAP, (itemOn == mp_connect_room) ? "<Select to change>" : "<Offline Mode>");
	else
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ConnectMenu[mp_connect_room].alphaKey,
		                         V_YELLOWMAP, room_list[menuRoomIndex].name);

	// Page num
	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ConnectMenu[mp_connect_page].alphaKey,
	                         V_YELLOWMAP, va("%u of %d", serverlistpage+1, numPages));

	// Horizontal line!
	V_DrawFill(1, currentMenu->y+40, 318, 1, 0);

	if (serverlistcount <= 0)
		V_DrawString(currentMenu->x,currentMenu->y+SERVERHEADERHEIGHT, 0, "No servers found");
	else
	for (i = 0; i < min(serverlistcount - serverlistpage * SERVERS_PER_PAGE, SERVERS_PER_PAGE); i++)
	{
		INT32 slindex = i + serverlistpage * SERVERS_PER_PAGE;
		UINT32 globalflags = ((serverlist[slindex].info.numberofplayer >= serverlist[slindex].info.maxplayer) ? V_TRANSLUCENT : 0)
			|((itemOn == FIRSTSERVERLINE+i) ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE;

		V_DrawString(currentMenu->x, S_LINEY(i), globalflags, serverlist[slindex].info.servername);

		// Don't use color flags intentionally, the global yellow color will auto override the text color code
		if (serverlist[slindex].info.modifiedgame)
			V_DrawSmallString(currentMenu->x+202, S_LINEY(i)+8, globalflags, "\x85" "Mod");
		if (serverlist[slindex].info.cheatsenabled)
			V_DrawSmallString(currentMenu->x+222, S_LINEY(i)+8, globalflags, "\x83" "Cheats");
		if (Net_IsNodeIPv6(serverlist[slindex].node))
			V_DrawSmallString(currentMenu->x+252, S_LINEY(i)+8, globalflags, "\x84" "IPv6");

		V_DrawSmallString(currentMenu->x, S_LINEY(i)+8, globalflags,
		                     va("Ping: %u", (UINT32)LONG(serverlist[slindex].info.time)));

		gt = "Unknown";
		if (serverlist[slindex].info.gametype < NUMGAMETYPES)
			gt = Gametype_Names[serverlist[slindex].info.gametype];

		V_DrawSmallString(currentMenu->x+46,S_LINEY(i)+8, globalflags,
		                         va("Players: %02d/%02d", serverlist[slindex].info.numberofplayer, serverlist[slindex].info.maxplayer));

		V_DrawSmallString(currentMenu->x+112, S_LINEY(i)+8, globalflags, va("Gametype: %s", gt));

		MP_ConnectMenu[i+FIRSTSERVERLINE].status = IT_STRING | IT_CALL;
	}

	localservercount = serverlistcount;

	M_DrawGenericMenu();
}

static boolean M_CancelConnect(void)
{
	D_CloseConnection();
	return true;
}

// Ascending order, not descending.
// The casts are safe as long as the caller doesn't do anything stupid.
#define SERVER_LIST_ENTRY_COMPARATOR(key) \
static int ServerListEntryComparator_##key(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sa->info.key != sb->info.key) \
		return sa->info.key - sb->info.key; \
	return strcmp(sa->info.servername, sb->info.servername); \
}

// This does descending instead of ascending.
#define SERVER_LIST_ENTRY_COMPARATOR_REVERSE(key) \
static int ServerListEntryComparator_##key##_reverse(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sb->info.key != sa->info.key) \
		return sb->info.key - sa->info.key; \
	return strcmp(sb->info.servername, sa->info.servername); \
}

SERVER_LIST_ENTRY_COMPARATOR(time)
SERVER_LIST_ENTRY_COMPARATOR(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(maxplayer)
SERVER_LIST_ENTRY_COMPARATOR(gametype)

// Special one for modified state.
static int ServerListEntryComparator_modified(const void *entry1, const void *entry2)
{
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2;

	// Modified acts as 2 points, cheats act as one point.
	int modstate_a = (sa->info.cheatsenabled ? 1 : 0) | (sa->info.modifiedgame ? 2 : 0);
	int modstate_b = (sb->info.cheatsenabled ? 1 : 0) | (sb->info.modifiedgame ? 2 : 0);

	if (modstate_a != modstate_b)
		return modstate_a - modstate_b;

	// Default to strcmp.
	return strcmp(sa->info.servername, sb->info.servername);
}
#endif

void M_SortServerList(void)
{
#ifndef NONET
	switch(cv_serversort.value)
	{
	case 0:		// Ping.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_time);
		break;
	case 1:		// Modified state.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_modified);
		break;
	case 2:		// Most players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer_reverse);
		break;
	case 3:		// Least players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer);
		break;
	case 4:		// Max players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_maxplayer_reverse);
		break;
	case 5:		// Gametype.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_gametype);
		break;
	}
#endif
}

#ifndef NONET
#ifdef UPDATE_ALERT
static boolean M_CheckMODVersion(void)
{
	char updatestring[500];
	const char *updatecheck = GetMODVersion();
	if(updatecheck)
	{
		sprintf(updatestring, UPDATE_ALERT_STRING, VERSIONSTRING, updatecheck);
		M_StartMessage(updatestring, NULL, MM_NOTHING);
		return false;
	} else
		return true;
}
#endif

static void M_ConnectMenu(INT32 choice)
{
	(void)choice;
	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	// first page of servers
	serverlistpage = 0;
	M_SetupNextMenu(&MP_ConnectDef);
	itemOn = 0;
	M_UpdateItemOn();
	M_Refresh(0);
}

static UINT32 roomIds[NUM_LIST_ROOMS];

static void M_RoomMenu(INT32 choice)
{
	INT32 i;

	(void)choice;

	// Display a little "please wait" message.
	M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Fetching room info...");
	V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer

	if (GetRoomsList(currentMenu == &MP_ServerDef) < 0)
		return;

#ifdef UPDATE_ALERT
	if (!M_CheckMODVersion())
		return;
#endif

	for (i = 1; i < NUM_LIST_ROOMS+1; ++i)
		MP_RoomMenu[i].status = IT_DISABLED;
	memset(roomIds, 0, sizeof(roomIds));

	for (i = 0; room_list[i].header.buffer[0]; i++)
	{
		if(*room_list[i].name != '\0')
		{
			MP_RoomMenu[i+1].text = room_list[i].name;
			roomIds[i] = room_list[i].id;
			MP_RoomMenu[i+1].status = IT_STRING|IT_CALL;
		}
	}

	MP_RoomDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_RoomDef);
}

static void M_ChooseRoom(INT32 choice)
{
	if (choice == 0)
		ms_RoomId = -1;
	else
	{
		ms_RoomId = roomIds[choice-1];
		menuRoomIndex = choice - 1;
	}

	serverlistpage = 0;
	M_SetupNextMenu(currentMenu->prevMenu);
	if (currentMenu == &MP_ConnectDef)
		M_Refresh(0);
}
#endif //NONET

//===========================================================================
// Start Server Menu
//===========================================================================

//
// FindFirstMap
//
// Finds the first map of a particular gametype
// Defaults to 1 if nothing found.
//
static INT32 M_FindFirstMap(INT32 gtype)
{
	INT32 i;

	for (i = 0; i < NUMMAPS; i++)
	{
		if (mapheaderinfo[i] && (mapheaderinfo[i]->typeoflevel & gtype))
			return i + 1;
	}

	return 1;
}

static void M_StartServer(INT32 choice)
{
	boolean StartSplitScreenGame = (currentMenu == &MP_SplitServerDef);

	(void)choice;
	if (!StartSplitScreenGame)
		netgame = true;

	multiplayer = true;

	// Still need to reset devmode
	cv_debug = 0;

	if (demoplayback)
		G_StopDemo();
	if (metalrecording)
		G_StopMetalDemo();

	if (!StartSplitScreenGame)
	{
		D_MapChange(cv_nextmap.value, cv_newgametype.value, false, 1, 1, false, false);
		COM_BufAddText("dummyconsvar 1\n");
	}
	else // split screen
	{
		paused = false;
		SV_StartSinglePlayerServer();
		if (!splitscreen)
		{
			splitscreen = true;
			SplitScreen_OnChange();
		}
		D_MapChange(cv_nextmap.value, cv_newgametype.value, false, 1, 1, false, false);
	}

	M_ClearMenus(true);
}

static void M_DrawServerMenu(void)
{
	lumpnum_t lumpnum;
	patch_t *PictureOfLevel;

	M_DrawGenericMenu();

#ifndef NONET
	// Room name
	if (currentMenu == &MP_ServerDef)
	{
		if (ms_RoomId < 0)
			V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ServerMenu[mp_server_room].alphaKey,
			                         V_YELLOWMAP, (itemOn == mp_server_room) ? "<Select to change>" : "<Offline Mode>");
		else
			V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ServerMenu[mp_server_room].alphaKey,
			                         V_YELLOWMAP, room_list[menuRoomIndex].name);
	}
#endif

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_PATCH);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_PATCH);

	V_DrawSmallScaledPatch((BASEVIDWIDTH*3/4)-(SHORT(PictureOfLevel->width)/4), ((BASEVIDHEIGHT*3/4)-(SHORT(PictureOfLevel->height)/4)+10), 0, PictureOfLevel);
}

static void M_MapChange(INT32 choice)
{
	(void)choice;

	levellistmode = LLM_CREATESERVER;

	CV_SetValue(&cv_newgametype, gametype);
	CV_SetValue(&cv_nextmap, gamemap);

	M_PrepareLevelSelect();
	M_SetupNextMenu(&MISC_ChangeLevelDef);
}

static void M_StartSplitServerMenu(INT32 choice)
{
	(void)choice;
	levellistmode = LLM_CREATESERVER;
	M_PrepareLevelSelect();
	M_SetupNextMenu(&MP_SplitServerDef);
}

#ifndef NONET
static void M_StartServerMenu(INT32 choice)
{
	(void)choice;
	levellistmode = LLM_CREATESERVER;
	M_PrepareLevelSelect();
	ms_RoomId = -1;
	M_SetupNextMenu(&MP_ServerDef);

}

// ==============
// CONNECT VIA IP
// ==============

static char setupm_ip[128];

// Draw the funky Connect IP menu. Tails 11-19-2002
// So much work for such a little thing!
static void M_DrawMPMainMenu(void)
{
	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// draw name string
	M_DrawTextBox(55,90,22,1);

	if ( strlen(setupm_ip) > 21 ) { // Is setupm_ip larger than the textbox can fit?

		char left_arrow[1+1] = "\x1C"; // Left arrow

		char new_setupm_ip[21]; // Last 21 characters of setupm_ip
		strcat(new_setupm_ip, setupm_ip+(strlen(setupm_ip)-21));

		if (itemOn == 2)
			V_DrawThinString(53 + (skullAnimCounter % 8) / 4,98, V_ALLOWLOWERCASE|V_MONOSPACE|V_YELLOWMAP, left_arrow); // Draw the left arrow

		V_DrawString(65,98, V_ALLOWLOWERCASE|V_MONOSPACE, new_setupm_ip); // Draw the truncated setupm_ip.

	 	// draw text cursor for name
		if (itemOn == 2 &&
		    skullAnimCounter < 4)   //blink cursor
			V_DrawCharacter(65+V_StringWidth(new_setupm_ip, V_ALLOWLOWERCASE|V_MONOSPACE),98,'_',false);
	} else {
		V_DrawString(65,98, V_ALLOWLOWERCASE|V_MONOSPACE, setupm_ip);

		// draw text cursor for name
		if (itemOn == 2 &&
		    skullAnimCounter < 4)   //blink cursor
			V_DrawCharacter(65+V_StringWidth(setupm_ip, V_ALLOWLOWERCASE|V_MONOSPACE),98,'_',false);
	}
}

// Tails 11-19-2002
static void M_ConnectIP(INT32 choice)
{
	(void)choice;

	M_ClearMenus(true);

	if (*setupm_ip == 0) // Length 0
	{
		M_StartMessage("You must specify an IP address.\n", NULL, MM_NOTHING);
		return;
	}

	if (!M_CheckIfValidIPv4(setupm_ip) && strstr(setupm_ip, ":") == NULL) // Not IPv4 and no colons
	{
		int i = 0;
		while (setupm_ip[i]) // For each char in setupm_ip
		{
			// Is it an alphabet letter or brackets?
			if (isalpha(setupm_ip[i]) || setupm_ip[i] == 91 || setupm_ip[i] == 93) {
				break; // It's probably valid
			} else { // Otherwise
				// It's invalid
				M_StartMessage("You must specify a valid IP address.\n", NULL, MM_NOTHING);
				return;
			}
			i++;
		}
	}

	if (setupm_ip[(strlen(setupm_ip)-1)] == 58) { // If there is a colon at the end of the char
		M_StartMessage("Please specify a valid port.\n", NULL, MM_NOTHING);
		return;
	}

	// Checks passed, attempt connection!
	COM_BufAddText(va("connect \"%s\"\n", setupm_ip));

	// A little "please wait" message.
	M_DrawTextBox(56, BASEVIDHEIGHT/2-12, 24, 2);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Connecting to server...");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer
}

static boolean M_CheckIfValidIPv4(const char *str)
{
    int segments = 0;   // Segment count.
    int chcnt = 0;  // Character count within segment.
    int accum = 0;  // Accumulator for segment.
    // Catch NULL pointer.
    if (str == NULL)
        return false;
    // Process every character in string.
    while (*str != '\0') {
        // Segment changeover.
        if (*str == '.') {
            // Must have some digits in segment.
            if (chcnt == 0)
                return false;
            // Limit number of segments.
            if (++segments == 4)
                return false;
            // Reset segment values and restart loop.
            chcnt = accum = 0;
            str++;
            continue;
        }
        // Check numeric.
        if ((*str < '0') || (*str > '9'))
            return false;
        // Accumulate and check segment.
        if ((accum = accum * 10 + *str - '0') > 255)
            return false;
        // Advance other segment specific stuff and continue loop.
        chcnt++;
        str++;
    }
    // Check enough segments and enough characters in last segment.
    if (segments != 3)
        return false;
    if (chcnt == 0)
        return false;
    // Address okay.
    return true;
}

static void M_ConnectLastServer(INT32 choice)
{
	(void)choice;

	if (!*cv_lastserver.string)
	{
		M_StartMessage("You haven't previously joined a server.\n", NULL, MM_NOTHING);
		return;
	}

	M_ClearMenus(true);
	COM_BufAddText(va("connect \"%s\"\n", cv_lastserver.string));
}

// Tails 11-19-2002

static void M_HandleConnectIP(INT32 choice)
{
	size_t l;
	boolean exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL,sfx_menu1); // Tails
			break;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL,sfx_menu1); // Tails
			break;

		case KEY_ENTER:
			S_StartSound(NULL,sfx_menu1); // Tails
			currentMenu->lastOn = itemOn;
			M_ConnectIP(1);
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			skullAnimCounter = 4; // For a nice looking cursor
			if ((l = strlen(setupm_ip)) != 0)
			{
				//S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l-1] = 0;
			}
			break;

		case KEY_DEL:
			skullAnimCounter = 4; // For a nice looking cursor
			if (setupm_ip[0] && !shiftdown) // Shift+Delete is used for something else.
			{
				//S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[0] = 0;
			}
			if (!shiftdown) // Shift+Delete is used for something else.
				break;

			/* FALLTHRU */

		default:
			skullAnimCounter = 4; // For a nice looking cursor

			l = strlen(setupm_ip);
			if (l >= 127)
				break;

			if ( ctrldown ) {
				switch (choice) {
					case 'V': // ctrl+v, pasting
					{
						const char *paste = I_ClipboardPaste();

						if (paste != NULL) {
							strncat(setupm_ip, paste, 28-1 - l); // Concat the ip field with clipboard
							if (strlen(paste) != 0) // Don't play sound if nothing was pasted
								S_StartSound(NULL,sfx_menu1); // Tails
						}

						break;
					}
					case KEY_INS:
					case 'c': // ctrl+c, ctrl+insert, copying
						I_ClipboardCopy(setupm_ip, l);
						S_StartSound(NULL,sfx_menu1); // Tails
						break;

					case 'x': // ctrl+x, cutting
						I_ClipboardCopy(setupm_ip, l);
						S_StartSound(NULL,sfx_menu1); // Tails
						setupm_ip[0] = 0;
						break;

					default: // otherwise do nothing
						break;
				}
				break; // don't check for typed keys
			}

			if ( shiftdown ) {
				switch (choice) {
					case KEY_INS: // shift+insert, pasting
						{
							const char *paste = I_ClipboardPaste();

							if (paste != NULL) {
								strncat(setupm_ip, paste, 28-1 - l); // Concat the ip field with clipboard
								if (strlen(paste) != 0) // Don't play sound if nothing was pasted
									S_StartSound(NULL,sfx_menu1); // Tails
							}

							break;
						}
					case KEY_DEL: // shift+delete, cutting
						I_ClipboardCopy(setupm_ip, l);
						setupm_ip[0] = 0;
						break;
					default: // otherwise do nothing.
						break;
				}
				break; // don't check for typed keys
			}

			// Rudimentary number, letter, period, and colon enforcing
			if (choice == 46 || choice == 91 || choice == 93 || (choice >= 65 && choice <= 90 ) || (choice >= 97 && choice <= 122 ) || (choice >= 48 && choice <= 58))
			{
				//S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l] = (char)choice;
				setupm_ip[l+1] = 0;
			}
			else if (choice >= 199 && choice <= 211 && choice != 202 && choice != 206) //numpad too!
			{
				char keypad_translation[] = {'7','8','9','-','4','5','6','+','1','2','3','0','.'};
				choice = keypad_translation[choice - 199];
				//S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l] = (char)choice;
				setupm_ip[l+1] = 0;
			}

			break;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu (currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}
#endif //!NONET

// ========================
// MULTIPLAYER PLAYER SETUP
// ========================
// Tails 03-02-2002

#define PLBOXW    8
#define PLBOXH    9

static fixed_t    multi_tics;
static state_t   *multi_state;

// this is set before entering the MultiPlayer setup menu,
// for either player 1 or 2
static char       setupm_name[MAXPLAYERNAME+1];
static player_t  *setupm_player;
static consvar_t *setupm_cvskin;
static consvar_t *setupm_cvcolor;
static consvar_t *setupm_cvname;
static consvar_t *setupm_cvdefaultskin;
static consvar_t *setupm_cvdefaultcolor;
static INT32      setupm_fakeskin;
static UINT16     setupm_fakecolor;

static void M_DrawSetupMultiPlayerMenu(void)
{
	INT32 mx, my, st, flags = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;
	UINT8 frame;

	mx = MP_PlayerSetupDef.x;
	my = MP_PlayerSetupDef.y;

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// draw name string
	M_DrawTextBox(mx + 90, my - 8, MAXPLAYERNAME, 1);
	V_DrawString(mx + 98, my, V_ALLOWLOWERCASE, setupm_name);

	// draw skin string
	V_DrawString(208, 72,
		((MP_PlayerSetupMenu[2].status & IT_TYPE) == IT_SPACE ? V_TRANSLUCENT : 0)|V_YELLOWMAP|V_ALLOWLOWERCASE,
		skins[setupm_fakeskin].realname);

	// draw the name of the color you have chosen
	// Just so people don't go thinking that "Default" is Green.
	V_DrawRightAlignedString(291, my + 96, V_YELLOWMAP|V_ALLOWLOWERCASE, skincolors[setupm_fakecolor].name);

	// draw text cursor for name
	if (!itemOn && skullAnimCounter < 4) // blink cursor
		V_DrawCharacter(mx + 98 + V_StringWidth(setupm_name, 0), my, '_', false);

	// anim the player in the box
	multi_tics -= renderdeltatics;
	while (multi_tics <= 0)
	{
		st = multi_state->nextstate;
		if (st != S_NULL)
			multi_state = &states[st];

		if (multi_state->tics <= -1)
			multi_tics += 15*FRACUNIT;
		else
			multi_tics += multi_state->tics * FRACUNIT;
	}


	// skin 0 is default player sprite
	if (R_SkinAvailable(skins[setupm_fakeskin].name) != -1)
		sprdef = &skins[R_SkinAvailable(skins[setupm_fakeskin].name)].spritedef;
	else
		sprdef = &skins[0].spritedef;

	if (!sprdef->numframes) // No frames ??
		return; // Can't render!

	frame = multi_state->frame & FF_FRAMEMASK;
	if (frame >= sprdef->numframes) // Walking animation missing
		frame = 0; // Try to use standing frame

	sprframe = &sprdef->spriteframes[frame];
	patch = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);
	if (sprframe->flip & 1) // Only for first sprite
		flags |= V_FLIP; // This sprite is left/right flipped!

	// draw box around guy
	M_DrawTextBox(mx + 90, my + 8, PLBOXW, PLBOXH);

	// draw player sprite
	if (!setupm_fakecolor) // should never happen but hey, who knows
	{
		if (skins[setupm_fakeskin].flags & SF_HIRES)
		{
			V_DrawSciencePatch((mx + 98 + (PLBOXW * 8 / 2)) << FRACBITS,
				(my + 16 + (PLBOXH * 8) - 12) << FRACBITS,
				flags, patch,
				skins[setupm_fakeskin].highresscale);
		}
		else
			V_DrawScaledPatch(mx + 98 + (PLBOXW * 8 / 2), my + 16 + (PLBOXH * 8) - 12, flags, patch);
	}
	else
	{
		UINT8 *colormap = R_GetTranslationColormap(setupm_fakeskin, setupm_fakecolor, GTC_CACHE);

		if (skins[setupm_fakeskin].flags & SF_HIRES)
		{
			V_DrawFixedPatch((mx + 98 + (PLBOXW * 8 / 2)) << FRACBITS,
				(my + 16 + (PLBOXH * 8) - 12) << FRACBITS,
				skins[setupm_fakeskin].highresscale,
				flags, patch, colormap);
		}
		else
			V_DrawMappedPatch(mx + 98 + (PLBOXW * 8 / 2), my + 16 + (PLBOXH * 8) - 12, flags, patch, colormap);
	}

	// Draw the palette below!
	// note: height is always 16
#define color_width 12
#define selected_width 80

	int x,y,count,i,j,color;
	count = 8;
	x = (BASEVIDWIDTH / 2) - (color_width / 2);
	y = 148;
	color = setupm_fakecolor;

	// selected color
	for (j = 0; j < 16; j++)
		V_DrawFill(x - (selected_width / 2), y+j, selected_width, 1, skincolors[color].ramp[j]);

	color = M_GetColorPrev(color);

	// prev colors
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < 16; j++)
			V_DrawFill(x - (i * color_width) - (selected_width / 2), y+j, color_width, 1, skincolors[color].ramp[j]);
		color = M_GetColorPrev(color);
	}

	color = M_GetColorNext(setupm_fakecolor);

	// next colors
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < 16; j++)
			V_DrawFill(x + (i * color_width) + (selected_width / 2), y+j, color_width, 1, skincolors[color].ramp[j]);
		color = M_GetColorNext(color);
	}

#undef selected_width
#undef color_width

	x = MP_PlayerSetupDef.x;
	y += 20;

	V_DrawString(x, y,
		((R_SkinAvailable(setupm_cvdefaultskin->string) != setupm_fakeskin
		|| setupm_cvdefaultcolor->value != setupm_fakecolor)
			? 0
			: V_TRANSLUCENT)
		| ((itemOn == 3) ? V_YELLOWMAP : 0),
		"Save as default");
}

// Handle 1P/2P MP Setup
static void M_HandleSetupMultiPlayer(INT32 choice)
{
	size_t   l;
	boolean  exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
	case KEY_DOWNARROW:
		M_NextOpt();
		S_StartSound(NULL, sfx_menu1); // Tails
		break;

	case KEY_UPARROW:
		M_PrevOpt();
		S_StartSound(NULL, sfx_menu1); // Tails
		break;

	case KEY_LEFTARROW:
		if (itemOn == 1)       //player skin
		{
			S_StartSound(NULL, sfx_menu1); // Tails
			setupm_fakeskin--;
		}
		else if (itemOn == 2) // player color
		{
			S_StartSound(NULL, sfx_menu1); // Tails
			setupm_fakecolor = M_GetColorPrev(setupm_fakecolor);
		}
		break;

	case KEY_ENTER:
		if (itemOn == 3
			&& (R_SkinAvailable(setupm_cvdefaultskin->string) != setupm_fakeskin
			|| setupm_cvdefaultcolor->value != setupm_fakecolor))
			{
				S_StartSound(NULL,sfx_strpst);
				// you know what? always putting these in the buffer won't hurt anything.
				COM_BufAddText (va("%s \"%s\"\n",setupm_cvdefaultskin->name,skins[setupm_fakeskin].name));
				COM_BufAddText (va("%s %d\n",setupm_cvdefaultcolor->name,setupm_fakecolor));
			}
		break;

	case KEY_RIGHTARROW:
		if (itemOn == 1)       //player skin
		{
			S_StartSound(NULL, sfx_menu1); // Tails
			setupm_fakeskin++;
		}
		else if (itemOn == 2) // player color
		{
			S_StartSound(NULL, sfx_menu1); // Tails
			setupm_fakecolor = M_GetColorNext(setupm_fakecolor);
		}
		break;

	case KEY_ESCAPE:
		exitmenu = true;
		break;

	case KEY_BACKSPACE:
		if ((l = strlen(setupm_name)) != 0 && itemOn == 0)
		{
			S_StartSound(NULL, sfx_menu1); // Tails
			setupm_name[l - 1] = 0;
		}
		else if (itemOn == 2)
		{
			UINT8 col = skins[setupm_fakeskin].prefcolor;
			if (setupm_fakecolor != col && skincolors[col].accessible)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_fakecolor = col;
			}
		}
		break;

	default:
		if (choice < 32 || choice > 127 || itemOn != 0)
			break;
		l = strlen(setupm_name);
		if (l < MAXPLAYERNAME)
		{
			S_StartSound(NULL, sfx_menu1); // Tails
			setupm_name[l] = (char)choice;
			setupm_name[l + 1] = 0;
		}
		break;
	}

	// check skin
	if (setupm_fakeskin < 0)
		setupm_fakeskin = numskins - 1;
	if (setupm_fakeskin > numskins - 1)
		setupm_fakeskin = 0;

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// start the multiplayer setup menu
static void M_SetupMultiPlayer(INT32 choice)
{
	(void)choice;

	multi_state = &states[mobjinfo[MT_PLAYER].seestate];
	multi_tics = multi_state->tics*FRACUNIT;
	strcpy(setupm_name, cv_playername.string);

	// set for player 1
	setupm_player = &players[consoleplayer];
	setupm_cvskin = &cv_skin;
	setupm_cvcolor = &cv_playercolor;
	setupm_cvname = &cv_playername;
	setupm_cvdefaultskin = &cv_defaultskin;
	setupm_cvdefaultcolor = &cv_defaultplayercolor;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;
	setupm_fakecolor = setupm_cvcolor->value;

	// disable skin changes if we can't actually change skins
	if (!CanChangeSkin(consoleplayer))
		MP_PlayerSetupMenu[2].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[2].status = (IT_KEYHANDLER|IT_STRING);

	MP_PlayerSetupDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_PlayerSetupDef);
}

// start the multiplayer setup menu, for secondary player (splitscreen mode)
static void M_SetupMultiPlayer2(INT32 choice)
{
	(void)choice;

	multi_state = &states[mobjinfo[MT_PLAYER].seestate];
	multi_tics = multi_state->tics*FRACUNIT;
	strcpy (setupm_name, cv_playername2.string);

	// set for splitscreen secondary player
	setupm_player = &players[secondarydisplayplayer];
	setupm_cvskin = &cv_skin2;
	setupm_cvcolor = &cv_playercolor2;
	setupm_cvname = &cv_playername2;
	setupm_cvdefaultskin = &cv_defaultskin2;
	setupm_cvdefaultcolor = &cv_defaultplayercolor2;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;
	setupm_fakecolor = setupm_cvcolor->value;

	// disable skin changes if we can't actually change skins
	if (splitscreen && !CanChangeSkin(secondarydisplayplayer))
		MP_PlayerSetupMenu[2].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[2].status = (IT_KEYHANDLER | IT_STRING);

	MP_PlayerSetupDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_PlayerSetupDef);
}

static boolean M_QuitMultiPlayerMenu(void)
{
	size_t l;
	// send name if changed
	if (strcmp(setupm_name, setupm_cvname->string))
	{
		// remove trailing whitespaces
		for (l= strlen(setupm_name)-1;
		    (signed)l >= 0 && setupm_name[l] ==' '; l--)
			setupm_name[l] =0;
		COM_BufAddText (va("%s \"%s\"\n",setupm_cvname->name,setupm_name));
	}
	// you know what? always putting these in the buffer won't hurt anything.
	COM_BufAddText (va("%s \"%s\"\n",setupm_cvskin->name,skins[setupm_fakeskin].name));
	COM_BufAddText (va("%s %d\n",setupm_cvcolor->name,setupm_fakecolor));
	return true;
}

// =================
// DATA OPTIONS MENU
// =================
static UINT8 erasecontext = 0;

static void M_EraseDataResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Delete the data
	if (erasecontext != 1)
		G_ClearRecords();
	if (erasecontext != 0)
		M_ClearSecrets();
	if (erasecontext == 2)
	{
		totalplaytime = 0;
		F_StartIntro();
	}
	M_ClearMenus(true);
}

static void M_EraseData(INT32 choice)
{
	const char *eschoice, *esstr = M_GetText("Are you sure you want to erase\n%s?\n\n(Press 'Y' to confirm)\n");

	erasecontext = (UINT8)choice;

	if (choice == 0)
		eschoice = M_GetText("Record Attack data");
	else if (choice == 1)
		eschoice = M_GetText("Secrets data");
	else
		eschoice = M_GetText("ALL game data");

	M_StartMessage(va(esstr, eschoice),M_EraseDataResponse,MM_YESNO);
}

static void M_ScreenshotOptions(INT32 choice)
{
	(void)choice;
	Screenshot_option_Onchange();
	Moviemode_mode_Onchange();

	M_SetupNextMenu(&OP_ScreenshotOptionsDef);
}

// =============
// JOYSTICK MENU
// =============

// Start the controls menu, setting it up for either the console player,
// or the secondary splitscreen player

static void M_DrawJoystick(void)
{
	INT32 i, compareval2, compareval;

	M_DrawGenericMenu();

	for (i = 0;i < 7; i++)
	{
		M_DrawTextBox(OP_JoystickSetDef.x-8, OP_JoystickSetDef.y+LINEHEIGHT*i-12, 28, 1);
		//M_DrawSaveLoadBorder(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i);

#ifdef JOYSTICK_HOTPLUG
		if (atoi(cv_usejoystick2.string) > I_NumJoys())
			compareval2 = atoi(cv_usejoystick2.string);
		else
			compareval2 = cv_usejoystick2.value;

		if (atoi(cv_usejoystick.string) > I_NumJoys())
			compareval = atoi(cv_usejoystick.string);
		else
			compareval = cv_usejoystick.value;
#else
		compareval2 = cv_usejoystick2.value;
		compareval = cv_usejoystick.value;
#endif

		if ((setupcontrols_secondaryplayer && (i == compareval2))
			|| (!setupcontrols_secondaryplayer && (i == compareval)))
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i-4,V_GREENMAP,joystickInfo[i]);
		else
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i-4,0,joystickInfo[i]);
	}
}

void M_SetupJoystickMenu(INT32 choice)
{
	INT32 i = 0;
	const char *joyNA = "Unavailable";
	INT32 n = I_NumJoys();
	(void)choice;

	strcpy(joystickInfo[i], "None");

	for (i = 1; i < 8; i++)
	{
		if (i <= n && (I_GetJoyName(i)) != NULL)
			strncpy(joystickInfo[i], I_GetJoyName(i), 28);
		else
			strcpy(joystickInfo[i], joyNA);

#ifdef JOYSTICK_HOTPLUG
		// We use cv_usejoystick.string as the USER-SET var
		// and cv_usejoystick.value as the INTERNAL var
		//
		// In practice, if cv_usejoystick.string == 0, this overrides
		// cv_usejoystick.value and always disables
		//
		// Update cv_usejoystick.string here so that the user can
		// properly change this value.
		if (i == cv_usejoystick.value)
			CV_SetValue(&cv_usejoystick, i);
		if (i == cv_usejoystick2.value)
			CV_SetValue(&cv_usejoystick2, i);
#endif
	}

	M_SetupNextMenu(&OP_JoystickSetDef);
}

static void M_Setup1PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = false;
	OP_JoystickSetDef.prevMenu = &OP_Joystick1Def;
	M_SetupJoystickMenu(choice);
}

static void M_Setup2PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = true;
	OP_JoystickSetDef.prevMenu = &OP_Joystick2Def;
	M_SetupJoystickMenu(choice);
}

static void M_AssignJoystick(INT32 choice)
{
#ifdef JOYSTICK_HOTPLUG
	INT32 oldchoice, oldstringchoice;
	INT32 numjoys = I_NumJoys();

	if (setupcontrols_secondaryplayer)
	{
		oldchoice = oldstringchoice = atoi(cv_usejoystick2.string) > numjoys ? atoi(cv_usejoystick2.string) : cv_usejoystick2.value;
		CV_SetValue(&cv_usejoystick2, choice);

		// Just in case last-minute changes were made to cv_usejoystick.value,
		// update the string too
		// But don't do this if we're intentionally setting higher than numjoys
		if (choice <= numjoys)
		{
			CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);

			// reset this so the comparison is valid
			if (oldchoice > numjoys)
				oldchoice = cv_usejoystick2.value;

			if (oldchoice != choice)
			{
				if (choice && oldstringchoice > numjoys) // if we did not select "None", we likely selected a used device
					CV_SetValue(&cv_usejoystick2, (oldstringchoice > numjoys ? oldstringchoice : oldchoice));

				if (oldstringchoice ==
					(atoi(cv_usejoystick2.string) > numjoys ? atoi(cv_usejoystick2.string) : cv_usejoystick2.value))
					M_StartMessage("This joystick is used by another\n"
					               "player. Reset the joystick\n"
					               "for that player first.\n\n"
					               "(Press a key)\n", NULL, MM_NOTHING);
			}
		}
	}
	else
	{
		oldchoice = oldstringchoice = atoi(cv_usejoystick.string) > numjoys ? atoi(cv_usejoystick.string) : cv_usejoystick.value;
		CV_SetValue(&cv_usejoystick, choice);

		// Just in case last-minute changes were made to cv_usejoystick.value,
		// update the string too
		// But don't do this if we're intentionally setting higher than numjoys
		if (choice <= numjoys)
		{
			CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

			// reset this so the comparison is valid
			if (oldchoice > numjoys)
				oldchoice = cv_usejoystick.value;

			if (oldchoice != choice)
			{
				if (choice && oldstringchoice > numjoys) // if we did not select "None", we likely selected a used device
					CV_SetValue(&cv_usejoystick, (oldstringchoice > numjoys ? oldstringchoice : oldchoice));

				if (oldstringchoice ==
					(atoi(cv_usejoystick.string) > numjoys ? atoi(cv_usejoystick.string) : cv_usejoystick.value))
					M_StartMessage("This joystick is used by another\n"
					               "player. Reset the joystick\n"
					               "for that player first.\n\n"
					               "(Press a key)\n", NULL, MM_NOTHING);
			}
		}
	}
#else
	if (setupcontrols_secondaryplayer)
		CV_SetValue(&cv_usejoystick2, choice);
	else
		CV_SetValue(&cv_usejoystick, choice);
#endif
}

// =============
// CONTROLS MENU
// =============

static void M_Setup1PControlsMenu(INT32 choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = false;
	setupcontrols = gamecontrol;        // was called from main Options (for console player, then)
	currentMenu->lastOn = itemOn;

	OP_P1ControlsDef.prevMenu = &OP_ControlsDef;
	M_SetupNextMenu(&OP_AllControlsDef);
}

static void M_Setup2PControlsMenu(INT32 choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = true;
	setupcontrols = gamecontrolbis;
	currentMenu->lastOn = itemOn;

	OP_P2ControlsDef.prevMenu = &OP_ControlsDef;
	M_SetupNextMenu(&OP_AllControls2Def);
}

// M_DrawControl from kart
#define controlheight 18

// Draws the Customise Controls menu
static void M_DrawControl(void)
{
	highlightflags = V_YELLOWMAP; // text highlighting doesn't work without this flag

	char tmp[50];
	INT32 x, y, i, max, cursory = 0, iter;
	INT32 keys[2];

	x = currentMenu->x;
	y = currentMenu->y;

	/*i = itemOn - (controlheight/2);
	if (i < 0)
		i = 0;
	*/

	iter = (controlheight / 2);
	for (i = itemOn; ((iter || currentMenu->menuitems[i].status == IT_GRAYEDOUT2) && i > 0); i--)
	{
		if (currentMenu->menuitems[i].status != IT_GRAYEDOUT2)
			iter--;
	}
	if (currentMenu->menuitems[i].status == IT_GRAYEDOUT2)
		i--;

	iter += (controlheight / 2);
	for (max = itemOn; (iter && max < currentMenu->numitems); max++)
	{
		if (currentMenu->menuitems[max].status != IT_GRAYEDOUT2)
			iter--;
	}

	if (iter)
	{
		iter += (controlheight / 2);
		for (i = itemOn; ((iter || currentMenu->menuitems[i].status == IT_GRAYEDOUT2) && i > 0); i--)
		{
			if (currentMenu->menuitems[i].status != IT_GRAYEDOUT2)
				iter--;
		}
	}

	/*max = i + controlheight;
	if (max > currentMenu->numitems)
	{
		max = currentMenu->numitems;
		if (max < controlheight)
			i = 0;
		else
			i = max - controlheight;
	}*/

	// draw title (or big pic)
	M_DrawMenuTitle();

	V_DrawCenteredString(BASEVIDWIDTH/2, 28, 0, "\x80""Press ""\x82""ENTER""\x80"" to change, ""\x82""BACKSPACE""\x80"" to clear");

	if (i)
		V_DrawCharacter(currentMenu->x - 16, y - (skullAnimCounter / 5),
			'\x1A' | highlightflags, false); // up arrow
	if (max != currentMenu->numitems)
		V_DrawCharacter(currentMenu->x - 16, y + (SMALLLINEHEIGHT * (controlheight - 1)) + (skullAnimCounter / 5) + (skullAnimCounter / 5),
			'\x1B' | highlightflags, false); // down arrow

	for (; i < max; i++)
	{
		if (currentMenu->menuitems[i].status == IT_GRAYEDOUT2)
			continue;

		if (i == itemOn)
			cursory = y;

		if (currentMenu->menuitems[i].status == IT_CONTROL)
		{
			V_DrawString(x, y, ((i == itemOn) ? highlightflags : 0), currentMenu->menuitems[i].text);
			keys[0] = setupcontrols[currentMenu->menuitems[i].alphaKey][0];
			keys[1] = setupcontrols[currentMenu->menuitems[i].alphaKey][1];

			tmp[0] = '\0';
			if (keys[0] == KEY_NULL && keys[1] == KEY_NULL)
			{
				strcpy(tmp, "---");
			}
			else
			{
				if (keys[0] != KEY_NULL)
					strcat(tmp, G_KeynumToString(keys[0]));

				if (keys[0] != KEY_NULL && keys[1] != KEY_NULL)
					strcat(tmp, ", ");

				if (keys[1] != KEY_NULL)
					strcat(tmp, G_KeynumToString(keys[1]));

			}
			V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, y, highlightflags, tmp);
		}
		/*else if (currentMenu->menuitems[i].status == IT_GRAYEDOUT2)
			V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);*/
		else if ((currentMenu->menuitems[i].status == IT_HEADER) && (i != max - 1))
			V_DrawString(x-16, y, highlightflags, currentMenu->menuitems[i].text); // had to change some values
		else if (currentMenu->menuitems[i].status & IT_STRING)
			V_DrawString(x, y, ((i == itemOn) ? highlightflags : 0), currentMenu->menuitems[i].text);

		y += SMALLLINEHEIGHT;
	}

	V_DrawScaledPatch(currentMenu->x - 20, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
}

#define scrollareaheight 72

// note that alphakey is multiplied by 2 for scrolling menus to allow greater usage in UINT8 range.
static void M_DrawGenericScrollMenu(void)
{
	INT32 x, y, i, max, tempcentery, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	if ((currentMenu->menuitems[itemOn].alphaKey*2 - currentMenu->menuitems[0].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[0].alphaKey*2;
	else if ((currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 - currentMenu->menuitems[itemOn].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 + 2*scrollareaheight;
	else
		tempcentery = currentMenu->y - currentMenu->menuitems[itemOn].alphaKey*2 + scrollareaheight;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (currentMenu->menuitems[i].status != IT_DISABLED && currentMenu->menuitems[i].alphaKey*2 + tempcentery >= currentMenu->y)
			break;
	}

	for (max = currentMenu->numitems; max > 0; max--)
	{
		if (currentMenu->menuitems[max-1].status != IT_DISABLED && currentMenu->menuitems[max-1].alphaKey*2 + tempcentery <= (currentMenu->y + 2*scrollareaheight))
			break;
	}

	if (i)
		V_DrawCharacter(currentMenu->x - 16, y - (skullAnimCounter / 5),
			'\x1A' |V_YELLOWMAP, false); // up arrow
	if (max != currentMenu->numitems)
		V_DrawCharacter(currentMenu->x - 16, y + (SMALLLINEHEIGHT * (controlheight - 1)) + (skullAnimCounter / 5) + (skullAnimCounter / 5),
			'\x1B' |V_YELLOWMAP, false); // down arrow

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (; i < max; i++)
	{
		y = currentMenu->menuitems[i].alphaKey*2 + tempcentery;
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
			case IT_DYBIGSPACE:
			case IT_BIGSLIDER:
			case IT_STRING2:
			case IT_DYLITLSPACE:
			case IT_GRAYPATCH:
			case IT_TRANSTEXT2:
				// unsupported
				break;
			case IT_NOTHING:
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (i != itemOn && (currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - SLIDER_WIDTH - 6 - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0) - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0), y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								break;
						}
						break;
					}
					break;
			case IT_TRANSTEXT:
				switch (currentMenu->menuitems[i].status & IT_TYPE)
				{
					case IT_PAIR:
						V_DrawString(x, y,
								V_TRANSLUCENT, currentMenu->menuitems[i].patch);
						V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
								V_TRANSLUCENT, currentMenu->menuitems[i].text);
						break;
					default:
						V_DrawString(x, y,
								V_TRANSLUCENT, currentMenu->menuitems[i].text);
				}
				break;
			case IT_QUESTIONMARKS:
				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				break;
			case IT_HEADERTEXT: // draws 16 pixels to the left, in yellow text
				V_DrawString(x-16, y, V_YELLOWMAP, currentMenu->menuitems[i].text);
				//M_DrawLevelPlatterHeader(y - (lsheadingheight - 12),currentMenu->menuitems[i].text, true);
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
	M_DoToolTips(currentMenu);
}

static INT32 controltochange;
static char controltochangetext[33];

static void M_ChangecontrolResponse(event_t *ev)
{
	INT32        control;
	INT32        found;
	INT32        ch = ev->data1;

	// ESCAPE cancels; dummy out PAUSE
	if (ch != KEY_ESCAPE && ch != KEY_PAUSE)
	{

		switch (ev->type)
		{
			// ignore mouse/joy movements, just get buttons
			case ev_mouse:
			case ev_mouse2:
			case ev_joystick:
			case ev_joystick2:
				ch = KEY_NULL;      // no key
			break;

			// keypad arrows are converted for the menu in cursor arrows
			// so use the event instead of ch
			case ev_keydown:
				ch = ev->data1;
			break;

			default:
			break;
		}

		control = controltochange;

		// check if we already entered this key
		found = -1;
		if (setupcontrols[control][0] ==ch)
			found = 0;
		else if (setupcontrols[control][1] ==ch)
			found = 1;
		if (found >= 0)
		{
			// replace mouse and joy clicks by double clicks
			if (ch >= KEY_MOUSE1 && ch <= KEY_MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_MOUSE1+KEY_DBLMOUSE1;
			else if (ch >= KEY_JOY1 && ch <= KEY_JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_JOY1+KEY_DBLJOY1;
			else if (ch >= KEY_2MOUSE1 && ch <= KEY_2MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_2MOUSE1+KEY_DBL2MOUSE1;
			else if (ch >= KEY_2JOY1 && ch <= KEY_2JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_2JOY1+KEY_DBL2JOY1;
		}
		else
		{
			// check if change key1 or key2, or replace the two by the new
			found = 0;
			if (setupcontrols[control][0] == KEY_NULL)
				found++;
			if (setupcontrols[control][1] == KEY_NULL)
				found++;
			if (found == 2)
			{
				found = 0;
				setupcontrols[control][1] = KEY_NULL;  //replace key 1,clear key2
			}
			(void)G_CheckDoubleUsage(ch, true);
			setupcontrols[control][found] = ch;
		}
		S_StartSound(NULL, sfx_strpst);
	}
	else if (ch == KEY_PAUSE)
	{
		// This buffer assumes a 125-character message plus a 32-character control name (per controltochangetext buffer size)
		static char tmp[158];
		menu_t *prev = currentMenu->prevMenu;

		if (controltochange == gc_pause)
			sprintf(tmp, M_GetText("The \x82Pause Key \x80is enabled, but \nyou may select another key. \n\nHit another key for\n%s\nESC for Cancel"),
				controltochangetext);
		else
			sprintf(tmp, M_GetText("The \x82Pause Key \x80is enabled, but \nit is not configurable. \n\nHit another key for\n%s\nESC for Cancel"),
				controltochangetext);

		M_StartMessage(tmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
		currentMenu->prevMenu = prev;

		S_StartSound(NULL, sfx_s3k42);
		return;
	}
	else
		S_StartSound(NULL, sfx_skid);

	M_StopMessage(0);
}

static void M_ChangeControl(INT32 choice)
{
	// This buffer assumes a 35-character message (per below) plus a max control name limit of 32 chars (per controltochangetext)
	// If you change the below message, then change the size of this buffer!
	static char tmp[68];

	controltochange = currentMenu->menuitems[choice].alphaKey;
	sprintf(tmp, M_GetText("Hit the new key for\n%s\nESC for Cancel"),
		currentMenu->menuitems[choice].text);
	strlcpy(controltochangetext, currentMenu->menuitems[choice].text, 33);

	M_StartMessage(tmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
}

// ===========
// Color stuff
// ===========

void M_InitSkincolors(void)
{
	numskincolors = SKINCOLOR_FIRSTFREESLOT;
}

boolean M_CheckColor(UINT16 color)
{
	if (!color || color >= numskincolors)
		return false;
	return true;
}

UINT16 M_GetColorNext(UINT16 base)
{
	UINT32 i;

	for (i = base + 1; ; i++)
	{
		if (i >= numskincolors)
			i = 0; // Recheck.
		if (skincolors[i].accessible)
			return i; // Good.
	}
}

UINT16 M_GetColorPrev(UINT16 base)
{
	UINT32 i;

	for (i = base - 1; ; i--)
	{
		if (!i)
			i = numskincolors - 1; // Recheck.
		if (skincolors[i].accessible)
			return i; // Good.
	}
}

// ===============
// VIDEO MODE MENU
// ===============

//added : 30-01-98:
#define MAXCOLUMNMODES   12     //max modes displayed in one column
#define MAXMODEDESCS     (MAXCOLUMNMODES*3)

static modedesc_t modedescs[MAXMODEDESCS];

static void M_VideoModeMenu(INT32 choice)
{
	INT32 i, j, vdup, nummodes, width, height;
	const char *desc;

	(void)choice;

	memset(modedescs, 0, sizeof(modedescs));

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	VID_PrepareModeList(); // FIXME: hack
#endif
	vidm_nummodes = 0;
	vidm_selected = 0;
	nummodes = VID_NumModes();

#ifdef _WINDOWS
	// clean that later: skip windowed mode 0, video modes menu only shows FULL SCREEN modes
	if (nummodes <= NUMSPECIALMODES)
		i = 0; // unless we have nothing
	else
		i = NUMSPECIALMODES;
#else
	// DOS does not skip mode 0, because mode 0 is ALWAYS present
	i = 0;
#endif
	for (; i < nummodes && vidm_nummodes < MAXMODEDESCS; i++)
	{
		desc = VID_GetModeName(i);
		if (desc)
		{
			vdup = 0;

			// when a resolution exists both under VGA and VESA, keep the
			// VESA mode, which is always a higher modenum
			for (j = 0; j < vidm_nummodes; j++)
			{
				if (!strcmp(modedescs[j].desc, desc))
				{
					// mode(0): 320x200 is always standard VGA, not vesa
					if (modedescs[j].modenum)
					{
						modedescs[j].modenum = i;
						vdup = 1;

						if (i == vid.modenum)
							vidm_selected = j;
					}
					else
						vdup = 1;

					break;
				}
			}

			if (!vdup)
			{
				modedescs[vidm_nummodes].modenum = i;
				modedescs[vidm_nummodes].desc = desc;

				if (i == vid.modenum)
					vidm_selected = vidm_nummodes;

				// Pull out the width and height
				sscanf(desc, "%u%*c%u", &width, &height);

				// Show multiples of 320x200 as green.
				if (SCR_IsAspectCorrect(width, height))
					modedescs[vidm_nummodes].goodratio = 1;

				vidm_nummodes++;
			}
		}
	}

	vidm_column_size = (vidm_nummodes+2) / 3;

	M_SetupNextMenu(&OP_VideoModeDef);
}

// Draw the video modes list, a-la-Quake
static void M_DrawVideoMode(void)
{
	INT32 i, j, row, col;

	// draw title
	M_DrawMenuTitle();

	V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y,
		V_YELLOWMAP, "Choose mode, reselect to change default");

	row = 41;
	col = OP_VideoModeDef.y + 14;
	for (i = 0; i < vidm_nummodes; i++)
	{
		if (i == vidm_selected)
			V_DrawString(row, col, V_YELLOWMAP, modedescs[i].desc);
		// Show multiples of 320x200 as green.
		else
			V_DrawString(row, col, (modedescs[i].goodratio) ? V_GREENMAP : 0, modedescs[i].desc);

		col += 8;
		if ((i % vidm_column_size) == (vidm_column_size-1))
		{
			row += 7*13;
			col = OP_VideoModeDef.y + 14;
		}
	}

	if (vidm_testingmode > 0)
	{
		INT32 testtime = (vidm_testingmode/TICRATE) + 1;

		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 116, 0,
			va("Previewing mode %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : 0x80,
				vid.width, vid.height));
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 138, 0,
			"Press ENTER again to keep this mode");
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 150, 0,
			va("Wait %d second%s", testtime, (testtime > 1) ? "s" : ""));
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 158, 0,
			"or press ESC to return");
	}
	else
	{
		V_DrawFill(60, OP_VideoModeDef.y + 98, 200, 12, 239);
		V_DrawFill(60, OP_VideoModeDef.y + 114, 200, 20, 239);

		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 100, 0,
			va("Current mode is %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : (!(VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value)+1) ? 0x85 : 0x80),
				vid.width, vid.height));
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 116, 0,
			va("Default mode is %c%dx%d",
				(SCR_IsAspectCorrect(cv_scr_width.value, cv_scr_height.value)) ? 0x83 : 0x80,
				cv_scr_width.value, cv_scr_height.value));
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 124, (cv_fullscreen.value ? V_TRANSLUCENT : 0),
			va("Windowed mode is %c%dx%d",
				(SCR_IsAspectCorrect(cv_scr_width_w.value, cv_scr_height_w.value)) ? 0x83 : (!(VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value)+1) ? 0x85 : 0x80),
				cv_scr_width_w.value, cv_scr_height_w.value));

		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 138,
			V_GREENMAP, "Green modes are recommended.");
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 150,
			V_YELLOWMAP, "Other modes may have visual errors.");
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 158,
			V_YELLOWMAP, "Use at own risk.");
	}

	// Draw the cursor for the VidMode menu
	i = 41 - 10 + ((vidm_selected / vidm_column_size)*7*13);
	j = OP_VideoModeDef.y + 14 + ((vidm_selected % vidm_column_size)*8);

	V_DrawScaledPatch(i - 8, j, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
}


// Just M_DrawGenericScrollMenu but showing a backing behind the headers.
static void M_DrawColorMenu(void)
{
	INT32 x, y, i, max, tempcentery, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	V_DrawFill(19       , y-4, 47, 1, 128);
	V_DrawFill(19+(  47), y-4, 47, 1, 104);
	V_DrawFill(19+(2*47), y-4, 47, 1, 184);
	V_DrawFill(19+(3*47), y-4, 47, 1, 247);
	V_DrawFill(19+(4*47), y-4, 47, 1, 249);
	V_DrawFill(19+(5*47), y-4, 46, 1, 193);

	V_DrawFill(300, y-4, 1, 1, 26);
	V_DrawFill(19, y-3, 282, 1, 26);

	if ((currentMenu->menuitems[itemOn].alphaKey*2 - currentMenu->menuitems[0].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[0].alphaKey*2;
	else if ((currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 - currentMenu->menuitems[itemOn].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 + 2*scrollareaheight;
	else
		tempcentery = currentMenu->y - currentMenu->menuitems[itemOn].alphaKey*2 + scrollareaheight;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (currentMenu->menuitems[i].status != IT_DISABLED && currentMenu->menuitems[i].alphaKey*2 + tempcentery >= currentMenu->y)
			break;
	}

	for (max = currentMenu->numitems; max > 0; max--)
	{
		if (currentMenu->menuitems[max].status != IT_DISABLED && currentMenu->menuitems[max-1].alphaKey*2 + tempcentery <= (currentMenu->y + 2*scrollareaheight))
			break;
	}

	if (i)
		V_DrawCharacter(currentMenu->x - 16, y - (skullAnimCounter / 5),
			'\x1A' |V_YELLOWMAP, false); // up arrow
	if (max != currentMenu->numitems)
		V_DrawCharacter(currentMenu->x - 16, y + (SMALLLINEHEIGHT * (controlheight - 1)) + (skullAnimCounter / 5) + (skullAnimCounter / 5),
			'\x1B' |V_YELLOWMAP, false); // down arrowow

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (; i < max; i++)
	{
		y = currentMenu->menuitems[i].alphaKey*2 + tempcentery;
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
			case IT_DYBIGSPACE:
			case IT_BIGSLIDER:
			case IT_STRING2:
			case IT_DYLITLSPACE:
			case IT_GRAYPATCH:
			case IT_TRANSTEXT2:
				// unsupported
				break;
			case IT_NOTHING:
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (i != itemOn && (currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - SLIDER_WIDTH - 6 - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
								if (y + 12 > (currentMenu->y + 2*scrollareaheight))
									break;
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								// draws the little arrows on the left and right
								// to indicate that it is changeable
								if (i == itemOn)
								{
									V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0) - ((skullAnimCounter < 4) ? 9 : 8), y, V_YELLOWMAP, "\x1C");
									V_DrawString(BASEVIDWIDTH - x + ((skullAnimCounter < 4) ? 5 : 4), y, V_YELLOWMAP, "\x1D");
								}
								V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								break;
						}
						break;
					}
					break;
			case IT_TRANSTEXT:
				V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				break;
			case IT_QUESTIONMARKS:
				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				break;
			case IT_HEADERTEXT:
				V_DrawString(x-16, y, V_YELLOWMAP, currentMenu->menuitems[i].text);
				//M_DrawLevelPlatterHeader(y - (lsheadingheight - 12), currentMenu->menuitems[i].text, false);
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_CACHE));
}

// special menuitem key handler for video mode list
static void M_HandleVideoMode(INT32 ch)
{
	if (vidm_testingmode > 0) switch (ch)
	{
		// change back to the previous mode quickly
		case KEY_ESCAPE:
			setmodeneeded = vidm_previousmode + 1;
			vidm_testingmode = 0;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			vidm_testingmode = 0; // stop testing
	}

	else switch (ch)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			if (++vidm_selected >= vidm_nummodes)
				vidm_selected = 0;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			if (--vidm_selected < 0)
				vidm_selected = vidm_nummodes - 1;
			break;

		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_selected -= vidm_column_size;
			if (vidm_selected < 0)
				vidm_selected = (vidm_column_size*3) + vidm_selected;
			if (vidm_selected >= vidm_nummodes)
				vidm_selected = vidm_nummodes - 1;
			break;

		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_selected += vidm_column_size;
			if (vidm_selected >= (vidm_column_size*3))
				vidm_selected %= vidm_column_size;
			if (vidm_selected >= vidm_nummodes)
				vidm_selected = vidm_nummodes - 1;
			break;

		case KEY_ENTER:
			if (vid.modenum == modedescs[vidm_selected].modenum)
			{
				S_StartSound(NULL, sfx_strpst);
				SCR_SetDefaultMode();
			}
			else
			{
				S_StartSound(NULL, sfx_menu1);
				vidm_testingmode = 15*TICRATE;
				vidm_previousmode = vid.modenum;
				if (!setmodeneeded) // in case the previous setmode was not finished
					setmodeneeded = modedescs[vidm_selected].modenum + 1;
			}
			break;

		case KEY_ESCAPE: // this one same as M_Responder
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu);
			else
				M_ClearMenus(true);
			break;

		case KEY_BACKSPACE:
			S_StartSound(NULL, sfx_menu1);
			CV_Set(&cv_scr_width, cv_scr_width.defaultvalue);
			CV_Set(&cv_scr_height, cv_scr_height.defaultvalue);
			CV_Set(&cv_scr_width_w, cv_scr_width_w.defaultvalue);
			CV_Set(&cv_scr_height_w, cv_scr_height_w.defaultvalue);
			if (cv_fullscreen.value)
				setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value)+1;
			else
				setmodeneeded = VID_GetModeForSize(cv_scr_width_w.value, cv_scr_height_w.value)+1;
			break;

		case KEY_F10: // Renderer toggle, also processed inside menus
			CV_AddValue(&cv_renderer, 1);
			break;

		case KEY_F11:
			S_StartSound(NULL, sfx_menu1);
			CV_SetValue(&cv_fullscreen, !cv_fullscreen.value);
			break;

		default:
			break;
	}
}

// ===============
// Monitor Toggles
// ===============
static void M_DrawMonitorToggles(void)
{
	INT32 i, y;
	INT32 sum = 0;
	consvar_t *cv;
	boolean cheating = false;

	M_DrawGenericMenu();

	// Assumes all are cvar type.
	for (i = 0; i < currentMenu->numitems; ++i)
	{
		if (!(currentMenu->menuitems[i].status & IT_CVAR) || !(cv = (consvar_t *)currentMenu->menuitems[i].itemaction))
			continue;
		sum += cv->value;

		if (!CV_IsSetToDefault(cv))
			cheating = true;
	}

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		if (!(currentMenu->menuitems[i].status & IT_CVAR) || !(cv = (consvar_t *)currentMenu->menuitems[i].itemaction))
			continue;
		y = currentMenu->y + currentMenu->menuitems[i].alphaKey;

		M_DrawSlider(currentMenu->x + 20, y, cv);

		if (!cv->value)
			V_DrawRightAlignedString(312, y, V_OLDSPACING|((i == itemOn) ? V_YELLOWMAP : 0), "None");
		else
			V_DrawRightAlignedString(312, y, V_OLDSPACING|((i == itemOn) ? V_YELLOWMAP : 0), va("%3d%%", (cv->value*100)/sum));
	}

	if (cheating)
		V_DrawCenteredString(BASEVIDWIDTH/2, currentMenu->y, V_REDMAP, "* MODIFIED, CHEATS ENABLED *");
}

// =========
// Quit Game
// =========
static INT32 quitsounds[] =
{
	// holy shit we're changing things up!
	sfx_itemup, // Tails 11-09-99
	sfx_jump, // Tails 11-09-99
	sfx_skid, // Inu 04-03-13
	sfx_spring, // Tails 11-09-99
	sfx_pop,
	sfx_spdpad, // Inu 04-03-13
	sfx_wdjump, // Inu 04-03-13
	sfx_mswarp, // Inu 04-03-13
	sfx_splash, // Tails 11-09-99
	sfx_floush, // Tails 11-09-99
	sfx_gloop, // Tails 11-09-99
	sfx_s3k66, // Inu 04-03-13
	sfx_s3k6a, // Inu 04-03-13
	sfx_s3k73, // Inu 04-03-13
	sfx_chchng // Tails 11-09-99
};

void M_QuitResponse(INT32 ch)
{
	tic_t ptime;
	INT32 mrand;

	if (ch != 'y' && ch != KEY_ENTER)
		return;
	if (Playing())
		LUAh_GameQuit();
	if (!(netgame || cv_debug))
	{
		mrand = M_RandomKey(sizeof(quitsounds)/sizeof(INT32));
		if (quitsounds[mrand]) S_StartSound(NULL, quitsounds[mrand]);

		//added : 12-02-98: do that instead of I_WaitVbl which does not work
		ptime = I_GetTime() + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds Tails 03-26-2001
		while (ptime > I_GetTime())
		{
			V_DrawScaledPatch(0, 0, 0, W_CachePatchName("GAMEQUIT", PU_PATCH)); // Demo 3 Quit Screen Tails 06-16-2001
			I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
			I_Sleep(cv_sleep.value);
			I_UpdateTime(cv_timescale.value);

		}
	}
	I_Quit();
}

static void M_QuitSRB2(INT32 choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	(void)choice;
	M_StartMessage(quitmsg[M_RandomKey(NUM_QUITMESSAGES)], M_QuitResponse, MM_YESNO);
}
