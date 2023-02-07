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

CInventoryContainer::CInventoryContainer():
		CCustomInventoryBox<CInventoryItemObject>()
{
	open();
}

void CInventoryContainer::Load(LPCSTR section){
	inherited::Load(section);
	m_fMaxVolume = READ_IF_EXISTS(pSettings, r_float, section, "max_volume", .0f);
}

u32 CInventoryContainer::Cost() const
{
	SItemsInfo info;
	CalcItems(info);
	return info.cost + m_cost;
}

float CInventoryContainer::Weight() const
{
	SItemsInfo info;
	CalcItems(info);
	return info.weight + m_weight;
}


u32	CInventoryContainer::CalcItems	(SItemsInfo &info) const
{
	CObjectList &objs = Level().Objects;
	u32  result = 0;
	Memory.mem_fill(&info, 0, sizeof(info));

	for (auto it = m_items.begin(); it != m_items.end(); it++)
	{
		u16 id = *it;
		PIItem itm = smart_cast<PIItem>(objs.net_Find(id));
		if (itm)
		{
			result++;
			info.weight += itm->Weight();
			info.cost	+= itm->Cost();
		}
	}

	return result;
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
	//if (cNameSect() == "shadow_inventory")
	//{
	//	close();
	//	inherited::set_tip_text("st_pick_rucksack");
	//}
	processing_activate();
	return res;
}
void CInventoryContainer::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	//if (GE_OWNERSHIP_TAKE == type)
	//	Msg("CInventoryContainer %s received object", Name());
	//if (GE_OWNERSHIP_REJECT == type)
	//	Msg("CInventoryContainer %s lost object", Name());
}


void CInventoryContainer::close()
{
	m_opened = false;
	inherited::set_tip_text_default();
}


void CInventoryContainer::open()
{
	m_opened = true;
	inherited::set_tip_text	 ("container_use");
}

void CInventoryContainer::UpdateCL()
{
	inherited::UpdateCL();
	UpdateDropTasks();
}

void CInventoryContainer::UpdateDropTasks()
{
	for (const auto& item : m_items){
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(item)); VERIFY(itm);
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