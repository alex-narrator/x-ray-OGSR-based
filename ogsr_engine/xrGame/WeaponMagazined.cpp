#include "stdafx.h"
#include "hudmanager.h"
#include "WeaponMagazined.h"
#include "entity.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "Addons.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "WeaponBinoculars.h"
#include "WeaponBinocularsVision.h"
#include "ai_object_location.h"

#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include <regex>

#include "uigamecustom.h"
#include "string_table.h"
#include "../xr_3da/LightAnimLibrary.h"

CWeaponMagazined::CWeaponMagazined(LPCSTR name, ESoundTypes eSoundType) : CWeapon(name)
{
	m_eSoundShow		= ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
	m_eSoundHide		= ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
	m_eSoundShot		= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
	m_eSoundEmptyClick	= ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
	m_eSoundReload		= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
	//
	m_eSoundShutter		= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);

	m_pSndShotCurrent = NULL;
	m_sSilencerFlameParticles = m_sSilencerSmokeParticles = NULL;

	m_bFireSingleShot = false;
	m_iShotNum = 0;
	m_iQueueSize = WEAPON_ININITE_QUEUE;
	m_bLockType = false;

	m_binoc_vision				= nullptr;
	m_bVision					= false;
	binoc_vision_sect			= nullptr;
	m_bNightVisionEnabled		= false;
	m_bNightVisionSwitchedOn	= true;
	m_bNightVisionOn			= false;
	//
	m_bHasChamber				= true;
	m_LastLoadedMagType			= 0;
	m_bIsMagazineAttached		= true;
	//
	m_fAttachedSilencerCondition		= 1.f;
	m_fAttachedScopeCondition			= 1.f;
	m_fAttachedGrenadeLauncherCondition = 1.f;
	//
}

CWeaponMagazined::~CWeaponMagazined()
{
	// sounds
	HUD_SOUND::DestroySound(sndShow);
	HUD_SOUND::DestroySound(sndHide);
	HUD_SOUND::DestroySound(sndShot);
	HUD_SOUND::DestroySound(sndSilencerShot);
	HUD_SOUND::DestroySound(sndEmptyClick);
	HUD_SOUND::DestroySound(sndReload);
	HUD_SOUND::DestroySound(sndReloadPartly);
	HUD_SOUND::DestroySound(sndFireModes);
	HUD_SOUND::DestroySound(sndZoomChange);
	//
	HUD_SOUND::DestroySound(sndShutter);
	//
	HUD_SOUND::DestroySound(sndZoomIn);
	HUD_SOUND::DestroySound(sndZoomOut);
	//
	HUD_SOUND::DestroySound(SndNightVisionOn);
	HUD_SOUND::DestroySound(SndNightVisionOff);
	HUD_SOUND::DestroySound(SndNightVisionIdle);
	HUD_SOUND::DestroySound(SndNightVisionBroken);
	//
	HUD_SOUND::DestroySound(SndLaserSwitch);
	HUD_SOUND::DestroySound(SndFlashlightSwitch);
	//
	if (m_binoc_vision)
		xr_delete(m_binoc_vision);
}


void CWeaponMagazined::StopHUDSounds		()
{
	HUD_SOUND::StopSound(sndShow);
	HUD_SOUND::StopSound(sndHide);
	
	HUD_SOUND::StopSound(sndEmptyClick);
	HUD_SOUND::StopSound(sndReload);
	HUD_SOUND::StopSound(sndReloadPartly);
	HUD_SOUND::StopSound(sndFireModes);
	HUD_SOUND::StopSound(sndZoomChange);

	HUD_SOUND::StopSound(sndShot);
	HUD_SOUND::StopSound(sndSilencerShot);
	//
	HUD_SOUND::StopSound(sndShutter);
	//
	HUD_SOUND::StopSound(sndZoomIn);
	HUD_SOUND::StopSound(sndZoomOut);
	//
	HUD_SOUND::StopSound(SndNightVisionOn);
	HUD_SOUND::StopSound(SndNightVisionOff);
	HUD_SOUND::StopSound(SndNightVisionIdle);
	HUD_SOUND::StopSound(SndNightVisionBroken);
	//
	HUD_SOUND::StopSound(SndLaserSwitch);
	HUD_SOUND::StopSound(SndFlashlightSwitch);

	inherited::StopHUDSounds();
}

void CWeaponMagazined::net_Destroy()
{
	inherited::net_Destroy();
	if (m_binoc_vision)
		xr_delete(m_binoc_vision);
}

BOOL CWeaponMagazined::net_Spawn(CSE_Abstract* DC)
{
	BOOL bRes = inherited::net_Spawn(DC);
	const auto wpn = smart_cast<CSE_ALifeItemWeaponMagazined*>(DC);
	m_iCurFireMode = wpn->m_u8CurFireMode;
	if ( HasFireModes() && m_iCurFireMode >= m_aFireModes.size() ) {
	  Msg( "! [%s]: %s: wrong m_iCurFireMode[%u/%u]", __FUNCTION__, cName().c_str(), m_iCurFireMode, m_aFireModes.size() - 1 );
	  m_iCurFireMode = m_aFireModes.size() - 1;
	  auto se_obj = alife_object();
	  if ( se_obj ) {
	    auto W = smart_cast<CSE_ALifeItemWeaponMagazined*>( se_obj );
	    W->m_u8CurFireMode = m_iCurFireMode;
	  }
	}
	SetQueueSize(GetCurrentFireMode());
	//multitype ammo loading
	xr_vector<u8> ammo_ids = wpn->m_AmmoIDs;
	for (u32 i = 0; i < ammo_ids.size(); i++)
	{
		u8 LocalAmmoType = ammo_ids[i];
		if (i >= m_magazine.size()) continue;
		CCartridge& l_cartridge = *(m_magazine.begin() + i);
		if (LocalAmmoType == l_cartridge.m_LocalAmmoType) continue;
		l_cartridge.Load(*m_ammoTypes[LocalAmmoType], LocalAmmoType);
	}
	//
	m_bIsMagazineAttached = wpn->m_bIsMagazineAttached;
//	Msg("weapon [%s] spawned with magazine status [%s]", cName().c_str(), wpn->m_bIsMagazineAttached ? "atached" : "detached");

	if (IsMagazineAttached())
		m_LastLoadedMagType = m_ammoType;
	//
	if (IsSilencerAttached())
		m_fAttachedSilencerCondition = wpn->m_fAttachedSilencerCondition;
	//
	if (IsScopeAttached())
	{
		m_fRTZoomFactor				= wpn->m_fRTZoomFactor;
		m_bNightVisionSwitchedOn	= wpn->m_bNightVisionSwitchedOn;
		m_fAttachedScopeCondition	= wpn->m_fAttachedScopeCondition;
	}
	if (IsGrenadeLauncherAttached())
		m_fAttachedGrenadeLauncherCondition = wpn->m_fAttachedGrenadeLauncherCondition;
	//
	if (HasDetachableMagazine() && IsMagazineAttached())
	{
		int chamber_ammo = HasChamber() ? 1 : 0;	//учтём дополнительный патрон в патроннике
		shared_str ammoSect = m_ammoTypes[m_ammoType];
		int mag_size = AmmoTypeIsMagazine(m_ammoType) ? (int)pSettings->r_s32(ammoSect, "box_size") : 0;
		iMagazineSize = mag_size + chamber_ammo;
	}
	//
	return bRes;
}

void CWeaponMagazined::Load	(LPCSTR section)
{
	inherited::Load		(section);
		
	// Sounds
	HUD_SOUND::LoadSound(section,"snd_draw"		, sndShow		, m_eSoundShow		);
	HUD_SOUND::LoadSound(section,"snd_holster"	, sndHide		, m_eSoundHide		);
	HUD_SOUND::LoadSound(section,"snd_shoot"	, sndShot		, m_eSoundShot		);
	HUD_SOUND::LoadSound(section,"snd_empty"	, sndEmptyClick	, m_eSoundEmptyClick	);

	if (pSettings->line_exist(section, "snd_reload_empty"))
		HUD_SOUND::LoadSound(section, "snd_reload_empty", sndReload, m_eSoundReload);
	else
		HUD_SOUND::LoadSound(section, "snd_reload", sndReload, m_eSoundReload);

	if (pSettings->line_exist(section, "snd_reload_empty")) { //OpenXRay-style неполная перезарядка
		HUD_SOUND::LoadSound(section, "snd_reload", sndReloadPartly, m_eSoundReload);
		sndReloadPartlyExist = true;
	}
	else if (pSettings->line_exist(section, "snd_reload_partly")) { //OGSR-style неполная перезарядка
		HUD_SOUND::LoadSound(section, "snd_reload_partly", sndReloadPartly, m_eSoundReload);
		sndReloadPartlyExist = true;
	}
	
	if ( pSettings->line_exist( section, "snd_fire_modes" ) )
		HUD_SOUND::LoadSound( section, "snd_fire_modes", sndFireModes, m_eSoundEmptyClick );
	if ( pSettings->line_exist( section, "snd_zoom_change" ) )
		HUD_SOUND::LoadSound( section, "snd_zoom_change", sndZoomChange, m_eSoundEmptyClick );

	//
	HUD_SOUND::LoadSound(section, !!pSettings->line_exist(section, "snd_shutter") ? "snd_shutter" : "snd_draw", sndShutter, m_eSoundShutter);
	
	m_pSndShotCurrent = &sndShot;
		
	//звуки и партиклы глушителя, еслит такой есть
	if(m_eSilencerStatus == ALife::eAddonAttachable)
	{
		if(pSettings->line_exist(section, "silencer_flame_particles"))
			m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
		if(pSettings->line_exist(section, "silencer_smoke_particles"))
			m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");
		HUD_SOUND::LoadSound(section,"snd_silncer_shot", sndSilencerShot, m_eSoundShot);
	}
	//  [7/20/2005]
	if (pSettings->line_exist(section, "dispersion_start"))
		m_iShootEffectorStart = pSettings->r_u8(section, "dispersion_start");
	else
		m_iShootEffectorStart = 0;
	//  [7/20/2005]
	//  [7/21/2005]
	if (pSettings->line_exist(section, "fire_modes"))
	{
		m_bHasDifferentFireModes = true;
		shared_str FireModesList = pSettings->r_string(section, "fire_modes");
		int ModesCount = _GetItemCount(FireModesList.c_str());
		m_aFireModes.clear();
		for (int i=0; i<ModesCount; i++)
		{
			string16 sItem;
			_GetItem(FireModesList.c_str(), i, sItem);
			int FireMode = atoi(sItem);
			m_aFireModes.push_back(FireMode);			
		}
		m_iCurFireMode = ModesCount - 1;
		m_iPrefferedFireMode = READ_IF_EXISTS(pSettings, r_s16,section,"preffered_fire_mode",-1);
		//
		if (pSettings->line_exist(section, "preffered_fire_mode"))
		{
			fTimeToFirePreffered = READ_IF_EXISTS(pSettings, r_float, section, "preffered_fire_mode_rpm", fTimeToFire); //скорострельность привилегированного режима стрельбы
			VERIFY(fTimeToFirePreffered > 0.f);
			fTimeToFirePreffered = 60.f / fTimeToFirePreffered;
		}
	}
	else
		m_bHasDifferentFireModes = false;

//	m_bVision = !!READ_IF_EXISTS(pSettings, r_bool, section, "vision_present", false);
	m_fire_zoomout_time = READ_IF_EXISTS( pSettings, r_u32, section, "fire_zoomout_time", u32(-1) );

	m_str_count_tmpl = READ_IF_EXISTS( pSettings, r_string, "features", "wpn_magazined_str_count_tmpl", "{AE}/{AC}" );

	if (pSettings->line_exist(section, "has_chamber"))
		m_bHasChamber = !!pSettings->r_bool(section, "has_chamber");
}

