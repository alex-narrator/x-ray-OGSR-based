#include "stdafx.h"
#include "inventory.h"
#include "actor.h"
#include "trade.h"
#include "weapon.h"

#include "ui/UIInventoryUtilities.h"

#include "eatable_item.h"
#include "script_engine.h"
#include "xrmessages.h"
//#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "level.h"
#include "ai_space.h"
#include "entitycondition.h"
#include "game_base_space.h"
#include "clsid_game.h"
#include "CustomOutfit.h"
#include "Warbelt.h"
#include "Vest.h"
#include "HudItem.h"
#include "Grenade.h"

#include "UIGameSP.h"
#include "HudManager.h"
#include "ui/UIInventoryWnd.h"

using namespace InventoryUtilities;

// what to block
u32	INV_STATE_BLOCK_ALL		= 0xffffffff;
u32	INV_STATE_BLOCK_2H		= 1<<ON_SHOULDER_SLOT|1<<ON_BACK_SLOT|1<<ARTEFACT_SLOT; //вважаємо що саме у цих слотах будуть "дворучні" предмети та блокуємо їх
u32	INV_STATE_INV_WND		= INV_STATE_BLOCK_2H;
u32	INV_STATE_BUY_MENU		= INV_STATE_BLOCK_ALL;
u32	INV_STATE_LADDER		= INV_STATE_BLOCK_2H;
u32	INV_STATE_CAR			= INV_STATE_BLOCK_2H;
u32	INV_STATE_PDA			= INV_STATE_BLOCK_2H;

CInventorySlot::CInventorySlot() 
{
	m_pIItem				= NULL;
	m_bVisible				= true;
	m_bPersistent			= false;
	m_blockCounter			= 0;
	m_maySwitchFast			= false;
}

CInventorySlot::~CInventorySlot() 
{
}

bool CInventorySlot::CanBeActivated() const 
{
	return (m_bVisible && !IsBlocked());
};

bool CInventorySlot::IsBlocked() const 
{
	return (m_blockCounter>0);
}

bool CInventorySlot::maySwitchFast() const {
  return m_maySwitchFast;
}

void CInventorySlot::setSwitchFast( bool value ) {
  m_maySwitchFast = value;
}

CInventory::CInventory() 
{
	m_fTakeDist									= pSettings->r_float	("inventory","take_dist");
	m_fMaxWeight								= pSettings->r_float	("inventory","max_weight");
	//m_iMaxBelt									= pSettings->r_u32		("inventory","max_belt");
	m_fMaxVolume								= READ_IF_EXISTS(pSettings, r_float, "inventory", "max_volume", .0f);
	
	m_slots.resize								(SLOTS_TOTAL);
	
	m_iActiveSlot								= NO_ACTIVE_SLOT;
	m_iNextActiveSlot							= NO_ACTIVE_SLOT;
	m_iPrevActiveSlot							= NO_ACTIVE_SLOT;
	m_iLoadActiveSlot							= NO_ACTIVE_SLOT;
	m_ActivationSlotReason						= eGeneral;
	m_pTarget									= nullptr;
	m_pOwner									= nullptr;

	string256 temp;
	for(u32 i=0; i<m_slots.size(); ++i ) 
	{
		sprintf_s(temp, "slot_persistent_%d", i+1);
		if(pSettings->line_exist("inventory",temp)) 
			m_slots[i].m_bPersistent = !!pSettings->r_bool("inventory",temp);
		sprintf_s( temp, "slot_switch_fast_%d", i + 1 );
                m_slots[ i ].setSwitchFast( READ_IF_EXISTS( pSettings, r_bool, "inventory", temp, false ) );
	}

	m_slots[PDA_SLOT].m_bVisible				= false;
	m_slots[OUTFIT_SLOT].m_bVisible				= false;
	m_slots[ON_HEAD_SLOT].m_bVisible				= false;
	m_slots[HELMET_SLOT].m_bVisible				= false;
//	m_slots[NIGHT_VISION_SLOT].m_bVisible		= false;
//	m_slots[BIODETECTOR_SLOT].m_bVisible		= false;
	m_slots[DETECTOR_SLOT].m_bVisible			= false; //KRodin: это очень важно! Слот для зп-стайл детекторов должен быть НЕ активируемым!

	for (u32 i = 0; i < m_slots.size(); ++i){
		sprintf_s(temp, "slot_visible_%d", i + 1);
		if (pSettings->line_exist("inventory", temp))
			m_slots[i].m_bVisible = !!pSettings->r_bool("inventory", temp);
	}

	m_bSlotsUseful								= true;

	m_fTotalWeight								= -1.f;
	m_fTotalVolume								= -1.f;
	m_dwModifyFrame								= 0;
	m_drop_last_frame							= false;
	m_iLoadActiveSlotFrame						= u32(-1);
}


CInventory::~CInventory() 
{
}

void CInventory::Clear()
{
	m_all.clear							();
	m_ruck.clear						();
	m_belt.clear						();
	m_vest.clear						();
	
	for(u32 i=0; i<m_slots.size(); i++)
	{
		m_slots[i].m_pIItem				= NULL;
	}
	

	m_pOwner							= NULL;

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();
}

void CInventory::Take(CGameObject *pObj, bool bNotActivate, bool strict_placement)
{
	CInventoryItem *pIItem				= smart_cast<CInventoryItem*>(pObj);
	VERIFY								(pIItem);
	
	if(pIItem->m_pCurrentInventory)
	{
		Msg("! ERROR CInventory::Take but object has m_pCurrentInventory");
		Msg("! Inventory Owner is [%d]", GetOwner()->object_id());
		Msg("! Object Inventory Owner is [%d]", pIItem->m_pCurrentInventory->GetOwner()->object_id());

		CObject* p	= pObj->H_Parent();
		if(p)
			Msg("! object parent is [%s] [%d]", p->cName().c_str(), p->ID());
	}

	R_ASSERT							(CanTakeItem(pIItem));
	
	pIItem->m_pCurrentInventory			= this;
	pIItem->SetDropManual				(FALSE);

	m_all.push_back						(pIItem);

	if(!strict_placement)
		pIItem->m_eItemPlace			= eItemPlaceUndefined;

	TryAmmoCustomPlacement(pIItem);

	bool result							= false;
	switch(pIItem->m_eItemPlace)
	{
	case eItemPlaceBelt:
		result = Belt(pIItem); 
		if (!result) {
			Msg("!![%s] cant put in belt item [%s], moving to ruck...", __FUNCTION__, pIItem->object().cName().c_str());
			pIItem->m_eItemPlace = eItemPlaceRuck;
			R_ASSERT(Ruck(pIItem));
		}
	break;
	case eItemPlaceVest:
		result = Vest(pIItem);
		if (!result) {
			Msg("!![%s] cant put in vest item [%s], moving to ruck...", __FUNCTION__, pIItem->object().cName().c_str());
			pIItem->m_eItemPlace = eItemPlaceRuck;
			R_ASSERT(Ruck(pIItem));
		}
	break;
	case eItemPlaceRuck:
		result = Ruck(pIItem);
#ifdef DEBUG
		if(!result) 
			Msg("cant put in ruck item %s", *pIItem->object().cName());
#endif

		break;
	case eItemPlaceSlot:
		if ( smart_cast<CActor*>( m_pOwner ) && Device.dwPrecacheFrame && m_iActiveSlot == NO_ACTIVE_SLOT && m_iNextActiveSlot == NO_ACTIVE_SLOT )
			bNotActivate = true;
		result							= Slot(pIItem, bNotActivate); 
#ifdef DEBUG
		if(!result) 
			Msg("cant put in slot item %s", *pIItem->object().cName());
#endif

		break;
	default:
		bool force_move_to_slot{}, force_ruck_default{};
		if (!m_pOwner->m_tmp_next_item_slot)
		{
			force_ruck_default = true;
			m_pOwner->m_tmp_next_item_slot = NO_ACTIVE_SLOT;
		}
		else if (m_pOwner->m_tmp_next_item_slot != NO_ACTIVE_SLOT)
		{
			pIItem->SetSlot(m_pOwner->m_tmp_next_item_slot);
			force_move_to_slot = true;
			m_pOwner->m_tmp_next_item_slot = NO_ACTIVE_SLOT;
		}

		auto pActor = smart_cast<CActor*>(m_pOwner);
		const bool def_to_slot = pActor ? !pIItem->RuckDefault() : true;
		if((!force_ruck_default && def_to_slot && CanPutInSlot(pIItem)) || force_move_to_slot)
		{
			if ( pActor && Device.dwPrecacheFrame )
				bNotActivate = true;
			result						= Slot(pIItem, bNotActivate); VERIFY(result);
		} 
		else if (!force_ruck_default && !pIItem->RuckDefault() && CanPutInBelt(pIItem))
		{
			result						= Belt(pIItem); VERIFY(result);
		}
		else if (!force_ruck_default && !pIItem->RuckDefault() && CanPutInVest(pIItem))
		{
			result						= Vest(pIItem); VERIFY(result);
		}
		else
		{
			result						= Ruck(pIItem); VERIFY(result);
		}
	}
	
	m_pOwner->OnItemTake				(pIItem);

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();

	pIItem->object().processing_deactivate();
	VERIFY								(pIItem->m_eItemPlace != eItemPlaceUndefined);
}

