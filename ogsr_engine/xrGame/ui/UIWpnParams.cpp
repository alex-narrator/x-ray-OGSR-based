#include "StdAfx.h"
#include "UIWpnParams.h"
#include "UIXmlInit.h"
#include "../Level.h"
#include "../ai_space.h"
#include "../script_engine.h"
#include "inventory_item.h"
#include "clsid_game.h"
#include "script_game_object.h"
//
#include "../string_table.h"
#include "WeaponMagazinedWGrenade.h"
#include "WeaponBinoculars.h"
#include "WeaponKnife.h"

constexpr auto WPN_PARAMS = "wpn_params.xml";

CUIWpnParams::CUIWpnParams(){
}

CUIWpnParams::~CUIWpnParams(){
}

void CUIWpnParams::Init(){
	
	CUIXml uiXml;
	uiXml.Init(CONFIG_PATH, UI_PATH, WPN_PARAMS);

	CUIXmlInit::InitWindow			(uiXml, "wpn_params", 0, this);
	//	
	AttachChild						(&m_CapInfo);
	CUIXmlInit::InitStatic			(uiXml, "wpn_params:cap_info",			0, &m_CapInfo);
}

void CUIWpnParams::Reinit() {
	m_CapInfo.DetachAll();
	DetachAll();
	Init();
}

