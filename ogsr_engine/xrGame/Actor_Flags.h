#pragma once

enum{
	AF_GODMODE					= 1<<0,
	AF_KEYPRESS_ON_START		= 1<<1,
	AF_ALWAYSRUN				= 1<<2,
	AF_UNLIMITEDAMMO			= 1<<3,
	AF_DOF_ZOOM					= 1<<4,
	AF_HOLD_TO_AIM				= 1<<5,
	AF_PSP						= 1<<6,
	AF_MUSIC_TRACKS				= 1<<7,
	AF_DOF_SCOPE				= 1<<8,
	AF_3D_SCOPES				= 1<<9,
	AF_ZONES_DBG				= 1<<10,
	AF_VERTEX_DBG				= 1<<11,
	AF_CROSSHAIR_DBG			= 1<<12,
	AF_CAM_COLLISION			= 1<<13,
	AF_MOUSE_WHEEL_SWITCH_SLOTS = 1<<14,
	AF_AI_VOLUMETRIC_LIGHTS		= 1<<15,
	AF_BLOODMARKS_ON_DYNAMIC	= 1<<16,
};

enum {
	CF_KEY_PRESS = (1 << 0),
	CF_KEY_HOLD = (1 << 1),
	CF_KEY_RELEASE = (1 << 2),
	CF_MOUSE_MOVE = (1 << 3),
	CF_MOUSE_WHEEL_ROT = (1 << 4),
};

extern Flags32 psActorFlags;
extern Flags32 psCallbackFlags;

extern BOOL		GodMode	();	

enum ESaveGameMode {
	eSaveGameDefault,
	eSaveGameEnemyCheck,
	eSaveGameSafehouseCheck,
};
extern ESaveGameMode g_eSaveGameMode;