void CWeaponMagazined::FireStart		()
{
	if(IsValid() && !IsMisfire()) 
	{
		if(!IsWorking() || AllowFireWhileWorking())
		{
			if(GetState()==eReload) return;
			if(GetState()==eShowing) return;
			if(GetState()==eHiding) return;
			if(GetState()==eMisfire) return;

			inherited::FireStart();
			
			if (iAmmoElapsed == 0)
				OnMagazineEmpty();
			else
				SwitchState(eFire);
		}
	}
	else if ( IsMisfire() ) 
	{
		if (smart_cast<CActor*>(H_Parent()) && Level().CurrentViewEntity() == H_Parent())
		{
			HUD().GetUI()->AddInfoMessage("item_state", "gun_jammed");
		}
	  OnEmptyClick();
	  // Callbacks added by Cribbledirge.
	  StateSwitchCallback( GameObject::eOnActorWeaponJammed, GameObject::eOnNPCWeaponJammed );
	}
	else 
		if(eReload!=GetState() && eMisfire!=GetState()) 
            OnMagazineEmpty();
}

void CWeaponMagazined::FireEnd() 
{
	inherited::FireEnd();

	if (!psActorFlags.is(AF_NO_AUTO_RELOAD)) {
		auto actor = smart_cast<CActor*>(H_Parent());
		if (!iAmmoElapsed && actor && GetState() != eReload)
			Reload();
	}
}

void CWeaponMagazined::Reload() 
{
	inherited::Reload();

	TryReload();
}

// Real Wolf: Одна реализация на все участки кода.20.01.15
bool CWeaponMagazined::TryToGetAmmo(u32 id)
{
	m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(*m_ammoTypes[id], ParentIsActor(), true));

	return m_pAmmo && (!HasDetachableMagazine() || AmmoTypeIsMagazine(id) || !iAmmoElapsed);
}

bool CWeaponMagazined::TryReload() 
{
	if(m_pCurrentInventory) 
	{
		if (TryToGetAmmo(m_ammoType) || unlimited_ammo() || (IsMisfire() && iAmmoElapsed))
		{
			SetPending(TRUE);
			SwitchState(eReload); 
			return true;
		}
		
		for(u32 i = 0; i < m_ammoTypes.size(); ++i) 
		{
			if (TryToGetAmmo(i))
			{ 
//				m_set_next_ammoType_on_reload = i; // https://github.com/revolucas/CoC-Xray/pull/5/commits/3c45cad1edb388664efbe3bb20a29f92e2d827ca
				m_ammoType = i;
				SetPending(TRUE);
				SwitchState(eReload);
				return true; 
			}
		}
	}
	
	SwitchState(eIdle);

	return false;
}

void CWeaponMagazined::OnMagazineEmpty() 
{
	//попытка стрелять когда нет патронов
	if(GetState() == eIdle) 
	{
		OnEmptyClick			();
		return;
	}

	if( GetNextState() != eMagEmpty && GetNextState() != eReload)
	{
		SwitchState(eMagEmpty);
	}

	inherited::OnMagazineEmpty();
}

void CWeaponMagazined::UnloadMagazine(bool spawn_ammo)
{
	//if (unlimited_ammo()) return;

	int chamber_ammo = HasChamber() ? 1 : 0;	//учтём дополнительный патрон в патроннике
	UnloadAmmo(iAmmoElapsed - chamber_ammo, spawn_ammo, !!GetMagazineEmptySect());
}

