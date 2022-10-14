#pragma once
#include "inventory_item_object.h"
class CBackpack :
    public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;
public:
								CBackpack			();
	virtual						~CBackpack			();

	virtual void				Load				(LPCSTR section);

	virtual void				Hit					(SHit* pHDS);
			void				HitItemsInBackPack	(SHit* pHDS, bool hit_random_item);

	virtual bool				can_be_attached		() const;
	virtual float				GetAdditionalVolume	() const;

protected:
			float				m_fAdditionalVolume{};
};

