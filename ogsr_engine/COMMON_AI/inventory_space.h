#pragma once

constexpr auto CMD_START			= (1<<0);
constexpr auto CMD_STOP				= (1<<1);

constexpr auto NO_ACTIVE_SLOT		= 0xff;

constexpr auto KNIFE_SLOT			= 0;
constexpr auto FIRST_WEAPON_SLOT	= 1;
constexpr auto SECOND_WEAPON_SLOT	= 2;
constexpr auto GRENADE_SLOT			= 3;
constexpr auto APPARATUS_SLOT		= 4;
constexpr auto BOLT_SLOT			= 5;
constexpr auto OUTFIT_SLOT			= 6;
constexpr auto PDA_SLOT				= 7;
constexpr auto DETECTOR_SLOT		= 8;
constexpr auto TORCH_SLOT			= 9;
constexpr auto ARTEFACT_SLOT		= 10;
constexpr auto HELMET_SLOT			= 11;
constexpr auto QUICK_SLOT_0			= 12;
constexpr auto QUICK_SLOT_1			= 13;
constexpr auto QUICK_SLOT_2			= 14;
constexpr auto QUICK_SLOT_3			= 15;
constexpr auto WARBELT_SLOT			= 16;
constexpr auto BACKPACK_SLOT		= 17;
constexpr auto SLOTS_TOTAL			= 18;

constexpr auto RUCK_HEIGHT			= 280;
constexpr auto RUCK_WIDTH			= 7;

class CInventoryItem;
class CInventory;

typedef CInventoryItem*				PIItem;
typedef xr_vector<PIItem>			TIItemContainer;


enum EItemPlace
{			
	eItemPlaceUndefined,
	eItemPlaceSlot,
	eItemPlaceBelt,
	eItemPlaceRuck,
	eItemPlaceBeltActor,
};

extern u32	INV_STATE_LADDER;
extern u32	INV_STATE_CAR;
extern u32	INV_STATE_BLOCK_ALL;
extern u32	INV_STATE_INV_WND;
extern u32	INV_STATE_BUY_MENU;
extern u32	INV_STATE_PDA;
