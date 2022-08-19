#include "stdafx.h"
#include "InventoryBox.h"
#include "level.h"
#include "actor.h"
#include "HUDManager.h"
#include "UIGameSP.h"
#include "game_object_space.h"

#include "script_callback_ex.h"
#include "script_game_object.h"

IInventoryBox::IInventoryBox() : m_items ()
{
	m_in_use = false;
	m_items.clear();

	m_fMaxVolume = READ_IF_EXISTS(pSettings, r_float, "inventory_box", "max_volume", .0f);
}

void IInventoryBox::ProcessEvent(CGameObject *O, NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_OWNERSHIP_TAKE:
	case GE_TRANSFER_TAKE:
		{
			u16 id;
            P.r_u16(id);
			CObject* itm = Level().Objects.net_Find(id);  VERIFY(itm);
			m_items.push_back	(id);
			itm->H_SetParent	(O);
			itm->setVisible		(FALSE);
			itm->setEnabled		(FALSE);

			// Real Wolf: Коллбек для ящика на получение предмета. 02.08.2014.		
			if (auto obj = smart_cast<CGameObject*>(itm)) 
			{
				O->callback(GameObject::eOnInvBoxItemTake)(obj->lua_game_object());

				if (m_in_use)
				{
					Actor()->callback(GameObject::eInvBoxItemPlace)(O->lua_game_object(), obj->lua_game_object());
				}
			}

		}break;
	case GE_OWNERSHIP_REJECT:
	case GE_TRANSFER_REJECT:
		{
			u16 id;
            P.r_u16(id);
			CObject* itm = Level().Objects.net_Find(id);
			
			auto it = std::find(m_items.begin(), m_items.end(), id);
			if (it != m_items.end())
				m_items.erase(it);
			else
				Msg("!![%s.GE_TRANSFER_REJECT] object with id [%u] not found! itm: [%p]", __FUNCTION__, id, itm);

			bool just_before_destroy = !P.r_eof() && P.r_u8();
			bool dont_create_shell = (type == GE_TRADE_SELL) || (type == GE_TRANSFER_REJECT) || just_before_destroy;

			if (!itm)
				return;
			/*Msg("~ [%s: REJECT] object [%s] with id [%u] parent [%s]", __FUNCTION__, itm->cName().c_str(), id, itm->H_Parent()->cName().c_str());*/
			itm->H_SetParent	(NULL, dont_create_shell);

			if (Actor() && HUD().GetUI() && HUD().GetUI()->UIGame())
				HUD().GetUI()->UIGame()->ReInitShownUI();

			if (auto obj = smart_cast<CGameObject*>(itm))
			{
				O->callback(GameObject::eOnInvBoxItemDrop)(obj->lua_game_object());

				if (m_in_use)
				{
					Actor()->callback(GameObject::eInvBoxItemTake)(O->lua_game_object(), obj->lua_game_object());
				}
			}
		}break;
	};
}

#include "inventory_item.h"
void IInventoryBox::AddAvailableItems(TIItemContainer& items_container) const
{
	for(const auto& item_id : m_items){
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));VERIFY(itm);
		items_container.push_back(itm);
	}
}

float IInventoryBox::GetCarryVolume() const{
	float res{};
	for (const auto& item_id : m_items) {
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
		res += itm->Volume();
	}
	return res;
}

float IInventoryBox::MaxCarryVolume() const{
	return m_fMaxVolume;
}

bool IInventoryBox::IsVolumeUnlimited() const {
	return fis_zero(MaxCarryVolume());
}

bool IInventoryBox::CanTakeItem(CInventoryItem* inventory_item) const {
	if (IsVolumeUnlimited()) return true;
	return (inventory_item->Volume() + GetCarryVolume() <= MaxCarryVolume());
}

CScriptGameObject* IInventoryBox::GetObjectByName(LPCSTR name)
{	
	const shared_str s_name(name);
	CObjectList &objects = Level().Objects;
	CObject* result = objects.FindObjectByName(name);
	if (result)
	{
		CObject *self = this->object().dcast_CObject();
		if (result->H_Parent() != self)
			 return NULL; // объект существует, но не принадлежит сему контейнеру
	} 
	else
	{		
		for (auto it = m_items.begin(); it != m_items.end(); ++it)
			if (auto obj = objects.net_Find(*it))
			{
				if (obj->cName() == s_name)		return smart_cast<CGameObject*>(obj)->lua_game_object();
				if (obj->cNameSect() == s_name) result = obj; // поиск по секции в качестве резерва
			}

	}
	return result ? smart_cast<CGameObject*>(result)->lua_game_object() : NULL;
}

CScriptGameObject* IInventoryBox::GetObjectByIndex(u32 id)
{
	if (id < m_items.size() )
	{
		u32 obj_id = u32(m_items[id]);
		if (auto obj = smart_cast<CGameObject*>(Level().Objects.net_Find(obj_id)); obj && !obj->getDestroy())
			return obj->lua_game_object();
	}
	return nullptr;
}

u32 IInventoryBox::GetSize() const
 { 
	return m_items.size(); 
}

bool IInventoryBox::IsEmpty() const
{   
	return m_items.empty(); 
}

void CInventoryBox::UpdateCL()
{
	//Msg("UpdateCL() for InventoryBox [%s]", object().Name_script());
	UpdateDropTasks();
}

void CInventoryBox::UpdateDropTasks()
{
	for (const auto& item : m_items){
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item)); VERIFY(itm);
		UpdateDropItem(itm);
	}
}

void CInventoryBox::UpdateDropItem(PIItem pIItem)
{
	if (pIItem->GetDropManual()){
		pIItem->SetDropManual(FALSE);
		if (OnServer()){
			NET_Packet					P;
			pIItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
			P.w_u16(u16(pIItem->object().ID()));
			pIItem->object().u_EventSend(P);

			//Msg("UpdateDropItem for [%s]", pIItem->object().Name_script());
		}
	}// dropManual
}