void CWeaponMagazined::ReloadMagazine()
{
	m_dwAmmoCurrentCalcFrame = 0;

	//устранить осечку при полной перезарядке
	if (IsMisfire() && (!HasChamber() || m_magazine.empty()))
	{
		bMisfire = false;
		if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
		{
			HUD().GetUI()->AddInfoMessage("item_state", "gun_not_jammed");
		}
	}

	//переменная блокирует использование
	//только разных типов патронов
	//	static bool l_lockType = false;
	if (!m_bLockType) {
		m_pAmmo = NULL;
	}

	if (!m_pCurrentInventory) return;

	if (m_set_next_ammoType_on_reload != u32(-1)) {
		m_ammoType = m_set_next_ammoType_on_reload;
		m_set_next_ammoType_on_reload = u32(-1);
	}

	if (!unlimited_ammo())
	{
		//попытаться найти в инвентаре патроны текущего типа
		m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(*m_ammoTypes[m_ammoType], ParentIsActor(), HasDetachableMagazine() && !IsSingleReloading()));

		if (!m_pAmmo && !m_bLockType)
		{
			for (u32 i = 0; i < m_ammoTypes.size(); ++i)
			{
				//проверить патроны всех подходящих типов
				m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(*m_ammoTypes[m_ammoType], ParentIsActor(), HasDetachableMagazine() && !IsSingleReloading()));

				if (m_pAmmo)
				{
					m_ammoType = i;
					break;
				}
			}
		}
	}
	else
		m_ammoType = m_ammoType;

	//нет патронов для перезарядки
	if (!m_pAmmo && !unlimited_ammo()) return;

	//разрядить магазин, если загружаем патронами другого типа
	if (!m_bLockType && !m_magazine.empty() &&
		(!m_pAmmo || xr_strcmp(m_pAmmo->cNameSect(), *m_magazine.back().m_ammoSect)) ||
		m_magazine.empty() && HasDetachableMagazine() && !unlimited_ammo())	//разрядить пустой магазин
		UnloadMagazine();

	if (HasDetachableMagazine() && !unlimited_ammo())
	{
		bool b_attaching_magazine = AmmoTypeIsMagazine(m_ammoType);

		int chamber_ammo = HasChamber() ? 1 : 0;	//учтём дополнительный патрон в патроннике
		int mag_size = b_attaching_magazine ? m_pAmmo->m_boxSize : 0;

		iMagazineSize = mag_size + chamber_ammo;
		m_LastLoadedMagType = b_attaching_magazine ? m_ammoType : 0;
		m_bIsMagazineAttached = b_attaching_magazine;
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
		m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
	CCartridge l_cartridge = m_DefaultCartridge;
	while (iAmmoElapsed < iMagazineSize)
	{
		if (!unlimited_ammo())
		{
			if (!m_pAmmo->Get(l_cartridge)) break;
		}
		++iAmmoElapsed;
		l_cartridge.m_LocalAmmoType = u8(m_ammoType);
		m_magazine.push_back(l_cartridge);
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	//выкинуть коробку патронов, если она пустая
	if (m_pAmmo && !m_pAmmo->m_boxCurr && OnServer())
		m_pAmmo->SetDropManual(TRUE);

	if (iMagazineSize > iAmmoElapsed && !HasDetachableMagazine() && !unlimited_ammo()) //дозарядить оружие до полного магазина
	{
		m_bLockType = true;
		ReloadMagazine();
		m_bLockType = false;
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeaponMagazined::OnStateSwitch(u32 S, u32 oldState)
{
	inherited::OnStateSwitch(S, oldState);
	switch (S)
	{
	case eIdle:
		switch2_Idle	();
		break;
	case eFire:
		switch2_Fire	();
		break;
	case eFire2:
		switch2_Fire2	();
		break;
	case eMisfire:
		if (smart_cast<CActor*>(H_Parent()) && (Level().CurrentViewEntity() == H_Parent())) {
			HUD().GetUI()->AddInfoMessage("item_state", "gun_jammed");
		}
		// Callbacks added by Cribbledirge.
		StateSwitchCallback(GameObject::eOnActorWeaponJammed, GameObject::eOnNPCWeaponJammed);
		break;
	case eMagEmpty:
		switch2_Empty	();
		// Callbacks added by Cribbledirge.
		StateSwitchCallback(GameObject::eOnActorWeaponEmpty, GameObject::eOnNPCWeaponEmpty);
		if (GetNextState() != eReload)
		{
			SwitchState(eIdle);
		}
		break;
	case eReload:
		switch2_Reload	();
		// Callbacks added by Cribbledirge.
		StateSwitchCallback(GameObject::eOnActorWeaponReload, GameObject::eOnNPCWeaponReload);
		break;
	case eShowing:
		switch2_Showing	();
		break;
	case eHiding:
		switch2_Hiding	();
		break;
	case eHidden:
		switch2_Hidden	();
		break;
	case eShutter:
		switch2_Shutter();
		break;
	}
}

void CWeaponMagazined::UpdateCL			()
{
	inherited::UpdateCL	();
	float dt = Device.fTimeDelta;
	

	//когда происходит апдейт состояния оружия
	//ничего другого не делать
	if(GetNextState() == GetState())
	{
		switch (GetState())
		{
		case eShowing:
		case eHiding:
		case eReload:
		case eIdle:
			fTime			-=	dt;
			if (fTime<0)	
				fTime = 0;
			break;
		case eFire:			
			if(iAmmoElapsed>0)
				state_Fire		(dt);
			
			if(fTime<=0)
			{
				if(iAmmoElapsed == 0)
					OnMagazineEmpty();
				StopShooting();
			}
			else
			{
				fTime			-=	dt;
			}

			if ( m_fire_zoomout_time != u32(-1) && IsZoomed() && m_dwStateTime > m_fire_zoomout_time )
			  OnZoomOut();

			break;
		case eMisfire:		state_Misfire	(dt);	break;
		case eMagEmpty:		state_MagEmpty	(dt);	break;
		case eHidden:		break;
		}
	}

	if (H_Parent() && IsZoomed() && !IsRotatingToZoom())
	{
		if (m_binoc_vision)
			m_binoc_vision->Update();

		UpdateSwitchNightVision();
	}

	UpdateSounds		();
}

void CWeaponMagazined::UpdateSounds	()
{
	if (Device.dwFrame == dwUpdateSounds_Frame)  
		return;
	
	dwUpdateSounds_Frame = Device.dwFrame;

	// ref_sound positions
	if (sndShow.playing			())	sndShow.set_position		(get_LastFP());
	if (sndHide.playing			())	sndHide.set_position		(get_LastFP());
	if (sndShot.playing			()) sndShot.set_position		(get_LastFP());
	if (sndSilencerShot.playing ()) sndSilencerShot.set_position(get_LastFP());
	if (sndReload.playing		()) sndReload.set_position		(get_LastFP());
	if (sndReloadPartly.playing())  sndReloadPartly.set_position(get_LastFP());
	if (sndEmptyClick.playing	())	sndEmptyClick.set_position	(get_LastFP());
	if (sndFireModes.playing	())	sndFireModes.set_position	(get_LastFP());
	if (sndZoomChange.playing	())	sndZoomChange.set_position	(get_LastFP());
	//
	if (sndShutter.playing		()) sndShutter.set_position			(get_LastFP());
	//
	if (sndZoomIn.playing		()) sndZoomChange.set_position		(get_LastFP());
	if (sndZoomOut.playing		()) sndZoomChange.set_position		(get_LastFP());
}

void CWeaponMagazined::state_Fire	(float dt)
{
	VERIFY(fTimeToFire>0.f);

	Fvector					p1, d; 
	p1.set(get_LastFP());
	d.set(get_LastFD());

	if (!H_Parent()) return;

#ifdef DEBUG
	CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
	if (!io->inventory().ActiveItem())
	{
		Log("current_state", GetState());
		Log("next_state", GetNextState());
		Log("state_time", m_dwStateTime);
		Log("item_sect", cNameSect().c_str());
		Log("H_Parent", H_Parent()->cNameSect().c_str());
	}
#endif

	smart_cast<CEntity*>	(H_Parent())->g_fireParams	(this, p1,d);
	if (m_iShotNum == 0)
	{
		m_vStartPos = p1;
		m_vStartDir = d;
	};
		
	VERIFY(!m_magazine.empty());
//	Msg("%d && %d && (%d || %d) && (%d || %d)", !m_magazine.empty(), fTime<=0, IsWorking(), m_bFireSingleShot, m_iQueueSize < 0, m_iShotNum < m_iQueueSize);
	while (!m_magazine.empty() && fTime<=0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
	{
		if ( CheckForMisfire() ) 
		{
			OnEmptyClick();
			StopShooting();
			//чтобы npc мог выбросить хреновый патрон
			if (!ParentIsActor())
			{
				ShutterAction();
			}
			return;
		}

		m_bFireSingleShot = false;

		//если у оружия есть разные размеры очереди
		//привилегированный режим очереди не полный автомат
		//текущий режим очереди является привилегированным
		//или кол-во выстрелов попадает в предел привилегированного режима
		if (m_bHasDifferentFireModes && m_iPrefferedFireMode != -1 && (GetCurrentFireMode() == m_iPrefferedFireMode || m_iShotNum < m_iPrefferedFireMode))
		{
			VERIFY(fTimeToFirePreffered > 0.f);
			fTime += fTimeToFirePreffered; //установим скорострельность привилегированного режима
			//Msg("fTimeToFirePreffered = %.6f", fTimeToFirePreffered);
		}
		else
		{
			VERIFY(fTimeToFire > 0.f);
			fTime += fTimeToFire;
			//Msg("fTimeToFire = %.6f", fTimeToFire);
		}
		//
		//повысить изношенность глушителя
		DeteriorateSilencerAttachable(-GetSilencerDeterioration());
		//
		
		++m_iShotNum;
		
		OnShot			();
		// Do Weapon Callback.  (Cribbledirge)
		StateSwitchCallback(GameObject::eOnActorWeaponFire, GameObject::eOnNPCWeaponFire);
		static int i = 0;
		if (i||m_iShotNum>m_iShootEffectorStart)
			FireTrace		(p1,d);
		else
			FireTrace		(m_vStartPos, m_vStartDir);
	}
	
	if(m_iShotNum == m_iQueueSize)
		m_bStopedAfterQueueFired = true;


	UpdateSounds			();
}

void CWeaponMagazined::state_Misfire	(float /**dt/**/)
{
	OnEmptyClick			();
	SwitchState				(eIdle);
	
	bMisfire				= true;

	UpdateSounds			();
}

void CWeaponMagazined::state_MagEmpty	(float dt)
{
}

void CWeaponMagazined::SetDefaults	()
{
	CWeapon::SetDefaults		();
}


void CWeaponMagazined::OnShot		()
{
	// Если актор бежит - останавливаем его
	if (ParentIsActor())
		Actor()->set_state_wishful(Actor()->get_state_wishful() & (~mcSprint));

	// Sound
	PlaySound( *m_pSndShotCurrent, get_LastFP(), true );

	// Camera	
	AddShotEffector		();

	// Animation
	PlayAnimShoot		();
	
	// Shell Drop
	Fvector vel; 
	PHGetLinearVell(vel);
	OnShellDrop					(get_LastSP(), vel);
	
	// Огонь из ствола
	StartFlameParticles	();

	//дым из ствола
	ForceUpdateFireParticles	();
	StartSmokeParticles			(get_LastFP(), vel);
}


void CWeaponMagazined::OnEmptyClick	()
{
	PlaySound	(sndEmptyClick,get_LastFP());
}

void CWeaponMagazined::OnAnimationEnd(u32 state) 
{
	switch(state) 
	{
		case eReload:
		  ReloadMagazine();
		  HandleCartridgeInChamber();
		  HUD_SOUND::StopSound( sndReload );
		  HUD_SOUND::StopSound(sndReloadPartly);
		  SwitchState( eIdle );
		  break;	// End of reload animation
		case eHiding:	SwitchState(eHidden);   break;	// End of Hide
		case eShowing:	SwitchState(eIdle);		break;	// End of Show
		case eIdle:		switch2_Idle();			break;  // Keep showing idle
		//
		case eShutter:	ShutterAction();	SwitchState(eIdle);	break;	// End of Shutter animation
	}
}

void CWeaponMagazined::switch2_Idle()
{
	SetPending(FALSE);
	PlayAnimIdle();
}

#ifdef DEBUG
#include "ai\stalker\ai_stalker.h"
#include "object_handler_planner.h"
#endif
void CWeaponMagazined::switch2_Fire	()
{
	CInventoryOwner* io		= smart_cast<CInventoryOwner*>(H_Parent());
#ifdef DEBUG
	CInventoryItem* ii		= smart_cast<CInventoryItem*>(this);
	VERIFY2					(io,make_string("no inventory owner, item %s",*cName()));

	if (ii != io->inventory().ActiveItem())
		Msg					("! not an active item, item %s, owner %s, active item %s",*cName(),*H_Parent()->cName(),io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().cName() : "no_active_item");

	if ( !(io && (ii == io->inventory().ActiveItem())) ) 
	{
		CAI_Stalker			*stalker = smart_cast<CAI_Stalker*>(H_Parent());
		if (stalker) {
			stalker->planner().show						();
			stalker->planner().show_current_world_state	();
			stalker->planner().show_target_world_state	();
		}
	}
#else
	if (!io)
		return;
#endif // DEBUG

//
//	VERIFY2(
//		io && (ii == io->inventory().ActiveItem()),
//		make_string(
//			"item[%s], parent[%s]",
//			*cName(),
//			H_Parent() ? *H_Parent()->cName() : "no_parent"
//		)
//	);

	m_bStopedAfterQueueFired = false;
	m_bFireSingleShot = true;
	m_iShotNum = 0;

	StateSwitchCallback(GameObject::eOnActorWeaponStartFiring, GameObject::eOnNPCWeaponStartFiring);

    if((OnClient() || Level().IsDemoPlay())&& !IsWorking())
		FireStart();

/*	if(SingleShotMode())
	{
		m_bFireSingleShot = true;
		bWorking = false;
	}*/
}
void CWeaponMagazined::switch2_Empty()
{
  if (psActorFlags.is(AF_NO_AUTO_RELOAD) && smart_cast<CActor*>( H_Parent() ) ) {
    OnEmptyClick();
    return;
  }

	OnZoomOut();
	
	if(!TryReload())
	{
		OnEmptyClick();
	}
	else
	{
		inherited::FireEnd();
	}
}
void CWeaponMagazined::PlayReloadSound()
{
	if (IsPartlyReloading() && sndReloadPartlyExist)
		PlaySound(sndReloadPartly, get_LastFP());
	else
		PlaySound(sndReload, get_LastFP());
}

void CWeaponMagazined::switch2_Reload()
{
	CWeapon::FireEnd();

	PlayReloadSound	();
	PlayAnimReload	();
	SetPending(TRUE);
}

void CWeaponMagazined::switch2_Hiding()
{
	CWeapon::FireEnd();

	StopHUDSounds();
	PlaySound	(sndHide,get_LastFP());

	PlayAnimHide();
	SetPending(TRUE);
}

void CWeaponMagazined::switch2_Hidden()
{
	CWeapon::FireEnd();

	HUD_SOUND::StopSound( sndReload );
	HUD_SOUND::StopSound(sndReloadPartly);
	StopCurrentAnimWithoutCallback();

	signal_HideComplete		();
	RemoveShotEffector		();
}
void CWeaponMagazined::switch2_Showing()
{
	PlaySound	(sndShow,get_LastFP());

	SetPending(TRUE);
	PlayAnimShow();
}

bool CWeaponMagazined::Action(s32 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;
	
	//если оружие чем-то занято, то ничего не делать
	if ( IsPending() && cmd != kWPN_FIREMODE_PREV && cmd != kWPN_FIREMODE_NEXT ) return false;
	
	switch(cmd) 
	{
	case kWPN_RELOAD:
	{
		if (flags & CMD_START)
		{
			if (Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)))
			{
				OnShutter();
				return true;
			}
			else if (iAmmoElapsed < iMagazineSize || IsMisfire() || HasDetachableMagazine())
			{
				Reload();
				return true;
			}
		}
	}break;
	case kWPN_FIREMODE_PREV:
		{
			if(flags&CMD_START) 
			{
				OnPrevFireMode();
				return true;
			};
		}break;
	case kWPN_FIREMODE_NEXT:
		{
			if(flags&CMD_START) 
			{
				OnNextFireMode();
				return true;
			};
		}break;
	}
	return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
	auto pScope				= smart_cast<CScope*>			(pIItem);
	auto pSilencer			= smart_cast<CSilencer*>		(pIItem);
	auto pGrenadeLauncher	= smart_cast<CGrenadeLauncher*>	(pIItem);
	auto pLaser				= smart_cast<CLaser*>			(pIItem);
	auto pFlashlight		= smart_cast<CFlashlight*>		(pIItem);

	if (pScope &&
		m_eScopeStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 && 
		std::find(m_scopes.begin(), m_scopes.end(), pIItem->object().cNameSect()) != m_scopes.end())
		return true;
	else if (pSilencer &&
		m_eSilencerStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
		std::find(m_silencers.begin(), m_silencers.end(), pIItem->object().cNameSect()) != m_silencers.end())
		return true;
	else if (pGrenadeLauncher &&
		m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
		std::find(m_glaunchers.begin(), m_glaunchers.end(), pIItem->object().cNameSect()) != m_glaunchers.end())
		return true;
	else if (pLaser &&
		m_eLaserStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0 &&
		std::find(m_lasers.begin(), m_lasers.end(), pIItem->object().cNameSect()) != m_lasers.end())
		return true;
	else if (pFlashlight &&
		m_eFlashlightStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) == 0 &&
		std::find(m_flashlights.begin(), m_flashlights.end(), pIItem->object().cNameSect()) != m_flashlights.end())
		return true;
	else
		return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
	if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) &&
		std::find(m_scopes.begin(), m_scopes.end(), item_section_name) != m_scopes.end())
		return true;
	else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
		std::find(m_silencers.begin(), m_silencers.end(), item_section_name) != m_silencers.end())
		return true;
	else if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		std::find(m_glaunchers.begin(), m_glaunchers.end(), item_section_name) != m_glaunchers.end())
		return true;
	else if (m_eLaserStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) &&
		std::find(m_lasers.begin(), m_lasers.end(), item_section_name) != m_lasers.end())
		return true;
	else if (m_eFlashlightStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) &&
		std::find(m_flashlights.begin(), m_flashlights.end(), item_section_name) != m_flashlights.end())
		return true;
	else
		return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem, bool b_send_event)
{
	bool result = false;

	auto pScope				= smart_cast<CScope*>			(pIItem);
	auto pSilencer			= smart_cast<CSilencer*>		(pIItem);
	auto pGrenadeLauncher	= smart_cast<CGrenadeLauncher*>	(pIItem);
	auto pLaser				= smart_cast<CLaser*>			(pIItem);
	auto pFlashlight		= smart_cast<CFlashlight*>		(pIItem);

	if (pScope &&
		m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
	{
		auto it = std::find(m_scopes.begin(), m_scopes.end(), pIItem->object().cNameSect());
		m_cur_scope = (u8)std::distance(m_scopes.begin(), it);
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
		result = true;
		m_fAttachedScopeCondition = pIItem->GetCondition();
	}
	else if (pSilencer &&
		m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
	{
		auto it = std::find(m_silencers.begin(), m_silencers.end(), pIItem->object().cNameSect());
		m_cur_silencer = (u8)std::distance(m_silencers.begin(), it);
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
		result = true;
		m_fAttachedSilencerCondition = pIItem->GetCondition();
	}
	else if (pGrenadeLauncher &&
		m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
	{
		auto it = std::find(m_glaunchers.begin(), m_glaunchers.end(), pIItem->object().cNameSect());
		m_cur_glauncher = (u8)std::distance(m_glaunchers.begin(), it);
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
		result = true;
		m_fAttachedGrenadeLauncherCondition = pIItem->GetCondition();
	}
	else if (pLaser &&
		m_eLaserStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) == 0)
	{
		auto it = std::find(m_lasers.begin(), m_lasers.end(), pIItem->object().cNameSect());
		m_cur_laser = (u8)std::distance(m_lasers.begin(), it);
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonLaser;
		result = true;
//		m_fAttachedGrenadeLauncherCondition = pIItem->GetCondition();
	}
	else if (pFlashlight &&
		m_eFlashlightStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) == 0)
	{
		auto it = std::find(m_flashlights.begin(), m_flashlights.end(), pIItem->object().cNameSect());
		m_cur_flashlight = (u8)std::distance(m_flashlights.begin(), it);
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonFlashlight;
		result = true;
//		m_fAttachedGrenadeLauncherCondition = pIItem->GetCondition();
	}

	if(result)
	{
		if (b_send_event && OnServer())
		{
			//уничтожить подсоединенную вещь из инвентаря
//.			pIItem->Drop					();
			pIItem->object().DestroyObject	();
		};

//		if ( !ScopeRespawn( pIItem ) ) {
			UpdateAddonsVisibility();
			InitAddons();
		//}

		return true;
	}
	else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazined::Detach(const char* item_section_name, bool b_spawn_item, float item_condition)
{
	if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) &&
		std::find(m_scopes.begin(), m_scopes.end(), item_section_name) != m_scopes.end())
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;
		//
		m_cur_scope = 0;
		if (b_spawn_item) item_condition = m_fAttachedScopeCondition;
		m_fAttachedScopeCondition = 1.f;
		//
		UpdateAddonsVisibility();
		InitAddons();

		return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
	}
	else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
		std::find(m_silencers.begin(), m_silencers.end(), item_section_name) != m_silencers.end())
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;
		//
		m_cur_silencer = 0;
		if (b_spawn_item) item_condition = m_fAttachedSilencerCondition;
		m_fAttachedSilencerCondition = 1.f;
		//
		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
	}
	else if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		std::find(m_glaunchers.begin(), m_glaunchers.end(), item_section_name) != m_glaunchers.end())
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
		//
		m_cur_glauncher = 0;
		if (b_spawn_item) item_condition = m_fAttachedGrenadeLauncherCondition;
		m_fAttachedGrenadeLauncherCondition = 1.f;
		//
		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
	}
	else if (m_eLaserStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser) &&
		std::find(m_lasers.begin(), m_lasers.end(), item_section_name) != m_lasers.end())
	{
		SwitchLaser(false);
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonLaser;
		//
		m_cur_laser = 0;
		//
		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
	}
	else if (m_eFlashlightStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight) &&
		std::find(m_flashlights.begin(), m_flashlights.end(), item_section_name) != m_flashlights.end())
	{
		SwitchFlashlight(false);
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonFlashlight;
		//
		m_cur_flashlight = 0;
		//
		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item, item_condition);
	}
	else
		return inherited::Detach(item_section_name, b_spawn_item, item_condition);
}