bool CInventory::DropItem(CGameObject *pObj) 
{
	CInventoryItem *pIItem				= smart_cast<CInventoryItem*>(pObj);
	VERIFY								(pIItem);
	if( !pIItem )						return false;

	if (!pIItem->m_pCurrentInventory)	return false;

	ASSERT_FMT(
	  pIItem->m_pCurrentInventory,
	  "CInventory::DropItem: [%s]: pIItem->m_pCurrentInventory",
	  pObj->cName().c_str()
	);
	ASSERT_FMT(
	  pIItem->m_pCurrentInventory == this,
	  "CInventory::DropItem: [%s]: this = %s, pIItem->m_pCurrentInventory = %s",
	  pObj->cName().c_str(),
	  smart_cast<const CGameObject*>(this)->cName().c_str(),
	  smart_cast<const CGameObject*>(pIItem->m_pCurrentInventory)->cName().c_str()
	);
	VERIFY								(pIItem->m_eItemPlace!=eItemPlaceUndefined);

	pIItem->object().processing_activate(); 
	
	switch(pIItem->m_eItemPlace)
	{
	case eItemPlaceBelt:{
		ASSERT_FMT(InBelt(pIItem), "CInventory::DropItem: InBelt(pIItem): %s, owner [%s]", pObj->cName().c_str(), m_pOwner->Name());
		m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));
		pIItem->object().processing_deactivate();
	}break;
	case eItemPlaceVest: {
		ASSERT_FMT(InVest(pIItem), "CInventory::DropItem: InVest(pIItem): %s, owner [%s]", pObj->cName().c_str(), m_pOwner->Name());
		m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));
		pIItem->object().processing_deactivate();
	}break;
	case eItemPlaceRuck:{
		ASSERT_FMT(InRuck(pIItem), "CInventory::DropItem: InRuck(pIItem): %s, owner [%s]", pObj->cName().c_str(), m_pOwner->Name());
		m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
	}break;
	case eItemPlaceSlot:{
			TryRestoreSlot(pIItem);
			ASSERT_FMT(InSlot(pIItem), "CInventory::DropItem: InSlot(pIItem): [%s], id: [%u], owner [%s]", pObj->cName().c_str(), pObj->ID(), m_pOwner->Name());
			if(m_iActiveSlot == pIItem->GetSlot()) 
				Activate	(NO_ACTIVE_SLOT);

			m_slots[pIItem->GetSlot()].m_pIItem = NULL;	

			pIItem->object().processing_deactivate();
		}break;
	default:
		NODEFAULT;
	};

	TIItemContainer::iterator	it = std::find(m_all.begin(), m_all.end(), pIItem);
	if ( it != m_all.end())
		m_all.erase				(it);
	else
		Msg						("! CInventory::Drop item not found in inventory!!!");

	pIItem->OnMoveOut(pIItem->m_eItemPlace);

	pIItem->m_pCurrentInventory = NULL;

	m_pOwner->OnItemDrop			(smart_cast<CInventoryItem*>(pObj));

	CalcTotalWeight					();
	CalcTotalVolume					();
	InvalidateState					();
	m_drop_last_frame				= true;
	return							true;
}

//положить вещь в слот
bool CInventory::Slot(PIItem pIItem, bool bNotActivate) 
{
	VERIFY(pIItem);
	//Msg("To Slot %s[%d]", *pIItem->object().cName(), pIItem->object().ID());
	
	if(!CanPutInSlot(pIItem)) 
	{
		/*
		Msg("there is item %s[%d,%x] in slot %d[%d,%x]", 
				*m_slots[pIItem->GetSlot()].m_pIItem->object().cName(), 
				m_slots[pIItem->GetSlot()].m_pIItem->object().ID(), 
				m_slots[pIItem->GetSlot()].m_pIItem, 
				pIItem->GetSlot(), 
				pIItem->object().ID(),
				pIItem);
		*/
		if(m_slots[pIItem->GetSlot()].m_pIItem == pIItem && !bNotActivate )
			if (activate_slot(pIItem->GetSlot()))
				Activate(pIItem->GetSlot());

		return false;
	}

	/*
	Вещь была в слоте. Да, такое может быть :).
	Тут необходимо проверять именно так, потому что
	GetSlot вернет новый слот, а не старый. Real Wolf.
	*/
	for (u32 i = 0; i < m_slots.size(); i++)
		if (m_slots[i].m_pIItem == pIItem)
		{
			if (i == m_iActiveSlot)
				Activate(NO_ACTIVE_SLOT);
			m_slots[i].m_pIItem = NULL;
			break;
		}

	m_slots[pIItem->GetSlot()].m_pIItem = pIItem;

	//удалить из рюкзака или пояса
	if (InRuck(pIItem)) m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
	if (InBelt(pIItem)) m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));
	if (InVest(pIItem)) m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));

	if ( ( m_iActiveSlot == pIItem->GetSlot() || ( m_iActiveSlot == NO_ACTIVE_SLOT && m_iNextActiveSlot == NO_ACTIVE_SLOT ) ) && !bNotActivate )
		if (activate_slot(pIItem->GetSlot()))
			Activate(pIItem->GetSlot());

	auto PrevPlace = pIItem->m_eItemPlace;
	pIItem->m_eItemPlace = eItemPlaceSlot;
	m_pOwner->OnItemSlot(pIItem, PrevPlace);
	pIItem->OnMoveToSlot(PrevPlace);
	
	pIItem->object().processing_activate();

	CalcTotalVolume();

	return true;
}

bool CInventory::Belt(PIItem pIItem) 
{
	if(!CanPutInBelt(pIItem))	return false;
	
	//вещь была в слоте
	bool in_slot = InSlot(pIItem);
	if(in_slot) {
		if(m_iActiveSlot == pIItem->GetSlot()) Activate(NO_ACTIVE_SLOT);
		m_slots[pIItem->GetSlot()].m_pIItem = NULL;
	}
	if (InRuck(pIItem)) m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
	if (InVest(pIItem)) m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));

	m_belt.push_back(pIItem);

	CalcTotalWeight();
	CalcTotalVolume();
	InvalidateState();

	auto PrevPlace = pIItem->m_eItemPlace;
	pIItem->m_eItemPlace = eItemPlaceBelt;
	m_pOwner->OnItemBelt(pIItem, PrevPlace);
	pIItem->OnMoveToBelt(PrevPlace);

	if(in_slot)
		pIItem->object().processing_deactivate();

	pIItem->object().processing_activate();

	return true;
}

