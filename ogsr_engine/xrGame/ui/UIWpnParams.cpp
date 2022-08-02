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
#include "../WeaponMagazinedWGrenade.h"

constexpr auto WPN_PARAMS = "wpn_params.xml";

struct SLuaWpnParams{
	luabind::functor<float>		m_functorRPM;
	luabind::functor<float>		m_functorAccuracy;
	luabind::functor<float>		m_functorDamage;
	luabind::functor<float>		m_functorHandling;
	SLuaWpnParams();
	~SLuaWpnParams();
};

SLuaWpnParams::SLuaWpnParams()
{
	bool	functor_exists;
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetRPM" ,		m_functorRPM);		VERIFY(functor_exists);
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetDamage" ,	m_functorDamage);	VERIFY(functor_exists);
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetHandling" ,m_functorHandling);	VERIFY(functor_exists);
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetAccuracy" ,m_functorAccuracy);	VERIFY(functor_exists);
}

SLuaWpnParams::~SLuaWpnParams()
{
}

SLuaWpnParams* g_lua_wpn_params = NULL;

void destroy_lua_wpn_params()
{
	if(g_lua_wpn_params)
		xr_delete(g_lua_wpn_params);
}


CUIWpnParams::CUIWpnParams(){
}

CUIWpnParams::~CUIWpnParams(){
}