//void CWeaponMagazined::InitZoomParams(LPCSTR section, bool useTexture)
//{
//	m_fMinZoomK = def_min_zoom_k;
//	m_fZoomStepCount = def_zoom_step_count;
//
//	LPCSTR dynamicZoomParams = READ_IF_EXISTS(pSettings, r_string, section, "scope_dynamic_zoom", NULL);
//	if (dynamicZoomParams)
//	{
//		int num_zoom_param = _GetItemCount(dynamicZoomParams);
//
//		ASSERT_FMT(num_zoom_param >= 1, "!![%s] : Invalid scope_dynamic_zoom parameter in section [%s]", __FUNCTION__, section);
//
//		string128 tmp;
//		m_bScopeDynamicZoom = CInifile::IsBOOL(_GetItem(dynamicZoomParams, 0, tmp));
//
//		if (num_zoom_param > 1)
//			m_fZoomStepCount = atof(_GetItem(dynamicZoomParams, 1, tmp));
//
//		if (num_zoom_param > 2)
//			m_fMinZoomK = atof(_GetItem(dynamicZoomParams, 2, tmp));
//	}
//	else
//		m_bScopeDynamicZoom = false;
//
//	m_fScopeInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "scope_inertion_factor", m_fControlInertionFactor);
//	clamp(m_fScopeInertionFactor, m_fControlInertionFactor, m_fScopeInertionFactor);
//
//	m_fScopeZoomFactor = pSettings->r_float(section, "scope_zoom_factor");
//	m_fSecondVPZoomFactor = READ_IF_EXISTS(pSettings, r_float, section, "scope_lense_fov_factor", 0.0f);
//
//	m_fZoomHudFov = READ_IF_EXISTS(pSettings, r_float, section, "scope_zoom_hud_fov", 0.0f);
//	m_fSecondVPHudFov = READ_IF_EXISTS(pSettings, r_float, section, "scope_lense_hud_fov", 0.0f);
//
//	if (m_UIScope)
//		xr_delete(m_UIScope);
//
//	if (useTexture)
//	{
//		shared_str scope_tex_name = READ_IF_EXISTS(pSettings, r_string, section, "scope_texture", "");
//
//		if (scope_tex_name.size() > 0)
//		{
//			m_UIScope = xr_new<CUIStaticItem>();
//			m_UIScope->Init(scope_tex_name.c_str(), Core.Features.test(xrCore::Feature::scope_textures_autoresize) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
//		}
//	}
//}
//
//void CWeaponMagazined::InitAddons()
//{
//	//////////////////////////////////////////////////////////////////////////
//	// Прицел
//	m_fIronSightZoomFactor = READ_IF_EXISTS(pSettings, r_float, cNameSect(), "ironsight_zoom_factor", 50.0f);
//
//	if(IsScopeAttached())
//	{
//		if(m_eScopeStatus == ALife::eAddonAttachable)
//		{
//			m_sScopeName = pSettings->r_string(cNameSect(), "scope_name");
//			m_iScopeX = pSettings->r_s32(cNameSect(), "scope_x");
//			m_iScopeY = pSettings->r_s32(cNameSect(), "scope_y");
//
//			InitZoomParams(*m_sScopeName, !m_bIgnoreScopeTexture);
//
//			m_fZoomHudFov = READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "scope_zoom_hud_fov", m_fZoomHudFov);
//			m_fSecondVPHudFov = READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "scope_lense_hud_fov", m_fSecondVPHudFov);
//		}
//		else if(m_eScopeStatus == ALife::eAddonPermanent)
//		{
//			InitZoomParams(cNameSect().c_str(), !m_bIgnoreScopeTexture);
//
//			// CWeaponBinoculars always use dynamic zoom
//			m_bScopeDynamicZoom = m_bScopeDynamicZoom || !!smart_cast<CWeaponBinoculars*>(this);
//		}
//	}
//	else
//	{
//		m_bScopeDynamicZoom = false;
//
//		if (IsZoomEnabled())
//		{
//			InitZoomParams(cNameSect().c_str(), !!READ_IF_EXISTS(pSettings, r_bool, cNameSect(), "force_scope_texture", false));
//
//			// for weapon without any scope - scope_zoom_factor will overrider ironsight_zoom_factor
//			m_fIronSightZoomFactor = m_fScopeZoomFactor;
//		}
//		else 
//		{
//			m_fSecondVPZoomFactor = 0.0f;
//			m_fZoomHudFov = 0.0f;
//			m_fSecondVPHudFov = 0.0f;
//			m_fScopeInertionFactor = m_fControlInertionFactor;
//		}
//	}
//
//	if (m_bScopeDynamicZoom)
//	{
//		if (SecondVPEnabled())
//		{
//			float delta, min_zoom_factor;
//			GetZoomData(m_fSecondVPZoomFactor, delta, min_zoom_factor);
//
//			m_fRTZoomFactor = min_zoom_factor;
//		}
//		else
//		{
//			if (Core.Features.test(xrCore::Feature::ogse_wpn_zoom_system))
//			{
//				float delta, min_zoom_factor;
//				GetZoomData(m_fScopeZoomFactor, delta, min_zoom_factor);
//
//				m_fRTZoomFactor = min_zoom_factor; // set minimal zoom by default for ogse mode
//			}
//			else
//			{
//				m_fRTZoomFactor = m_fScopeZoomFactor;
//			}
//		}
//	}
//
//	if(IsSilencerAttached() && SilencerAttachable())
//	{		
//		m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
//		m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
//		m_pSndShotCurrent = &sndSilencerShot;
//
//		//сила выстрела
//		LoadFireParams	(*cNameSect(), "");
//
//		//подсветка от выстрела
//		LoadLights		(*cNameSect(), "silencer_");
//
//		ApplySilencerKoeffs();
//	}
//	else
//	{
//		m_sFlameParticlesCurrent = m_sFlameParticles;
//		m_sSmokeParticlesCurrent = m_sSmokeParticles;
//		m_pSndShotCurrent = &sndShot;
//
//		//сила выстрела
//		LoadFireParams	(*cNameSect(), "");
//
//		//подсветка от выстрела
//		LoadLights		(*cNameSect(), "");
//	}
//
//	inherited::InitAddons();
//	callback(GameObject::eOnAddonInit)(1);
//
//	m_fZoomFactor = CurrentZoomFactor();
//}

