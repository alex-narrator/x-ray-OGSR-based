/////////////////////////////////////////////////////////////
// різноманітна кастомна інформація про
// предмети спорядження, обвіс, магазини
/////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIEquipParams.h"
#include "UIXmlInit.h"

#include "string_table.h"

#include "inventory_item.h"
#include "CustomOutfit.h"
#include "Warbelt.h"
#include "WeaponAmmo.h"
#include "Scope.h"
#include "InventoryContainer.h"

constexpr auto equip_params = "equip_params.xml";

CUIEquipParams::CUIEquipParams(){
}

CUIEquipParams::~CUIEquipParams(){
}

void CUIEquipParams::Init() {

	CUIXml uiXml;
	uiXml.Init(CONFIG_PATH, UI_PATH, equip_params);

	AttachChild(&m_CapInfo);
	CUIXmlInit::InitStatic(uiXml, "equip_params:cap_info", 0, &m_CapInfo);
}

bool CUIEquipParams::Check(CInventoryItem* obj){
	if (smart_cast<CWeaponAmmo*>		(obj) || 
		smart_cast<CWarbelt*>			(obj) ||
		smart_cast<CCustomOutfit*>		(obj) ||
		smart_cast<CScope*>				(obj) ||
		smart_cast<CInventoryContainer*>(obj)) {
		return true;
	}else
		return false;
}