bool CInventory::Vest(PIItem pIItem)
{
	if (!CanPutInVest(pIItem))	return false;

	//вещь была в слоте
	bool in_slot = InSlot(pIItem);
	if (in_slot)
	{
		if (m_iActiveSlot == pIItem->GetSlot()) Activate(NO_ACTIVE_SLOT);
		m_slots[pIItem->GetSlot()].m_pIItem = NULL;
	}

	if (InRuck(pIItem)) m_ruck.erase(std::find(m_ruck.begin(), m_ruck.end(), pIItem));
	if (InBelt(pIItem)) m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));

	m_vest.push_back(pIItem);

	CalcTotalWeight();
	CalcTotalVolume();
	InvalidateState();

	auto PrevPlace = pIItem->m_eItemPlace;
	pIItem->m_eItemPlace = eItemPlaceVest;
	m_pOwner->OnItemVest(pIItem, PrevPlace);
	pIItem->OnMoveToVest(PrevPlace);

	if (in_slot)
		pIItem->object().processing_deactivate();

	pIItem->object().processing_activate();

	return true;
}

bool CInventory::Ruck(PIItem pIItem, bool skip_volume_check)
{
	if(!CanPutInRuck(pIItem, skip_volume_check)) return false;
	
	bool in_slot = InSlot(pIItem);
	//вещь была в слоте
	if(in_slot) 
	{
		if(m_iActiveSlot == pIItem->GetSlot()) Activate(NO_ACTIVE_SLOT);
		m_slots[pIItem->GetSlot()].m_pIItem = NULL;
	}
	if (InBelt(pIItem)) m_belt.erase(std::find(m_belt.begin(), m_belt.end(), pIItem));
	if (InVest(pIItem)) m_vest.erase(std::find(m_vest.begin(), m_vest.end(), pIItem));
	
	m_ruck.push_back								(pIItem); 
	
	CalcTotalWeight									();
	CalcTotalVolume									();
	InvalidateState									();

	EItemPlace prevPlace = pIItem->m_eItemPlace;
	m_pOwner->OnItemRuck							(pIItem, prevPlace);
	pIItem->m_eItemPlace							= eItemPlaceRuck;

	pIItem->OnMoveToRuck(prevPlace);

	if(in_slot)
		pIItem->object().processing_deactivate();

	return true;
}

void CInventory::Activate_deffered	(u32 slot, u32 _frame)
{
	 m_iLoadActiveSlot			= slot;
	 m_iLoadActiveSlotFrame		= _frame;
}

bool CInventory::Activate(u32 slot, EActivationReason reason, bool bForce, bool now ) 
{	
	if(	m_ActivationSlotReason==eKeyAction	&& reason==eImportUpdate )
		return false;

	bool res = false;

	if(Device.dwFrame == m_iLoadActiveSlotFrame) 
	{
		 if( (m_iLoadActiveSlot == slot) && m_slots[slot].m_pIItem )
			m_iLoadActiveSlotFrame = u32(-1);
		 else
			{
			 res = false;
			 goto _finish;
			}
	}

	if( (slot!=NO_ACTIVE_SLOT && m_slots[slot].IsBlocked()) && !bForce)
	{
		res = false;
		goto _finish;
	}

	ASSERT_FMT(slot == NO_ACTIVE_SLOT || slot<m_slots.size(), "wrong slot number: [%u]", slot);

	if(slot != NO_ACTIVE_SLOT && !m_slots[slot].m_bVisible) 
	{
		res = false;
		goto _finish;
	}
	
	if(m_iActiveSlot == slot && m_iActiveSlot != NO_ACTIVE_SLOT && m_slots[m_iActiveSlot].m_pIItem)
	{
		m_slots[m_iActiveSlot].m_pIItem->Activate();
	}

	if(	m_iActiveSlot == slot || 
		(m_iNextActiveSlot == slot &&
		 m_iActiveSlot != NO_ACTIVE_SLOT &&
		 m_slots[m_iActiveSlot].m_pIItem &&
		 m_slots[m_iActiveSlot].m_pIItem->cast_hud_item() &&
		 m_slots[m_iActiveSlot].m_pIItem->cast_hud_item()->IsHiding()
		 )
	   )
	{
		res = false;
		goto _finish;
	}

	//активный слот не выбран
	if(m_iActiveSlot == NO_ACTIVE_SLOT)
	{
		if(m_slots[slot].m_pIItem)
		{
			m_iNextActiveSlot		= slot;
			m_ActivationSlotReason	= reason;
			res = true;
			goto _finish;
		}
		else 
		{
			//if(slot==GRENADE_SLOT)//fake for grenade
			//{
			//	PIItem gr = SameSlot(GRENADE_SLOT, NULL, true);
			//	if(gr)
			//	{
			//		Slot(gr);
			//		goto _finish;
			//	}else
			//	{
			//		res = false;
			//		goto _finish;
			//	}

			//}else
			{
				res = false;
				goto _finish;
			}
		}
	}
	//активный слот задействован
	else if(slot == NO_ACTIVE_SLOT || m_slots[slot].m_pIItem)
	{
		if ( m_slots[ m_iActiveSlot ].m_pIItem ) {
		  m_slots[ m_iActiveSlot ].m_pIItem->Deactivate( now || ( slot != NO_ACTIVE_SLOT && m_slots[ slot ].maySwitchFast() ) );
		}

		m_iNextActiveSlot		= slot;
		m_ActivationSlotReason	= reason;
	
		res = true;
		goto _finish;
	}

	_finish:

	if(res)
		m_ActivationSlotReason	= reason;
	return res;
}


PIItem CInventory::ItemFromSlot(u32 slot) const
{
	VERIFY(NO_ACTIVE_SLOT != slot);
	return m_slots[slot].m_pIItem;
}

void CInventory::SendActionEvent(s32 cmd, u32 flags) 
{
	CActor *pActor = smart_cast<CActor*>(m_pOwner);
	if (!pActor) return;

	NET_Packet		P;
	pActor->u_EventGen		(P,GE_INV_ACTION, pActor->ID());
	P.w_s32					(cmd);
	P.w_u32					(flags);
	P.w_s32					(pActor->GetZoomRndSeed());
	P.w_s32					(pActor->GetShotRndSeed());
	pActor->u_EventSend		(P, net_flags(TRUE, TRUE, FALSE, TRUE));
};