void CWeaponMagazined::InitAddons()
{
	//////////////////////////////////////////////////////////////////////////
	// Прицел
	LPCSTR sect = IsScopeAttached() && ScopeAttachable() ? GetScopeName().c_str() : cNameSect().c_str();
	LoadZoomParams(sect);
	//ЛЦУ
	if (IsLaserAttached()) {
		sect = LaserAttachable() ? GetLaserName().c_str() : cNameSect().c_str();
		LoadLaserParams(sect);
	}
	//ліхтарик
	if (IsFlashlightAttached()) {
		sect = FlashlightAttachable() ? GetFlashlightName().c_str() : cNameSect().c_str();
		LoadFlashlightParams(sect);
	}

	if (IsSilencerAttached() && SilencerAttachable())
	{
		conditionDecreasePerShotSilencer = READ_IF_EXISTS(pSettings, r_float, GetSilencerName(), "condition_shot_dec_silencer", 1.f);
		conditionDecreasePerShotSilencerSelf = READ_IF_EXISTS(pSettings, r_float, GetSilencerName(), "condition_shot_dec", .0f);
	}

	if (IsSilencerAttached() && !IsSilencerBroken())
	{
		//flame
		if (SilencerAttachable() && pSettings->line_exist(GetSilencerName(), "silencer_flame_particles"))
			m_sSilencerFlameParticles = pSettings->r_string(GetSilencerName(), "silencer_flame_particles");
		else if (pSettings->line_exist(cNameSect(), "silencer_flame_particles"))
			m_sSilencerFlameParticles = pSettings->r_string(cNameSect(), "silencer_flame_particles");
		else
			m_sSilencerFlameParticles = m_sFlameParticles.c_str();
		//smoke
		if (SilencerAttachable() && pSettings->line_exist(GetSilencerName(), "silencer_smoke_particles"))
			m_sSilencerSmokeParticles = pSettings->r_string(GetSilencerName(), "silencer_smoke_particles");
		else if (pSettings->line_exist(cNameSect(), "silencer_smoke_particles"))
			m_sSilencerSmokeParticles = pSettings->r_string(cNameSect(), "silencer_smoke_particles");
		else
			m_sSilencerSmokeParticles = m_sSmokeParticles.c_str();

		HUD_SOUND::StopSound(sndSilencerShot);

		if (SilencerAttachable() && pSettings->line_exist(GetSilencerName(), "snd_silncer_shot"))
			HUD_SOUND::LoadSound(GetSilencerName().c_str(), "snd_silncer_shot", sndSilencerShot, m_eSoundShot);
		else if (pSettings->line_exist(cNameSect(), "snd_silncer_shot"))
			HUD_SOUND::LoadSound(cNameSect().c_str(), "snd_silncer_shot", sndSilencerShot, m_eSoundShot);
		else
			sndSilencerShot = sndShot;

		m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
		m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
		m_pSndShotCurrent = &sndSilencerShot;

		//сила выстрела
		LoadFireParams(*cNameSect(), "");

		//подсветка от выстрела
		LoadLights(*cNameSect(), "silencer_");
		if (SilencerAttachable())
			ApplySilencerKoeffs();
	}
	else
	{
		m_sFlameParticlesCurrent = m_sFlameParticles;
		m_sSmokeParticlesCurrent = m_sSmokeParticles;
		m_pSndShotCurrent = &sndShot;

		//сила выстрела
		LoadFireParams(*cNameSect(), "");
		//подсветка от выстрела
		LoadLights(*cNameSect(), "");
	}

	inherited::InitAddons();
}

void CWeaponMagazined::LoadZoomParams(LPCSTR section)
{
	if (IsZoomed()) OnZoomOut();

	m_fIronSightZoomFactor = READ_IF_EXISTS(pSettings, r_float, section, "ironsight_zoom_factor", 1.0f);

	if (m_UIScope)
		xr_delete(m_UIScope);

	if (!IsScopeAttached() || IsScopeBroken())
	{
		m_bScopeDynamicZoom		= false;
		m_bVision				= false;
		m_bNightVisionEnabled	= false;

		if (IsScopeBroken())
		{
			m_fScopeZoomFactor = m_fIronSightZoomFactor;

			LPCSTR scope_tex_name_broken = READ_IF_EXISTS(pSettings, r_string, section, "scope_texture_broken", nullptr);

			if (scope_tex_name_broken)
			{
				m_UIScope = xr_new<CUIStaticItem>();
				m_UIScope->Init(scope_tex_name_broken, psHUD_Flags.test(HUD_TEXTURES_AUTORESIZE) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
			}
		}

		return;
	}

	HUD_SOUND::StopSound(sndZoomIn);
	HUD_SOUND::StopSound(sndZoomOut);

	if (pSettings->line_exist(section, "snd_zoomin"))
		HUD_SOUND::LoadSound(section, "snd_zoomin", sndZoomIn, SOUND_TYPE_ITEM_USING);
	if (pSettings->line_exist(section, "snd_zoomout"))
		HUD_SOUND::LoadSound(section, "snd_zoomout", sndZoomOut, SOUND_TYPE_ITEM_USING);

	m_bVision = !!READ_IF_EXISTS(pSettings, r_bool, section, "vision_present", false);
	if (m_bVision) binoc_vision_sect = section;

	m_bNightVisionEnabled = !!READ_IF_EXISTS(pSettings, r_bool, section, "night_vision", false);
	if (m_bNightVisionEnabled)
	{
		HUD_SOUND::StopSound(SndNightVisionOn);
		HUD_SOUND::StopSound(SndNightVisionOff);
		HUD_SOUND::StopSound(SndNightVisionIdle);
		HUD_SOUND::StopSound(SndNightVisionBroken);

		if (pSettings->line_exist(section, "snd_night_vision_on"))
			HUD_SOUND::LoadSound(section, "snd_night_vision_on", SndNightVisionOn, SOUND_TYPE_ITEM_USING);
		if (pSettings->line_exist(section, "snd_night_vision_off"))
			HUD_SOUND::LoadSound(section, "snd_night_vision_off", SndNightVisionOff, SOUND_TYPE_ITEM_USING);
		if (pSettings->line_exist(section, "snd_night_vision_idle"))
			HUD_SOUND::LoadSound(section, "snd_night_vision_idle", SndNightVisionIdle, SOUND_TYPE_ITEM_USING);
		if (pSettings->line_exist(section, "snd_night_vision_broken"))
			HUD_SOUND::LoadSound(section, "snd_night_vision_broken", SndNightVisionBroken, SOUND_TYPE_ITEM_USING);

		m_NightVisionSect = READ_IF_EXISTS(pSettings, r_string, section, "night_vision_effector", nullptr);
	}

	m_fScopeZoomFactor = pSettings->r_float(section, "scope_zoom_factor");

	m_bScopeDynamicZoom = !!READ_IF_EXISTS(pSettings, r_bool, section, "scope_dynamic_zoom", false);
	if (m_bScopeDynamicZoom)
	{
		m_uZoomStepCount		= READ_IF_EXISTS(pSettings, r_u32, section, "zoom_step_count", 3);
		m_fMinScopeZoomFactor	= READ_IF_EXISTS(pSettings, r_float, section, "min_scope_zoom_factor", m_fScopeZoomFactor / m_uZoomStepCount);

		HUD_SOUND::StopSound(sndZoomChange);
		HUD_SOUND::DestroySound(sndZoomChange);

		if (pSettings->line_exist(section, "snd_zoom_change"))
			HUD_SOUND::LoadSound(section, "snd_zoom_change", sndZoomChange, SOUND_TYPE_ITEM_USING);
	}

	LPCSTR scope_tex_name = READ_IF_EXISTS(pSettings, r_string, section, "scope_texture", nullptr);
	if (!scope_tex_name) return;

	m_UIScope = xr_new<CUIStaticItem>();
	m_UIScope->Init(scope_tex_name, psHUD_Flags.test(HUD_TEXTURES_AUTORESIZE) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
}

void CWeaponMagazined::ApplySilencerKoeffs	()
{
	float BHPk = 1.0f, BSk = 1.0f;
	float FDB_k = 1.0f, CD_k = 1.0f;
	
	if (pSettings->line_exist(GetSilencerName(), "bullet_hit_power_k"))
	{
		BHPk = pSettings->r_float(GetSilencerName(), "bullet_hit_power_k");
		clamp(BHPk, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(GetSilencerName(), "bullet_speed_k"))
	{
		BSk = pSettings->r_float(GetSilencerName(), "bullet_speed_k");
		clamp(BSk, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(GetSilencerName(), "fire_dispersion_base_k"))
	{
		FDB_k = pSettings->r_float(GetSilencerName(), "fire_dispersion_base_k");
//		clamp(FDB_k, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(GetSilencerName(), "cam_dispersion_k"))
	{
		CD_k = pSettings->r_float(GetSilencerName(), "cam_dispersion_k");
		clamp(CD_k, 0.0f, 1.0f);
	};

	//fHitPower			= fHitPower*BHPk;
	fvHitPower			.mul(BHPk);
	fHitImpulse			*= BSk;
	m_fStartBulletSpeed *= BSk;
	fireDispersionBase	*= FDB_k;
	camDispersion		*= CD_k;
	camDispersionInc	*= CD_k;
}

void CWeaponMagazined::LoadLaserParams(LPCSTR section) {
	laserdot_attach_bone = READ_IF_EXISTS(pSettings, r_string, cNameSect().c_str(), "laserdot_attach_bone", "");
	laserdot_attach_offset = Fvector{ 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "laserdot_attach_offset_x", 0.0f), 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "laserdot_attach_offset_y", 0.0f), 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "laserdot_attach_offset_z", 0.0f) };
	laserdot_world_attach_offset = Fvector{ 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "laserdot_world_attach_offset_x", 0.0f), 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "laserdot_world_attach_offset_y", 0.0f), 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "laserdot_world_attach_offset_z", 0.0f) };

	const bool b_r2 = psDeviceFlags.test(rsR2) || psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4);

	const char* m_light_section = pSettings->r_string(section, "laser_light_section");

	laser_lanim = LALib.FindItem(READ_IF_EXISTS(pSettings, r_string, m_light_section, "color_animator", ""));

	laser_light_render = ::Render->light_create();
	laser_light_render->set_type(IRender_Light::SPOT);
	laser_light_render->set_shadow(true);

	const Fcolor clr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "color_r2" : "color", (Fcolor{ 1.0f, 0.0f, 0.0f, 1.0f }));
	laser_fBrightness = clr.intensity();
	laser_light_render->set_color(clr);
	const float range = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "range_r2" : "range", 100.f);
	laser_light_render->set_range(range);
	laser_light_render->set_cone(deg2rad(READ_IF_EXISTS(pSettings, r_float, m_light_section, "spot_angle", 1.f)));
	laser_light_render->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "spot_texture", nullptr));

	HUD_SOUND::StopSound(SndLaserSwitch);
	if (pSettings->line_exist(section, "snd_laser_switch"))
		HUD_SOUND::LoadSound(section, "snd_laser_switch", SndLaserSwitch, SOUND_TYPE_ITEM_USING);
}

