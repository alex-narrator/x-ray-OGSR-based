#include "stdafx.h"
#include "Backpack.h"

#include "Inventory.h"
#include "Actor.h"

CBackpack::CBackpack(){
	SetSlot(BACKPACK_SLOT);
}

CBackpack::~CBackpack(){
}

void CBackpack::Load(LPCSTR section)
{
	inherited::Load(section);
}

void CBackpack::Hit(SHit* pHDS){
	Msg("pHDS before hit: [%.2f]", pHDS->power);
	inherited::Hit(pHDS);
	Msg("pHDS after hit: [%.2f]", pHDS->power);
	bool hit_random_item =
		(pHDS->type() == ALife::eHitTypeFireWound ||
			pHDS->type() == ALife::eHitTypeWound ||
			pHDS->type() == ALife::eHitTypeWound_2);
	HitItemsInBackPack(pHDS, hit_random_item);
}

void CBackpack::HitItemsInBackPack(SHit* pHDS, bool hit_random_item){
	auto& inv = m_pCurrentInventory;
	if (inv){
		CActor* pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor && pActor->GetBackpack() == this && pActor->IsHitToBackPack(pHDS)){
			Msg("pHDS power: [%.2f]", pHDS->power);
			pHDS->power *= (1.0f - GetHitTypeProtection(pHDS->type()));
			Msg("new_hit power: [%.2f]", pHDS->power);

			auto ruck = inv->m_ruck;
			u32 random_item = NULL;

			for (const auto& item : ruck){
				if (hit_random_item)
					random_item = ::Random.randI(0, ruck.size());

				auto pIItem = hit_random_item ? ruck[random_item] : item;

				pIItem->Hit(pHDS);
				Msg("Hit item [%s] in backpack", pIItem->Name());
				if (hit_random_item) break;
			}
		}
	}
}