void CUIWpnParams::SetInfo(CInventoryItem* obj)
{
	Reinit();

	const shared_str& item_section = obj->object().cNameSect();

	string1024 text_to_show;
	char temp_text[64];
	auto pWeapon			= smart_cast<CWeapon*>					(obj);
	auto pWeaponMag			= smart_cast<CWeaponMagazined*>			(obj);
	auto pWeaponMagWGren	= smart_cast<CWeaponMagazinedWGrenade*>	(obj);
	auto pWeaponKnife		= smart_cast<CWeaponKnife*>				(obj);
	auto pWeaponBinoc		= smart_cast<CWeaponBinoculars*>		(obj);

	//динамічний лист інформації про зброю
	CUIXml	uiXml;
	float pos_top{};
	
	//елемент списку
	uiXml.Init(CONFIG_PATH, UI_PATH, WPN_PARAMS);
	auto marker_ = uiXml.ReadAttrib("wpn_params:list_item", 0, "marker", "• ");
	float list_item_h = uiXml.ReadAttribFlt("wpn_params:list_item", 0, "height");

	float _h{};
	//ушкодження
	if (!pWeaponBinoc) {
		auto damage_static = xr_new<CUIStatic>(); damage_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:damage", 0, damage_static);
		pos_top = damage_static->GetPosTop();
		damage_static->SetWndPos(damage_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %.2f", pWeapon->GetHitPowerForActor());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_damage").c_str(), temp_text);
		damage_static->SetText(text_to_show);
		m_CapInfo.AttachChild(damage_static);
		_h += list_item_h + pos_top;
	}
	//дальність вогню
	if (!pWeaponBinoc) {
		auto fire_distance_static = xr_new<CUIStatic>(); fire_distance_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:fire_distance", 0, fire_distance_static);
		pos_top = fire_distance_static->GetPosTop();
		fire_distance_static->SetWndPos(fire_distance_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %.1f %s", pSettings->r_float(item_section, "fire_distance"), CStringTable().translate("st_m").c_str());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_fire_distance").c_str(), temp_text);
		fire_distance_static->SetText(text_to_show);
		m_CapInfo.AttachChild(fire_distance_static);
		_h += list_item_h + pos_top;
	}
	//швидкострільність
	if (!pWeaponKnife && !pWeaponBinoc) {
		auto rpm_static = xr_new<CUIStatic>(); rpm_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:rpm", 0, rpm_static);
		pos_top = rpm_static->GetPosTop();
		rpm_static->SetWndPos(rpm_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %.f %s", pSettings->r_float(item_section, "rpm"), CStringTable().translate("st_rpm_units").c_str());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_rpm").c_str(), temp_text);
		rpm_static->SetText(text_to_show);
		m_CapInfo.AttachChild(rpm_static);
		_h += list_item_h + pos_top;
	}
	//розкид
	if (!pWeaponKnife && !pWeaponBinoc) {
		auto dispertion_static = xr_new<CUIStatic>(); dispertion_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:dispertion", 0, dispertion_static);
		pos_top = dispertion_static->GetPosTop();
		dispertion_static->SetWndPos(dispertion_static->GetPosLeft(), _h + pos_top);
		float fire_dispertion = pSettings->r_float(item_section, "fire_dispersion_base");
		fire_dispertion *= 10.f;
		sprintf_s(temp_text, " %.1f %s", fire_dispertion, CStringTable().translate("st_dispertion_units").c_str());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_dispertion").c_str(), temp_text);
		dispertion_static->SetText(text_to_show);
		m_CapInfo.AttachChild(dispertion_static);
		_h += list_item_h + pos_top;
	}
	//громіздкість
	auto bulkiness_static = xr_new<CUIStatic>(); bulkiness_static->SetAutoDelete(true);
	CUIXmlInit::InitStatic(uiXml, "wpn_params:bulkiness", 0, bulkiness_static);
	pos_top = bulkiness_static->GetPosTop();
	bulkiness_static->SetWndPos(bulkiness_static->GetPosLeft(), _h + pos_top);
	sprintf_s(temp_text, " %.1f", pWeapon->GetControlInertionFactor());
	strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_bulkiness").c_str(), temp_text);
	bulkiness_static->SetText(text_to_show);
	m_CapInfo.AttachChild(bulkiness_static);
	_h += list_item_h + pos_top;
	//тип спорядженого боєприпасу
	if (!pWeaponKnife && !pWeaponBinoc && (pWeapon->HasDetachableMagazine() && pWeaponMag->IsMagazineAttached() || pWeapon->GetAmmoElapsed())) {
		auto current_ammo_type_static = xr_new<CUIStatic>(); current_ammo_type_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:current_ammo_type", 0, current_ammo_type_static);
		pos_top = current_ammo_type_static->GetPosTop();
		current_ammo_type_static->SetWndPos(current_ammo_type_static->GetPosLeft(), _h + pos_top);
		if (pWeapon->HasDetachableMagazine() && pWeaponMag->IsMagazineAttached() && pWeapon->GetAmmoElapsed())
			sprintf_s(temp_text, " %s, %s", pWeapon->GetCurrentAmmo_ShortName(), pWeaponMag->GetCurrentMagazine_ShortName());
		else if (pWeapon->GetAmmoElapsed())
			sprintf_s(temp_text, " %s", pWeapon->GetCurrentAmmo_ShortName());
		else
			sprintf_s(temp_text, " %s", pWeaponMag->GetCurrentMagazine_ShortName());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_current_ammo_type").c_str(), temp_text);
		current_ammo_type_static->SetText(text_to_show);
		m_CapInfo.AttachChild(current_ammo_type_static);
		_h += list_item_h + pos_top;
	}
	//поточний приціл
	if (pWeapon->IsScopeAttached()) {
		//поточний приціл - заголовок
		auto cap_current_scope_static = xr_new<CUIStatic>(); cap_current_scope_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_current_scope", 0, cap_current_scope_static);
		pos_top = cap_current_scope_static->GetPosTop();
		cap_current_scope_static->SetWndPos(cap_current_scope_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_current_scope_static);
		_h += list_item_h + pos_top;
		//поточний приціл - параметри
		//зум
		auto scope_zoom_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, scope_zoom_static);
		scope_zoom_static->SetAutoDelete(true);
		scope_zoom_static->SetWndPos(scope_zoom_static->GetPosLeft(), _h);
		if (pWeapon->IsScopeDynamicZoom()) {
			sprintf_s(temp_text, " %.1f-%.1fx", pWeapon->GetMinScopeZoomFactor(), pWeapon->GetScopeZoomFactor());
		}
		else
			sprintf_s(temp_text, " %.1fx", pWeapon->GetScopeZoomFactor());
		strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate("st_scope_zoom").c_str(), temp_text);
		scope_zoom_static->SetText(text_to_show);
		m_CapInfo.AttachChild(scope_zoom_static);
		_h += list_item_h;
		//нічне бачення
		auto scope_night_vision_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, scope_night_vision_static);
		scope_night_vision_static->SetAutoDelete(true);
		scope_night_vision_static->SetWndPos(scope_night_vision_static->GetPosLeft(), _h);
		sprintf_s(temp_text, " %s", pWeaponMag->IsNightVisionEnabled() ? CStringTable().translate("st_yes").c_str() : CStringTable().translate("st_no").c_str());
		strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate("st_scope_night_vision").c_str(), temp_text);
		scope_night_vision_static->SetText(text_to_show);
		m_CapInfo.AttachChild(scope_night_vision_static);
		_h += list_item_h;
		//автозахоплення цілей
		auto scope_vision_present_static = xr_new<CUIStatic>();
		CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, scope_vision_present_static);
		scope_vision_present_static->SetAutoDelete(true);
		scope_vision_present_static->SetWndPos(scope_vision_present_static->GetPosLeft(), _h);
		sprintf_s(temp_text, " %s", pWeaponMag->IsVisionPresent() ? CStringTable().translate("st_yes").c_str() : CStringTable().translate("st_no").c_str());
		strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate("st_scope_vision_present").c_str(), temp_text);
		scope_vision_present_static->SetText(text_to_show);
		m_CapInfo.AttachChild(scope_vision_present_static);
		_h += list_item_h;
	}
	if (!pWeaponKnife && !pWeaponBinoc) {
		//сумісні набої - заголовок
		auto cap_ammo_static = xr_new<CUIStatic>(); cap_ammo_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_ammo", 0, cap_ammo_static);
		pos_top = cap_ammo_static->GetPosTop();
		cap_ammo_static->SetWndPos(cap_ammo_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_ammo_static);
		_h += list_item_h + pos_top;
		//сумісні набої - список
		for (const auto& ammo : pWeapon->m_ammoTypes) {
			if (pSettings->line_exist(ammo, "empty_box")) //магазини покажемо окремо нижче
				continue;
			auto ammo_name = pSettings->r_string(ammo, "inv_name");
			auto ammo_static = xr_new<CUIStatic>();
			CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, ammo_static);
			ammo_static->SetAutoDelete(true);
			ammo_static->SetWndPos(ammo_static->GetPosLeft(), _h);
			strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(ammo_name).c_str());
			ammo_static->SetText(text_to_show);
			m_CapInfo.AttachChild(ammo_static);
			_h += list_item_h;
		}
		//сумісні набої грантометів - список
		if (pWeapon->IsGrenadeLauncherAttached()) {
			for (const auto& ammo : pWeaponMagWGren->m_ammoTypes2) {
				auto ammo_name = pSettings->r_string(ammo, "inv_name");
				auto ammo_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, ammo_static);
				ammo_static->SetAutoDelete(true);
				ammo_static->SetWndPos(ammo_static->GetPosLeft(), _h);
				strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(ammo_name).c_str());
				ammo_static->SetText(text_to_show);
				m_CapInfo.AttachChild(ammo_static);
				_h += list_item_h;
			}
		}
		//сумісні магазини
		if (pWeapon->HasDetachableMagazine()) {
			//сумісні магазини - заголовок
			auto cap_magazine_static = xr_new<CUIStatic>(); cap_magazine_static->SetAutoDelete(true);
			CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_magazine", 0, cap_magazine_static);
			pos_top = cap_magazine_static->GetPosTop();
			cap_magazine_static->SetWndPos(cap_magazine_static->GetPosLeft(), _h + pos_top);
			m_CapInfo.AttachChild(cap_magazine_static);
			_h += list_item_h + pos_top;
			//сумісні магазини - список
			LPCSTR empty_sect_prev = nullptr;
			for (const auto& ammo : pWeapon->m_ammoTypes) {
				if (!pSettings->line_exist(ammo, "empty_box"))
					continue;
				//для магазинів будемо відображати тільки загальний "пустий" магазин
				auto empty_sect = pSettings->r_string(ammo, "empty_box");
				auto ammo_name = pSettings->r_string(empty_sect, "inv_name");
				if (empty_sect == empty_sect_prev) //та скіпнемо відображення якщо такий пустий магазин вже у переліку
					continue;
				empty_sect_prev = empty_sect;
				auto ammo_static = xr_new<CUIStatic>();
				CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, ammo_static);
				ammo_static->SetAutoDelete(true);
				ammo_static->SetWndPos(ammo_static->GetPosLeft(), _h);
				strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(ammo_name).c_str());
				ammo_static->SetText(text_to_show);
				m_CapInfo.AttachChild(ammo_static);
				_h += list_item_h;
			}
		}
		
		bool has_addon =
			pWeapon->ScopeAttachable() ||
			pWeapon->SilencerAttachable() ||
			pWeapon->GrenadeLauncherAttachable() ||
			pWeapon->LaserAttachable() ||
			pWeapon->FlashlightAttachable();

		if (has_addon) {
			//адони - заголовок
			auto cap_addons_static = xr_new<CUIStatic>(); cap_addons_static->SetAutoDelete(true);
			CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_addons", 0, cap_addons_static);
			pos_top = cap_addons_static->GetPosTop();
			cap_addons_static->SetWndPos(cap_addons_static->GetPosLeft(), _h + pos_top);
			m_CapInfo.AttachChild(cap_addons_static);
			_h += list_item_h + pos_top;
			if (pWeapon->ScopeAttachable()) {
				//сумісні приціли - список
				for (const auto& scope : pWeapon->m_scopes) {
					auto scope_name = pSettings->r_string(scope, "inv_name");
					auto scope_static = xr_new<CUIStatic>();
					CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, scope_static);
					scope_static->SetAutoDelete(true);
					scope_static->SetWndPos(scope_static->GetPosLeft(), _h);
					strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(scope_name).c_str());
					scope_static->SetText(text_to_show);
					m_CapInfo.AttachChild(scope_static);
					_h += list_item_h;
				}
			}
			//сумісні глушники
			if (pWeapon->SilencerAttachable()) {
				//глушник - список
				for (const auto& silencer : pWeapon->m_silencers) {
					auto silencer_name = pSettings->r_string(silencer, "inv_name");
					auto silencer_static = xr_new<CUIStatic>();
					CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, silencer_static);
					silencer_static->SetAutoDelete(true);
					silencer_static->SetWndPos(silencer_static->GetPosLeft(), _h);
					strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(silencer_name).c_str());
					silencer_static->SetText(text_to_show);
					m_CapInfo.AttachChild(silencer_static);
					_h += list_item_h;
				}
			}
			//сумісні гранатомети
			if (pWeapon->GrenadeLauncherAttachable()) {
				//сумісні гранатомети - список
				for (const auto& glauncher : pWeapon->m_glaunchers) {
					auto glauncher_name = pSettings->r_string(glauncher, "inv_name");
					auto glauncher_static = xr_new<CUIStatic>();
					CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, glauncher_static);
					glauncher_static->SetAutoDelete(true);
					glauncher_static->SetWndPos(glauncher_static->GetPosLeft(), _h);
					strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(glauncher_name).c_str());
					glauncher_static->SetText(text_to_show);
					m_CapInfo.AttachChild(glauncher_static);
					_h += list_item_h;
				}
			}
			//сумісні ЛЦВ
			if (pWeapon->LaserAttachable()) {
				//сумісні ЛЦВ - список
				for (const auto& laser : pWeapon->m_lasers) {
					auto laser_name = pSettings->r_string(laser, "inv_name");
					auto laser_static = xr_new<CUIStatic>();
					CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, laser_static);
					laser_static->SetAutoDelete(true);
					laser_static->SetWndPos(laser_static->GetPosLeft(), _h);
					strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(laser_name).c_str());
					laser_static->SetText(text_to_show);
					m_CapInfo.AttachChild(laser_static);
					_h += list_item_h;
				}
			}
			//сумісні ліхтарі
			if (pWeapon->FlashlightAttachable()) {
				//сумісні ліхтарі - список
				for (const auto& flashlight : pWeapon->m_flashlights) {
					auto flashlight_name = pSettings->r_string(flashlight, "inv_name");
					auto flashlight_static = xr_new<CUIStatic>();
					CUIXmlInit::InitStatic(uiXml, "wpn_params:list_item", 0, flashlight_static);
					flashlight_static->SetAutoDelete(true);
					flashlight_static->SetWndPos(flashlight_static->GetPosLeft(), _h);
					strconcat(sizeof(text_to_show), text_to_show, marker_, CStringTable().translate(flashlight_name).c_str());
					flashlight_static->SetText(text_to_show);
					m_CapInfo.AttachChild(flashlight_static);
					_h += list_item_h;
				}
			}
		}
	}

	SetHeight(_h);
}

bool CUIWpnParams::Check(CInventoryItem* obj)
{
	if (!READ_IF_EXISTS(pSettings, r_bool, obj->object().cNameSect(), "show_wpn_properties", true)) // allow to suppress default wpn params
	{
		return false;
	}

	return obj->cast_weapon();
}