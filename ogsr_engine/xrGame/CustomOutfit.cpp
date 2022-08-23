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

	m_boneProtection = xr_new<SBoneProtections>();
}

CCustomOutfit::~CCustomOutfit() 
{
	xr_delete(m_boneProtection);
}

void CCustomOutfit::Load(LPCSTR section) 
{
	inherited::Load(section);

	m_ActorVisual			= READ_IF_EXISTS(pSettings, r_string, section, "actor_visual", nullptr);

	m_ef_equipment_type		= pSettings->r_u32(section,"ef_equipment_type");

	m_fPowerLoss			= READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.f);

	m_full_icon_name		= pSettings->r_string(section,"full_icon_name");

	m_bIsHelmetAllowed		= !!READ_IF_EXISTS(pSettings, r_bool, section, "helmet_allowed", true);
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
	
	if (m_pCurrentInventory){
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor){
			if (m_ActorVisual.size()){
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

	if (m_pCurrentInventory && !Level().is_removing_objects()){
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && prevPlace == eItemPlaceSlot){
			if (m_ActorVisual.size()){
				shared_str DefVisual = pActor->GetDefaultVisualOutfit();
				if (DefVisual.size()){
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