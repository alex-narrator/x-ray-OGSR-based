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
		AF_AMMO_FROM_BELT			= 1<<9,
		AF_3D_SCOPES				= 1<<10,
		AF_ZONES_DBG				= 1<<11,
		AF_VERTEX_DBG				= 1<<12,
		AF_CROSSHAIR_DBG			= 1<<13,
		AF_CAM_COLLISION			= 1<<14,
		AF_MOUSE_WHEEL_SWITCH_SLOTS = 1<<15,
		AF_QUICK_FROM_BELT			= 1<<16,
		AF_SURVIVAL					= 1<<17,
		AF_SMOOTH_OVERWEIGHT		= 1<<18,
		AF_WPN_ACTIONS_RESET_SPRINT = 1<<19,
		AF_PICKUP_TARGET_ONLY		= 1<<20,
		AF_ARTEFACTS_FROM_ALL		= 1<<21,
		AF_ARTEFACT_DETECTOR_CHECK	= 1<<22,
		AF_NO_AUTO_RELOAD			= 1<<23,
		AF_KNIFE_TO_CUT_PART		= 1<<24,
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

//освобождение рук для взаимодействия с предметами
enum EFreeHandsMode
{
	eFreeHandsOff,		//отключено
	eFreeHandsAuto,		//автоосвобождение
	eFreeHandsManual	//освобождать вручную
};
extern EFreeHandsMode	g_eFreeHands; //освобождение рук для взаимодействия с предметами: 0 - отключено, 1 - автоматически, 2 - вручную