bool CInventory::Action(s32 cmd, u32 flags) 
{
	CActor *pActor = smart_cast<CActor*>(m_pOwner);
	
	if (pActor)
	{
		switch(cmd)
		{
			case kWPN_FIRE:
			{
				pActor->SetShotRndSeed();
			}break;
			case kWPN_ZOOM : 
			{
				pActor->SetZoomRndSeed();
			}break;
		};
	};

	if (g_pGameLevel && OnClient() && pActor) {
		switch(cmd)
		{
		case kUSE:
			{
			}break;
		
		case kDROP:		
		
			{
				SendActionEvent(cmd, flags);
				return true;
			}break;

		case kWPN_NEXT:
		case kWPN_RELOAD:
		case kWPN_FIRE:
		case kWPN_FUNC:
		case kWPN_FIREMODE_NEXT:
		case kWPN_FIREMODE_PREV:
		case kWPN_ZOOM : 
		case kTORCH:
		case kNIGHT_VISION:
			{
				SendActionEvent(cmd, flags);
			}break;
		}
	}


	if (m_iActiveSlot < m_slots.size() && 
			m_slots[m_iActiveSlot].m_pIItem && 
			m_slots[m_iActiveSlot].m_pIItem->Action(cmd, flags)) 
											return true;
	bool b_send_event = false;
	switch(cmd) 
	{
	case kWPN_1:
	case kWPN_2:
	case kWPN_3:
	case kWPN_4:
	case kWPN_5:
	case kWPN_6:
       {
			if(flags&CMD_START)
			{
				if((int)m_iActiveSlot == cmd - kWPN_1 && m_slots[m_iActiveSlot].m_pIItem )
					b_send_event = Activate(NO_ACTIVE_SLOT);
				else				
					b_send_event = Activate(cmd - kWPN_1, eKeyAction);
			}
		}break;
	case kWPN_7:
	{
		if (flags & CMD_START)
		{
			if ((int)m_iActiveSlot == ARTEFACT_SLOT &&
				m_slots[m_iActiveSlot].m_pIItem){
				b_send_event = Activate(NO_ACTIVE_SLOT);
			}else {
				b_send_event = Activate(ARTEFACT_SLOT, eKeyAction);
			}
		}
	}break;
	case kUSE_QUICK_SLOT_0:
	case kUSE_QUICK_SLOT_1:
	case kUSE_QUICK_SLOT_2:
	case kUSE_QUICK_SLOT_3:
	{
		PIItem itm = NULL;

		switch (cmd)
		{
		case kUSE_QUICK_SLOT_0:
			itm = m_slots[QUICK_SLOT_0].m_pIItem;
			break;
		case kUSE_QUICK_SLOT_1:
			itm = m_slots[QUICK_SLOT_1].m_pIItem;
			break;
		case kUSE_QUICK_SLOT_2:
			itm = m_slots[QUICK_SLOT_2].m_pIItem;
			break;
		case kUSE_QUICK_SLOT_3:
			itm = m_slots[QUICK_SLOT_3].m_pIItem;
			break;
		}

		if (flags & CMD_START)
		{
			auto pHudItem = smart_cast<CHudItem*>(itm);
			//если в слоте предмет без худа то и активировать его не нужно
			if (!pHudItem) return false;

			if (m_iActiveSlot == itm->GetSlot())
				b_send_event = Activate(NO_ACTIVE_SLOT);
			else
				b_send_event = Activate(itm->GetSlot(), eKeyAction);
		}
	}break;
	}

	if(b_send_event && g_pGameLevel && OnClient() && pActor)
			SendActionEvent(cmd, flags);

	return false;
}


void CInventory::Update()
{
	// Да, KRodin писал это в здравом уме и понимает, что это полная хуйня. Но ни одного нормального решения придумать не удалось. Может потом какие-то мысли появятся.
	// А проблема вся в том, что арты и костюм выходят в онлайн в хаотичном порядке. И получается, что арты на пояс уже пытаются залезть, а костюма вроде как ещё нет,
	// соотв. и слотов под арты как бы нет. Вот поэтому до первого апдейта CInventory актора считаем, что все слоты та пояс доступны.
	// По моим наблюдениям на момент первого апдейта CInventory, все предметы в инвентаре актора уже вышли в онлайн.
	++UpdatesCount;

	bool bActiveSlotVisible;
	
	if(m_iActiveSlot == NO_ACTIVE_SLOT || 
		!m_slots[m_iActiveSlot].m_pIItem ||
        !m_slots[m_iActiveSlot].m_pIItem->cast_hud_item() ||
		m_slots[m_iActiveSlot].m_pIItem->cast_hud_item()->IsHidden())
	{ 
		bActiveSlotVisible = false;
	}
	else 
	{
		bActiveSlotVisible = true;
	}

	if(m_iNextActiveSlot != m_iActiveSlot && !bActiveSlotVisible)
	{
		if(m_iNextActiveSlot != NO_ACTIVE_SLOT &&
			m_slots[m_iNextActiveSlot].m_pIItem)
			m_slots[m_iNextActiveSlot].m_pIItem->Activate();

		m_iActiveSlot = m_iNextActiveSlot;
	}
	UpdateDropTasks		();
	UpdateVolumeDropOut	();
}

void CInventory::UpdateDropTasks()
{
	for (const auto& item : m_all){
		PIItem itm = smart_cast<PIItem>(item); VERIFY(itm);
		UpdateDropItem(itm);
	}

	if (m_drop_last_frame)
	{
		m_drop_last_frame			= false;
		m_pOwner->OnItemDropUpdate	();
	}
}

void CInventory::UpdateDropItem(PIItem pIItem)
{
	if( pIItem->GetDropManual() ){
		pIItem->SetDropManual(FALSE);
		if ( OnServer() ) {
			NET_Packet					P;
			pIItem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
			P.w_u16						(u16(pIItem->object().ID()));
			pIItem->object().u_EventSend(P);
		}
	}// dropManual
}

//ищем на поясе гранату такоже типа
PIItem CInventory::Same(const PIItem pIItem, bool bSearchRuck) const
{
	if (bSearchRuck) {
		for (const auto& item : m_ruck) {
			if ((item != pIItem) &&
				!xr_strcmp(item->object().cNameSect(),
					pIItem->object().cNameSect()))
				return item;
		}
	} else {
		for (const auto& item : m_vest) {
			if ((item != pIItem) &&
				!xr_strcmp(item->object().cNameSect(),
					pIItem->object().cNameSect()))
				return item;
		}

		for (const auto& item : m_belt) {
			if ((item != pIItem) &&
				!xr_strcmp(item->object().cNameSect(),
					pIItem->object().cNameSect()))
				return item;
		}

		for (u32 i = 0; i < m_slots.size(); ++i) {
			const auto _item = m_slots[i].m_pIItem;
			if (!_item) continue;

			if ((_item != pIItem) &&
				!xr_strcmp(_item->object().cNameSect(),
					pIItem->object().cNameSect())) {
				return _item;
			}
		}
	}

	return NULL;
}

//ищем на поясе гранату для слота
PIItem CInventory::SameSlot(const u32 slot, PIItem pIItem, bool bSearchRuck) const
{
	if(slot == NO_ACTIVE_SLOT) 	return NULL;

	if (bSearchRuck) {
		for (const auto& _item : m_ruck) {
			if (_item != pIItem && _item->GetSlot() == slot) {
				return _item;
			}
		}
	} else {
		for (const auto& _item : m_vest) {
			if (_item != pIItem && _item->GetSlot() == slot) {
				return _item;
			}
		}

		for (const auto& _item : m_belt) {
			if (_item != pIItem && _item->GetSlot() == slot) {
				return _item;
			}
		}

		for (u32 i = 0; i < m_slots.size(); ++i) {
			const auto _item = m_slots[i].m_pIItem;
			if (!_item) continue;

			if (_item != pIItem && _item->GetSlot() == slot) {
				return _item;
			}
		}
	}

	return NULL;
}

//ищем на поясе гранату для слота
PIItem CInventory::SameGrenade(PIItem pIItem, bool bSearchRuck) const
{
	if (bSearchRuck) {
		for (const auto& _item : m_ruck) {
			if (_item != pIItem && smart_cast<CGrenade*>(_item)) {
				return _item;
			}
		}
	} else {
		for (const auto& _item : m_vest) {
			if (_item != pIItem && smart_cast<CGrenade*>(_item)) {
				return _item;
			}
		}

		for (const auto& _item : m_belt) {
			if (_item != pIItem && smart_cast<CGrenade*>(_item)) {
				return _item;
			}
		}

		for (u32 i = 0; i < m_slots.size(); ++i) {
			const auto _item = m_slots[i].m_pIItem;
			if (!_item) continue;

			if (_item != pIItem && smart_cast<CGrenade*>(_item)) {
				return _item;
			}
		}
	}

	return NULL;
}

//найти в инвенторе вещь с указанным именем
PIItem CInventory::Get(const char *name, bool bSearchRuck) const
{
	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;
	
	for(const auto& item : list) {
		if(item && !xr_strcmp(item->object().cNameSect(), name) && item->Useful())
				return item;
	}


	return NULL;
}

PIItem CInventory::Get(CLASS_ID cls_id, bool bSearchRuck) const
{
	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;
	
	for(const auto& item : list){
		if(item && item->object().CLS_ID == cls_id && item->Useful())
				return item;
	}
	return NULL;
}

