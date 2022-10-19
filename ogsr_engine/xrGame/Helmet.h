#pragma once
#include "inventory_item_object.h"
struct SBoneProtections;
class CHelmet :
    public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;
public:
									CHelmet();
	virtual							~CHelmet();

	virtual void					Load(LPCSTR section);

	float							HitThruArmour(SHit* pHDS);
	//коэффициент на который домножается потеря силы
	//если на персонаже надет костюм
	float							GetPowerLoss();

	virtual	BOOL					BonePassBullet(int boneID);

	virtual bool					can_be_attached() const override;

	virtual void					OnMoveToSlot(EItemPlace prevPlace);
	virtual void					OnMoveToRuck(EItemPlace prevPlace);

	virtual void					DrawHUDMask	();

			bool					HasVisor	() const { return m_UIVisor && !!m_VisorTexture; };

private:
	float							m_fPowerLoss{};
	SBoneProtections*				m_boneProtection;
protected:
	CUIStaticItem*					m_UIVisor{};
	shared_str						m_VisorTexture{};
};

