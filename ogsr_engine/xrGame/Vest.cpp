#include "stdafx.h"
#include "Vest.h"
#include "Actor.h"
#include "Inventory.h"

CVest::CVest() {
	SetSlot(VEST_SLOT);
}

void CVest::Load(LPCSTR section){
	inherited::Load(section);
	m_iVestWidth	= READ_IF_EXISTS(pSettings, r_u32,	section, "vest_width",	1);
	m_iVestHeight	= READ_IF_EXISTS(pSettings, r_u32,	section, "vest_height",	1);
}

bool  CVest::can_be_attached() const {
	const CActor* pA = smart_cast<const CActor*>(H_Parent());
	return pA ? (pA->GetVest() == this) : true;
	return true;
}

void CVest::OnMoveToSlot(EItemPlace prevPlace) {
	inherited::OnMoveToSlot(prevPlace);
	auto& inv = m_pCurrentInventory;
	if (inv) {
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && inv->IsAllItemsLoaded()) {
			inv->DropVestToRuck();
		}
	}
}

void CVest::OnMoveToRuck(EItemPlace prevPlace) {
	inherited::OnMoveToRuck(prevPlace);
	auto& inv = m_pCurrentInventory;
	if (inv && prevPlace == eItemPlaceSlot) {
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && inv->IsAllItemsLoaded()) {
			inv->DropVestToRuck();
		}
	}
}