PIItem CInventory::Get(const u16 id, bool bSearchRuck) const
{
	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;

	for(const auto& item : list){
		if(item && item->object().ID() == id)
			return item;
	}
	return NULL;
}

//search both (ruck and belt)
PIItem CInventory::GetAny(const char *name) const
{
	PIItem itm = Get(name, false);
	if(!itm)
		itm = Get(name, true);
	return itm;
}

PIItem CInventory::GetAmmo(const char *name, bool forActor) const
{
	bool include_ruck = !forActor || !psActorFlags.test(AF_ITEMS_FROM_BELT) || Actor()->IsRuckAmmoPlacement();

	PIItem itm = Get(name, include_ruck);
	if (!include_ruck && !itm)
		itm = GetFromSlots(name);
	return itm;
}

PIItem CInventory::item(CLASS_ID cls_id) const
{
	for(const auto& item : m_all){
		if(item && item->object().CLS_ID == cls_id &&
			item->Useful())
			return item;
	}
	return NULL;
}

float CInventory::TotalWeight() const
{
	VERIFY(m_fTotalWeight>=0.f);
	return m_fTotalWeight;
}


float CInventory::CalcTotalWeight()
{
	float weight{};
	for (const auto& item : m_all) {
		weight += item->Weight();
	}

	m_fTotalWeight = weight;
	return m_fTotalWeight;
}

float CInventory::TotalVolume() const
{
	VERIFY(m_fTotalVolume >= 0.f);
	return m_fTotalVolume;
}

float CInventory::CalcTotalVolume()
{
	float volume{};
	for (const auto& item : m_ruck) {
		volume += item->Volume();
	}

	m_fTotalVolume = volume;
	return m_fTotalVolume;
}

u32 CInventory::dwfGetSameItemCount(LPCSTR caSection, bool SearchAll)
{
	u32			l_dwCount = 0;
	TIItemContainer	&l_list = SearchAll ? m_all : m_ruck;
	for(const auto& item : l_list){
		if (item && !xr_strcmp(item->object().cNameSect(), caSection))
            ++l_dwCount;
	}
	
	return		(l_dwCount);
}
u32		CInventory::dwfGetGrenadeCount(LPCSTR caSection, bool SearchAll)
{
	u32			l_dwCount = 0;
	TIItemContainer	&l_list = SearchAll ? m_all : m_ruck;
	for(const auto& item : l_list){
		if (item && (item->object().CLS_ID == CLSID_GRENADE_F1 || item->object().CLS_ID == CLSID_GRENADE_RGD5))
			++l_dwCount;
	}

	return		(l_dwCount);
}

bool CInventory::bfCheckForObject(ALife::_OBJECT_ID tObjectID)
{
	for(const auto& item : m_all) {
		if (item && item->object().ID() == tObjectID)
			return(true);
	}
	return		(false);
}

CInventoryItem *CInventory::get_object_by_id(ALife::_OBJECT_ID tObjectID)
{
	for(const auto& item : m_all) {
		if (item && item->object().ID() == tObjectID)
			return	(item);
	}
	return		(0);
}

//скушать предмет 
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
bool CInventory::Eat(PIItem pIItem, CInventoryOwner* eater)
{
//	R_ASSERT(pIItem->m_pCurrentInventory==this);
	//устанаовить съедобна ли вещь
	CEatableItem* pItemToEat = smart_cast<CEatableItem*>(pIItem);
	R_ASSERT				(pItemToEat);

	auto eatem_eater = eater ? eater : m_pOwner;

	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(eatem_eater);
	R_ASSERT				(entity_alive);

	if (Actor()->m_inventory == this)
		Actor()->callback(GameObject::eOnBeforeUseItem)((smart_cast<CGameObject*>(pIItem))->lua_game_object());
	
	pItemToEat->UseBy		(entity_alive);

	if(Actor()->m_inventory == this)
		Actor()->callback(GameObject::eUseObject)((smart_cast<CGameObject*>(pIItem))->lua_game_object());

	if(pItemToEat->Empty() && entity_alive->Local())
	{
		auto object = pIItem->cast_game_object();

		NET_Packet					P;
		CGameObject::u_EventGen		(P,GE_OWNERSHIP_REJECT, object->H_Parent()->ID());
		P.w_u16						(pIItem->object().ID());
		CGameObject::u_EventSend	(P);

		CGameObject::u_EventGen		(P,GE_DESTROY, object->ID());
		CGameObject::u_EventSend	(P);

		return		false;
	}
	return			true;
}

bool CInventory::InSlot(PIItem pIItem) const{
	if(pIItem->GetSlot() < m_slots.size() && 
		m_slots[pIItem->GetSlot()].m_pIItem == pIItem)
		return true;
	return false;
}
bool CInventory::InBelt(PIItem pIItem) const{
	if(Get(pIItem->object().ID(), false)) return true;
	return false;
}
bool CInventory::InVest(PIItem pIItem) const {
	return (std::find(m_vest.begin(), m_vest.end(), pIItem) != m_vest.end());
}
bool CInventory::InRuck(PIItem pIItem) const{
	if(Get(pIItem->object().ID(), true)) return true;
	return false;
}


bool CInventory::CanPutInSlot(PIItem pIItem, bool check_all) const
{
	if(!m_bSlotsUseful) return false;

	if(!GetOwner()->CanPutInSlot(pIItem, pIItem->GetSlot())) return false;

	if (!IsSlotAllowed(pIItem->GetSlot()) && !check_all) return false;

	if(pIItem->GetSlot() < m_slots.size() && 
		!m_slots[pIItem->GetSlot()].m_pIItem 
		&& IsSlotAllowed(pIItem->GetSlot()))
		return true;

	//оглянемо усі доступні слоти
	//якщо знайдемо вільний - його й встановимо цільовим
	//з цим кодом є проблеми - він встановлює слот при спробі забрати ітем зі слоту чужого інвентаря
	//тому коли виконується CInventory::DropItem предмет повертає GetSlot() != слоту в якому є предмет
	if (check_all) {
		for (const auto& slot : pIItem->GetSlots()) {
			if (CanPutInSlot(pIItem, slot)) {
				pIItem->SetSlot(slot);
				return true;
			}
		}
	}
	
	return false;
}

//KRodin: добавлено специально для равнозначных слотов.
bool CInventory::CanPutInSlot(PIItem pIItem, u8 slot) const
{
	if (!m_bSlotsUseful)
		return false;

	if (!GetOwner()->CanPutInSlot(pIItem, slot))
		return false;

	if (!IsSlotAllowed(slot)) return false;

	if (slot < m_slots.size() && !m_slots[slot].m_pIItem)
		return true;

	return false;
}

//проверяет можем ли поместить вещь на пояс,
//при этом реально ничего не меняется
bool CInventory::CanPutInBelt(PIItem pIItem) const
{
	if (InBelt(pIItem))							return false;
	if (!m_bBeltUseful)							return false;
	if (!pIItem || !pIItem->Belt())				return false;
	if (!IsAllItemsLoaded())					return true;
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (pActor && !pActor->GetWarbelt())		return false;
	if (!InVest(pIItem) && HasSameModuleEquiped(pIItem))
		return false;
//	if (m_belt.size() >= BeltWidth() * BeltHeight())return false;
	auto belt = static_cast<TIItemContainer>(m_belt);

	return FreeRoom_inBelt(belt, pIItem, BeltWidth(), BeltHeight());
}

