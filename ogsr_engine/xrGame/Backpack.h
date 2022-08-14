#pragma once
#include "inventory_item_object.h"
class CBackpack :
    public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;
public:
								CBackpack();
	virtual						~CBackpack();
	virtual void				Load(LPCSTR section);

	virtual void				Hit(SHit* pHDS);
	void						HitItemsInBackPack(SHit* pHDS, bool hit_random_item);

	float						GetHitTypeProtection(ALife::EHitType hit_type);

	float m_fBleedingRestoreSpeed{};
	float m_fHealthRestoreSpeed{};
	float m_fPowerRestoreSpeed{};
	float m_fSatietyRestoreSpeed{};
	float m_fThirstRestoreSpeed{};
	float m_fPsyHealthRestoreSpeed{};
	float m_fAlcoholRestoreSpeed{};

	float m_fAdditionalMaxWeight{};
	float m_fAdditionalMaxVolume{};
	float m_fAdditionalWalkAccel{};
	float m_fAdditionalJumpSpeed{};

	float GetAdditionalMaxWeight();
	float GetAdditionalMaxVolume();
	float GetAdditionalWalkAccel();
	float GetAdditionalJumpSpeed();

private:
	HitImmunity::HitTypeSVec		m_HitTypeProtection;
};

