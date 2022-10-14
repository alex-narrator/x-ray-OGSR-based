#include "stdafx.h"
#include "ui_af_params.h"
#include "UIStatic.h"
#include "../object_broker.h"
#include "UIXmlInit.h"

#include "../string_table.h"

#include "../inventory_item.h"
#include "../Artifact.h"
#include "../CustomOutfit.h"
#include "../Backpack.h"
#include "../Actor.h"
#include "../ActorCondition.h"

constexpr auto AF_PARAMS = "af_params.xml";

CUIArtefactParams::CUIArtefactParams()
{
	Memory.mem_fill			(m_info_items, 0, sizeof(m_info_items));
}

CUIArtefactParams::~CUIArtefactParams()
{
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];
		xr_delete				(_s);
	}
}

LPCSTR af_item_sect_names[] = {
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
	"additional_max_weight",
//	"additional_max_volume",
	//
	"burn_immunity",
	"strike_immunity",
	"shock_immunity",
	"wound_immunity",		
	"radiation_immunity",
	"telepatic_immunity",
	"chemical_burn_immunity",
	"explosion_immunity",
	"fire_wound_immunity",
};

LPCSTR af_item_param_names[] = {
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
	"ui_inv_weight",
//	"ui_inv_volume",
	//
	"ui_inv_outfit_burn_protection",			// "(burn_imm)",
	"ui_inv_outfit_shock_protection",			// "(shock_imm)",
	"ui_inv_outfit_strike_protection",			// "(strike_imm)",
	"ui_inv_outfit_wound_protection",			// "(wound_imm)",
	"ui_inv_outfit_radiation_protection",		// "(radiation_imm)",
	"ui_inv_outfit_telepatic_protection",		// "(telepatic_imm)",
	"ui_inv_outfit_chemical_burn_protection",	// "(chemical_burn_imm)",
	"ui_inv_outfit_explosion_protection",		// "(explosion_imm)",
	"ui_inv_outfit_fire_wound_protection",		// "(fire_wound_imm)",
};

void CUIArtefactParams::Init()
{
	CUIXml uiXml;
	uiXml.Init(CONFIG_PATH, UI_PATH, AF_PARAMS);

	LPCSTR _base				= "af_params";
	if (!uiXml.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow		(uiXml, _base, 0, this);

	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		strconcat				(sizeof(_buff),_buff, _base, ":static_", af_item_sect_names[i]);

		if (uiXml.NavigateToNode(_buff, 0))
		{
			m_info_items[i] = xr_new<CUIStatic>();
			CUIStatic* _s = m_info_items[i];
			_s->SetAutoDelete(false);
			CUIXmlInit::InitStatic(uiXml, _buff, 0, _s);
		}
	}
}

void CUIArtefactParams::SetInfo(CInventoryItem* obj)
{
	if (!obj) return;

	auto artefact	= smart_cast<CArtefact*>		(obj);

//	R_ASSERT2(art, "object is not CArtefact");
	CActor *pActor = Actor();
	if (!pActor) return;

	bool show_window = true;
	if (artefact) {
		show_window = !psActorFlags.is(AF_ARTEFACT_DETECTOR_CHECK) || pActor->HasDetectorWorkable();
	}

	Show(show_window);

	string128					_buff;
	float						_h{};
	DetachAll					();

	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];

		float					_val{};

		if(i< _hit_type_protection_index){
			_val = obj->GetItemEffect(CInventoryItem::ItemEffects(i));
		}
		else{
			_val = obj->GetHitTypeProtection(ALife::EHitType(i - _hit_type_protection_index));
		}

		if (fis_zero(_val))				continue;
		if (i != _item_additional_weight)
			_val *= 100.0f;

		LPCSTR _sn = "%";

		if (i == _item_radiation_restore_speed)
		{
			_val /= 100.0f;
			_sn = *CStringTable().translate("st_rad");
		}
		//
		else if (i == _item_additional_weight)
		{
			_sn = *CStringTable().translate("st_kg");
		}

		LPCSTR _color = (_val>0)?"%c[green]":"%c[red]";
		
		if (i == _item_bleeding_restore_speed || i == _item_alcohol_restore_speed)
			_val		*=	-1.0f;

		if (i == _item_bleeding_restore_speed || i == _item_radiation_restore_speed || i == _item_alcohol_restore_speed)
			_color = (_val>0)?"%c[red]":"%c[green]";



		sprintf_s				(_buff, "%s %s %+.1f %s", 
									CStringTable().translate(af_item_param_names[i]).c_str(), 
									_color, 
									_val, 
									_sn);
		_s->SetText				(_buff);
		_s->SetWndPos			(_s->GetWndPos().x, _h);
		_h						+= _s->GetWndSize().y;
		AttachChild				(_s);
	}
	SetHeight					(show_window ? _h : 0.f);
}
