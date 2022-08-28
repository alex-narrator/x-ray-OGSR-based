#include "stdafx.h"
#include "NightVisionDevice.h"
#include "xrserver_objects_alife_items.h"
#include "actor.h"
#include "Inventory.h"
#include "level.h"
#include "HUDManager.h"
#include "actorEffector.h"
#include "ai_sounds.h"

CNightVisionDevice::CNightVisionDevice(void){
	SetSlot(ON_HEAD_SLOT);
}

CNightVisionDevice::~CNightVisionDevice(void){
	HUD_SOUND::DestroySound(SndNightVisionOn);
	HUD_SOUND::DestroySound(SndNightVisionOff);
	HUD_SOUND::DestroySound(SndNightVisionIdle);
	HUD_SOUND::DestroySound(SndNightVisionBroken);
	xr_delete(m_UINightVision);
}

void CNightVisionDevice::Load(LPCSTR section){
	inherited::Load(section);

	if (pSettings->line_exist(section, "snd_night_vision_on"))
		HUD_SOUND::LoadSound(section, "snd_night_vision_on", SndNightVisionOn, SOUND_TYPE_ITEM_USING);
	if (pSettings->line_exist(section, "snd_night_vision_off"))
		HUD_SOUND::LoadSound(section, "snd_night_vision_off", SndNightVisionOff, SOUND_TYPE_ITEM_USING);
	if (pSettings->line_exist(section, "snd_night_vision_idle"))
		HUD_SOUND::LoadSound(section, "snd_night_vision_idle", SndNightVisionIdle, SOUND_TYPE_ITEM_USING);
	if (pSettings->line_exist(section, "snd_night_vision_broken"))
		HUD_SOUND::LoadSound(section, "snd_night_vision_broken", SndNightVisionBroken, SOUND_TYPE_ITEM_USING);

	m_NightVisionSect		= READ_IF_EXISTS(pSettings, r_string, section, "night_vision_effector", nullptr);
	m_NightVisionTexture	= READ_IF_EXISTS(pSettings, r_string, section, "night_vision_texture", nullptr);
}

void CNightVisionDevice::SwitchNightVision(){
	if (OnClient()) return;
	SwitchNightVision(!m_bNightVisionOn);
}

void CNightVisionDevice::SwitchNightVision(bool vision_on){
	auto pA = smart_cast<CActor*>(H_Parent());
	if (!pA)
		return;

	auto* pActorTorch = pA->GetNightVisionDevice();
	if (pActorTorch && pActorTorch != this)
		return;

	bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());

	if (!!m_NightVisionSect)
	{
		const char* disabled_names = READ_IF_EXISTS(pSettings, r_string, cNameSect(), "disabled_maps", nullptr);
		const char* curr_map = Level().name().c_str();
		bool b_allow = true;
		if (disabled_names) {
			u32 cnt = _GetItemCount(disabled_names);
			string512 tmp;
			for (u32 i = 0; i < cnt; ++i) {
				_GetItem(disabled_names, i, tmp);
				if (!stricmp(tmp, curr_map)) {
					b_allow = false;
					break;
				}
			}
		}

		if (!b_allow)
		{
			HUD_SOUND::PlaySound(SndNightVisionBroken, pA->Position(), pA, bPlaySoundFirstPerson);
			return;
		}
		else
		{
			m_bNightVisionOn = vision_on;

			if (m_bNightVisionOn)
			{
				CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
				if (!pp)
				{
					AddEffector(pA, effNightvision, m_NightVisionSect);
					HUD_SOUND::PlaySound(SndNightVisionOn, pA->Position(), pA, bPlaySoundFirstPerson);
					HUD_SOUND::PlaySound(SndNightVisionIdle, pA->Position(), pA, bPlaySoundFirstPerson, true);
				}
			}
		}
	}
	else {
		m_bNightVisionOn = false;
	}

	if (!m_bNightVisionOn)
	{
		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
		if (pp)
		{
			pp->Stop(1.0f);
			HUD_SOUND::PlaySound(SndNightVisionOff, pA->Position(), pA, bPlaySoundFirstPerson);
			HUD_SOUND::StopSound(SndNightVisionIdle);
		}
	}
}

void CNightVisionDevice::UpdateSwitchNightVision(){
	if (OnClient()) return;
	auto* pA = smart_cast<CActor*>(H_Parent());
	if (pA && m_bNightVisionOn && !pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision))
		SwitchNightVision(true);
}

BOOL CNightVisionDevice::net_Spawn(CSE_Abstract* DC){
	bool res = inherited::net_Spawn(DC);

	auto night_vis = smart_cast<CSE_ALifeItemNightVisionDevice*>(DC);

	m_bNightVisionOn = night_vis->m_nightvision_active;

	return res;
}

void CNightVisionDevice::net_Destroy(){
	SwitchNightVision(false);
	inherited::net_Destroy();
}

void CNightVisionDevice::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	SwitchNightVision(false);

	HUD_SOUND::StopSound(SndNightVisionOn);
	HUD_SOUND::StopSound(SndNightVisionOff);
	HUD_SOUND::StopSound(SndNightVisionIdle);

	//m_NightVisionChargeTime		= m_NightVisionRechargeTime;
}

void CNightVisionDevice::UpdateCL(){
	inherited::UpdateCL();
	UpdateSwitchNightVision();
}

void CNightVisionDevice::net_Export(CSE_Abstract* E) {
	inherited::net_Export(E);
	auto night_vis = smart_cast<CSE_ALifeItemNightVisionDevice*>(E);
	night_vis->m_nightvision_active = m_bNightVisionOn;
	const CActor* pA = smart_cast<const CActor*>(H_Parent());
	night_vis->m_attached = (pA && pA->attached(this));
}

bool  CNightVisionDevice::can_be_attached() const{
	const CActor* pA = smart_cast<const CActor*>(H_Parent());
	return pA ? (pA->GetNightVisionDevice() == this) : true;
}

void CNightVisionDevice::afterAttach() {
	inherited::afterAttach();
	if (smart_cast<CActor*>(H_Parent())) {
		if (m_UINightVision)
			xr_delete(m_UINightVision);
		if (!!m_NightVisionTexture) {
			m_UINightVision = xr_new<CUIStaticItem>();
			m_UINightVision->Init(m_NightVisionTexture.c_str(), psHUD_Flags.test(HUD_TEXTURES_AUTORESIZE) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
		}
	}
}

void CNightVisionDevice::afterDetach(){
	inherited::afterDetach();
	SwitchNightVision(false);
	if (smart_cast<CActor*>(H_Parent())) {
		SwitchNightVision(false);
		if (m_UINightVision)
			xr_delete(m_UINightVision);
	}
}

void CNightVisionDevice::DrawHUDMask() {
	if (m_UINightVision && m_bNightVisionOn && !!m_NightVisionTexture) {
		m_UINightVision->SetPos(0, 0);
		m_UINightVision->SetRect(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
		m_UINightVision->Render();
	}
}