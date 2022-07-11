#include "stdafx.h"
#include "ui_af_params.h"
#include "UIStatic.h"
#include "../object_broker.h"
#include "UIXmlInit.h"

#include "../inventory_item.h"
#include "../Artifact.h"
#include "../CustomOutfit.h"
//#include "../BackPack.h"
#include "../Actor.h"
#include "../ActorCondition.h"

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
	"additional_max_volume",
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
	"ui_inv_volume",
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

void CUIArtefactParams::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR _base				= "af_params";
	if (!xml_doc.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow		(xml_doc, _base, 0, this);

	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		strconcat				(sizeof(_buff),_buff, _base, ":static_", af_item_sect_names[i]);

		if (xml_doc.NavigateToNode(_buff, 0)) 
		{
			m_info_items[i] = xr_new<CUIStatic>();
			CUIStatic* _s = m_info_items[i];
			_s->SetAutoDelete(false);
			CUIXmlInit::InitStatic(xml_doc, _buff, 0, _s);
		}
	}
}

float CUIArtefactParams::GetRestoreParam(u32 i, CInventoryItem* obj)
{
	float r = 0;

	if (!obj) return r;

	auto artefact = smart_cast<CArtefact*>		(obj);
	auto outfit = smart_cast<CCustomOutfit*>	(obj);

	switch (i)
	{
	case _item_health_restore_speed:
	{
		if(artefact)
			r = artefact->m_fHealthRestoreSpeed * artefact->GetRandomKoef();
		else if(outfit)
			r = outfit->m_fHealthRestoreSpeed;
	}break;
	case _item_radiation_restore_speed:
	{
		r = obj->m_fRadiationRestoreSpeed;
		if (artefact)
			r *= artefact->GetRandomKoef();
	}break;
	case _item_satiety_restore_speed:
	{
		if (artefact)
			r = artefact->m_fSatietyRestoreSpeed * artefact->GetRandomKoef();
		else if (outfit)
			r = outfit->m_fSatietyRestoreSpeed;
	}break;
	case _item_thirst_restore_speed:
	{
		if (artefact)
			r = artefact->m_fThirstRestoreSpeed * artefact->GetRandomKoef();
		else if (outfit)
			r = outfit->m_fThirstRestoreSpeed;
	}break;
	case _item_power_restore_speed:
	{
		if (artefact)
			r = artefact->m_fPowerRestoreSpeed * artefact->GetRandomKoef();
		else if (outfit)
			r = outfit->m_fPowerRestoreSpeed;
	}break;
	case _item_bleeding_restore_speed:
	{
		if (artefact)
			r = artefact->m_fBleedingRestoreSpeed * artefact->GetRandomKoef();
		else if (outfit)
			r = outfit->m_fBleedingRestoreSpeed;
	}break;
	case _item_psy_health_restore_speed:
	{
		if (artefact)
			r = artefact->m_fPsyHealthRestoreSpeed * artefact->GetRandomKoef();
		else if (outfit)
			r = outfit->m_fPsyHealthRestoreSpeed;
	}break;
	case _item_alcohol_restore_speed:
	{
		if (artefact)
			r = artefact->m_fAlcoholRestoreSpeed * artefact->GetRandomKoef();
		else if (outfit)
			r = outfit->m_fAlcoholRestoreSpeed;
	}break;
	case _item_additional_walk_accel:
	{
		if (artefact)
			r = artefact->GetAdditionalWalkAccel();
		else if (outfit)
			r = outfit->GetAdditionalWalkAccel();
	}break;
	case _item_additional_jump_speed:
	{
		if (artefact)
			r = artefact->GetAdditionalJumpSpeed();
		else if (outfit)
			r = outfit->GetAdditionalJumpSpeed();
	}break;
	case _item_additional_weight:
	{
		if (artefact)
			r = artefact->GetAdditionalMaxWeight();
		else if (outfit)
			r = outfit->GetAdditionalMaxWeight();
	}break;
	case _item_additional_volume:
	{
		if (artefact)
			r = artefact->GetAdditionalMaxVolume();
		else if (outfit)
			r = outfit->GetAdditionalMaxVolume();
	}break;
	}

	if (i < _item_additional_walk_accel)
	{
		if(i != _item_radiation_restore_speed || artefact)
			r *= obj->GetCondition();
	}

	return r;
}

#include "../string_table.h"
void CUIArtefactParams::SetInfo(CInventoryItem* obj)
{
	if (!obj) return;

	auto artefact	= smart_cast<CArtefact*>		(obj);
	auto outfit		= smart_cast<CCustomOutfit*>	(obj);
//	auto backpack	= smart_cast<CBackPack*>		(obj);

//	R_ASSERT2(art, "object is not CArtefact");
	CActor *pActor = Actor();
	if (!pActor) return;

	bool show_window = !psActorFlags.is(AF_ARTEFACT_DETECTOR_CHECK) || pActor->HasDetector() /*|| outfit || backpack*/;

	Show(show_window);

	string128					_buff;
	float						_h = 0.0f;
	DetachAll					();

	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];

		float					_val = 0.f;

		//if (artefact)
		//{
		//	if (pDetector && !pDetector->IsGeigerCounter() && (i == _item_radiation_restore_speed || i == _item_radiation_immunity)) continue;
		//	if (pDetector && !pDetector->IsAnomDetector() && (i != _item_radiation_restore_speed && i != _item_radiation_immunity)) continue;
		//}
//
		if(i<_max_item_index1)
		{
			_val = GetRestoreParam(i, obj);
		}
		else
		{
			if		(artefact)	_val = artefact->GetHitTypeProtection	(ALife::EHitType(i - _max_item_index1));
			else if (outfit)	_val = outfit->GetHitTypeProtection		(ALife::EHitType(i - _max_item_index1));
//			else if (backpack)	_val = backpack->GetHitTypeProtection	(ALife::EHitType(i - _max_item_index1));
			else continue;
		}

		if (fis_zero(_val))				continue;
		if (i != _item_additional_weight && i != _item_additional_volume)
			_val *= 100.0f;

		LPCSTR _sn = "%";

		if (i == _item_radiation_restore_speed)
		{
			_val /= 100.0f;
			_sn = *CStringTable().translate("st_rad");
		}
		//
		else if (i == _item_additional_weight || i == _item_additional_volume)
		{
			_sn = i == _item_additional_weight ? *CStringTable().translate("st_kg") : *CStringTable().translate("st_l");
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
