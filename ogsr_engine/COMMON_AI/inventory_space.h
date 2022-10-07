#pragma once

constexpr auto CMD_START			= (1<<0);
constexpr auto CMD_STOP				= (1<<1);

enum : u32
{
	KNIFE_SLOT,
	ON_SHOULDER_SLOT,
	ON_BACK_SLOT,
	GRENADE_SLOT,
	HOLSTER_SLOT,
	BOLT_SLOT,
	OUTFIT_SLOT,
	PDA_SLOT,
	DETECTOR_SLOT,
	ON_HEAD_SLOT,
	ARTEFACT_SLOT,
	HELMET_SLOT,
	//quick slots
	QUICK_SLOT_0,
	QUICK_SLOT_1,
	QUICK_SLOT_2,
	QUICK_SLOT_3,
	//equipment
	WARBELT_SLOT,
	BACKPACK_SLOT,
	VEST_SLOT,
	//vest pouch - left side
	VEST_POUCH_1,
	VEST_POUCH_2,
	VEST_POUCH_3,
	//vest pouch - chest
	VEST_POUCH_4,
	VEST_POUCH_5,
	VEST_POUCH_6,
	VEST_POUCH_7,
	//vest pouch - right side
	VEST_POUCH_8,
	VEST_POUCH_9,
	VEST_POUCH_10,
	//
    SLOTS_TOTAL,
    NO_ACTIVE_SLOT = 255
};

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