bool CInventory::CanPutInVest(PIItem pIItem) const
{
	if (InVest(pIItem))							return false;
	if (!m_bVestUseful)							return false;
	if (!pIItem || !pIItem->Vest())				return false;
	if (!IsAllItemsLoaded())					return true;
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (pActor && !pActor->GetVest())			return false;
	if (!InBelt(pIItem) && HasSameModuleEquiped(pIItem))			
		return false;
//	if (m_vest.size() >= VestWidth() * VestHeight())return false;
	auto vest = static_cast<TIItemContainer>(m_vest);

	return FreeRoom_inVest(vest, pIItem, VestWidth(), VestHeight());
}
//проверяет можем ли поместить вещь в рюкзак,
//при этом реально ничего не меняется
bool CInventory::CanPutInRuck(PIItem pIItem, bool skip_volume_check) const
{
	if(InRuck(pIItem)) return false;

	if (skip_volume_check) return true;

	//для НПЦ може бути анлімітед обсяг
	if (m_pOwner->IsVolumeUnlimited()) return true;

	if (OwnerIsActor() && !IsAllItemsLoaded())	return true;

	if (!pIItem->IsQuestItem() &&
		(TotalVolume() + pIItem->Volume() > m_pOwner->MaxCarryVolume()))
		return false;

	return true;
}

u32	CInventory::dwfGetObjectCount()
{
	return		(m_all.size());
}

CInventoryItem	*CInventory::tpfGetObjectByIndex(int iIndex)
{
	if ((iIndex >= 0) && (iIndex < (int)m_all.size())) {
		TIItemContainer	&l_list = m_all;
		int			i = 0;
		for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it, ++i) 
			if (i == iIndex)
                return	(*l_it);
	}
	else {
		ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"invalid inventory index!");
		return	(0);
	}
	R_ASSERT	(false);
	return		(0);
}

CInventoryItem* CInventory::GetItemFromInventory(LPCSTR SectName)
{
	auto It = std::find_if(m_all.begin(), m_all.end(), [SectName](const auto* pInvItm) { return pInvItm->object().cNameSect() == SectName; });
	if (It != m_all.end())
		return *It;

	return nullptr;
}


bool CInventory::CanTakeItem(CInventoryItem *inventory_item) const
{
	if (inventory_item->object().getDestroy()) return false;

	if(!inventory_item->CanTake()) return false;

	for(TIItemContainer::const_iterator it = m_all.begin(); it != m_all.end(); it++)
		if((*it)->object().ID() == inventory_item->object().ID()) break;
	VERIFY3(it == m_all.end(), "item already exists in inventory",*inventory_item->object().cName());

	//перевантаження не враховуєтсья для актора
	if(!OwnerIsActor() && (TotalWeight() + inventory_item->Weight() > m_pOwner->MaxCarryWeight()))
		return false;

	if (!CanPutInSlot(inventory_item, true) && 
		!CanPutInBelt(inventory_item) && 
		!CanPutInVest(inventory_item) && 
		!CanPutInRuck(inventory_item))
		return	false;

	return	true;
}

void  CInventory::AddAvailableItems(TIItemContainer& items_container, bool for_trade) const
{
	for(const auto& item : m_ruck){
		if(!for_trade || item->CanTrade())
			items_container.push_back(item);
	}

	if(m_bBeltUseful){
		for(const auto& item : m_belt) {
			if(!for_trade || item->CanTrade())
				items_container.push_back(item);
		}
	}

	if (m_bVestUseful) {
		for (const auto& item : m_vest) {
			if (!for_trade || item->CanTrade())
				items_container.push_back(item);
		}
	}
	
	if(m_bSlotsUseful){
		for(const auto& slot : m_slots){
			if(slot.m_pIItem && !slot.m_bPersistent && 
				(!for_trade || slot.m_pIItem->CanTrade())){
				items_container.push_back(slot.m_pIItem);
			}
		}
	}		
}

bool CInventory::isBeautifulForActiveSlot	(CInventoryItem *pIItem)
{
	for(const auto& slot : m_slots) {
		if (slot.m_pIItem && slot.m_pIItem->IsNecessaryItem(pIItem))
			return		(true);
	}
	return				(false);
}

void CInventory::Items_SetCurrentEntityHud(bool current_entity)
{
	for(const auto& item : m_all){
		CWeapon* pWeapon = smart_cast<CWeapon*>(item);
		if (pWeapon){
			pWeapon->InitAddons();
			pWeapon->UpdateAddonsVisibility();
		}
	}
};

//call this only via Actor()->SetWeaponHideState()
void CInventory::SetSlotsBlocked( u16 mask, bool bBlock, bool now )
{
	bool bChanged = false;
	for(int i =0; i<SLOTS_TOTAL; ++i)
	{
		if(mask & (1<<i))
		{
			bool bCanBeActivated = m_slots[i].CanBeActivated();
			if(bBlock){
				++m_slots[i].m_blockCounter;
				VERIFY2(m_slots[i].m_blockCounter< 5,"block slots overflow");
			}else{
				--m_slots[i].m_blockCounter;
				VERIFY2(m_slots[i].m_blockCounter>-5,"block slots underflow");
			}
			if(bCanBeActivated != m_slots[i].CanBeActivated())
				bChanged = true;
		}
	}
	if(bChanged)
	{
		u32 ActiveSlot		= GetActiveSlot();
		u32 PrevActiveSlot	= GetPrevActiveSlot();

		if ( PrevActiveSlot == NO_ACTIVE_SLOT ) {
		  if ( GetNextActiveSlot() != NO_ACTIVE_SLOT && m_slots[GetNextActiveSlot()].m_pIItem && m_slots[GetNextActiveSlot()].m_pIItem->cast_hud_item() && m_slots[GetNextActiveSlot()].m_pIItem->cast_hud_item()->IsShowing() ) {
		    ActiveSlot = GetNextActiveSlot();
		    SetActiveSlot( GetNextActiveSlot() );
		    m_slots[ ActiveSlot ].m_pIItem->Activate( true );
		  }
		}
		else if ( m_slots[PrevActiveSlot].m_pIItem && m_slots[PrevActiveSlot].m_pIItem->cast_hud_item() && m_slots[PrevActiveSlot].m_pIItem->cast_hud_item()->IsHiding() ) {
		  m_slots[ PrevActiveSlot ].m_pIItem->Deactivate( true );
		  ActiveSlot = NO_ACTIVE_SLOT;
		  SetActiveSlot( NO_ACTIVE_SLOT );
		}

		if(ActiveSlot==NO_ACTIVE_SLOT)
		{//try to restore hidden weapon
			if(PrevActiveSlot!=NO_ACTIVE_SLOT && m_slots[PrevActiveSlot].CanBeActivated())
				if ( Activate( PrevActiveSlot, eGeneral, false, now ) )
					SetPrevActiveSlot(NO_ACTIVE_SLOT);
		}else
		{//try to hide active weapon
			if(!m_slots[ActiveSlot].CanBeActivated() )
				if ( Activate( NO_ACTIVE_SLOT, eGeneral, false, now ) )
					SetPrevActiveSlot(ActiveSlot);
		}
	}
}


void CInventory::Iterate( bool bSearchRuck, std::function<bool( const PIItem )> callback ) const {
  const auto& list = bSearchRuck ? m_ruck : m_belt;
  for ( const auto& it : list )
    if ( callback( it ) ) break;
}


void CInventory::IterateAmmo( bool bSearchRuck, std::function<bool( const PIItem )> callback ) const {
	
	if (bSearchRuck) {
		for (const auto& item : m_ruck) {
			const auto* ammo = smart_cast<CWeaponAmmo*>(item);
			if (ammo && item->Useful() && callback(item))
				break;
		}
	} else {
		for (const auto& item : m_vest) {
			const auto* ammo = smart_cast<CWeaponAmmo*>(item);
			if (ammo && item->Useful() && callback(item))
				break;
		}
		for (const auto& item : m_belt) {
			const auto* ammo = smart_cast<CWeaponAmmo*>(item);
			if (ammo && item->Useful() && callback(item))
				break;
		}
		for (u32 i = 0; i < m_slots.size(); ++i){
			const auto item = m_slots[i].m_pIItem;
			const auto* ammo = smart_cast<CWeaponAmmo*>(item);
			if (ammo && item->Useful() && callback(item))
				break;
		}
	}
}


