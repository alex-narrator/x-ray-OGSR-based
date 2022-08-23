#pragma once

#include "UIWindow.h"

class CUIScrollView;
class CCustomOutfit;
class CUIStatic;
class CUIXml;

class CUIOutfitInfo : public CUIWindow
{
public:
					CUIOutfitInfo			();
	virtual			~CUIOutfitInfo			();

			void 	Update					();	
			void 	InitFromXml				(CUIXml& xml_doc);
protected:

	CUIScrollView* m_listWnd{};

	enum {
		_item_start = 0,
		//restore
		_item_health_restore_speed = _item_start,
		_item_radiation_restore_speed,
		_item_satiety_restore_speed,
		_item_thirst_restore_speed,
		_item_power_restore_speed,
		_item_bleeding_restore_speed,
		_item_psy_health_restore_speed,
		_item_alcohol_restore_speed,
		//additional
		_item_additional_walk_accel,
		_item_additional_jump_speed,
		//_item_additional_weight,
		//_item_additional_volume,

		_hit_type_protection_index,

		_item_burn_immunity = _hit_type_protection_index,
		_item_shock_immunity,
		_item_strike_immunity,
		_item_wound_immunity,
		_item_radiation_immunity,
		_item_telepatic_immunity,
		_item_chemical_burn_immunity,
		_item_explosion_immunit,
		_item_fire_wound_immunity,

		_max_item_index,
	};
	CUIStatic* m_items[_max_item_index]{};
};