#include "stdafx.h"

#include "customoutfit.h"
#include "PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "BoneProtections.h"
#include "..\Include/xrRender/Kinematics.h"
#include "../Include/xrRender/RenderVisual.h"
#include "UIGameSP.h"
#include "HudManager.h"
#include "ui/UIInventoryWnd.h"
#include "player_hud.h"
#include "xrserver_objects_alife_items.h"

CCustomOutfit::CCustomOutfit()
{
	SetSlot( OUTFIT_SLOT );

	m_HitTypeProtection.clear	();
	m_HitTypeProtection.resize	(ALife::eHitTypeMax);

	m_boneProtection = xr_new<SBoneProtections>();
}

CCustomOutfit::~CCustomOutfit() 
{
	xr_delete(m_boneProtection);
}

void CCustomOutfit::Load(LPCSTR section) 
{
	inherited::Load(section);

	//*_restore_speed
	m_fHealthRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed",		0.f);
//	m_fRadiationRestoreSpeed							= READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed",	0.f);
	m_fSatietyRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed",		0.f);
	m_fPowerRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed",		0.f);
	m_fBleedingRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "bleeding_restore_speed",		0.f);
	m_fPsyHealthRestoreSpeed							= READ_IF_EXISTS(pSettings, r_float, section, "psy_health_restore_speed",	0.f);
	m_fAlcoholRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "alcohol_restore_speed",		0.f);
	m_fThirstRestoreSpeed								= READ_IF_EXISTS( pSettings, r_float, section, "thirst_restore_speed",		0.f );
	//addition
	m_fAdditionalMaxWeight								= READ_IF_EXISTS(pSettings, r_float, section, "additional_max_weight",		0.f);
	m_fAdditionalMaxVolume								= READ_IF_EXISTS(pSettings, r_float, section, "additional_max_volume",		0.f);
	m_fAdditionalWalkAccel								= READ_IF_EXISTS(pSettings, r_float, section, "additional_walk_accel",		0.f);
	m_fAdditionalJumpSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "additional_jump_speed",		0.f);
	//protection
	m_HitTypeProtection[ALife::eHitTypeBurn]			= READ_IF_EXISTS(pSettings, r_float, section, "burn_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeStrike]			= READ_IF_EXISTS(pSettings, r_float, section, "strike_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeShock]			= READ_IF_EXISTS(pSettings, r_float, section, "shock_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeWound]			= READ_IF_EXISTS(pSettings, r_float, section, "wound_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeRadiation]		= READ_IF_EXISTS(pSettings, r_float, section, "radiation_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeTelepatic]		= READ_IF_EXISTS(pSettings, r_float, section, "telepatic_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]	= READ_IF_EXISTS(pSettings, r_float, section, "chemical_burn_protection",	0.f);
	m_HitTypeProtection[ALife::eHitTypeExplosion]		= READ_IF_EXISTS(pSettings, r_float, section, "explosion_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeFireWound]		= READ_IF_EXISTS(pSettings, r_float, section, "fire_wound_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeWound_2]			= READ_IF_EXISTS(pSettings, r_float, section, "wound_2_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypePhysicStrike]	= READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection",	0.f);

	m_ActorVisual			= READ_IF_EXISTS(pSettings, r_string, section, "actor_visual", nullptr);

	m_ef_equipment_type		= pSettings->r_u32(section,"ef_equipment_type");

	m_fPowerLoss			= READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.f);

	m_full_icon_name		= pSettings->r_string(section,"full_icon_name");

	m_bIsHelmetAllowed		= !!READ_IF_EXISTS(pSettings, r_bool, section, "helmet_allowed", true);
}

float CCustomOutfit::GetHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type] * GetCondition();
}

float	CCustomOutfit::HitThruArmour(SHit* pHDS)
{
	float hit_power = pHDS->power;
	float BoneArmour = m_boneProtection->getBoneArmour(pHDS->boneID) * GetCondition() * (1 - pHDS->ap);
	float NewHitPower = hit_power - BoneArmour;

	if (NewHitPower < hit_power * m_boneProtection->m_fHitFrac) 
		return hit_power * m_boneProtection->m_fHitFrac;

	return NewHitPower;
};

BOOL	CCustomOutfit::BonePassBullet					(int boneID)
{
	return m_boneProtection->getBonePassBullet(s16(boneID));
};

#include "torch.h"
void	CCustomOutfit::OnMoveToSlot		(EItemPlace prevPlace)
{
	inherited::OnMoveToSlot(prevPlace);
	
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			if (m_ActorVisual.size())
			{
				pActor->ChangeVisual(m_ActorVisual);
			}
			if(pSettings->line_exist(cNameSect(),"bones_koeff_protection")){
				m_boneProtection->reload( pSettings->r_string(cNameSect(),"bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()) );
			}

			if (pSettings->line_exist(cNameSect(), "player_hud_section"))
				g_player_hud->load(pSettings->r_string(cNameSect(), "player_hud_section"));
			else
				g_player_hud->load_default();

			if(!m_bIsHelmetAllowed)
				m_pCurrentInventory->DropSlotsToRuck(HELMET_SLOT);
		}
	}
}

void CCustomOutfit::OnMoveToRuck(EItemPlace prevPlace) 
{
	inherited::OnMoveToRuck(prevPlace);

	if (m_pCurrentInventory && !Level().is_removing_objects())
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && prevPlace == eItemPlaceSlot)
		{
			if (m_ActorVisual.size())
			{
				shared_str DefVisual = pActor->GetDefaultVisualOutfit();
				if (DefVisual.size())
				{
					pActor->ChangeVisual(DefVisual);
				}
			}

			g_player_hud->load_default();
		}
	}
}

u32	CCustomOutfit::ef_equipment_type	() const
{
	return		(m_ef_equipment_type);
}

float CCustomOutfit::GetPowerLoss() 
{
	if (m_fPowerLoss<1 && GetCondition() <= 0)
	{
		return 1.0f;			
	};
	return m_fPowerLoss;
};

float CCustomOutfit::GetAdditionalMaxWeight()
{
	return m_fAdditionalMaxWeight * GetCondition();
}

float CCustomOutfit::GetAdditionalMaxVolume()
{
	return m_fAdditionalMaxVolume * GetCondition();
}

float CCustomOutfit::GetAdditionalWalkAccel()
{
	return m_fAdditionalWalkAccel * GetCondition();
}

float CCustomOutfit::GetAdditionalJumpSpeed()
{
	return m_fAdditionalJumpSpeed * GetCondition();
}