PIItem CInventory::GetAmmoByLimit(const char* name, bool forActor, bool limit_max) const {
	PIItem box		= nullptr;
	u32    limit	= 0;

	auto callback = [&](const auto pIItem) -> bool {
		if (!xr_strcmp(pIItem->object().cNameSect(), name)) {

			const auto* ammo			= smart_cast<CWeaponAmmo*>(pIItem);
			const bool size_fits_limit	= (ammo->m_boxCurr == (limit_max ? ammo->m_boxSize : 1));
			const bool update_limit		= limit_max ? ammo->m_boxCurr > limit : (limit == 0 || ammo->m_boxCurr < limit);

			if (size_fits_limit) {
				box = pIItem;
				return true;
			}
			if (update_limit) {
				box = pIItem;
				limit = ammo->m_boxCurr;
			}
		}
		return false;
	};

	bool include_ruck = !forActor || !psActorFlags.test(AF_ITEMS_FROM_BELT) || Actor()->IsRuckAmmoPlacement();

	IterateAmmo(include_ruck, callback);

	return box;
}


int CInventory::GetIndexOnBelt( PIItem pIItem ) const {
  const auto& it = std::find( m_belt.begin(), m_belt.end(), pIItem );
  return it == m_belt.end() ? -1 : std::distance( m_belt.begin(), it );
}


void CInventory::RestoreBeltOrder() {
  std::sort(
    m_belt.begin(), m_belt.end(), []( const auto& a, const auto& b ) {
      return a->GetLoadedBeltIndex() < b->GetLoadedBeltIndex();
    }
  );

  if ( auto pActor = smart_cast<CActor*>( m_pOwner ) )
    pActor->UpdateUIPanels(eItemPlaceBelt);
}


bool CInventory::IsActiveSlotBlocked() const {
	for (const auto& slot : m_slots)
		if (slot.CanBeActivated())
			return false;
	return true;
}

bool CInventory::IsFreeHands()
{
	const auto active_hud_item = smart_cast<CHudItem*>(ActiveItem());

	switch (g_eFreeHands)
	{
	case eFreeHandsOff:
		return true;
		break;
	case eFreeHandsAuto:
		if (active_hud_item && active_hud_item->IsPending())
			return false;
		break;
	case eFreeHandsManual:
		if (active_hud_item && (active_hud_item->IsPending() || !ActiveItem()->IsSingleHanded()))
			return false;
		break;
	}
	return true;
}

void CInventory::TryToHideWeapon(bool b_hide_state, bool b_save_prev_slot)
{
	if (g_eFreeHands == eFreeHandsOff) return;

	if (b_hide_state)
	{
		if (ActiveItem() && !ActiveItem()->IsSingleHanded())
		{
			m_iPrevActiveSlot = b_save_prev_slot ? GetActiveSlot() : NO_ACTIVE_SLOT;
			Activate(NO_ACTIVE_SLOT);
		}
	}
	else
	{
		if (m_iPrevActiveSlot != NO_ACTIVE_SLOT)
		{
			Activate(m_iPrevActiveSlot);
			m_iPrevActiveSlot = NO_ACTIVE_SLOT;
		}
	}
}

//получаем айтем из всего инвентаря или с пояса
PIItem CInventory::GetSame(const PIItem pIItem, bool bSearchRuck) const
{
	if (bSearchRuck) {
		for (const auto& _item : m_ruck) {
			if ((_item != pIItem) &&
				!xr_strcmp(_item->object().cNameSect(),
					pIItem->object().cNameSect())) {
				return _item;
			}
		}
	} else {
		for (const auto& _item : m_vest) {
			if ((_item != pIItem) &&
				!xr_strcmp(_item->object().cNameSect(),
					pIItem->object().cNameSect())) {
				return _item;
			}
		}

		for (const auto& _item : m_belt) {
			if ((_item != pIItem) &&
				!xr_strcmp(_item->object().cNameSect(),
					pIItem->object().cNameSect())) {
				return _item;
			}
		}

		for (u32 i = 0; i < m_slots.size(); ++i) {
			const auto _item = m_slots[i].m_pIItem;
			if (!_item) continue;

			if ((_item != pIItem) &&
				!xr_strcmp(_item->object().cNameSect(),
					pIItem->object().cNameSect())) {
				return _item;
			}
		}
	}

	return NULL;
}

u32 CInventory::GetSameItemCount(LPCSTR caSection, bool SearchRuck)
{
	u32			l_dwCount = 0;
	TIItemContainer& l_list = SearchRuck ? m_ruck : m_belt;
	for (const auto& item : l_list){
		if (item && !xr_strcmp(item->object().cNameSect(), caSection))
			++l_dwCount;
	}
	//помимо пояса еще и в слотах поищем
	/*if (!SearchRuck)*/
	for (const auto& item : m_vest) {
		if (item && !xr_strcmp(item->object().cNameSect(), caSection))
			++l_dwCount;
	}

	for (u32 i = 0; i < m_slots.size(); ++i){
		PIItem l_pIItem = m_slots[i].m_pIItem;
		if (l_pIItem && !xr_strcmp(l_pIItem->object().cNameSect(), caSection))
			++l_dwCount;
	}

	return		(l_dwCount);
}

//найти в слотах вещь с указанным именем
PIItem CInventory::GetFromSlots(const char* name) const
{
	for (const auto& slot : m_slots) {
		auto item = slot.m_pIItem;
		if (item && item->Useful() && !xr_strcmp(item->object().cNameSect(), name))
			return item;
	}
	return nullptr;
}

void CInventory::TryAmmoCustomPlacement(CInventoryItem* pIItem)
{
	auto pAmmo = smart_cast<CWeaponAmmo*>(pIItem);
	if (!pAmmo) return;

	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (!pActor) return;

	auto pWeapon = smart_cast<CWeapon*>(ActiveItem());
	if (!pWeapon) return;

	//Msg("ammo [%s] with ID [%d] has taken", pIItem->object().cNameSect().c_str(), pIItem->object().ID());

	if (psActorFlags.test(AF_ITEMS_FROM_BELT) && pWeapon->IsAmmoWasSpawned() && !pActor->IsRuckAmmoPlacement()) { //если включены патроны с пояса, то для боеприпасов актора, которые спавнятся при разрядке
		//auto pWarbelt	= pActor->GetWarbelt();
		//auto pVest		= pActor->GetVest();
//		bool b_has_drop_pouch = pWarbelt && pWarbelt->HasDropPouch() || pVest && pVest->HasDropPouch();

		if (pAmmo->IsBoxReloadableEmpty() && HasDropPouch()) //якщо пустий магазин та є сумка для скидання - кладемо до рюкзаку
			return;

		if (CanPutInVest(pAmmo)){  //спробуємо до розгрузки
			//Msg("ammo [%s] with ID [%d] was placed to belt", pIItem->object().cNameSect().c_str(), pIItem->object().ID());
			pAmmo->m_eItemPlace = eItemPlaceVest;
		}
		else if (CanPutInBelt(pAmmo)) {  //спробуємо до поясу
			//Msg("ammo [%s] with ID [%d] was placed to vest", pIItem->object().cNameSect().c_str(), pIItem->object().ID());
			pAmmo->m_eItemPlace = eItemPlaceBelt;
		}else{ //попробуем определить свободный слот и положить в него
			auto& slots = pAmmo->GetSlots();
			for (u8 i = 0; i < (u8)slots.size(); ++i){
				pAmmo->SetSlot(slots[i]);

				if (CanPutInSlot(pAmmo)){
					//Msg("ammo [%s] with ID [%d] was placed to slot [%d]", pIItem->object().cNameSect().c_str(), pIItem->object().ID(), pIItem->GetSlot());
					pAmmo->m_eItemPlace = eItemPlaceSlot;
					break;
				}
			}
		}

		if (!CanPutInBelt(pAmmo) && !CanPutInVest(pAmmo) && !CanPutInSlot(pAmmo) && !HasDropPouch()) //нікуди не вміщається та немає сумки для скидання - кидаємо на землю
			pAmmo->SetDropManual(TRUE);
	}
	pWeapon->SetAmmoWasSpawned(false);	//сбрасываем флажок спавна патронов
}

