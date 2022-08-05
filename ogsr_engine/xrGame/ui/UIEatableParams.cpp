#include "stdafx.h"
#include "UIEatableParams.h"
#include "UIXmlInit.h"

#include "string_table.h"

#include "inventory_item.h"
#include "eatable_item.h"

constexpr auto EATABLE_PARAMS = "eatable_params.xml";

LPCSTR effect_names[] = {
	"ui_inv_portions_count",
	"ui_inv_health",
	"ui_inv_power",
	"ui_inv_max_power",
	"ui_inv_satiety",
	"ui_inv_radiation",
	"ui_inv_psy_health",
	"ui_inv_alcohol",
	"ui_inv_thirst",
	"ui_inv_wounds_heal_perc",
};

LPCSTR effect_static_names[] = {
	"portions_count",
	"eat_health",
	"eat_power",
	"eat_max_power",
	"eat_satiety",
	"eat_radiation",
	"eat_psyhealth",
	"eat_alcohol",
	"eat_thirst",
	"wounds_heal_perc",
};

CUIEatableParams::CUIEatableParams() {
}

CUIEatableParams::~CUIEatableParams() {
	for (u32 i = 0; i < _max_item_index; ++i){
		CUIStatic* _s = m_info_items[i];
		xr_delete(_s);
	}
}

void CUIEatableParams::Init() {
	CUIXml uiXml;
	uiXml.Init(CONFIG_PATH, UI_PATH, EATABLE_PARAMS);

	LPCSTR _base = "eatable_params";
	if (!uiXml.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow(uiXml, _base, 0, this);

	for (u32 i = 0; i < _max_item_index; ++i)
	{
		strconcat(sizeof(_buff), _buff, _base, ":static_", effect_static_names[i]);

		if (uiXml.NavigateToNode(_buff, 0)){
			m_info_items[i] = xr_new<CUIStatic>();
			CUIStatic* _s = m_info_items[i];
			_s->SetAutoDelete(false);
			CUIXmlInit::InitStatic(uiXml, _buff, 0, _s);
		}
	}
}

bool CUIEatableParams::Check(CInventoryItem* obj) {
	if (smart_cast<CEatableItem*>(obj)) {
		return true;
	}
	else
		return false;
}

float CUIEatableParams::GetEffectValue(u32 i, CInventoryItem* obj){
	float r = 0;

	if (!obj) return r;

	auto pEatable = smart_cast<CEatableItem*>(obj);

	switch (i)
	{
	case _portions_count: {
		r = (pEatable->GetStartPortionsNum() <= 1) ? 0.f : 1.f;
	}break;
	case _health_influence:{
		r = pEatable->GetHealthInfluence();
	}break;
	case _power_influence: {
		r = pEatable->GetPowerInfluence();
	}break;
	case _max_power_influence: {
		r = pEatable->GetMaxPowerUpInfluence();
	}break;
	case _satiety_influence: {
		r = pEatable->GetSatietyInfluence();
	}break;
	case _radiation_influence: {
		r = pEatable->GetRadiationInfluence();
	}break;
	case _psy_health_influence: {
		r = pEatable->GetPsyHealthInfluence();
	}break;
	case _alcohol_influence: {
		r = pEatable->GetAlcoholInfluence();
	}break;
	case _thirst_influence: {
		r = pEatable->GetThirstInfluence();
	}break;
	case _wounds_heal_perc: {
		r = pEatable->GetWoundsHealPerc();
	}break;
	}

	return r;
}

void CUIEatableParams::SetInfo(CInventoryItem* obj) {
	string128					text_to_show;
	float						_h = 0.0f;
	DetachAll();

	auto pEatable = smart_cast<CEatableItem*>(obj);
	for (u32 i = 0; i < _max_item_index; ++i){
		
		CUIStatic* _s = m_info_items[i];
		
		float _val = GetEffectValue(i, obj);
		if (fis_zero(_val)) continue;

		auto effect_name = CStringTable().translate(effect_names[i]).c_str();

		if (i == _portions_count) {
			sprintf_s(text_to_show, "%s  %d/%d",
				effect_name,
				pEatable->GetPortionsNum(),
				pEatable->GetStartPortionsNum());
		}else{
			_val *= 100.0f;
			LPCSTR _sn = "%";
			LPCSTR _color = (_val > 0) ? "%c[green]" : "%c[red]";
			sprintf_s(text_to_show, "%s %s %+.1f %s",
				effect_name,
				_color,
				_val,
				_sn);
		}

		_s->SetText(text_to_show);
		_s->SetWndPos(_s->GetWndPos().x, _h);
		_h += _s->GetWndSize().y;
		AttachChild(_s);
	}
	SetHeight(_h);
}