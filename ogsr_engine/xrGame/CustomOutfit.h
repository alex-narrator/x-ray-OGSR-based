#pragma once

#include "inventory_item_object.h"

struct SBoneProtections;

class CCustomOutfit: public CInventoryItemObject {
	friend class COutfitScript;
private:
    typedef	CInventoryItemObject inherited;
public:
									CCustomOutfit		(void);
	virtual							~CCustomOutfit		(void);

	virtual void					Load				(LPCSTR section);
	
	//коэффициенты на которые домножается хит
	//при соответствующем типе воздействия
	//если на персонаже надет костюм
	float							GetHitTypeProtection(ALife::EHitType hit_type);

	float							HitThruArmour		(SHit* pHDS);
	//коэффициент на который домножается потеря силы
	//если на персонаже надет костюм
	float							GetPowerLoss		();


	virtual void					OnMoveToSlot		(EItemPlace prevPlace);
	virtual void					OnMoveToRuck		(EItemPlace prevPlace) override;

private:
	HitImmunity::HitTypeSVec		m_HitTypeProtection;
	float							m_fPowerLoss{};

	shared_str						m_ActorVisual;
	shared_str						m_full_icon_name;
	SBoneProtections*				m_boneProtection;	

	u32								m_ef_equipment_type{};

public:
	virtual u32						ef_equipment_type		() const;
	virtual	BOOL					BonePassBullet			(int boneID);
	const shared_str&				GetFullIconName			() const	{return m_full_icon_name;};

	float m_fBleedingRestoreSpeed;
	float m_fHealthRestoreSpeed;
	float m_fPowerRestoreSpeed;
	float m_fSatietyRestoreSpeed;
	float m_fThirstRestoreSpeed;
	float m_fPsyHealthRestoreSpeed;
	float m_fAlcoholRestoreSpeed;

	float m_fAdditionalMaxWeight;
	float m_fAdditionalMaxVolume;
	float m_fAdditionalWalkAccel;
	float m_fAdditionalJumpSpeed;

	float GetAdditionalMaxWeight();
	float GetAdditionalMaxVolume();
	float GetAdditionalWalkAccel();
	float GetAdditionalJumpSpeed();

	bool m_bIsHelmetAllowed;
};