void CWeaponMagazined::LoadFlashlightParams(LPCSTR section) {
	flashlight_attach_bone = READ_IF_EXISTS(pSettings, r_string, cNameSect().c_str(), "torch_light_bone", "");
	flashlight_attach_offset = Fvector{ 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_attach_offset_x", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_attach_offset_y", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_attach_offset_z", 0.0f) };
	flashlight_omni_attach_offset = Fvector{ 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_omni_attach_offset_x", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_omni_attach_offset_y", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_omni_attach_offset_z", 0.0f) };
	flashlight_world_attach_offset = Fvector{ 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_world_attach_offset_x", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_world_attach_offset_y", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_world_attach_offset_z", 0.0f) };
	flashlight_omni_world_attach_offset = Fvector{ 
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_omni_world_attach_offset_x", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_omni_world_attach_offset_y", 0.0f),
		READ_IF_EXISTS(pSettings, r_float, cNameSect().c_str(), "torch_omni_world_attach_offset_z", 0.0f) };

	const bool b_r2 = psDeviceFlags.test(rsR2) || psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4);

	const char* m_light_section = pSettings->r_string(section, "flashlight_section");

	flashlight_lanim = LALib.FindItem(READ_IF_EXISTS(pSettings, r_string, m_light_section, "color_animator", ""));

	flashlight_render = ::Render->light_create();
	flashlight_render->set_type(IRender_Light::SPOT);
	flashlight_render->set_shadow(true);

	const Fcolor clr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "color_r2" : "color", (Fcolor{ 0.6f, 0.55f, 0.55f, 1.0f }));
	flashlight_fBrightness = clr.intensity();
	flashlight_render->set_color(clr);
	const float range = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "range_r2" : "range", 50.f);
	flashlight_render->set_range(range);
	flashlight_render->set_cone(deg2rad(READ_IF_EXISTS(pSettings, r_float, m_light_section, "spot_angle", 60.f)));
	flashlight_render->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "spot_texture", nullptr));

	flashlight_omni = ::Render->light_create();
	flashlight_omni->set_type((IRender_Light::LT)(READ_IF_EXISTS(pSettings, r_u8, m_light_section, "omni_type", 2))); //KRodin: вообще omni это обычно поинт, но поинт светит во все стороны от себя, поэтому тут спот используется по умолчанию.
	flashlight_omni->set_shadow(false);

	const Fcolor oclr = READ_IF_EXISTS(pSettings, r_fcolor, m_light_section, b_r2 ? "omni_color_r2" : "omni_color", (Fcolor{ 1.0f , 1.0f , 1.0f , 0.0f }));
	flashlight_omni->set_color(oclr);
	const float orange = READ_IF_EXISTS(pSettings, r_float, m_light_section, b_r2 ? "omni_range_r2" : "omni_range", 0.25f);
	flashlight_omni->set_range(orange);

	flashlight_glow = ::Render->glow_create();
	flashlight_glow->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "glow_texture", "glow\\glow_torch_r2"));
	flashlight_glow->set_color(clr);
	flashlight_glow->set_radius(READ_IF_EXISTS(pSettings, r_float, m_light_section, "glow_radius", 0.3f));

	HUD_SOUND::StopSound(SndFlashlightSwitch);
	if (pSettings->line_exist(section, "snd_flashlight_switch"))
		HUD_SOUND::LoadSound(section, "snd_flashlight_switch", SndFlashlightSwitch, SOUND_TYPE_ITEM_USING);
}

//виртуальные функции для проигрывания анимации HUD
void CWeaponMagazined::PlayAnimShow()
{
	VERIFY(GetState()==eShowing);
	PlayHUDMotion({ "anim_draw", "anm_show" }, false, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
	VERIFY(GetState()==eHiding);
	PlayHUDMotion({ "anim_holster", "anm_hide" }, true, GetState());
}


void CWeaponMagazined::PlayAnimReload()
{
	VERIFY(GetState() == eReload);
	if (IsPartlyReloading())
		PlayHUDMotion({ "anim_reload_partly", "anm_reload_partly", "anim_reload", "anm_reload" }, true, GetState());
	else if (IsSingleReloading())
	{
		if(AnimationExist("anim_reload_single"))
			PlayHUDMotion("anim_reload_single", true, GetState());
		else
			PlayHUDMotion({ "anim_draw", "anm_show" }, false, GetState());
	}
	else
		PlayHUDMotion({ "anm_reload_empty", "anim_reload", "anm_reload" }, true, GetState());
}

const char* CWeaponMagazined::GetAnimAimName()
{
	if (auto pActor = smart_cast<const CActor*>(H_Parent())) {
		if (!HudBobbingAllowed()) {
			if (const u32 state = pActor->get_state(); state & mcAnyMove) {
				if (IsScopeAttached()) {
					strcpy_s(guns_aim_anm, "anm_idle_aim_scope_moving");
					return guns_aim_anm;
				}
				else
					return xr_strconcat(guns_aim_anm, "anm_idle_aim_moving", (state & mcFwd) ? "_forward" : ((state & mcBack) ? "_back" : ""), (state & mcLStrafe) ? "_left" : ((state & mcRStrafe) ? "_right" : ""));
			}
		}
	}
	return nullptr;
}

void CWeaponMagazined::PlayAnimAim()
{
	if (IsRotatingToZoom()) {
		if (AnimationExist("anm_idle_aim_start")) {
			PlayHUDMotion("anm_idle_aim_start", true, GetState());
			return;
		}
	}

	if (const char* guns_aim_anm = GetAnimAimName()) {
		if (AnimationExist(guns_aim_anm)) {
			PlayHUDMotion(guns_aim_anm, true, GetState());
			return;
		}
	}

	PlayHUDMotion({ "anim_idle_aim", "anm_idle_aim" }, true, GetState());
}

void CWeaponMagazined::PlayAnimIdle()
{
	if (GetState() != eIdle)
		return;

	if (IsZoomed())
		PlayAnimAim();
	else {
		if (IsRotatingFromZoom()) {
			if (AnimationExist("anm_idle_aim_end")) {
				PlayHUDMotion("anm_idle_aim_end", true, GetState());
				return;
			}
		}

		inherited::PlayAnimIdle();
	}
}

void CWeaponMagazined::PlayAnimShoot()
{
	VERIFY(GetState()==eFire || GetState()==eFire2);

	string_path guns_shoot_anm{};
	xr_strconcat(guns_shoot_anm, "anm_shoot", (IsZoomed() && !IsRotatingToZoom()) ? (IsScopeAttached() ? "_aim_scope" : "_aim") : "", IsSilencerAttached() ? "_sil" : "");

	PlayHUDMotion({ guns_shoot_anm, "anim_shoot", "anm_shots" }, false, GetState());
}

void CWeaponMagazined::OnZoomIn			()
{
	inherited::OnZoomIn();

	if(GetState() == eIdle)
		PlayAnimIdle();


	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if(pActor)
	{
		CEffectorCam* ec = pActor->Cameras().GetCamEffector( eCEActorMoving );
		if ( ec )
		  pActor->Cameras().RemoveCamEffector( eCEActorMoving );

		CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>	(pActor->Cameras().GetCamEffector(eCEZoom));
		if (!S)	
		{
			S = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(xr_new<CEffectorZoomInertion> ());
			S->Init(this);
		};
		S->SetRndSeed(pActor->GetZoomRndSeed());
		R_ASSERT				(S);

		//if (m_bVision && !m_binoc_vision)
		//	m_binoc_vision = xr_new<CBinocularsVision>(this);
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());							//
		if (IsScopeAttached() && !IsGrenadeMode())
		{
			HUD_SOUND::PlaySound(sndZoomIn, H_Parent()->Position(), H_Parent(), b_hud_mode);

			if (m_bVision && !m_binoc_vision)
				m_binoc_vision = xr_new<CBinocularsVision>(this);
			if (m_bNightVisionSwitchedOn)
				SwitchNightVision(true, false);
		}
	}
}
void CWeaponMagazined::OnZoomOut()
{
	if (!m_bZoomMode)
		return;

	inherited::OnZoomOut();

	if (GetState() == eIdle)
		PlayAnimIdle();

	bool b_hud_mode = (Level().CurrentEntity() == H_Parent());							//
	if (IsScopeAttached() && !IsGrenadeMode())
	{
		if (H_Parent())
			HUD_SOUND::PlaySound(sndZoomOut, H_Parent()->Position(), H_Parent(), b_hud_mode);
	}

	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (pActor)
	{
		pActor->SetHardHold(false);

		pActor->Cameras().RemoveCamEffector(eCEZoom);
		if (m_bVision)
		{
			VERIFY(m_binoc_vision);
			xr_delete(m_binoc_vision);
		}

		SwitchNightVision(false, false);
	}
}

