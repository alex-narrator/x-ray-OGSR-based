#pragma once
#include "UIWindow.h"
#include "UIStatic.h"

class CInventoryItem;

class CUIEatableParams : public CUIWindow
{
public:
								CUIEatableParams		();
	virtual						~CUIEatableParams		();

	void 						Init					();
	void 						SetInfo					(CInventoryItem* obj);
	bool 						Check					(CInventoryItem* obj);

protected:
	enum {
		_portions_count,
		_health_influence,
		_power_influence,
		_max_power_influence,
		_satiety_influence,
		_radiation_influence,
		_psy_health_influence,
		_alcohol_influence,
		_thirst_influence,
		_wounds_heal_perc,

		_max_item_index,
	};
	CUIStatic*					m_info_items[_max_item_index]{};
	float						GetEffectValue			(u32 i, CInventoryItem* obj);
};

