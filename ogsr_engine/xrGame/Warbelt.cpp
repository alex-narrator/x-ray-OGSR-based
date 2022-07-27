#include "stdafx.h"
#include "WarBelt.h"

#include "Inventory.h"
#include "Actor.h"


CWarbelt::CWarbelt()
{
	m_iMaxBeltWidth		= 0;
	m_bDropPouch		= false;
}


CWarbelt::~CWarbelt()
{
}

void CWarbelt::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iMaxBeltWidth		= READ_IF_EXISTS(pSettings, r_u32, section, "max_belt_width", 0);
	m_bDropPouch		= READ_IF_EXISTS(pSettings, r_bool, section, "drop_pouch", false);
}

void CWarbelt::OnMoveToSlot(EItemPlace previous_place){
	if (m_pCurrentInventory){
		auto pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->IsAllItemsLoaded())
			m_pCurrentInventory->DropBeltToRuck();
	}
}

void CWarbelt::OnMoveToRuck(EItemPlace previous_place){
	if (m_pCurrentInventory && previous_place == eItemPlaceSlot){
		auto pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->IsAllItemsLoaded())
			m_pCurrentInventory->DropBeltToRuck();
	}
}