void CWeaponMagazined::OnZoomChanged()
{
	PlaySound(sndZoomChange, get_LastFP());
	m_fRTZoomFactor = m_fZoomFactor;//store current
}

//переключение режимов стрельбы одиночными и очередями
bool CWeaponMagazined::SwitchMode()
{
	if (eIdle != GetState() || IsPending()) 
		return false;

	if(SingleShotMode())
		m_iQueueSize = WEAPON_ININITE_QUEUE;
	else
		m_iQueueSize = 1;
	
	PlaySound	(sndEmptyClick, get_LastFP());

	return true;
}

void	CWeaponMagazined::OnNextFireMode		()
{
	if (!m_bHasDifferentFireModes) return;
	m_iCurFireMode = (m_iCurFireMode+1+m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
	PlaySound( sndFireModes, get_LastFP() );
};

void	CWeaponMagazined::OnPrevFireMode		()
{
	if (!m_bHasDifferentFireModes) return;
	m_iCurFireMode = (m_iCurFireMode-1+m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());	
	PlaySound( sndFireModes, get_LastFP() );
};

void	CWeaponMagazined::OnH_A_Chield		()
{
	if (m_bHasDifferentFireModes)
	{
		CActor	*actor = smart_cast<CActor*>(H_Parent());
		if (!actor) SetQueueSize(-1);
		else SetQueueSize(GetCurrentFireMode());
	};	
	inherited::OnH_A_Chield();
};

void	CWeaponMagazined::SetQueueSize			(int size)  
{
	m_iQueueSize = size; 
	if (m_iQueueSize == -1)
		strcpy_s(m_sCurFireMode, " (A)");
	else
		sprintf_s(m_sCurFireMode, " (%d)", m_iQueueSize);
};

float CWeaponMagazined::GetWeaponDeterioration() {
	if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <= u32(m_iPrefferedFireMode))
		return inherited::GetWeaponDeterioration();
	//
	float silencer_dec_k = IsSilencerAttached() && SilencerAttachable() ? conditionDecreasePerShotSilencer : 1.0f;
	//
	return m_iShotNum * conditionDecreasePerShot * silencer_dec_k;
}

void CWeaponMagazined::save(NET_Packet &output_packet)
{
	inherited::save	(output_packet);
	save_data		(m_iQueueSize, output_packet);
	save_data		(m_iShotNum, output_packet);
}

void CWeaponMagazined::load(IReader &input_packet)
{
	inherited::load	(input_packet);
	load_data		(m_iQueueSize, input_packet);SetQueueSize(m_iQueueSize);
	load_data		(m_iShotNum, input_packet);
}

void CWeaponMagazined::net_Export( CSE_Abstract* E ) {
  inherited::net_Export( E );
  CSE_ALifeItemWeaponMagazined* wpn = smart_cast<CSE_ALifeItemWeaponMagazined*>( E );
  wpn->m_u8CurFireMode = u8( m_iCurFireMode&0x00ff );
  //
  wpn->m_AmmoIDs.clear();
  for (u8 i = 0; i < m_magazine.size(); i++)
  {
	  CCartridge& l_cartridge = *(m_magazine.begin() + i);
	  wpn->m_AmmoIDs.push_back(l_cartridge.m_LocalAmmoType);
  }
  //
  wpn->m_bIsMagazineAttached				= m_bIsMagazineAttached;
  //
  wpn->m_fAttachedSilencerCondition			= m_fAttachedSilencerCondition;
  wpn->m_fAttachedScopeCondition			= m_fAttachedScopeCondition;
  wpn->m_fAttachedGrenadeLauncherCondition	= m_fAttachedGrenadeLauncherCondition;
  //
  wpn->m_fRTZoomFactor						= m_fRTZoomFactor;
  //
  wpn->m_bNightVisionSwitchedOn				= m_bNightVisionSwitchedOn;
}

#include "ui/UIMainIngameWnd.h"
void CWeaponMagazined::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
	auto CurrentHUD		= HUD().GetUI()->UIMainIngameWnd;
	bool b_wpn_info		= CurrentHUD->IsHUDElementAllowed(eActiveItem);
	bool b_gear_info	= CurrentHUD->IsHUDElementAllowed(eGear);

	bool b_use_mags = HasDetachableMagazine();

	const int AE = GetAmmoElapsed(), AC = b_use_mags ? GetMagazineCount() : GetAmmoCurrent();
	
	if (AE == 0 || m_magazine.empty())
		icon_sect_name = "";//m_ammoTypes[m_ammoType].c_str();
	else
		icon_sect_name = m_magazine.back().m_ammoSect.c_str();//m_ammoTypes[m_magazine.back().m_LocalAmmoType].c_str();

	string256 sItemName;
	strcpy_s(sItemName, *CStringTable().translate(AE > 0 ? pSettings->r_string(icon_sect_name.c_str(), "inv_name_short") : "st_not_loaded"));

	if (HasFireModes() && b_wpn_info)
		strcat_s(sItemName, GetCurrentFireModeStr());

	str_name = sItemName;

	//static const std::regex ae_re{ R"(\{AE\})" }, ac_re{ R"(\{AC\})" };
	//str_count = m_str_count_tmpl;
	//str_count = std::regex_replace(str_count, ae_re, std::to_string(AE));
	//str_count = std::regex_replace(str_count, ac_re, unlimited_ammo() ? "--" : std::to_string(b_use_mags ? AC : AC - AE));

	if (b_wpn_info && b_gear_info)
		sprintf_s(sItemName, "%d|%d", AE, b_use_mags ? AC : AC - AE);
	else if (b_wpn_info)
		sprintf_s(sItemName, "[%d]", AE);
	else if (b_gear_info)
		sprintf_s(sItemName, "%d", b_use_mags ? AC : AC - AE);
	else if (unlimited_ammo())
		sprintf_s(sItemName, "%d|--", AE);

	str_count = sItemName;
}

void CWeaponMagazined::OnDrawUI()
{
	if (H_Parent() && IsZoomed() && !IsRotatingToZoom() && m_binoc_vision)
		m_binoc_vision->Draw();
	inherited::OnDrawUI();
}
void CWeaponMagazined::net_Relcase(CObject *object)
{
	if (!m_binoc_vision)
		return;

	m_binoc_vision->remove_links(object);
}

void CWeaponMagazined::UnloadAmmo(int unload_count, bool spawn_ammo, bool detach_magazine)
{
	if (detach_magazine && !unlimited_ammo())
	{
		int chamber_ammo = HasChamber() ? 1 : 0;	//учтём дополнительный патрон в патроннике

		if (iAmmoElapsed <= chamber_ammo && spawn_ammo)	//spawn mag empty
			SpawnAmmo(0, GetMagazineEmptySect());

		iMagazineSize = 1;
		m_bIsMagazineAttached = false;
	}

	xr_map<LPCSTR, u16> l_ammo;
	for (int i = 0; i < unload_count; ++i)
	{
		CCartridge& l_cartridge = m_magazine.back();
		LPCSTR ammo_sect = detach_magazine ? *m_ammoTypes[m_LastLoadedMagType] : *l_cartridge.m_ammoSect;

		if (!l_ammo[ammo_sect])
			l_ammo[ammo_sect] = 1;
		else
			l_ammo[ammo_sect]++;

		if (detach_magazine)
			m_magazine.erase(m_magazine.begin());
		else
			m_magazine.pop_back();

		--iAmmoElapsed;
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	if (!spawn_ammo)
		return;

	xr_map<LPCSTR, u16>::iterator l_it;
	for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
	{
		if (m_pCurrentInventory && !detach_magazine) //упаковать разряжаемые патроны в неполную пачку
		{
			CWeaponAmmo* l_pA = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(l_it->first, ParentIsActor(), true));

			if (l_pA)
			{
				u16 l_free = l_pA->m_boxSize - l_pA->m_boxCurr;
				l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < l_it->second ? l_free : l_it->second);
				l_it->second = l_it->second - (l_free < l_it->second ? l_free : l_it->second);
			}
		}
		if (l_it->second && !unlimited_ammo()) SpawnAmmo(l_it->second, l_it->first);
	}
}

bool CWeaponMagazined::HasDetachableMagazine() const
{
	for (u32 i = 0; i < m_ammoTypes.size(); ++i)
		if (AmmoTypeIsMagazine(i))
			return true;

	return false;
}

bool CWeaponMagazined::IsMagazineAttached() const
{
	return m_bIsMagazineAttached && HasDetachableMagazine();
}

void CWeaponMagazined::HandleCartridgeInChamber()
{
	if (!HasChamber() || !HasDetachableMagazine() || m_magazine.empty())
		return;
	//отстрел и заряжание нового патрона идёт от конца вектора m_magazine.back() - первым подаётся последний добавленный патрон
	if (*m_magazine.back().m_ammoSect != *m_magazine.front().m_ammoSect) //первый и последний патрон различны, значит зарядка смешанная
	{//конец заряжания магазина
		//перекладываем патрон отличного типа (первый заряженный, он же последний на отстрел) из начала вектора в конец
		//Msg("~~ weapon:[%s]|back:[%s]|front:[%s]|[1]:[%s] on reloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect, *m_magazine[1].m_ammoSect);
		rotate(m_magazine.begin(), m_magazine.begin() + 1, m_magazine.end());
		//Msg("~~ weapon:[%s]|back:[%s]|front:[%s]|[1]:[%s] after rotate on reloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect, *m_magazine[1].m_ammoSect);
	}
}

float	CWeaponMagazined::GetSilencerDeterioration()
{
	return conditionDecreasePerShotSilencerSelf;
};

