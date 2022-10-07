#include "stdafx.h"
#include "WarBelt.h"

#include "Inventory.h"
#include "Actor.h"
#include "Vest.h"


CWarbelt::CWarbelt(){
	SetSlot(WARBELT_SLOT);
}


CWarbelt::~CWarbelt(){}

void CWarbelt::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iBeltWidth	= READ_IF_EXISTS(pSettings, r_u32,	section, "belt_width",	1);
	m_iBeltHeight	= READ_IF_EXISTS(pSettings, r_u32,	section, "belt_height",	1);
	m_bDropPouch	= READ_IF_EXISTS(pSettings, r_bool, section, "drop_pouch",	false);

	if (pSettings->line_exist(section, "slots_allowed")) {
		char buf[16]{};
		LPCSTR str = pSettings->r_string(section, "slots_allowed");
		for (int i = 0, count = _GetItemCount(str); i < count; ++i) {
			u8 belt_slot = u8(atoi(_GetItem(str, i, buf)));
			if (belt_slot < SLOTS_TOTAL)
				m_belt_slots.push_back(belt_slot);
		}
	}
}

void CWarbelt::OnMoveToSlot(EItemPlace prevPlace){
	inherited::OnMoveToSlot(prevPlace);
	auto &inv = m_pCurrentInventory;
	if (inv){
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && inv->IsAllItemsLoaded()) {
			inv->DropBeltToRuck();
			auto vest = pActor->GetVest();
			for (const auto& slot : m_belt_slots){
				if (slot != GRENADE_SLOT || !vest || !vest->SlotAllowed(GRENADE_SLOT))
					inv->DropSlotsToRuck(slot);
			}
		}
	}
}

void CWarbelt::OnMoveToRuck(EItemPlace prevPlace){
	inherited::OnMoveToRuck(prevPlace);
	auto& inv = m_pCurrentInventory;
	if (inv && prevPlace == eItemPlaceSlot){
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && inv->IsAllItemsLoaded()) {
			inv->DropBeltToRuck();
			auto vest = pActor->GetVest();
			for (const auto& slot : m_belt_slots) {
				if (slot != GRENADE_SLOT || !vest || !vest->SlotAllowed(GRENADE_SLOT))
					inv->DropSlotsToRuck(slot);
			}
		}
	}
}

bool CWarbelt::SlotAllowed(u32 slot) const {
	return (std::find(m_belt_slots.begin(), m_belt_slots.end(), slot) != m_belt_slots.end());
}