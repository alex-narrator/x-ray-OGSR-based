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
#include "Vest.h"
#include "Backpack.h"
#include "WeaponAmmo.h"
#include "Addons.h"
#include "PowerBattery.h"
#include "InventoryContainer.h"
#include "Grenade.h"

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
		smart_cast<CVest*>				(obj) ||
		smart_cast<CCustomOutfit*>		(obj) ||
		smart_cast<CBackpack*>			(obj) ||
		smart_cast<CScope*>				(obj) ||
		smart_cast<CSilencer*>			(obj) ||
		smart_cast<CInventoryContainer*>(obj) ||
		smart_cast<CPowerBattery*>		(obj) ||
		smart_cast<CGrenade*>			(obj) ||
		obj->IsPowerConsumer()				  ||
		obj->GetDetailPartSection()				) {
		return true;
	}else
		return false;
}

void CUIEquipParams::SetInfo(CInventoryItem* obj){
	m_CapInfo.DetachAll();

	const shared_str& item_section = obj->object().cNameSect();

	string1024	text_to_show{};
	char		temp_text[64];

	//динамічний лист інформації
	CUIXml	uiXml;
	float	pos_top{};

	//елемент списку
	uiXml.Init(CONFIG_PATH, UI_PATH, equip_params);
	auto marker_ = uiXml.ReadAttrib("equip_params:list_item", 0, "marker", "• ");
	float list_item_h = uiXml.ReadAttribFlt("equip_params:list_item", 0, "height");

	float _h{};

	auto pBattery = smart_cast<CPowerBattery*>(obj);
	if (obj->IsPowerConsumer() || pBattery && pBattery->IsCharger()) {
		auto power_level_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:power_level", 0, power_level_static);
		power_level_static->SetAutoDelete(true);
		pos_top = power_level_static->GetPosTop();
		power_level_static->SetWndPos(power_level_static->GetPosLeft(), _h + pos_top);
		float power_level = obj->GetPowerLevel();
		power_level *= 100.f;
		sprintf_s(temp_text, " %.0f", power_level);
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_power_level").c_str(), temp_text,"%");
		power_level_static->SetText(text_to_show);
		m_CapInfo.AttachChild(power_level_static);
		_h += list_item_h;
	}
	if (obj->IsPowerConsumer()) {
		auto power_level_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:power_ttl", 0, power_level_static);
		power_level_static->SetAutoDelete(true);
		pos_top = power_level_static->GetPosTop();
		power_level_static->SetWndPos(power_level_static->GetPosLeft(), _h + pos_top);
		float power_ttl = obj->GetPowerTTL();
		sprintf_s(temp_text, " %.0f ", power_ttl);
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_power_ttl").c_str(), temp_text, CStringTable().translate("st_time_hour").c_str());
		power_level_static->SetText(text_to_show);
		m_CapInfo.AttachChild(power_level_static);
		_h += list_item_h;
	}

	auto pSilencer = smart_cast<CSilencer*>(obj);
	if (pSilencer) {
		float hit_k = READ_IF_EXISTS(pSettings, r_float, item_section, "bullet_hit_power_k", 0.f);
		if (!fis_zero(hit_k)) {
			auto _hit_k_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:silencer_hit_k", 0, _hit_k_static);
			_hit_k_static->SetAutoDelete(true);
			pos_top = _hit_k_static->GetPosTop();
			_hit_k_static->SetWndPos(_hit_k_static->GetPosLeft(), _h + pos_top);
			hit_k *= 100.f;
			sprintf_s(temp_text, " %s%.1f", hit_k > 0.f ? "+" : "", hit_k);
			strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_damage").c_str(), temp_text, "%");
			_hit_k_static->SetText(text_to_show);
			m_CapInfo.AttachChild(_hit_k_static);
			_h += list_item_h;
		}
		float bullet_speed_k = READ_IF_EXISTS(pSettings, r_float, item_section, "bullet_speed_k", 0.f);
		if (!fis_zero(bullet_speed_k)) {
			auto _bullet_speed_k_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:silencer_bullet_speed_k", 0, _bullet_speed_k_static);
			_bullet_speed_k_static->SetAutoDelete(true);
			pos_top = _bullet_speed_k_static->GetPosTop();
			_bullet_speed_k_static->SetWndPos(_bullet_speed_k_static->GetPosLeft(), _h + pos_top);
			bullet_speed_k *= 100.f;
			sprintf_s(temp_text, " %s%.1f", bullet_speed_k > 0.f ? "+" : "", bullet_speed_k);
			strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_bullet_speed").c_str(), temp_text, "%");
			_bullet_speed_k_static->SetText(text_to_show);
			m_CapInfo.AttachChild(_bullet_speed_k_static);
			_h += list_item_h;
		}
		float dispersion_k = READ_IF_EXISTS(pSettings, r_float, item_section, "fire_dispersion_base_k", 0.f);
		if (!fis_zero(dispersion_k)) {
			auto _dispersion_k_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:silencer_dispersion_k", 0, _dispersion_k_static);
			_dispersion_k_static->SetAutoDelete(true);
			pos_top = _dispersion_k_static->GetPosTop();
			_dispersion_k_static->SetWndPos(_dispersion_k_static->GetPosLeft(), _h + pos_top);
			dispersion_k *= 100.f;
			sprintf_s(temp_text, " %s%.1f", dispersion_k > 0.f ? "+" : "", dispersion_k);
			strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_dispersion").c_str(), temp_text, "%");
			_dispersion_k_static->SetText(text_to_show);
			m_CapInfo.AttachChild(_dispersion_k_static);
			_h += list_item_h;
		}
		float recoil_k = READ_IF_EXISTS(pSettings, r_float, item_section, "cam_dispersion_k", 0.f);
		if (!fis_zero(recoil_k)) {
			auto _recoil_k_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:silencer_recoil_k", 0, _recoil_k_static);
			_recoil_k_static->SetAutoDelete(true);
			pos_top = _recoil_k_static->GetPosTop();
			_recoil_k_static->SetWndPos(_recoil_k_static->GetPosLeft(), _h + pos_top);
			recoil_k *= 100.f;
			sprintf_s(temp_text, " %s%.1f", recoil_k > 0.f ? "+" : "", recoil_k);
			strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_recoil").c_str(), temp_text, "%");
			_recoil_k_static->SetText(text_to_show);
			m_CapInfo.AttachChild(_recoil_k_static);
			_h += list_item_h;
		}
		float weapon_dec_k = READ_IF_EXISTS(pSettings, r_float, item_section, "condition_shot_dec_silencer", 0.f);
		if (!fis_zero(weapon_dec_k)) {
			auto _weapon_dec_k_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:weapon_dec_k", 0, _weapon_dec_k_static);
			_weapon_dec_k_static->SetAutoDelete(true);
			pos_top = _weapon_dec_k_static->GetPosTop();
			_weapon_dec_k_static->SetWndPos(_weapon_dec_k_static->GetPosLeft(), _h + pos_top);
			weapon_dec_k *= 100.f;
			sprintf_s(temp_text, " %s%.1f", weapon_dec_k > 0.f ? "+" : "", weapon_dec_k);
			strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_weapon_dec").c_str(), temp_text, "%");
			_weapon_dec_k_static->SetText(text_to_show);
			m_CapInfo.AttachChild(_weapon_dec_k_static);
			_h += list_item_h;
		}
		float shot_dec = READ_IF_EXISTS(pSettings, r_float, item_section, "condition_shot_dec", 0.f);
		if (!fis_zero(shot_dec)) {
			auto _resource_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:silencer_resource", 0, _resource_static);
			_resource_static->SetAutoDelete(true);
			pos_top = _resource_static->GetPosTop();
			_resource_static->SetWndPos(_resource_static->GetPosLeft(), _h + pos_top);
			float resource = 1.f / shot_dec;
			sprintf_s(temp_text, " %.0f %s", resource, CStringTable().translate("st_resource_units").c_str());
			strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_resource").c_str(), temp_text);
			_resource_static->SetText(text_to_show);
			m_CapInfo.AttachChild(_resource_static);
			_h += list_item_h;
		}
	}

	auto pScope = smart_cast<CScope*>(obj);
	if (pScope) {
		auto cap_scope_static = xr_new<CUIStatic>(); cap_scope_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "equip_params:cap_scope", 0, cap_scope_static);
		pos_top = cap_scope_static->GetPosTop();
		cap_scope_static->SetWndPos(cap_scope_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_scope_static);
		_h += list_item_h + pos_top;

		auto scope_zoom_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, scope_zoom_static);
		scope_zoom_static->SetAutoDelete(true);
		pos_top = scope_zoom_static->GetPosTop();
		scope_zoom_static->SetWndPos(scope_zoom_static->GetPosLeft(), _h + pos_top);
		float zoom_factor = pSettings->r_float(item_section, "scope_zoom_factor");
		bool has_zoom_dynamic = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "scope_dynamic_zoom", false);
		if (has_zoom_dynamic) {
			float zoom_step_count = READ_IF_EXISTS(pSettings, r_u32, item_section, "zoom_step_count", 3);
			float min_zoom_factor = READ_IF_EXISTS(pSettings, r_float, item_section, "min_scope_zoom_factor", zoom_factor / zoom_step_count);
			sprintf_s(text_to_show, "%s%s %.1f-%.1fx", marker_, CStringTable().translate("st_scope_zoom").c_str(), min_zoom_factor, zoom_factor);
		}else
			sprintf_s(text_to_show, "%s%s %.1fx", marker_, CStringTable().translate("st_scope_zoom").c_str(), zoom_factor);
		scope_zoom_static->SetText(text_to_show);
		m_CapInfo.AttachChild(scope_zoom_static);
		_h += list_item_h;

		bool has_night_vision = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "night_vision", false);
		if (has_night_vision) {
			auto scope_night_vision_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, scope_night_vision_static);
			scope_night_vision_static->SetAutoDelete(true);
			pos_top = scope_night_vision_static->GetPosTop();
			scope_night_vision_static->SetWndPos(scope_night_vision_static->GetPosLeft(), _h + pos_top);
			sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate("st_scope_night_vision").c_str());
			scope_night_vision_static->SetText(text_to_show);
			m_CapInfo.AttachChild(scope_night_vision_static);
			_h += list_item_h;
		}

		bool vision_present = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "vision_present", false);
		if (vision_present) {
			auto scope_vision_present_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, scope_vision_present_static);
			scope_vision_present_static->SetAutoDelete(true);
			pos_top = scope_vision_present_static->GetPosTop();
			scope_vision_present_static->SetWndPos(scope_vision_present_static->GetPosLeft(), _h + pos_top);
			sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate("st_scope_vision_present").c_str());
			scope_vision_present_static->SetText(text_to_show);
			m_CapInfo.AttachChild(scope_vision_present_static);
			_h += list_item_h;
		}

		bool has_range_meter = !!READ_IF_EXISTS(pSettings, r_bool, item_section, "range_meter", false);
		if (has_range_meter) {
			auto scope_range_meter_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, scope_range_meter_static);
			scope_range_meter_static->SetAutoDelete(true);
			pos_top = scope_range_meter_static->GetPosTop();
			scope_range_meter_static->SetWndPos(scope_range_meter_static->GetPosLeft(), _h + pos_top);
			sprintf_s(text_to_show, "%s%s", marker_, CStringTable().translate("st_scope_range_meter").c_str());
			scope_range_meter_static->SetText(text_to_show);
			m_CapInfo.AttachChild(scope_range_meter_static);
			_h += list_item_h;
		}
	}

	auto pOutfit = smart_cast<CCustomOutfit*>(obj);
	if (pOutfit && pOutfit->m_bIsHelmetBuiltIn) {
		auto inbuild_helmet_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:outfit_helmet_inbuild", 0, inbuild_helmet_static);
		inbuild_helmet_static->SetAutoDelete(true);
		pos_top = inbuild_helmet_static->GetPosTop();
		inbuild_helmet_static->SetWndPos(inbuild_helmet_static->GetPosLeft(), _h + pos_top);
		sprintf_s(text_to_show, "%s", CStringTable().translate("st_inbuild_helmet").c_str());
		inbuild_helmet_static->SetText(text_to_show);
		m_CapInfo.AttachChild(inbuild_helmet_static);
		_h += list_item_h;
	}

	auto pBackpack = smart_cast<CBackpack*>(obj);
	if (pBackpack) {
		auto power_level_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:additional_max_volume", 0, power_level_static);
		power_level_static->SetAutoDelete(true);
		pos_top = power_level_static->GetPosTop();
		power_level_static->SetWndPos(power_level_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %.1f", pBackpack->GetAdditionalVolume());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("ui_inv_volume").c_str(), temp_text, CStringTable().translate("st_l").c_str());
		power_level_static->SetText(text_to_show);
		m_CapInfo.AttachChild(power_level_static);
		_h += list_item_h;
	}

	auto pWarbelt	= smart_cast<CWarbelt*>(obj);
	auto pVest		= smart_cast<CVest*>(obj);
	if (pWarbelt || pVest) {
		auto belt_cells_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:equipment_cells", 0, belt_cells_static);
		belt_cells_static->SetAutoDelete(true);
		pos_top = belt_cells_static->GetPosTop();
		u32 cells_width		= pWarbelt ? pWarbelt->GetBeltWidth() : pVest->GetVestWidth();
		u32 cells_height	= pWarbelt ? pWarbelt->GetBeltHeight() : pVest->GetVestHeight();
		belt_cells_static->SetWndPos(belt_cells_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " [%dx%d]", cells_width, cells_height);
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_cells_available").c_str(), temp_text);
		belt_cells_static->SetText(text_to_show);
		m_CapInfo.AttachChild(belt_cells_static);
		_h += list_item_h;
	}

	if (pVest && pVest->m_plates.size()) {
		if (pVest->IsPlateInstalled()) {
			auto plate_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:armor_plate", 0, plate_static);
			plate_static->SetAutoDelete(true);
			pos_top = plate_static->GetPosTop();
			plate_static->SetWndPos(plate_static->GetPosLeft(), _h + pos_top);
			sprintf_s(temp_text, " %s", pVest->GetPlateName().c_str());
			strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_installed_plate").c_str(), temp_text);
			plate_static->SetText(text_to_show);
			m_CapInfo.AttachChild(plate_static);
			_h += list_item_h;
		}
		auto cap_plates_static = xr_new<CUIStatic>(); cap_plates_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "equip_params:cap_plates", 0, cap_plates_static);
		pos_top = cap_plates_static->GetPosTop();
		cap_plates_static->SetWndPos(cap_plates_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_plates_static);
		_h += list_item_h + pos_top;
		for (const auto& plate : pVest->m_plates) {
			auto plate_name = pSettings->r_string(plate, "inv_name");
			auto item_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, item_static);
			item_static->SetAutoDelete(true);
			item_static->SetWndPos(item_static->GetPosLeft(), _h);
			strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(plate_name).c_str());
			item_static->SetText(text_to_show);
			m_CapInfo.AttachChild(item_static);
			_h += list_item_h;
		}
	}

	//сумісні набої магазинів
	auto pAmmo = smart_cast<CWeaponAmmo*>(obj);
	if (pAmmo) {
		if (pAmmo->IsBoxReloadable() || pAmmo->IsBoxReloadableEmpty()) {
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
		else {
			//дальність вогню
			if (!fis_zero(pAmmo->m_kDist)) {
				auto fire_distance_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, fire_distance_static);
				fire_distance_static->SetAutoDelete(true);
				pos_top = fire_distance_static->GetPosTop();
				fire_distance_static->SetWndPos(fire_distance_static->GetPosLeft(), _h + pos_top);
				float add_dist = pAmmo->m_kDist * 100.f;
				sprintf_s(temp_text, " %s%.1f%s", add_dist > 1.f ? "+" : "", add_dist, "%");
				strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_fire_distance").c_str(), temp_text);
				fire_distance_static->SetText(text_to_show);
				m_CapInfo.AttachChild(fire_distance_static);
				_h += list_item_h;
			}
			//розкид
			if (!fis_zero(pAmmo->m_kDisp)) {
				auto dispersion_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, dispersion_static);
				dispersion_static->SetAutoDelete(true);
				pos_top = dispersion_static->GetPosTop();
				dispersion_static->SetWndPos(dispersion_static->GetPosLeft(), _h + pos_top);
				float add_disp = pAmmo->m_kDisp * 100.f;
				sprintf_s(temp_text, " %s%.1f%s", add_disp > 1.f ? "+" : "", add_disp, "%");
				strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_dispersion").c_str(), temp_text);
				dispersion_static->SetText(text_to_show);
				m_CapInfo.AttachChild(dispersion_static);
				_h += list_item_h;
			}
			//ушкодження
			if (!fis_zero(pAmmo->m_kHit)) {
				auto damage_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, damage_static);
				damage_static->SetAutoDelete(true);
				pos_top = damage_static->GetPosTop();
				damage_static->SetWndPos(damage_static->GetPosLeft(), _h + pos_top);
				float add_hit = pAmmo->m_kHit * 100.f;
				sprintf_s(temp_text, " %s%.1f%s", add_hit > 1.f ? "+" : "", add_hit, "%");
				strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_damage").c_str(), temp_text);
				damage_static->SetText(text_to_show);
				m_CapInfo.AttachChild(damage_static);
				_h += list_item_h;
			}
			//швидкість кулі
			if (!fis_zero(pAmmo->m_kSpeed)) {
				auto bullet_speed_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, bullet_speed_static);
				bullet_speed_static->SetAutoDelete(true);
				pos_top = bullet_speed_static->GetPosTop();
				bullet_speed_static->SetWndPos(bullet_speed_static->GetPosLeft(), _h + pos_top);
				float add_speed = pAmmo->m_kSpeed * 100.f;
				sprintf_s(temp_text, " %s%.f%s", add_speed > 1.f ? "+" : "", add_speed, "%");
				strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_bullet_speed").c_str(), temp_text);
				bullet_speed_static->SetText(text_to_show);
				m_CapInfo.AttachChild(bullet_speed_static);
				_h += list_item_h;
			}
			//бронебійність
			if (!fis_zero(pAmmo->m_kAP)) {
				auto ap_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, ap_static);
				ap_static->SetAutoDelete(true);
				pos_top = ap_static->GetPosTop();
				ap_static->SetWndPos(ap_static->GetPosLeft(), _h + pos_top);
				sprintf_s(temp_text, " %.1f", pAmmo->m_kAP);
				strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_ap").c_str(), temp_text);
				ap_static->SetText(text_to_show);
				m_CapInfo.AttachChild(ap_static);
				_h += list_item_h;
			}
			//зношування зброї
			if (!fis_zero(pAmmo->m_impair)) {
				auto bullet_speed_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, bullet_speed_static);
				bullet_speed_static->SetAutoDelete(true);
				pos_top = bullet_speed_static->GetPosTop();
				bullet_speed_static->SetWndPos(bullet_speed_static->GetPosLeft(), _h + pos_top);
				float add_impar = pAmmo->m_impair * 100.f;
				sprintf_s(temp_text, " %s%.f%s", add_impar > 1.f ? "+" : "", add_impar, "%");
				strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_weapon_dec").c_str(), temp_text);
				bullet_speed_static->SetText(text_to_show);
				m_CapInfo.AttachChild(bullet_speed_static);
				_h += list_item_h;
			}
			//кількість шротин у набої
			if (pAmmo->m_buckShot > 1) {
				auto buck_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, buck_static);
				buck_static->SetAutoDelete(true);
				pos_top = buck_static->GetPosTop();
				buck_static->SetWndPos(buck_static->GetPosLeft(), _h + pos_top);
				sprintf_s(text_to_show, "%s %d", CStringTable().translate("st_buck_count").c_str(), pAmmo->m_buckShot);
				buck_static->SetText(text_to_show);
				m_CapInfo.AttachChild(buck_static);
				_h += list_item_h;
			}
		}
	}

	auto pGrenade = smart_cast<CGrenade*>(obj);
	if (pGrenade || pSettings->line_exist(item_section, "fake_grenade_name")) {
		const shared_str& explosive_sect = pGrenade ? item_section : pSettings->r_string(item_section, "fake_grenade_name");
		//хіт вибуху
		auto explosion_hit_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, explosion_hit_static);
		explosion_hit_static->SetAutoDelete(true);
		pos_top = explosion_hit_static->GetPosTop();
		explosion_hit_static->SetWndPos(explosion_hit_static->GetPosLeft(), _h + pos_top);
		sprintf_s(text_to_show, "%s %.1f", CStringTable().translate("st_explosion_hit").c_str(), pSettings->r_float(explosive_sect, "blast"));
		explosion_hit_static->SetText(text_to_show);
		m_CapInfo.AttachChild(explosion_hit_static);
		_h += list_item_h;
		//радіус вибуху
		auto explosion_radius_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, explosion_radius_static);
		explosion_radius_static->SetAutoDelete(true);
		pos_top = explosion_radius_static->GetPosTop();
		explosion_radius_static->SetWndPos(explosion_radius_static->GetPosLeft(), _h + pos_top);
		sprintf_s(text_to_show, "%s %.1f %s", CStringTable().translate("st_explosion_radius").c_str(), pSettings->r_float(explosive_sect, "blast_r"), CStringTable().translate("st_m").c_str());
		explosion_radius_static->SetText(text_to_show);
		m_CapInfo.AttachChild(explosion_radius_static);
		_h += list_item_h;
		//хіт уламків
		auto frags_hit_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, frags_hit_static);
		frags_hit_static->SetAutoDelete(true);
		pos_top = frags_hit_static->GetPosTop();
		frags_hit_static->SetWndPos(frags_hit_static->GetPosLeft(), _h + pos_top);
		sprintf_s(text_to_show, "%s %.1f", CStringTable().translate("st_frags_hit").c_str(), pSettings->r_float(explosive_sect, "frag_hit"));
		frags_hit_static->SetText(text_to_show);
		m_CapInfo.AttachChild(frags_hit_static);
		_h += list_item_h;
		//радіус уламків
		auto frags_radius_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, frags_radius_static);
		frags_radius_static->SetAutoDelete(true);
		pos_top = frags_radius_static->GetPosTop();
		frags_radius_static->SetWndPos(frags_radius_static->GetPosLeft(), _h + pos_top);
		sprintf_s(text_to_show, "%s %.1f %s", CStringTable().translate("st_frags_radius").c_str(), pSettings->r_float(explosive_sect, "frags_r"), CStringTable().translate("st_m").c_str());
		frags_radius_static->SetText(text_to_show);
		m_CapInfo.AttachChild(frags_radius_static);
		_h += list_item_h;
		//хіт уламків
		auto frags_count_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, frags_count_static);
		frags_count_static->SetAutoDelete(true);
		pos_top = frags_count_static->GetPosTop();
		frags_count_static->SetWndPos(frags_count_static->GetPosLeft(), _h + pos_top);
		sprintf_s(text_to_show, "%s %d", CStringTable().translate("st_frags_count").c_str(), pSettings->r_u8(explosive_sect, "frags")*2);
		frags_count_static->SetText(text_to_show);
		m_CapInfo.AttachChild(frags_count_static);
		_h += list_item_h;
		//дистанція зведення
		if (pSettings->line_exist(explosive_sect, "safe_dist_to_explode")) {
			auto safe_dist_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, safe_dist_static);
			safe_dist_static->SetAutoDelete(true);
			pos_top = safe_dist_static->GetPosTop();
			safe_dist_static->SetWndPos(safe_dist_static->GetPosLeft(), _h + pos_top);
			sprintf_s(text_to_show, "%s %.1f %s", CStringTable().translate("st_safe_dist").c_str(), pSettings->r_float(explosive_sect, "safe_dist_to_explode"), CStringTable().translate("st_m").c_str());
			safe_dist_static->SetText(text_to_show);
			m_CapInfo.AttachChild(safe_dist_static);
			_h += list_item_h;
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

	if (obj->GetDetailPartSection()) {
		auto cap_containment_static = xr_new<CUIStatic>(); cap_containment_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "equip_params:cap_detail_parts", 0, cap_containment_static);
		pos_top = cap_containment_static->GetPosTop();
		cap_containment_static->SetWndPos(cap_containment_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_containment_static);
		_h += list_item_h + pos_top;

		string128 item_sect;
		LPCSTR detail_part_sect = obj->GetDetailPartSection();
		int count = _GetItemCount(detail_part_sect);
		for (int i = 0; i < count; i += 2) {
			_GetItem(detail_part_sect, i, item_sect);
			string128 tmp;
			int item_count = atoi(_GetItem(detail_part_sect, i + 1, tmp));
			auto item_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "equip_params:list_item", 0, item_static);
			item_static->SetAutoDelete(true);
			item_static->SetWndPos(item_static->GetPosLeft(), _h);
			auto detail_name = pSettings->r_string(item_sect, "inv_name");
			string64 _txt;
			sprintf(_txt,"x%d %s", item_count, CStringTable().translate(detail_name).c_str());
			strconcat(sizeof(text_to_show), text_to_show, marker_, _txt);
			item_static->SetText(text_to_show);
			m_CapInfo.AttachChild(item_static);
			_h += list_item_h;
		}
	}

	SetHeight(_h);
}