#include "StdAfx.h"
#include "UIOutfitInfo.h"
#include "UIXmlInit.h"
#include "UIStatic.h"
#include "UIScrollView.h"
#include "Actor.h"
#include "Inventory.h"
#include "Artifact.h"
#include "CustomOutfit.h"
#include "Backpack.h"
#include "Helmet.h"
#include "Vest.h"
#include "../string_table.h"

CUIOutfitInfo::CUIOutfitInfo()
{
}

CUIOutfitInfo::~CUIOutfitInfo()
{
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_items[i];
		xr_delete				(_s);
	}
}

LPCSTR _imm_names []={
	"health_restore_speed",
	"radiation_restore_speed",
	"satiety_restore_speed",
	"thirst_restore_speed",
	"power_restore_speed",
	"bleeding_restore_speed",
	"psy_health_restore_speed",
	"alcohol_restore_speed",
	//
	"additional_walk_accel",
	"additional_jump_speed",
	//
	//"additional_max_weight",
	//"additional_max_volume",
	//
	"burn_immunity",
	"shock_immunity",
	"strike_immunity",
	"wound_immunity",		
	"radiation_immunity",
	"telepatic_immunity",
	"chemical_burn_immunity",
	"explosion_immunity",
	"fire_wound_immunity",
};

LPCSTR _imm_st_names[]={
	"ui_inv_health",
	"ui_inv_radiation",
	"ui_inv_satiety",
	"ui_inv_thirst",
	"ui_inv_power",
	"ui_inv_bleeding",
	"ui_inv_psy_health",
	"ui_inv_alcohol",
	//
	"ui_inv_walk_accel",
	"ui_inv_jump_speed",
	//
	//"ui_inv_weight",
	//"ui_inv_volume",
	//
	"ui_inv_outfit_burn_protection",
	"ui_inv_outfit_shock_protection",
	"ui_inv_outfit_strike_protection",
	"ui_inv_outfit_wound_protection",
	"ui_inv_outfit_radiation_protection",
	"ui_inv_outfit_telepatic_protection",
	"ui_inv_outfit_chemical_burn_protection",
	"ui_inv_outfit_explosion_protection",
	"ui_inv_outfit_fire_wound_protection",
};

void CUIOutfitInfo::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR _base				= "outfit_info";

	string256					_buff;
	CUIXmlInit::InitWindow		(xml_doc, _base, 0, this);

	m_listWnd					= xr_new<CUIScrollView>(); m_listWnd->SetAutoDelete(true);
	AttachChild					(m_listWnd);
	strconcat					(sizeof(_buff),_buff, _base, ":scroll_view");
	CUIXmlInit::InitScrollView	(xml_doc, _buff, 0, m_listWnd);

	for (u32 i = _item_start; i < _max_item_index; ++i)
	{
		strconcat(sizeof(_buff), _buff, _base, ":static_", _imm_names[i]);

		if (xml_doc.NavigateToNode(_buff, 0))
		{
			m_items[i] = xr_new<CUIStatic>();
			CUIStatic* _s = m_items[i];
			_s->SetAutoDelete(false);
			CUIXmlInit::InitStatic(xml_doc, _buff, 0, _s);
		}
	}

}

#include "script_game_object.h"

void CUIOutfitInfo::Update()
{
	string128 _buff;

	auto outfit		= Actor()->GetOutfit();
	auto backpack	= Actor()->GetBackpack();
	auto helmet		= Actor()->GetHelmet();
	auto vest		= Actor()->GetVest();

	m_listWnd->Clear(false); // clear existing items and do not scroll to top

	for (u32 i = _item_start; i < _max_item_index; ++i)
	{
		CUIStatic* _s = m_items[i];

		if (!_s) continue;

		float _val{};

		if (i < _hit_type_protection_index){
			if (i < _item_additional_walk_accel){
				_val = Actor()->GetRestoreParam(CActor::ActorRestoreParams(i));
			}
			else {
				if (outfit)
					_val = outfit->GetItemEffect(CInventoryItem::ItemEffects(i));
				if (vest)
					_val += vest->GetItemEffect(CInventoryItem::ItemEffects(i));
				if (backpack)
					_val += backpack->GetItemEffect(CInventoryItem::ItemEffects(i));
				if (helmet)
					_val += helmet->GetItemEffect(CInventoryItem::ItemEffects(i));

				if (!psActorFlags.is(AF_ARTEFACT_DETECTOR_CHECK) || Actor()->HasDetectorWorkable()) {
					_val += Actor()->GetTotalArtefactsEffect(i);
				}
			}
		} else {
			if (outfit)
				_val = outfit->GetHitTypeProtection(ALife::EHitType(i - _hit_type_protection_index));
			if (vest)
				_val += vest->GetHitTypeProtection(ALife::EHitType(i - _hit_type_protection_index));
			if (backpack)
				_val += backpack->GetHitTypeProtection(ALife::EHitType(i - _hit_type_protection_index));
			if (helmet)
				_val += helmet->GetHitTypeProtection(ALife::EHitType(i - _hit_type_protection_index));

			if (!psActorFlags.is(AF_ARTEFACT_DETECTOR_CHECK) || Actor()->HasDetectorWorkable()) {
				_val += (1.0f - Actor()->GetArtefactsProtection(1.0f, ALife::EHitType(i - _hit_type_protection_index)));
			}
		}

		if (fis_zero(_val))
			continue;

		LPCSTR _sn = "%";
		_val *= 100.0f;

		if (i == _item_radiation_restore_speed){
			_val /= 100.0f;
			_sn = CStringTable().translate("st_rad").c_str();
		}

		if (i == _item_bleeding_restore_speed || i == _item_alcohol_restore_speed) {
			_val *= -1.0f;
		}

		LPCSTR _color = (_val > 0) ? "%c[green]" : "%c[red]";

		if (i == _item_bleeding_restore_speed || i == _item_radiation_restore_speed || i == _item_alcohol_restore_speed) {
			_color = (_val > 0) ? "%c[red]" : "%c[green]";
		}

		sprintf_s(_buff, "%s %s %+.f %s",
			CStringTable().translate(_imm_st_names[i]).c_str(),
			_color,
			_val,
			_sn);

		_s->SetText(_buff);

		m_listWnd->AddWindow(_s, false);
	}

	if (pSettings->line_exist("engine_callbacks", "ui_actor_info_callback")){
		const char* callback = pSettings->r_string("engine_callbacks", "ui_actor_info_callback");
		if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function)){
			lua_function(m_listWnd, outfit ? outfit->lua_game_object() : nullptr);
		}
	}
}
