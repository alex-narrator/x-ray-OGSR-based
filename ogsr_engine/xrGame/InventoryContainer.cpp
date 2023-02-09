////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_container.cpp
//	Created 	: 12.11.2014
//  Modified 	: 12.12.2014
//	Author		: Alexander Petrov
//	Description : Mobile container class, based on inventory item 
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Level.h"
#include "InventoryContainer.h"

#include "Artifact.h"

void CInventoryContainer::Load(LPCSTR section){
	inherited::Load(section);
}

u32 CInventoryContainer::Cost() const
{
	u32 res = inherited::Cost();
	for (const auto& item_id : m_items) {
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
		if (itm) {
			res += itm->Cost();
		}
	}
	return res;
}

float CInventoryContainer::Weight() const
{
	float res = inherited::Weight();
	for (const auto& item_id : m_items) {
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
		if (itm) {
			res += itm->Weight();
		}
	}
	return res;
}


bool CInventoryContainer::CanTrade() const
{	
	if (!IsEmpty()) // продавать можно только пустым
		return false;
	return inherited::CanTrade();
}

DLL_Pure* CInventoryContainer::_construct()
{
	return inherited::_construct();
}


BOOL CInventoryContainer::net_Spawn(CSE_Abstract* DC)
{
	BOOL res = inherited::net_Spawn(DC);
	processing_activate();
	return res;
}
void CInventoryContainer::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
}

void CInventoryContainer::UpdateCL()
{
	inherited::UpdateCL();
	UpdateDropTasks();
}

void CInventoryContainer::UpdateDropTasks()
{
	for (const auto& item_id : m_items){
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id)); VERIFY(itm);
		UpdateDropItem(itm);
	}
}

void CInventoryContainer::UpdateDropItem(PIItem pIItem)
{
	if (pIItem->GetDropManual()){
		pIItem->SetDropManual(FALSE);
		if (OnServer()){
			NET_Packet					P;
			pIItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
			P.w_u16(u16(pIItem->object().ID()));
			pIItem->object().u_EventSend(P);

			/*Msg("UpdateDropItem for [%s]", pIItem->object().Name_script());*/
		}
	}// dropManual
}

float CInventoryContainer::GetItemEffect(int effect) const {
	bool for_rad = effect == eRadiationRestoreSpeed;
	float res = inherited::GetItemEffect(effect);
	for (const auto& item_id : m_items) {
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
		if (itm) {
			if (for_rad) {
				float rad = itm->GetItemEffect(effect);
				rad *= (1.f - GetHitTypeProtection(ALife::eHitTypeRadiation));
				res += rad;
			}
			else if(smart_cast<CArtefact*>(itm))
				res += itm->GetItemEffect(effect);
		}
	}
	return res;
}

bool CInventoryContainer::NeedForcedDescriptionUpdate() const {
	for (const auto& item_id : m_items) {
		auto item = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
		if (item) {
			if (auto artefact = smart_cast<CArtefact*>(item)) {
				if (!!artefact->GetCondition() && !!artefact->m_fTTLOnDecrease)
					return true;
			}
		}
	}
	return inherited::NeedForcedDescriptionUpdate();
}

float CInventoryContainer::GetContainmentArtefactEffect(int effect) const {
	float res{};
	for (const auto& item_id : m_items) {
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item_id));
		if (itm && smart_cast<CArtefact*>(itm)) {
			res += itm->GetItemEffect(effect);
		}
	}
	return res;
}

float CInventoryContainer::MaxCarryVolume() const {
	return GetItemEffect(eAdditionalVolume);
}