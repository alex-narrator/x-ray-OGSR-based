#include "stdafx.h"
#include "Helmet.h"
#include "BoneProtections.h"
#include "Actor.h"
#include "Inventory.h"
#include "HudManager.h"
#include "..\Include/xrRender/Kinematics.h"

CHelmet::CHelmet() {
	SetSlot(HELMET_SLOT);
	m_boneProtection = xr_new<SBoneProtections>();
}

CHelmet::~CHelmet() {
	xr_delete(m_boneProtection);
	xr_delete(m_UIVisor);
}

void CHelmet::Load(LPCSTR section){
	inherited::Load(section);
	m_fPowerLoss	= READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.f);
	m_VisorTexture	= READ_IF_EXISTS(pSettings, r_string, section, "visor_texture", nullptr);
}

float CHelmet::HitThruArmour(SHit* pHDS){
	float hit_power = pHDS->power;
	float BoneArmour = m_boneProtection->getBoneArmour(pHDS->boneID) * GetCondition() * (1 - pHDS->ap);
	float NewHitPower = hit_power - BoneArmour;

	if (NewHitPower < hit_power * m_boneProtection->m_fHitFrac)
		return hit_power * m_boneProtection->m_fHitFrac;

	return NewHitPower;
};

BOOL CHelmet::BonePassBullet(int boneID){
	return m_boneProtection->getBonePassBullet(s16(boneID));
};

void CHelmet::OnMoveToSlot(EItemPlace prevPlace)
{
	inherited::OnMoveToSlot(prevPlace);

	if (m_pCurrentInventory) {
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor) {
			if (pSettings->line_exist(cNameSect(), "bones_koeff_protection")) {
				m_boneProtection->reload(pSettings->r_string(cNameSect(), "bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()));
			}
			if (m_UIVisor)
				xr_delete(m_UIVisor);
			if (!!m_VisorTexture) {
				m_UIVisor = xr_new<CUIStaticItem>();
				m_UIVisor->Init(m_VisorTexture.c_str(), psHUD_Flags.test(HUD_TEXTURES_AUTORESIZE) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
			}
			pActor->UpdateVisorEfects();
		}
	}
}

void CHelmet::OnMoveToRuck(EItemPlace prevPlace)
{
	inherited::OnMoveToRuck(prevPlace);

	if (m_pCurrentInventory && !Level().is_removing_objects()) {
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && prevPlace == eItemPlaceSlot) {
			if (m_UIVisor)
				xr_delete(m_UIVisor);
			pActor->UpdateVisorEfects();
		}
	}
}

float CHelmet::GetPowerLoss(){
	if (m_fPowerLoss < 1 && GetCondition() <= 0){
		return 1.0f;
	};
	return m_fPowerLoss;
};

void CHelmet::DrawHUDMask() {
	if (HasVisor()) {
		m_UIVisor->SetPos(0, 0);
		m_UIVisor->SetRect(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
		m_UIVisor->Render();
	}
}

bool  CHelmet::can_be_attached() const {
	const CActor* pA = smart_cast<const CActor*>(H_Parent());
	return pA ? (pA->GetHelmet() == this) : true;
}