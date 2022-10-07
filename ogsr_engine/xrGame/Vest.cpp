#include "stdafx.h"
#include "Vest.h"
#include "Actor.h"
#include "Inventory.h"
#include "Warbelt.h"

CVest::CVest() {
	SetSlot(VEST_SLOT);
}

void CVest::Load(LPCSTR section)
{
	inherited::Load(section);

	if (pSettings->line_exist(section, "slots_allowed")) {
		char buf[16]{};
		LPCSTR str = pSettings->r_string(section, "slots_allowed");
		for (int i = 0, count = _GetItemCount(str); i < count; ++i) {
			u8 vest_slot = u8(atoi(_GetItem(str, i, buf)));
			if (vest_slot < SLOTS_TOTAL)
				m_vest_slots.push_back(vest_slot);
		}
	}
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
			auto warbelt = pActor->GetWarbelt();
			if(!warbelt || !warbelt->SlotAllowed(GRENADE_SLOT))
				inv->DropSlotsToRuck(GRENADE_SLOT);
			inv->DropSlotsToRuck(VEST_POUCH_1, VEST_POUCH_10);
		}
	}
}

void CVest::OnMoveToRuck(EItemPlace prevPlace) {
	inherited::OnMoveToRuck(prevPlace);
	auto& inv = m_pCurrentInventory;
	if (inv && prevPlace == eItemPlaceSlot) {
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && inv->IsAllItemsLoaded()) {
			auto warbelt = pActor->GetWarbelt();
			if (!warbelt || !warbelt->SlotAllowed(GRENADE_SLOT))
				inv->DropSlotsToRuck(GRENADE_SLOT);
			inv->DropSlotsToRuck(VEST_POUCH_1, VEST_POUCH_10);
		}
	}
}

bool CVest::SlotAllowed(u32 slot) const {
	return (std::find(m_vest_slots.begin(), m_vest_slots.end(), slot) != m_vest_slots.end());
}