u32  CInventory::BeltWidth() const{
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (pActor){
		auto warbelt = pActor->GetWarbelt();
		if(warbelt){
			return warbelt->GetBeltWidth();
		}
	}
	return 0; //m_iMaxBeltWidth;
}

u32  CInventory::BeltHeight() const {
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (pActor) {
		auto warbelt = pActor->GetWarbelt();
		if (warbelt) {
			return warbelt->GetBeltHeight();
		}
	}
	return 0; //m_iMaxBeltWidth;
}

u32  CInventory::VestWidth() const {
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (pActor) {
		auto vest = pActor->GetVest();
		if (vest) {
			return vest->GetVestWidth();
		}
	}
	return 0; //m_iMaxBeltWidth;
}

u32  CInventory::VestHeight() const {
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (pActor) {
		auto vest = pActor->GetVest();
		if (vest) {
			return vest->GetVestHeight();
		}
	}
	return 0; //m_iMaxBeltWidth;
}

bool CInventory::IsAllItemsLoaded() const {
	return UpdatesCount;
}

bool CInventory::OwnerIsActor() const {
	return smart_cast<CActor*>(m_pOwner);
}

void CInventory::DropBeltToRuck(bool skip_volume_check){
	if (!OwnerIsActor() || !IsAllItemsLoaded()) return;
	for(const auto& item : m_belt)
		Ruck(item, skip_volume_check);
}

void CInventory::DropVestToRuck(bool skip_volume_check) {
	if (!OwnerIsActor() || !IsAllItemsLoaded()) return;
	for (const auto& item : m_vest)
		Ruck(item, skip_volume_check);
}

void CInventory::DropSlotsToRuck(u32 min_slot, u32 max_slot) {
	if (!OwnerIsActor() || !IsAllItemsLoaded()) return;
	
	if (max_slot == NO_ACTIVE_SLOT)
		max_slot = min_slot;

	for (const auto& slot : m_slots) {
		if (!slot.m_pIItem) continue;
		auto s = slot.m_pIItem->GetSlot();
		if (min_slot <= s && s <= max_slot)
			Ruck(ItemFromSlot(s));
	}
}

void CInventory::BackpackItemsTransfer(CInventoryItem* container, bool move_to_container) {
	if (!OwnerIsActor() || !IsAllItemsLoaded()) return;
	auto actor_id = m_pOwner->object_id();
	auto container_id = container->object().ID();
	if (move_to_container) {
		for (const auto& item : m_ruck) {
			TransferItem(actor_id, container_id, item->object().ID());
		}
	} else {
		auto cont = smart_cast<CInventoryContainer*>(container);
		if (!cont) return;
		for (const auto& item_id : cont->GetItems()) {
			TransferItem(container_id, actor_id, item_id);
		}
	}
}

void CInventory::TransferItem(u16 from_id, u16 to_id, u16 what_id){
	NET_Packet P;
	//віддаємо річ
	CGameObject::u_EventGen(P, GE_TRANSFER_REJECT, from_id);
	P.w_u16(what_id);
	CGameObject::u_EventSend(P);
	//беремо річ
	CGameObject::u_EventGen(P, GE_TRANSFER_TAKE, to_id);
	P.w_u16(what_id);
	CGameObject::u_EventSend(P);
}

void CInventory::UpdateVolumeDropOut()
{
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (!pActor || m_pOwner->IsVolumeUnlimited() ||
		!IsAllItemsLoaded() || 
		!smart_cast<CEntityAlive*>(m_pOwner)->g_Alive()) return;

	float total_volume = TotalVolume();

	if (TotalVolume() > m_pOwner->MaxCarryVolume()){
		for (const auto& item : m_ruck){
			if (fis_zero(item->Volume()) || item->IsQuestItem())
				continue;
			item->SetDropManual(true);
			total_volume -= item->Volume();
			if (total_volume <= m_pOwner->MaxCarryVolume())
				break;
		}
	}
}

bool CInventory::IsSlotAllowed(u32 slot) const
{
	auto pActor = smart_cast<CActor*>(m_pOwner);
	if (!pActor || !IsAllItemsLoaded()) 
		return true;

	auto outfit		= pActor->GetOutfit();
	//auto warbelt	= pActor->GetWarbelt();
	//auto vest		= pActor->GetVest();
	switch (slot)
	{
	case KNIFE_SLOT:
	case HOLSTER_SLOT:
	case GRENADE_SLOT:
	case ARTEFACT_SLOT:
		return HasModuleForSlot(slot);//(warbelt && warbelt->SlotAllowed(slot));
	break;
	//case GRENADE_SLOT:
	//	return (warbelt && warbelt->SlotAllowed(slot) || vest && vest->SlotAllowed(slot));
	//break;
	case HELMET_SLOT:
		return (!outfit || !outfit->m_bIsHelmetBuiltIn);
	break;
	}

	return true;
}

bool CInventory::HasModuleForSlot(u32 check_slot) const {
	for (const auto& item : m_vest) {
		if (item->GetSlotEnabled() == check_slot)
			return true;
	}
	for (const auto& item : m_belt) {
		if (item->GetSlotEnabled() == check_slot)
			return true;
	}
	return false;
}

bool CInventory::HasSameModuleEquiped(PIItem item) const {
	if (!item->IsModule()) 
		return false;
	for (const auto& _item : m_vest) {
		if (_item->GetSlotEnabled() == item->GetSlotEnabled())
			return true;
	}
	for (const auto& _item : m_belt) {
		if (_item->GetSlotEnabled() == item->GetSlotEnabled())
			return true;
	}
	return false;
}

bool CInventory::HasDropPouch() const {
	for (const auto& item : m_vest) {
		if (item->IsDropPouch())
			return true;
	}
	for (const auto& item : m_belt) {
		if (item->IsDropPouch())
			return true;
	}
	return false;
}

void CInventory::TryRestoreSlot(CInventoryItem* pIItem) {
	//у випадку коли інвентар віддає речі до іншого інвентаря та виконано CanPutInSlot(pIItem, true)
	//pIItem->GetSlot() встановлюється != слоту у якому предмет знаходиться, тому відновимо реальний слот
	for (const auto& slot : pIItem->GetSlots()) {
		if (m_slots[slot].m_pIItem == pIItem && slot != pIItem->GetSlot()) {
			/*Msg("~ %s: [%s], id: [%u], owner [%s], getslot [%u], real slot [%u]",__FUNCTION__, pIItem->object().cName().c_str(), pIItem->object().ID(), m_pOwner->Name(), pIItem->GetSlot(), slot);*/
			pIItem->SetSlot(slot);
			break;
		}
	}
}

bool CInventory::activate_slot(u32 slot)
{
	switch (slot)
	{
	case KNIFE_SLOT:
	case ON_SHOULDER_SLOT:
	case ON_BACK_SLOT:
	case GRENADE_SLOT:
	case HOLSTER_SLOT:
	case BOLT_SLOT:
	case ARTEFACT_SLOT:
	case QUICK_SLOT_0:
	case QUICK_SLOT_1:
	case QUICK_SLOT_2:
	case QUICK_SLOT_3:
		return ItemFromSlot(slot) && ItemFromSlot(slot)->cast_hud_item();
	}

	return false;
}

TIItemContainer CInventory::GetActiveArtefactPlace() const {
	return psActorFlags.test(AF_ARTEFACTS_FROM_ALL) ? m_all : m_belt;
}