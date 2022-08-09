#include "stdafx.h"
#include "WarBelt.h"

#include "Inventory.h"
#include "Actor.h"


CWarbelt::CWarbelt(){
}


CWarbelt::~CWarbelt(){
}

void CWarbelt::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iMaxBelt		= READ_IF_EXISTS(pSettings, r_u32,	section, "max_belt",	0);
	m_bDropPouch	= READ_IF_EXISTS(pSettings, r_bool, section, "drop_pouch",	false);
}

void CWarbelt::OnMoveToSlot(EItemPlace previous_place){
	inherited::OnMoveToSlot(previous_place);
	auto &inv = m_pCurrentInventory;
	if (inv){
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && inv->IsAllItemsLoaded()) {
			inv->DropBeltToRuck();
		}
	}
}

void CWarbelt::OnMoveToRuck(EItemPlace previous_place){
	inherited::OnMoveToRuck(previous_place);
	auto& inv = m_pCurrentInventory;
	if (inv && previous_place == eItemPlaceSlot){
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && inv->IsAllItemsLoaded()) {
			inv->DropBeltToRuck();
		}
	}
}