void CUIWpnParams::Init(){
	
	CUIXml uiXml;
	uiXml.Init(CONFIG_PATH, UI_PATH, WPN_PARAMS);

	CUIXmlInit::InitWindow			(uiXml, "wpn_params", 0, this);
	
	AttachChild						(&m_textAccuracy);
	CUIXmlInit::InitStatic			(uiXml, "wpn_params:cap_accuracy",		0, &m_textAccuracy);

	AttachChild						(&m_textDamage);
	CUIXmlInit::InitStatic			(uiXml, "wpn_params:cap_damage",		0, &m_textDamage);

	AttachChild						(&m_textHandling);
	CUIXmlInit::InitStatic			(uiXml, "wpn_params:cap_handling",		0, &m_textHandling);

	AttachChild						(&m_textRPM);
	CUIXmlInit::InitStatic			(uiXml, "wpn_params:cap_rpm",			0, &m_textRPM);

	AttachChild						(&m_progressAccuracy);
	CUIXmlInit::InitProgressBar		(uiXml, "wpn_params:progress_accuracy",	0, &m_progressAccuracy);
	m_progressAccuracy.SetRange		(0, 100);

	AttachChild						(&m_progressDamage);
	CUIXmlInit::InitProgressBar		(uiXml, "wpn_params:progress_damage",	0, &m_progressDamage);
	m_progressDamage.SetRange		(0, 100);

	AttachChild						(&m_progressHandling);
	CUIXmlInit::InitProgressBar		(uiXml, "wpn_params:progress_handling",	0, &m_progressHandling);
	m_progressHandling.SetRange		(0, 100);

	AttachChild						(&m_progressRPM);
	CUIXmlInit::InitProgressBar		(uiXml, "wpn_params:progress_rpm",		0, &m_progressRPM);
	m_progressRPM.SetRange			(0, 100);
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

	if(!g_lua_wpn_params)
		g_lua_wpn_params = xr_new<SLuaWpnParams>();

	const shared_str& wpn_section = obj->object().cNameSect();

	m_progressRPM.SetProgressPos		(g_lua_wpn_params->m_functorRPM		(*wpn_section, obj->object().lua_game_object()));
	m_progressAccuracy.SetProgressPos	(g_lua_wpn_params->m_functorAccuracy(*wpn_section, obj->object().lua_game_object()));
	m_progressDamage.SetProgressPos		(g_lua_wpn_params->m_functorDamage	(*wpn_section, obj->object().lua_game_object()));
	m_progressHandling.SetProgressPos	(g_lua_wpn_params->m_functorHandling(*wpn_section, obj->object().lua_game_object()));
	//
	string1024 text_to_show;
	char temp_text[64];
	auto pWeapon			= smart_cast<CWeapon*>					(obj);
	auto pWeaponMag			= smart_cast<CWeaponMagazined*>			(obj);
	auto pWeaponMagWGren	= smart_cast<CWeaponMagazinedWGrenade*>	(obj);

	//динамічний лист інформації про зброю
	CUIXml	uiXml;
	float	add_h = 0.f;
	float pos_top = 0.f;
	
	//елемент списку
	uiXml.Init(CONFIG_PATH, UI_PATH, WPN_PARAMS);
	auto marker_ = uiXml.ReadAttrib("wpn_params:list_item", 0, "marker", "• ");
	float list_item_h = uiXml.ReadAttribFlt("wpn_params:list_item", 0, "height");

	float _h = list_item_h;

	//кількість споряджених боєприпасів/обсяг магазину/режим вогню
	auto current_ammo_count_firemode_static = xr_new<CUIStatic>(); current_ammo_count_firemode_static->SetAutoDelete(true);
	CUIXmlInit::InitStatic(uiXml, "wpn_params:current_ammo_count_firemode", 0, current_ammo_count_firemode_static);
	pos_top = current_ammo_count_firemode_static->GetPosTop();
	current_ammo_count_firemode_static->SetWndPos(current_ammo_count_firemode_static->GetPosLeft(), _h + pos_top);
	sprintf_s(temp_text, " %d/%d%s", pWeapon->GetAmmoElapsed(), pWeapon->GetAmmoMagSize(), pWeaponMag->HasFireModes() ? pWeaponMag->GetCurrentFireModeStr() : "");
	strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_current_ammo_count").c_str(), temp_text);
	current_ammo_count_firemode_static->SetText(text_to_show);
	m_CapInfo.AttachChild(current_ammo_count_firemode_static);
	_h += list_item_h + pos_top;
	//тип спорядженого боєприпасу
	if (pWeapon->GetAmmoElapsed()) {
		auto current_ammo_type_static = xr_new<CUIStatic>(); current_ammo_type_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:current_ammo_type", 0, current_ammo_type_static);
		pos_top = current_ammo_type_static->GetPosTop();
		current_ammo_type_static->SetWndPos(current_ammo_type_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %s", pWeapon->GetCurrentAmmo_ShortName());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_current_ammo_type").c_str(), temp_text);
		current_ammo_type_static->SetText(text_to_show);
		m_CapInfo.AttachChild(current_ammo_type_static);
		_h += list_item_h + pos_top;
	}
	//тип приєднаного магазину
	if (pWeapon->HasDetachableMagazine() && pWeaponMag->IsMagazineAttached()) {
		auto current_mag_type_static = xr_new<CUIStatic>(); current_mag_type_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:current_mag_type", 0, current_mag_type_static);
		pos_top = current_mag_type_static->GetPosTop();
		current_mag_type_static->SetWndPos(current_mag_type_static->GetPosLeft(), _h + pos_top);
		sprintf_s(temp_text, " %s", pWeaponMag->GetCurrentMagazine_ShortName());
		strconcat(sizeof(text_to_show), text_to_show, CStringTable().translate("st_current_mag_type").c_str(), temp_text);
		current_mag_type_static->SetText(text_to_show);
		m_CapInfo.AttachChild(current_mag_type_static);
		_h += list_item_h + pos_top;
	}
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
	//сумісні приціли
	if (pWeapon->ScopeAttachable()) {
		//приціл - заголовок
		auto cap_scope_static = xr_new<CUIStatic>(); cap_scope_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_scope", 0, cap_scope_static);
		pos_top = cap_scope_static->GetPosTop();
		cap_scope_static->SetWndPos(cap_scope_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_scope_static);
		_h += list_item_h + pos_top;
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
		//сумісні глушниик - заголовок
		auto cap_silencer_static = xr_new<CUIStatic>(); cap_silencer_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_silencer", 0, cap_silencer_static);
		pos_top = cap_silencer_static->GetPosTop();
		cap_silencer_static->SetWndPos(cap_silencer_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_silencer_static);
		_h += list_item_h + pos_top;
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
		//гранатомет - заголовок
		auto cap_glauncher_static = xr_new<CUIStatic>(); cap_glauncher_static->SetAutoDelete(true);
		CUIXmlInit::InitStatic(uiXml, "wpn_params:cap_glauncher", 0, cap_glauncher_static);
		pos_top = cap_glauncher_static->GetPosTop();
		cap_glauncher_static->SetWndPos(cap_glauncher_static->GetPosLeft(), _h + pos_top);
		m_CapInfo.AttachChild(cap_glauncher_static);
		_h += list_item_h + pos_top;
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

	add_h = _h - list_item_h;
	SetHeight(GetHeight() + add_h);
}

#include "WeaponBinoculars.h"
#include "WeaponKnife.h"
#include "Silencer.h"
#include "Scope.h"
#include "GrenadeLauncher.h"
bool CUIWpnParams::Check(CInventoryItem* obj)
{
	if (!READ_IF_EXISTS(pSettings, r_bool, obj->object().cNameSect(), "show_wpn_properties", true)) // allow to suppress default wpn params
	{
		return false;
	}

	if (pSettings->line_exist(obj->object().cNameSect(), "fire_dispersion_base"))
	{
		if (smart_cast<CWeaponBinoculars*>(obj) ||
			smart_cast<CWeaponKnife*>(obj) ||
			smart_cast<CSilencer*>(obj) ||
			smart_cast<CScope*>(obj) ||
			smart_cast<CGrenadeLauncher*>(obj))
			return false;

        return true;		
	}
	else
		return false;
}