void CUIEquipParams::SetInfo(CInventoryItem* obj){
	m_CapInfo.DetachAll();

	const shared_str& item_section = obj->object().cNameSect();

	string1024	text_to_show;
	char		temp_text[64];

	//динамічний лист інформації
	CUIXml	uiXml;
	float	pos_top = 0.f;

	//елемент списку
	uiXml.Init(CONFIG_PATH, UI_PATH, equip_params);
	auto marker_ = uiXml.ReadAttrib("equip_params:list_item", 0, "marker", "• ");
	float list_item_h = uiXml.ReadAttribFlt("equip_params:list_item", 0, "height");

	float _h = 0.f;

	auto pScope = smart_cast<CScope*>(obj);
	if (pScope) {
		auto scope_zoom_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:scope_zoom", 0, scope_zoom_static);
		scope_zoom_static->SetAutoDelete(true);
		pos_top = scope_zoom_static->GetPosTop();
		scope_zoom_static->SetWndPos(scope_zoom_static->GetPosLeft(), _h + pos_top);
		float zoom_factor = pSettings->r_float(item_section, "scope_zoom_factor");
		bool has_zoom_dynamic = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "scope_dynamic_zoom", false);
		if (has_zoom_dynamic) {
			float zoom_step_count = READ_IF_EXISTS(pSettings, r_u32, item_section, "zoom_step_count", 3);
			float min_zoom_factor = READ_IF_EXISTS(pSettings, r_float, item_section, "min_scope_zoom_factor", zoom_factor / zoom_step_count);
			sprintf_s(temp_text, " %.1f-%.1fx", min_zoom_factor, zoom_factor);
		}else
			sprintf_s(temp_text, " %.1fx", zoom_factor);
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_scope_zoom").c_str(), temp_text);
		scope_zoom_static->SetText(text_to_show);
		m_CapInfo.AttachChild(scope_zoom_static);
		_h += list_item_h;

		bool has_night_vision = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "night_vision", false);
		auto scope_night_vision_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:scope_night_vision", 0, scope_night_vision_static);
		scope_night_vision_static->SetAutoDelete(true);
		pos_top = scope_night_vision_static->GetPosTop();
		scope_night_vision_static->SetWndPos(scope_night_vision_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %s", has_night_vision ? CStringTable().translate("st_yes").c_str() : CStringTable().translate("st_no").c_str());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_scope_night_vision").c_str(), temp_text);
		scope_night_vision_static->SetText(text_to_show);
		m_CapInfo.AttachChild(scope_night_vision_static);
		_h += list_item_h;

		bool vision_present = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "vision_present", false);
		auto scope_vision_present_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:scope_vision_present", 0, scope_vision_present_static);
		scope_vision_present_static->SetAutoDelete(true);
		pos_top = scope_vision_present_static->GetPosTop();
		scope_vision_present_static->SetWndPos(scope_vision_present_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %s", vision_present ? CStringTable().translate("st_yes").c_str() : CStringTable().translate("st_no").c_str());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_scope_vision_present").c_str(), temp_text);
		scope_vision_present_static->SetText(text_to_show);
		m_CapInfo.AttachChild(scope_vision_present_static);
		_h += list_item_h;
	}

	auto pOutfit = smart_cast<CCustomOutfit*>(obj);
	if (pOutfit) {
		auto inbuild_helmet_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:outfit_helmet_inbuild", 0, inbuild_helmet_static);
		inbuild_helmet_static->SetAutoDelete(true);
		pos_top = inbuild_helmet_static->GetPosTop();
		inbuild_helmet_static->SetWndPos(inbuild_helmet_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %s", pOutfit->m_bIsHelmetAllowed ? CStringTable().translate("st_no").c_str() : CStringTable().translate("st_yes").c_str());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_inbuild_helmet").c_str(), temp_text);
		inbuild_helmet_static->SetText(text_to_show);
		m_CapInfo.AttachChild(inbuild_helmet_static);
		_h += list_item_h;
	}

	auto pWarbelt = smart_cast<CWarbelt*>(obj);
	if (pWarbelt) {
		auto belt_cells_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:warbelt_cells", 0, belt_cells_static);
		belt_cells_static->SetAutoDelete(true);
		pos_top = belt_cells_static->GetPosTop();
		belt_cells_static->SetWndPos(belt_cells_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " [%d]", pWarbelt->GetMaxBelt());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_belt_cells_available").c_str(), temp_text);
		belt_cells_static->SetText(text_to_show);
		m_CapInfo.AttachChild(belt_cells_static);
		_h += list_item_h;

		auto drop_pouch_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:warbelt_drop_pouch", 0, drop_pouch_static);
		drop_pouch_static->SetAutoDelete(true);
		pos_top = drop_pouch_static->GetPosTop();
		drop_pouch_static->SetWndPos(drop_pouch_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %s", pWarbelt->HasDropPouch() ? CStringTable().translate("st_yes").c_str() : CStringTable().translate("st_no").c_str());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_has_drop_pouch").c_str(), temp_text);
		drop_pouch_static->SetText(text_to_show);
		m_CapInfo.AttachChild(drop_pouch_static);
		_h += list_item_h;
	}

	//сумісні набої магазинів
	auto pAmmo = smart_cast<CWeaponAmmo*>(obj);
	if (pAmmo && (pAmmo->IsBoxReloadable() || pAmmo->IsBoxReloadableEmpty())) {
		//сумісні набої магазинів - заголовок
		auto cap_ammo_static = xr_new<CUIStatic>(); cap_ammo_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "equip_params:cap_ammo", 0, cap_ammo_static);
		pos_top = cap_ammo_static->GetPosTop();
		cap_ammo_static->SetWndPos(cap_ammo_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_ammo_static);
		_h += list_item_h + pos_top;
		//сумісні набої порожніх магазинів - список
		if (pAmmo->IsBoxReloadableEmpty()) {
			for (const auto& ammo : pAmmo->m_ammoTypes) {
				auto ammo_name = pSettings->r_string(ammo, "inv_name");
				auto ammo_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, ammo_static);
				ammo_static->SetAutoDelete(true);
				ammo_static->SetWndPos(ammo_static->GetPosLeft(), _h);
				strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(ammo_name).c_str());
				ammo_static->SetText(text_to_show);
				m_CapInfo.AttachChild(ammo_static);
				_h += list_item_h;
			}
		}
		//сумісні набої заряджених магазинів - список
		if (pAmmo->IsBoxReloadable()) {
			xr_vector<shared_str> m_ammoTypes;
			m_ammoTypes.clear();
			LPCSTR _at = pSettings->r_string(pSettings->r_string(item_section, "empty_box"), "ammo_types");
			if (_at && _at[0]) {
				string128		_ammoItem;
				int				count = _GetItemCount(_at);
				for (int it = 0; it < count; ++it) {
					_GetItem(_at, it, _ammoItem);
					m_ammoTypes.push_back(_ammoItem);
				}
			}
			for (const auto& ammo : m_ammoTypes) {
				auto ammo_name = pSettings->r_string(ammo, "inv_name");
				auto ammo_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, ammo_static);
				ammo_static->SetAutoDelete(true);
				ammo_static->SetWndPos(ammo_static->GetPosLeft(), _h);
				strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(ammo_name).c_str());
				ammo_static->SetText(text_to_show);
				m_CapInfo.AttachChild(ammo_static);
				_h += list_item_h;
			}
		}
	}

	auto pContainer = smart_cast<CInventoryContainer*>(obj);
	if (pContainer && !pContainer->IsEmpty()) {
		auto cap_containment_static = xr_new<CUIStatic>(); cap_containment_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "equip_params:cap_containment", 0, cap_containment_static);
		pos_top = cap_containment_static->GetPosTop();
		cap_containment_static->SetWndPos(cap_containment_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_containment_static);
		_h += list_item_h + pos_top;

		TIItemContainer	container_list;
		pContainer->AddAvailableItems(container_list);

		for (const auto& item : container_list) {
			auto item_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, item_static);
			item_static->SetAutoDelete(true);
			item_static->SetWndPos(item_static->GetPosLeft(), _h);
			strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(item->Name()).c_str());
			item_static->SetText(text_to_show);
			m_CapInfo.AttachChild(item_static);
			_h += list_item_h;
		}
	}

	SetHeight(_h);
}