//работа затвора
void CWeaponMagazined::OnShutter()
{
	SwitchState(eShutter);
}
//
void CWeaponMagazined::switch2_Shutter()
{
	PlaySound(sndShutter, get_LastFP());
	PlayAnimShutter();
	SetPending(TRUE);
}
//
void CWeaponMagazined::PlayAnimShutter()
{
	VERIFY(GetState() == eShutter);
	if(AnimationExist("anim_shutter"))
		PlayHUDMotion("anim_shutter", true, GetState());
	else
		PlayHUDMotion({ "anim_draw", "anm_show" }, true, GetState());
}
//
void CWeaponMagazined::ShutterAction() //передёргивание затвора
{
	if (IsMisfire()){
		bMisfire = false;
		if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent())){
			HUD().GetUI()->AddInfoMessage("item_state", "gun_not_jammed");
		}
	}
	
	if (HasChamber() && !m_magazine.empty()){
		UnloadAmmo(1);
		// Shell Drop
		Fvector vel;
		PHGetLinearVell(vel);
		OnShellDrop(get_LastSP(), vel);
	}
}

void CWeaponMagazined::SwitchNightVision(bool vision_on, bool switch_sound)
{
	if (!m_bNightVisionEnabled || IsGrenadeMode()) return;

	CActor* pA = smart_cast<CActor*>(H_Parent());
	if (!pA)					return;

	if (vision_on)
	{
		m_bNightVisionOn = true;
	}
	else
	{
		m_bNightVisionOn = false;
	}

	bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());

	if (m_bNightVisionOn) {
		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvisionScope);
		if (!pp) {
			if (!!m_NightVisionSect)
			{
				AddEffector(pA, effNightvisionScope, m_NightVisionSect);
				if (switch_sound)
					HUD_SOUND::PlaySound(SndNightVisionOn, pA->Position(), pA, bPlaySoundFirstPerson);
				HUD_SOUND::PlaySound(SndNightVisionIdle, pA->Position(), pA, bPlaySoundFirstPerson, true);
			}
		}
	}
	else {
		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvisionScope);
		if (pp) {
			pp->Stop(1.0f);
			if (switch_sound)
				HUD_SOUND::PlaySound(SndNightVisionOff, pA->Position(), pA, bPlaySoundFirstPerson);
			HUD_SOUND::StopSound(SndNightVisionIdle);
		}
	}
}

void CWeaponMagazined::UpdateSwitchNightVision()
{
	if (!m_bNightVisionEnabled || !m_bNightVisionSwitchedOn) return;
	if (OnClient()) return;

	auto* pA = smart_cast<CActor*>(H_Parent());
	if (pA && m_bNightVisionOn && !pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision))
		SwitchNightVision(true, false);
}

void CWeaponMagazined::SwitchNightVision()
{
	if (OnClient() || !m_bNightVisionEnabled || IsGrenadeMode()) return;
	m_bNightVisionSwitchedOn = !m_bNightVisionSwitchedOn;
	SwitchNightVision(!m_bNightVisionOn, true);
}

void CWeaponMagazined::DeteriorateSilencerAttachable(float fDeltaCondition)
{
	if (IsSilencerAttached() && SilencerAttachable() && !fis_zero(conditionDecreasePerShotSilencerSelf))
	{
		if (fis_zero(m_fAttachedSilencerCondition))
			Detach(GetSilencerName().c_str(), false);
		else
		{
			CCartridge& l_cartridge = m_magazine.back(); //с учетом влияния конкретного патрона
			m_fAttachedSilencerCondition += fDeltaCondition * l_cartridge.m_impair;
			clamp(m_fAttachedSilencerCondition, 0.f, 1.f);
		}
	}
};
//
float CWeaponMagazined::GetConditionMisfireProbability() const
{
	float mis = inherited::GetConditionMisfireProbability();
	//вероятность осечки от магазина
	if (GetMagazineEmptySect())
	{
		float mag_missfire_prob = READ_IF_EXISTS(pSettings, r_float, GetMagazineEmptySect(), "misfire_probability_box", 0.0f);
		mis += mag_missfire_prob;
	}
	clamp(mis, 0.0f, 0.99f);
	return mis;
}
//
int CWeaponMagazined::GetMagazineCount() const
{
	int iMagazinesCount = 0;

	for (int i = 0; i < (int)m_ammoTypes.size(); ++i)
	{
		bool b_search_ruck = !psActorFlags.test(AF_AMMO_FROM_BELT);

		iMagazinesCount += (int)Actor()->inventory().GetSameItemCount(m_ammoTypes[i].c_str(), b_search_ruck);
	}
	return iMagazinesCount;
}

bool CWeaponMagazined::IsSingleReloading()
{
	if (IsPartlyReloading() || !HasDetachableMagazine() || !HasChamber())
		return false;

	bool reload_by_single_ammo = m_set_next_ammoType_on_reload == u32(-1) &&
		TryToGetAmmo(m_ammoType) && !AmmoTypeIsMagazine(m_ammoType);

	bool next_load_by_single_ammo = m_set_next_ammoType_on_reload != u32(-1) &&
		TryToGetAmmo(m_set_next_ammoType_on_reload) && !AmmoTypeIsMagazine(m_set_next_ammoType_on_reload);

	return reload_by_single_ammo || next_load_by_single_ammo;
}

bool CWeaponMagazined::AmmoTypeIsMagazine(u32 type) const
{
	return pSettings->line_exist(m_ammoTypes[type], "ammo_in_box") &&
		pSettings->line_exist(m_ammoTypes[type], "empty_box");
}

LPCSTR CWeaponMagazined::GetMagazineEmptySect() const
{
	LPCSTR empty_sect = nullptr;

	if (HasDetachableMagazine() && IsMagazineAttached())
		empty_sect = pSettings->r_string(m_ammoTypes[m_LastLoadedMagType], "empty_box");

	return empty_sect;
}

float CWeaponMagazined::Weight() const
{
	float res = inherited::Weight();

	//додамо вагу порожнього магазину, бо вагу набоїв розрахували раніше
	if (GetMagazineEmptySect())
		res += pSettings->r_float(GetMagazineEmptySect(), "inv_weight");

	return res;
}

float CWeaponMagazined::Volume() const
{
	float res = inherited::Volume();

	//додамо обсяг магазину (так це не для кожної зброї підходить але поки так)
	if (GetMagazineEmptySect())
		res += READ_IF_EXISTS(pSettings, r_float, GetMagazineEmptySect(), "inv_volume", .0f);

	return res;
}

void CWeaponMagazined::ChangeAttachedSilencerCondition(float fDeltaCondition)
{
	if (fis_zero(m_fAttachedSilencerCondition)) return;
	m_fAttachedSilencerCondition += fDeltaCondition;
	clamp(m_fAttachedSilencerCondition, 0.f, 1.f);
}

void CWeaponMagazined::ChangeAttachedScopeCondition(float fDeltaCondition)
{
	if (fis_zero(m_fAttachedScopeCondition)) return;
	m_fAttachedScopeCondition += fDeltaCondition;
	clamp(m_fAttachedScopeCondition, 0.f, 1.f);
}

void CWeaponMagazined::ChangeAttachedGrenadeLauncherCondition(float fDeltaCondition)
{
	if (fis_zero(m_fAttachedGrenadeLauncherCondition)) return;
	m_fAttachedGrenadeLauncherCondition += fDeltaCondition;
	clamp(m_fAttachedGrenadeLauncherCondition, 0.f, 1.f);
}

void CWeaponMagazined::Hit(SHit* pHDS)
{
	if (IsHitToAddon(pHDS)) return;
	inherited::Hit(pHDS);
}

#include "../Include/xrRender/Kinematics.h"
bool CWeaponMagazined::IsHitToAddon(SHit* pHDS)
{
	auto pWeaponVisual = smart_cast<IKinematics*>(Visual());
	VERIFY(pWeaponVisual);

	bool result = false;
	float hit_power = pHDS->damage();
	hit_power *= m_HitTypeK[pHDS->hit_type];

	u16 bone_id{};
	for (const auto& sbone : m_sWpn_scope_bones)
	{
		bone_id = pWeaponVisual->LL_BoneID(sbone);

		if (IsScopeAttached() && pWeaponVisual->LL_GetBoneVisible(bone_id))
		{
			if (pHDS->bone() == bone_id)
			{
				ChangeAttachedScopeCondition(-hit_power);
				result = true;
			}
		}
	}

	if (IsSilencerAttached())
	{
		bone_id = pWeaponVisual->LL_BoneID(m_sWpn_silencer_bone);
		if (pHDS->bone() == bone_id)
		{
			ChangeAttachedSilencerCondition(-hit_power);
			result = true;
		}
	}

	if (IsGrenadeLauncherAttached())
	{
		bone_id = pWeaponVisual->LL_BoneID(m_sWpn_launcher_bone);
		if (pHDS->bone() == bone_id)
		{
			ChangeAttachedGrenadeLauncherCondition(-hit_power);
			result = true;
		}
	}

	if (result && hud_mode && IsZoomed())
		OnZoomOut();

	if (IsSilencerBroken() ||
		IsScopeBroken() ||
		IsGrenadeLauncherBroken())
		InitAddons();

	return result;
}

bool CWeaponMagazined::IsSilencerBroken()
{
	return fis_zero(m_fAttachedSilencerCondition) || IsSilencerAttached() && !SilencerAttachable() && fis_zero(GetCondition());
}

bool CWeaponMagazined::IsScopeBroken()
{
	return fis_zero(m_fAttachedScopeCondition) || IsScopeAttached() && !ScopeAttachable() && fis_zero(GetCondition());
}

bool CWeaponMagazined::IsGrenadeLauncherBroken()
{
	return fis_zero(m_fAttachedGrenadeLauncherCondition) || IsGrenadeLauncherAttached() && !GrenadeLauncherAttachable() && fis_zero(GetCondition());
}

LPCSTR	CWeaponMagazined::GetCurrentMagazine_ShortName()
{
	if (!IsMagazineAttached()) return ("");
	LPCSTR mag_short_name = pSettings->r_string(m_ammoTypes[m_LastLoadedMagType], "inv_name_short");
	int chamber_ammo = HasChamber() ? 1 : 0;	//учтём дополнительный патрон в патроннике
	if (iAmmoElapsed <= chamber_ammo)
		mag_short_name = pSettings->r_string(GetMagazineEmptySect(), "inv_name_short");
	return CStringTable().translate(mag_short_name).c_str();
}

void CWeaponMagazined::SwitchLaser(bool on) {
	if (!IsLaserAttached())
		return;

	m_bIsLaserOn = on;
	PlaySound(SndLaserSwitch, get_LastFP());

	if (!m_bIsLaserOn) {
		laser_light_render->set_active(false);
	}
}

void  CWeaponMagazined::SwitchFlashlight(bool on) {
	if (!IsFlashlightAttached())
		return;

	m_bIsFlashlightOn = on;
	PlaySound(SndFlashlightSwitch, get_LastFP());

	if (!m_bIsFlashlightOn) {
		flashlight_render->set_active(false);
		flashlight_omni->set_active(false);
		flashlight_glow->set_active(false);
	}
}