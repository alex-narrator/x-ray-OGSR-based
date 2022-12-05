// Weapon.cpp: implementation of the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Weapon.h"
#include "ParticlesObject.h"
#include "entity_alive.h"
#include "player_hud.h"
#include "inventory_item_impl.h"

#include "inventory.h"
#include "xrserver_objects_alife_items.h"

#include "actor.h"
#include "actoreffector.h"
#include "ActorCondition.h"
#include "level.h"

#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "clsid_game.h"
#include "object_broker.h"
#include "../xr_3da/LightAnimLibrary.h"
#include "game_object_space.h"
#include "script_game_object.h"

#include "string_table.h"

#include "WeaponMagazinedWGrenade.h"
#include "GamePersistent.h"

#include "HUDManager.h"
#include "UIGameCustom.h"
#include "Torch.h"
#include "NightVisionDevice.h"

constexpr auto ROTATION_TIME = 0.25f;

extern ENGINE_API Fvector4 w_states;
extern ENGINE_API Fvector3 w_timers;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWeapon::CWeapon(LPCSTR name)
{
	SetState				(eHidden);
	SetNextState			(eHidden);
	SetDefaults				();

	m_Offset.identity		();
	m_StrapOffset.identity	();
}

CWeapon::~CWeapon		()
{
	xr_delete	(m_UIScope);
	xr_delete	(m_UIScopeSecond);

	laser_light_render.destroy();
	flashlight_render.destroy();
	flashlight_omni.destroy();
	flashlight_glow.destroy();

	delete_data(m_scopes);
	delete_data(m_silencers);
	delete_data(m_glaunchers);
	delete_data(m_lasers);
	delete_data(m_flashlights);
}

void CWeapon::UpdateXForm	()
{
	if (Device.dwFrame!=dwXF_Frame)
	{
		dwXF_Frame = Device.dwFrame;

		if (0==H_Parent())	return;

		// Get access to entity and its visual
		CEntityAlive*	E		= smart_cast<CEntityAlive*>(H_Parent());
		
		if(!E) 
			return;

		const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
		if (!parent || parent && parent->use_simplified_visual())
			return;

		if (parent->attached(this))
			return;

		R_ASSERT		(E);
		IKinematics*	V		= smart_cast<IKinematics*>	(E->Visual());
		VERIFY			(V);

		// Get matrices
		int boneL{ BI_NONE }, boneR{ BI_NONE }, boneR2{ BI_NONE };

		E->g_WeaponBones(boneL,boneR,boneR2);

		if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
			boneL = boneR2;

		//KRodin: видимо такое случается иногда у некоторых визуалов нпс. Например если создать нпс с визуалом монстра наверно.
		if (boneL == BI_NONE || boneR == BI_NONE)
			return;

		// от mortan:
		// https://www.gameru.net/forum/index.php?s=&showtopic=23443&view=findpost&p=1677678
		V->CalculateBones_Invalidate();
		V->CalculateBones( true ); //V->CalculateBones	();
		Fmatrix& mL			= V->LL_GetTransform(u16(boneL));
		Fmatrix& mR			= V->LL_GetTransform(u16(boneR));
		// Calculate
		Fmatrix				mRes;
		Fvector				R,D,N;
		D.sub				(mL.c,mR.c);	

		if(fis_zero(D.magnitude()))
		{
			mRes.set(E->XFORM());
			mRes.c.set(mR.c);
		}
		else
		{		
			D.normalize();
			R.crossproduct	(mR.j,D);

			N.crossproduct	(D,R);			
			N.normalize();

			mRes.set		(R,N,D,mR.c);
			mRes.mulA_43	(E->XFORM());
		}

		UpdatePosition	(mRes);
	}
}

void CWeapon::UpdateFireDependencies_internal()
{
	if (skip_updated_frame == Device.dwFrame || Device.dwFrame != dwFP_Frame)
	{
		dwFP_Frame = Device.dwFrame;

		UpdateXForm();

		if (GetHUDmode())
		{
			HudItemData()->setup_firedeps(m_current_firedeps);
			VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
		}
		else
		{
			// 3rd person or no parent
			Fmatrix& parent = XFORM();
			Fvector& fp = vLoadedFirePoint;
			Fvector& fp2 = vLoadedFirePoint2;
			Fvector& sp = vLoadedShellPoint;

			parent.transform_tiny(m_current_firedeps.vLastFP, fp);
			parent.transform_tiny(m_current_firedeps.vLastFP2, fp2);
			parent.transform_tiny(m_current_firedeps.vLastSP, sp);
			parent.transform_tiny(m_current_firedeps.vLastShootPoint, fp);

			m_current_firedeps.vLastFD.set(0.f, 0.f, 1.f);
			parent.transform_dir(m_current_firedeps.vLastFD);

			m_current_firedeps.m_FireParticlesXForm.set(parent);
			VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
		}
	}
}

void CWeapon::ForceUpdateFireParticles()
{
	if (!GetHUDmode())
	{ // update particlesXFORM real bullet direction

		if (!H_Parent())
			return;

		Fvector p, d;
		smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

		Fmatrix _pxf{};
		_pxf.k = d;
		_pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
		_pxf.j.crossproduct(_pxf.k, _pxf.i);
		_pxf.c = XFORM().c;

		m_current_firedeps.m_FireParticlesXForm.set(_pxf);
	}
}

constexpr const char* wpn_scope_def_bone = "wpn_scope";
constexpr const char* wpn_silencer_def_bone = "wpn_silencer";
constexpr const char* wpn_launcher_def_bone_shoc = "wpn_launcher";
constexpr const char* wpn_launcher_def_bone_cop = "wpn_grenade_launcher";

void CWeapon::Load		(LPCSTR section)
{
	inherited::Load					(section);
	CShootingObject::Load			(section);

	if(pSettings->line_exist(section, "flame_particles_2"))
		m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

	if (!m_bForcedParticlesHudMode)
		m_bParticlesHudMode = !!pSettings->line_exist(hud_sect, "item_visual");

#ifdef DEBUG
	{
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"position");
		ypr					= pSettings->r_fvector3		(section,"orientation");
		ypr.mul				(PI/180.f);

		m_Offset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_Offset.translate_over	(pos);
	}

	m_StrapOffset			= m_Offset;
	if (pSettings->line_exist(section,"strap_position") && pSettings->line_exist(section,"strap_orientation")) {
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"strap_position");
		ypr					= pSettings->r_fvector3		(section,"strap_orientation");
		ypr.mul				(PI/180.f);

		m_StrapOffset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_StrapOffset.translate_over	(pos);
	}
#endif

	// load ammo classes
	m_ammoTypes.clear	(); 
	LPCSTR				S = pSettings->r_string(section,"ammo_class");
	if (S && S[0]) {
		string128		_ammoItem;
		int				count		= _GetItemCount	(S);
		for (int it=0; it<count; ++it){
			_GetItem				(S,it,_ammoItem);
			m_ammoTypes.push_back	(_ammoItem);
		}
	}

	iAmmoElapsed		= pSettings->r_s32		(section,"ammo_elapsed"		);
	iMagazineSize		= pSettings->r_s32		(section,"ammo_mag_size"	);
	
	////////////////////////////////////////////////////
	// дисперсия стрельбы

	//подбрасывание камеры во время отдачи
	camMaxAngle			= pSettings->r_float		(section,"cam_max_angle"	); 
	camMaxAngle			= deg2rad					(camMaxAngle);
	camRelaxSpeed		= pSettings->r_float		(section,"cam_relax_speed"	); 
	camRelaxSpeed		= deg2rad					(camRelaxSpeed);
	if (pSettings->line_exist(section, "cam_relax_speed_ai"))
	{
		camRelaxSpeed_AI		= pSettings->r_float		(section,"cam_relax_speed_ai"	); 
		camRelaxSpeed_AI		= deg2rad					(camRelaxSpeed_AI);
	}
	else
	{
		camRelaxSpeed_AI	= camRelaxSpeed;
	}
	
//	camDispersion		= pSettings->r_float		(section,"cam_dispersion"	); 
//	camDispersion		= deg2rad					(camDispersion);

	camMaxAngleHorz		= pSettings->r_float		(section,"cam_max_angle_horz"	); 
	camMaxAngleHorz		= deg2rad					(camMaxAngleHorz);
	camStepAngleHorz	= pSettings->r_float		(section,"cam_step_angle_horz"	); 
	camStepAngleHorz	= deg2rad					(camStepAngleHorz);	
	camDispertionFrac			= READ_IF_EXISTS(pSettings, r_float, section, "cam_dispertion_frac",	0.7f);
	//  [8/2/2005]
	//m_fParentDispersionModifier = READ_IF_EXISTS(pSettings, r_float, section, "parent_dispersion_modifier",1.0f);
	m_fPDM_disp_base			= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_base",			1.0f);
	m_fPDM_disp_vel_factor		= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_vel_factor",	1.0f);
	m_fPDM_disp_accel_factor	= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_accel_factor",	1.0f);
	m_fPDM_disp_crouch			= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch",		1.0f);
	m_fPDM_disp_crouch_no_acc	= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch_no_acc",	1.0f);
	//  [8/2/2005]
	camRecoilCompensation		= !!READ_IF_EXISTS(pSettings, r_bool, section, "cam_recoil_compensation", /*false*/true);

	fireDispersionConditionFactor = pSettings->r_float(section,"fire_dispersion_condition_factor"); 
	misfireProbability			  = pSettings->r_float(section,"misfire_probability"); 
	misfireConditionK			  = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k",	1.0f);
	conditionDecreasePerShot	  = pSettings->r_float(section,"condition_shot_dec"); 
	conditionDecreasePerShotOnHit = READ_IF_EXISTS( pSettings, r_float, section, "condition_shot_dec_on_hit", 0.f );
	conditionDecreasePerShotSilencer = READ_IF_EXISTS( pSettings, r_float, section, "condition_shot_dec_silencer", conditionDecreasePerShot );
		
	vLoadedFirePoint	= pSettings->r_fvector3		(section,"fire_point"		);
	
	if(pSettings->line_exist(section,"fire_point2")) 
		vLoadedFirePoint2= pSettings->r_fvector3	(section,"fire_point2");
	else 
		vLoadedFirePoint2= vLoadedFirePoint;
 
	m_fMinRadius		= pSettings->r_float		(section,"min_radius");
	m_fMaxRadius		= pSettings->r_float		(section,"max_radius");


	// информация о возможных апгрейдах и их визуализации в инвентаре
	if (pSettings->line_exist(section, "scope_status"))
		m_eScopeStatus				= (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "scope_status");
	if (pSettings->line_exist(section, "silencer_status"))
		m_eSilencerStatus			= (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "silencer_status");
	if (pSettings->line_exist(section, "grenade_launcher_status"))
		m_eGrenadeLauncherStatus	= (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "grenade_launcher_status");
	if (pSettings->line_exist(section, "laser_status"))
		m_eLaserStatus				= (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "laser_status");
	if (pSettings->line_exist(section, "flashlight_status"))
		m_eFlashlightStatus			= (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "flashlight_status");

	m_bZoomEnabled = !!pSettings->r_bool(section,"zoom_enabled");
	m_bUseScopeZoom = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_scope_zoom", false);
	m_bUseScopeGrenadeZoom = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_scope_grenade_zoom", false);
	m_bUseScopeDOF = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_scope_dof", true);
	m_bForceScopeDOF = !!READ_IF_EXISTS(pSettings, r_bool, section, "force_scope_dof", false);
	m_bScopeShowIndicators = !!READ_IF_EXISTS(pSettings, r_bool, section, "scope_show_indicators", true);
	m_bIgnoreScopeTexture = !!READ_IF_EXISTS(pSettings, r_bool, section, "ignore_scope_texture", false);

	m_fZoomRotateTime = READ_IF_EXISTS(pSettings, r_float, hud_sect, "zoom_rotate_time", ROTATION_TIME);

	m_fZoomFactor = CurrentZoomFactor();

	m_highlightAddons.clear();

	if (m_eScopeStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "scope_name"))
		{
			LPCSTR str = pSettings->r_string(section, "scope_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 scope_section;
				_GetItem(str, i, scope_section);
				m_scopes.push_back(scope_section);
				m_highlightAddons.push_back(scope_section);
			}
		}
	}

	if (m_eSilencerStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "silencer_name"))
		{
			LPCSTR str = pSettings->r_string(section, "silencer_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 silencer_section;
				_GetItem(str, i, silencer_section);
				m_silencers.push_back(silencer_section);
				m_highlightAddons.push_back(silencer_section);
			}
		}
	}

	if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "grenade_launcher_name"))
		{
			LPCSTR str = pSettings->r_string(section, "grenade_launcher_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 glauncher_section;
				_GetItem(str, i, glauncher_section);
				m_glaunchers.push_back(glauncher_section);
				m_highlightAddons.push_back(glauncher_section);
			}
		}
	}

	if (m_eLaserStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "laser_name"))
		{
			LPCSTR str = pSettings->r_string(section, "laser_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 laser_section;
				_GetItem(str, i, laser_section);
				m_lasers.push_back(laser_section);
				m_highlightAddons.push_back(laser_section);
			}
		}
	}

	if (m_eFlashlightStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "flashlight_name"))
		{
			LPCSTR str = pSettings->r_string(section, "flashlight_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 flashlight_section;
				_GetItem(str, i, flashlight_section);
				m_flashlights.push_back(flashlight_section);
				m_highlightAddons.push_back(flashlight_section);
			}
		}
	}

	// Кости мировой модели оружия
	if (pSettings->line_exist(section, "scope_bone"))
	{
		const char* S = pSettings->r_string(section, "scope_bone");
		if (S && strlen(S))
		{
			const int count = _GetItemCount(S);
			string128 _scope_bone{};
			for (int it = 0; it < count; ++it)
			{
				_GetItem(S, it, _scope_bone);
				m_sWpn_scope_bones.push_back(_scope_bone);
			}
		}
		else
			m_sWpn_scope_bones.push_back(wpn_scope_def_bone);
	}
	else
		m_sWpn_scope_bones.push_back(wpn_scope_def_bone);
	m_sWpn_silencer_bone	= READ_IF_EXISTS(pSettings, r_string, section, "silencer_bone", wpn_silencer_def_bone);
	m_sWpn_launcher_bone	= READ_IF_EXISTS(pSettings, r_string, section, "launcher_bone", wpn_launcher_def_bone_shoc);
	m_sWpn_laser_bone		= READ_IF_EXISTS(pSettings, r_string, section, "laser_ray_bones", "");
	m_sWpn_flashlight_bone	= READ_IF_EXISTS(pSettings, r_string, section, "torch_cone_bones", "");

	if (pSettings->line_exist(section, "hidden_bones"))
	{
		const char* S = pSettings->r_string(section, "hidden_bones");
		if (S && strlen(S))
		{
			const int count = _GetItemCount(S);
			string128 _hidden_bone{};
			for (int it = 0; it < count; ++it)
			{
				_GetItem(S, it, _hidden_bone);
				hidden_bones.push_back(_hidden_bone);
			}
		}
	}

	// Кости худовой модели оружия - если не прописаны, используются имена из конфига мировой модели.
	if (pSettings->line_exist(hud_sect, "scope_bone"))
	{
		const char* S = pSettings->r_string(hud_sect, "scope_bone");
		if (S && strlen(S))
		{
			const int count = _GetItemCount(S);
			string128 _scope_bone{};
			for (int it = 0; it < count; ++it)
			{
				_GetItem(S, it, _scope_bone);
				m_sHud_wpn_scope_bones.push_back(_scope_bone);
			}
		}
		else
			m_sHud_wpn_scope_bones = m_sWpn_scope_bones;
	}
	else
		m_sHud_wpn_scope_bones = m_sWpn_scope_bones;
	m_sHud_wpn_silencer_bone	= READ_IF_EXISTS(pSettings, r_string, hud_sect, "silencer_bone", m_sWpn_silencer_bone);
	m_sHud_wpn_launcher_bone	= READ_IF_EXISTS(pSettings, r_string, hud_sect, "launcher_bone", m_sWpn_launcher_bone);
	m_sHud_wpn_laser_bone		= READ_IF_EXISTS(pSettings, r_string, hud_sect, "laser_ray_bones", m_sWpn_laser_bone);
	m_sHud_wpn_flashlight_bone	= READ_IF_EXISTS(pSettings, r_string, hud_sect, "torch_cone_bones", m_sWpn_flashlight_bone);

	if (pSettings->line_exist(hud_sect, "hidden_bones"))
	{
		const char* S = pSettings->r_string(hud_sect, "hidden_bones");
		if (S && strlen(S))
		{
			const int count = _GetItemCount(S);
			string128 _hidden_bone{};
			for (int it = 0; it < count; ++it)
			{
				_GetItem(S, it, _hidden_bone);
				hud_hidden_bones.push_back(_hidden_bone);
			}
		}
	}
	else
		hud_hidden_bones = hidden_bones;


	//Можно и из конфига прицела читать и наоборот! Пока так.
	m_fSecondVPZoomFactor = 0.0f;
	m_fZoomHudFov = 0.0f;
	m_fSecondVPHudFov = 0.0f;
	m_fScopeInertionFactor = m_fControlInertionFactor;

	InitAddons();

	m_bHideCrosshairInZoom = true;
	if(pSettings->line_exist(hud_sect, "zoom_hide_crosshair"))
		m_bHideCrosshairInZoom = !!pSettings->r_bool(hud_sect, "zoom_hide_crosshair");

	m_bZoomInertionAllow = READ_IF_EXISTS(pSettings, r_bool, hud_sect, "allow_zoom_inertion", READ_IF_EXISTS(pSettings, r_bool, "features", "default_allow_zoom_inertion", true));
	m_bScopeZoomInertionAllow = READ_IF_EXISTS(pSettings, r_bool, hud_sect, "allow_scope_zoom_inertion", READ_IF_EXISTS(pSettings, r_bool, "features", "default_allow_scope_zoom_inertion", true));

	//////////////////////////////////////////////////////////

	m_bHasTracers = READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
	m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

	string256						temp;
	for (int i=egdNovice; i<egdCount; ++i) {
		strconcat					(sizeof(temp),temp,"hit_probability_",get_token_name(difficulty_type_token,i));
		m_hit_probability[i]		= READ_IF_EXISTS(pSettings,r_float,section,temp,1.f);
	}
	
	if (pSettings->line_exist(section, "highlight_addons")) {
		LPCSTR S = pSettings->r_string(section, "highlight_addons");
		if (S && S[0]) {
			string128 _addonItem;
			int count = _GetItemCount(S);
			for (int it = 0; it < count; ++it) {
				_GetItem(S, it, _addonItem);
				ASSERT_FMT(pSettings->section_exist(_addonItem), "Section [%s] not found!", _addonItem);
				m_highlightAddons.emplace_back(_addonItem);
			}
		}
	}
}

void CWeapon::LoadFireParams		(LPCSTR section, LPCSTR prefix)
{
	camDispersion		= pSettings->r_float		(section,"cam_dispersion"	); 
	camDispersion		= deg2rad					(camDispersion);

	if (pSettings->line_exist(section,"cam_dispersion_inc"))
	{
		camDispersionInc		= pSettings->r_float		(section,"cam_dispersion_inc"	); 
		camDispersionInc		= deg2rad					(camDispersionInc);
	}
	else
		camDispersionInc = 0;

	CShootingObject::LoadFireParams(section, prefix);
}

BOOL CWeapon::net_Spawn		(CSE_Abstract* DC)
{
	BOOL bResult					= inherited::net_Spawn(DC);
	CSE_Abstract					*e	= (CSE_Abstract*)(DC);
	CSE_ALifeItemWeapon			    *E	= smart_cast<CSE_ALifeItemWeapon*>(e);

	//iAmmoCurrent					= E->a_current;
	iAmmoElapsed					= E->a_elapsed;
	m_flagsAddOnState				= E->m_addon_flags.get();
	m_ammoType						= E->ammo_type;
	SetState						(E->wpn_state);
	SetNextState					(E->wpn_state);
	//
	bMisfire		= E->bMisfire;
	//addons check
	if (ScopeAttachable() && IsScopeAttached() && E->m_cur_scope >= m_scopes.size()) {
		Msg("! [%s]: %s: wrong scope current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_scope, m_scopes.size() - 1);
		E->m_cur_scope = 0;
	}
	m_cur_scope	= E->m_cur_scope;

	if (SilencerAttachable() && IsSilencerAttached() && E->m_cur_silencer >= m_silencers.size()) {
		Msg("! [%s]: %s: wrong silencer current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_silencer, m_silencers.size() - 1);
		E->m_cur_silencer = 0;
	}
	m_cur_silencer = E->m_cur_silencer;

	if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached() && E->m_cur_glauncher >= m_glaunchers.size()) {
		Msg("! [%s]: %s: wrong grenade launcher current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_glauncher, m_glaunchers.size() - 1);
		E->m_cur_glauncher = 0;
	}
	m_cur_glauncher = E->m_cur_glauncher;

	if (LaserAttachable() && IsLaserAttached() && E->m_cur_laser >= m_lasers.size()) {
		Msg("! [%s]: %s: wrong laser current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_laser, m_lasers.size() - 1);
		E->m_cur_laser = 0;
	}
	m_cur_laser = E->m_cur_laser;

	if (FlashlightAttachable() && IsFlashlightAttached() && E->m_cur_flashlight >= m_flashlights.size()) {
		Msg("! [%s]: %s: wrong laser current [%u/%u]", __FUNCTION__, cName().c_str(), E->m_cur_laser, m_flashlights.size() - 1);
		E->m_cur_flashlight = 0;
	}
	m_cur_flashlight = E->m_cur_flashlight;

	//
	iMagazineSize	= E->m_MagazineSize;
	//
	if ( m_ammoType >= m_ammoTypes.size() ) {
	  Msg( "! [%s]: %s: wrong m_ammoType[%u/%u]", __FUNCTION__, cName().c_str(), m_ammoType, m_ammoTypes.size() - 1 );
	  m_ammoType = 0;
	  auto se_obj = alife_object();
	  if ( se_obj ) {
	    auto W = smart_cast<CSE_ALifeItemWeapon*>( se_obj );
	    W->ammo_type = m_ammoType;
	  }
	}

	m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));	
	if(iAmmoElapsed) 
	{
		// нож автоматически заряжается двумя патронами, хотя
		// размер магазина у него 0. Что бы зря не ругаться, проверим
		// что в конфиге размер магазина не нулевой.
		if ( iMagazineSize && iAmmoElapsed > iMagazineSize ) {
		  Msg( "! [%s]: %s: wrong iAmmoElapsed[%u/%u]", __FUNCTION__, cName().c_str(), iAmmoElapsed, iMagazineSize );
		  iAmmoElapsed = iMagazineSize;
		  auto se_obj = alife_object();
		  if ( se_obj ) {
		    auto W = smart_cast<CSE_ALifeItemWeapon*>( se_obj );
		    W->a_elapsed = iAmmoElapsed;
		  }
		}
		m_fCurrentCartirdgeDisp = m_DefaultCartridge.m_kDisp;
		for(int i = 0; i < iAmmoElapsed; ++i) 
			m_magazine.push_back(m_DefaultCartridge);
	}

	if (IsLaserAttached())
		m_bIsLaserOn = E->m_bIsLaserOn;
	if (IsFlashlightAttached())
		m_bIsFlashlightOn = E->m_bIsFlashlightOn;

	m_bZoomMode = E->m_bZoom;
	if (m_bZoomMode)	OnZoomIn();
	else			OnZoomOut();

	UpdateAddonsVisibility();
	InitAddons();

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	m_bAmmoWasSpawned = false;

	return bResult;
}

void CWeapon::net_Destroy	()
{
	inherited::net_Destroy	();

	//удалить объекты партиклов
	StopFlameParticles	();
	StopFlameParticles2	();
	StopLight			();
	Light_Destroy		();

	m_magazine.clear();
	m_magazine.shrink_to_fit();
}

BOOL CWeapon::IsUpdating()
{	
	bool bIsActiveItem = m_pCurrentInventory && m_pCurrentInventory->ActiveItem()==this;
	return bIsActiveItem || bWorking || IsPending() || getVisible();
}

void CWeapon::net_Export( CSE_Abstract* E ) {
  inherited::net_Export( E );

  CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>( E );
  wpn->wpn_flags = IsUpdating() ? 1 : 0;
  wpn->a_elapsed = u16( iAmmoElapsed );
  wpn->m_addon_flags.flags = m_flagsAddOnState;
  wpn->ammo_type = (u8)m_ammoType;
  wpn->wpn_state = (u8)GetState();
  wpn->m_bZoom   = (u8)m_bZoomMode;
  //
  wpn->bMisfire			= bMisfire;
  wpn->m_cur_scope		= m_cur_scope;
  wpn->m_cur_silencer	= m_cur_silencer;
  wpn->m_cur_glauncher	= m_cur_glauncher;
  wpn->m_cur_scope		= m_cur_laser;
  wpn->m_cur_scope		= m_cur_flashlight;
  //
  wpn->m_MagazineSize	= iMagazineSize;
  //
  wpn->m_bIsLaserOn = m_bIsLaserOn;
  wpn->m_bIsFlashlightOn = m_bIsFlashlightOn;
}

void CWeapon::save(NET_Packet &output_packet){
	inherited::save	(output_packet);
}

void CWeapon::load(IReader &input_packet){
	inherited::load	(input_packet);
}


void CWeapon::OnEvent(NET_Packet& P, u16 type) 
{
	switch (type)
	{
	case GE_WPN_STATE_CHANGE:
		{
			u8				state;
			P.r_u8			(state);
			P.r_u8			(m_sub_state);		
//			u8 NewAmmoType = 
				P.r_u8();
			u8 AmmoElapsed = P.r_u8();
			u8 NextAmmo = P.r_u8();
			if (NextAmmo == u8(-1))
				m_set_next_ammoType_on_reload = u32(-1);
			else
				m_set_next_ammoType_on_reload = u8(NextAmmo);

			if (OnClient())
				SetAmmoElapsed(int(AmmoElapsed));			
			OnStateSwitch(u32(state), GetState());
		}
		break;
	default:
		{
			inherited::OnEvent(P,type);
		}break;
	}
};

void CWeapon::shedule_Update	(u32 dT)
{
	// Inherited
	inherited::shedule_Update	(dT);
}

void CWeapon::OnH_B_Independent	(bool just_before_destroy)
{
	RemoveShotEffector			();

	inherited::OnH_B_Independent(just_before_destroy);

	//завершить принудительно все процессы что шли
	FireEnd						();
	SetPending					(FALSE);
	SwitchState					(eIdle);

	m_strapped_mode				= false;
	OnZoomOut					();
	m_fZoomRotationFactor	= 0.f;
	UpdateXForm					();
}

void CWeapon::OnH_A_Independent	()
{
	inherited::OnH_A_Independent();
	Light_Destroy				();
};

void CWeapon::OnH_A_Chield		()
{
	inherited::OnH_A_Chield		();

	UpdateAddonsVisibility		();
};

void CWeapon::OnActiveItem ()
{
	inherited::OnActiveItem		();
	//если мы занружаемся и оружие было в руках
	SetState					(eIdle);
	SetNextState				(eIdle);
}

void CWeapon::OnHiddenItem ()
{
	inherited::OnHiddenItem();
	SetState					(eHidden);
	SetNextState				(eHidden);
	m_set_next_ammoType_on_reload	= u32(-1);
}


void CWeapon::OnH_B_Chield		()
{
	inherited::OnH_B_Chield		();

	OnZoomOut					();
	m_set_next_ammoType_on_reload	= u32(-1);
}

static float state_time = 0;				// таймер нахождения оружия в текущем состоянии
static float state_time_heat = 0;			// таймер нагрева оружия
static float previous_heating = 0;			// "нагретость" оружия в предыдущем состоянии

#include "WeaponBinoculars.h"

void CWeapon::UpdateWeaponParams()
{
#pragma todo("KRodin: адаптировать тепловизор и тп. под новый рендер, если это возможно.")

	if (!IsHidden()) {
		w_states.x = m_fZoomRotationFactor;			//x = zoom mode, y - текущее состояние, z - старое состояние
		if (psActorFlags.test(AF_DOF_SCOPE) && !(IsZoomed() && !IsRotatingToZoom() && (IsScopeAttached() || m_bForceScopeDOF) && !IsGrenadeMode() && m_bUseScopeDOF))
			w_states.x = 0.f;
		if (w_states.y != GetState())	// первый апдейт или стейт изменился
		{
			w_states.z = w_states.y;						// записываем старое состояние
			state_time_heat = state_time = Device.fTimeGlobal;	// инитим счетчики времени
			previous_heating = w_timers.z;				// сохраняем "нагретость" оружия
			w_timers.y = w_timers.x;						// записываем время нахождения в предыдущем состоянии
			w_states.y = (float)GetState();				// обновляем состояние
		}
		// флаг бинокля в руках (в этом режиме не нужно размытие)
		if (smart_cast<CWeaponBinoculars*>(this))
			w_states.w = 0;
		else
			w_states.w = 1;
		if (w_states.y == static_cast<float>(eFire) || w_states.y == static_cast<float>(eFire2))	 //стреляем, значит оружие греется
		{
			w_timers.z = Device.fTimeGlobal - state_time_heat + previous_heating;
		}
		else		// не стреляем - оружие охлаждается
		{
			if (w_timers.z > EPS)		// оружие все еще нагрето
			{
				float tm = state_time_heat + previous_heating - Device.fTimeGlobal;
				w_timers.z = (tm<EPS) ? 0.f : tm;
			}
		}
		w_timers.x = Device.fTimeGlobal - state_time;		// обновляем таймер текущего состояния
	}
}


u8 CWeapon::idle_state() {
	auto* actor = smart_cast<CActor*>(H_Parent());

	if (actor) {
		u32 st = actor->get_state();
		if (st & mcSprint)
			return eSubstateIdleSprint;
		else if (st & mcAnyAction && !(st & mcJump) && !(st & mcFall))
			return eSubstateIdleMoving;
	}

	return eIdle;
}


void CWeapon::UpdateCL		()
{
	inherited::UpdateCL		();

	UpdateHUDAddonsVisibility();

	//подсветка от выстрела
	UpdateLight				();

	if (ParentIsActor())
		UpdateWeaponParams();	// параметры для рендера оружия в режиме тепловидения

	//нарисовать партиклы
	UpdateFlameParticles	();
	UpdateFlameParticles2	();
	
	VERIFY(smart_cast<IKinematics*>(Visual()));

        if ( GetState() == eIdle ) {
          auto state = idle_state();
          if ( m_idle_state != state ) {
            m_idle_state = state;
			if (GetNextState() != eMagEmpty && GetNextState() != eReload)
			{
				SwitchState(eIdle);
			}
          }
        }
        else
          m_idle_state = eIdle;

	UpdateLaser();
	UpdateFlashlight();

	if (ParentIsActor())
	{
		bool b_shoots = ((GetState() == eFire) || (GetState() == eFire2));
		if (GetHUDmode())
			Actor()->TryToBlockSprint(b_shoots);

		if (Actor()->conditions().IsCantWalk() && IsZoomed())
			OnZoomOut();
	}
}

void CWeapon::UpdateLaser()
{
	if (IsLaserAttached())
	{
		auto io = smart_cast<CInventoryOwner*>(H_Parent());
		if (!laser_light_render->get_active() && IsLaserOn() && (!H_Parent() || (io && this == io->inventory().ActiveItem()))) {
			laser_light_render->set_active(true);
			UpdateAddonsVisibility();
		}
		else if (laser_light_render->get_active() && (!IsLaserOn() || !(!H_Parent() || (io && this == io->inventory().ActiveItem())))) {
			laser_light_render->set_active(false);
			UpdateAddonsVisibility();
		}

		if (laser_light_render->get_active()) {
			laser_pos = get_LastFP();
			Fvector laser_dir = get_LastFD();

			if (GetHUDmode()) {
				if (laserdot_attach_bone.size()) {
					GetBoneOffsetPosDir(laserdot_attach_bone, laser_pos, laser_dir, laserdot_attach_offset);
					CorrectDirFromWorldToHud(laser_dir);
				}
			}
			else {
				XFORM().transform_tiny(laser_pos, laserdot_world_attach_offset);
			}

			Fmatrix laserXForm;
			laserXForm.identity();
			laserXForm.k.set(laser_dir);
			Fvector::generate_orthonormal_basis_normalized(laserXForm.k, laserXForm.j, laserXForm.i);

			laser_light_render->set_position(laser_pos);
			laser_light_render->set_rotation(laserXForm.k, laserXForm.i);

			// calc color animator
			if (laser_lanim)
			{
				int frame;
				const u32 clr = laser_lanim->CalculateBGR(Device.fTimeGlobal, frame);

				Fcolor fclr{ (float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f };
				fclr.mul_rgb(laser_fBrightness / 255.f);
				laser_light_render->set_color(fclr);
			}
		}
	}
}

void CWeapon::UpdateFlashlight()
{
	if (IsFlashlightAttached())
	{
		auto io = smart_cast<CInventoryOwner*>(H_Parent());
		if (!flashlight_render->get_active() && IsFlashlightOn() && (!H_Parent() || (io && this == io->inventory().ActiveItem()))) {
			flashlight_render->set_active(true);
			flashlight_omni->set_active(true);
			flashlight_glow->set_active(true);
			UpdateAddonsVisibility();
		}
		else if (flashlight_render->get_active() && (!IsFlashlightOn() || !(!H_Parent() || (io && this == io->inventory().ActiveItem())))) {
			flashlight_render->set_active(false);
			flashlight_omni->set_active(false);
			flashlight_glow->set_active(false);
			UpdateAddonsVisibility();
		}

		if (flashlight_render->get_active()) {
			Fvector flashlight_pos_omni, flashlight_dir, flashlight_dir_omni;

			if (GetHUDmode()) {
				GetBoneOffsetPosDir(flashlight_attach_bone, flashlight_pos, flashlight_dir, flashlight_attach_offset);
				CorrectDirFromWorldToHud(flashlight_dir);

				GetBoneOffsetPosDir(flashlight_attach_bone, flashlight_pos_omni, flashlight_dir_omni, flashlight_omni_attach_offset);
				CorrectDirFromWorldToHud(flashlight_dir_omni);
			}
			else {
				flashlight_dir = get_LastFD();
				XFORM().transform_tiny(flashlight_pos, flashlight_world_attach_offset);

				flashlight_dir_omni = get_LastFD();
				XFORM().transform_tiny(flashlight_pos_omni, flashlight_omni_world_attach_offset);
			}

			Fmatrix flashlightXForm;
			flashlightXForm.identity();
			flashlightXForm.k.set(flashlight_dir);
			Fvector::generate_orthonormal_basis_normalized(flashlightXForm.k, flashlightXForm.j, flashlightXForm.i);
			flashlight_render->set_position(flashlight_pos);
			flashlight_render->set_rotation(flashlightXForm.k, flashlightXForm.i);

			flashlight_glow->set_position(flashlight_pos);
			flashlight_glow->set_direction(flashlightXForm.k);

			Fmatrix flashlightomniXForm;
			flashlightomniXForm.identity();
			flashlightomniXForm.k.set(flashlight_dir_omni);
			Fvector::generate_orthonormal_basis_normalized(flashlightomniXForm.k, flashlightomniXForm.j, flashlightomniXForm.i);
			flashlight_omni->set_position(flashlight_pos_omni);
			flashlight_omni->set_rotation(flashlightomniXForm.k, flashlightomniXForm.i);

			// calc color animator
			if (flashlight_lanim)
			{
				int frame;
				const u32 clr = flashlight_lanim->CalculateBGR(Device.fTimeGlobal, frame);

				Fcolor fclr{ (float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f };
				fclr.mul_rgb(flashlight_fBrightness / 255.f);
				flashlight_render->set_color(fclr);
				flashlight_omni->set_color(fclr);
				flashlight_glow->set_color(fclr);
			}
		}
	}
}

void CWeapon::renderable_Render		()
{
	UpdateXForm				();

	//нарисовать подсветку
	RenderLight				();	

	//если мы в режиме снайперки, то сам HUD рисовать не надо
	if(IsZoomed() && !IsRotatingToZoom() && ZoomTexture())
		RenderHud(FALSE);
	else
		RenderHud(TRUE);

	inherited::renderable_Render		();
}

bool CWeapon::need_renderable() 
{
	return !Device.m_SecondViewport.IsSVPFrame() && !(IsZoomed() && ZoomTexture() && !IsRotatingToZoom());
}

void CWeapon::signal_HideComplete()
{
	if(H_Parent())
		setVisible(FALSE);
	SetPending(FALSE);
}

void CWeapon::SetDefaults()
{
	bWorking2			= false;
	SetPending			(FALSE);

	m_flags.set			(FUsingCondition, TRUE);
	bMisfire			= false;
	m_flagsAddOnState	= 0;
	m_bZoomMode			= false;
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
	Position().set		(trans.c);
	XFORM().mul			(trans,m_strapped_mode ? m_StrapOffset : m_Offset);
	VERIFY				(!fis_zero(DET(renderable.xform)));
}


bool CWeapon::Action(s32 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;

	switch(cmd) 
	{
		case kWPN_FIRE: 
			{
				//если оружие чем-то занято, то ничего не делать
				{				
					if(flags&CMD_START) 
					{
						if(IsPending())		return false;
						FireStart			();
					}else 
						FireEnd();
				};

			} 
			return true;
		case kWPN_NEXT: 
			{
				if(IsPending() || OnClient()) {
					return false;
				}
									
				if (flags & CMD_START){
					bool manually = psActorFlags.test(AF_NO_AUTO_RELOAD);

					if (GetNextAmmoType(manually) != u32(-1) && TryToGetAmmo(GetNextAmmoType(manually))){
						m_set_next_ammoType_on_reload = GetNextAmmoType(manually);

						if (OnServer()){
							if (!manually) Reload();

//							Msg("m_set_next_ammoType_on_reload [%d]", m_set_next_ammoType_on_reload);

							LPCSTR ammo_sect = pSettings->r_string(m_ammoTypes[m_set_next_ammoType_on_reload].c_str(), "inv_name_short");
							string1024	str;
							strconcat(sizeof(str), str, *CStringTable().translate("st_next_ammo_type"), ": ", *CStringTable().translate(ammo_sect));
							HUD().GetUI()->AddInfoMessage("item_usage", str, false);
						}
					}
				}
			} 
            return true;
		case kWPN_ZOOM:
		{
			if (IsZoomEnabled())
			{
				auto pActor = smart_cast<const CActor*>(H_Parent());
				auto pNightVis = pActor->GetNightVisionDevice();
				if (pNightVis && pNightVis->IsPowerOn())
				{
					//if (IsScopeAttached() && !IsGrenadeMode()){
						HUD().GetUI()->AddInfoMessage("actor_state", "cant_aim");
						return false;
					//}
				}

				if (!pActor->conditions().IsCantWalk())
				{
					if (flags & CMD_START && !IsPending())
					{
						if (!psActorFlags.is(AF_HOLD_TO_AIM) && IsZoomed())
						{
							OnZoomOut();
						}
						else
							OnZoomIn();
					}
					else if (IsZoomed() && psActorFlags.is(AF_HOLD_TO_AIM))
					{
						OnZoomOut();
					}
					return true;
				}
				else
					HUD().GetUI()->AddInfoMessage("actor_state", "cant_walk");
			}
			else
				return false;
		}
		case kWPN_ZOOM_INC:
		case kWPN_ZOOM_DEC:
		{
			if (IsZoomEnabled() && IsZoomed() && m_bScopeDynamicZoom && IsScopeAttached() && (flags&CMD_START))
			{
				// если в режиме ПГ - не будем давать использовать динамический зум
				if (IsGrenadeMode())
					return false;

				ZoomChange(cmd == kWPN_ZOOM_INC);

				return true;
			}
			else
				return false;
		}
		case kWPN_FUNC:
		{
			if (Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION))) {
				if (HasScopeSecond() && (flags & CMD_START)) {
					m_bScopeSecondMode = !m_bScopeSecondMode;
					if (IsZoomed())
						OnZoomOut(true);
					return true;
				}
			}
		}
	}
	return false;
}

float  CWeapon::GetZoomStepDelta(float scope_factor, float min_scope_factor, u32 step_count)
{
	float total_zoom_delta = scope_factor - min_scope_factor;
	return total_zoom_delta / step_count;
}

void CWeapon::ZoomChange(bool inc)
{
	bool wasChanged = false;

	if (SecondVPEnabled()){
		const float currentZoomFactor = m_fRTZoomFactor;

		m_fZoomFactor += GetZoomStepDelta(m_fSecondVPZoomFactor, m_fMinScopeZoomFactor, m_uZoomStepCount) * (inc ? 1 : -1);//delta;
		clamp(m_fSecondVPZoomFactor, m_fMinScopeZoomFactor, m_fScopeZoomFactor);

		wasChanged = !fsimilar(currentZoomFactor, m_fRTZoomFactor);
	}else{
		const float currentZoomFactor = m_fZoomFactor;

		m_fZoomFactor += GetZoomStepDelta(m_fScopeZoomFactor, m_fMinScopeZoomFactor, m_uZoomStepCount) * (inc ? 1 : -1);//delta;
		clamp(m_fZoomFactor, m_fMinScopeZoomFactor, m_fScopeZoomFactor);

		wasChanged = !fsimilar(currentZoomFactor, m_fZoomFactor);

		if (H_Parent() && !IsRotatingToZoom() && !SecondVPEnabled())
			m_fRTZoomFactor = m_fZoomFactor; //store current
	}

	if (wasChanged)
	{
		OnZoomChanged();
	}
}

void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, u32 ParentID) 
{
	if(!m_ammoTypes.size())			return;
	if (OnClient())					return;
	m_bAmmoWasSpawned = true;
	
	if (!ammoSect) ammoSect = m_ammoTypes.front().c_str();
	
	CSE_Abstract *D					= F_entity_Create(ammoSect);
	CSE_ALifeItemAmmo* l_pA = smart_cast<CSE_ALifeItemAmmo*>(D);
	if (l_pA){	
		R_ASSERT					(l_pA);
		l_pA->m_boxSize				= (u16)pSettings->r_s32(ammoSect, "box_size");
		D->s_name					= ammoSect;
		D->set_name_replace			("");
		D->s_gameid					= u8(GameID());
		D->s_RP						= 0xff;
		D->ID						= 0xffff;
		if (ParentID == 0xffffffff)	
			D->ID_Parent			= (u16)H_Parent()->ID();
		else
			D->ID_Parent			= (u16)ParentID;

		D->ID_Phantom				= 0xffff;
		D->s_flags.assign			(M_SPAWN_OBJECT_LOCAL);
		D->RespawnTime				= 0;
		l_pA->m_tNodeID				= ai_location().level_vertex_id();

		if(boxCurr == 0xffffffff) 	
			boxCurr					= l_pA->m_boxSize;

		if (boxCurr > 0)
		{
			while (boxCurr)
			{
				l_pA->a_elapsed = (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
				NET_Packet				P;
				D->Spawn_Write(P, TRUE);
				Level().Send(P, net_flags(TRUE));

				if (boxCurr > l_pA->m_boxSize)
					boxCurr -= l_pA->m_boxSize;
				else
					boxCurr = 0;
			}
		}
		else
		{
			NET_Packet				P;
			D->Spawn_Write(P, TRUE);
			Level().Send(P, net_flags(TRUE));
		}
	};
	F_entity_Destroy				(D);
}

int CWeapon::GetAmmoCurrent(bool use_item_to_spawn) const
{
	int l_count = iAmmoElapsed;
	if (!m_pCurrentInventory) return l_count;

	//чтоб не делать лишних пересчетов
	if (m_pCurrentInventory->ModifyFrame() <= m_dwAmmoCurrentCalcFrame)
		if (!smart_cast<const CActor*>(H_Parent()) || !psActorFlags.test(AF_AMMO_FROM_BELT)) //очень некрасивый костыль!
			return l_count + iAmmoCurrent;

//	Msg("[%s] get ammo current [%d]:[%s] weapon [%s] - before types cycle", m_pCurrentInventory->GetOwner()->Name(), l_count + iAmmoCurrent, *m_ammoTypes[m_ammoType], this->Name_script());

	m_dwAmmoCurrentCalcFrame = Device.dwFrame;
	iAmmoCurrent = 0;

	for (int i = 0; i < (int)m_ammoTypes.size(); ++i)
	{
		iAmmoCurrent += GetAmmoCount_forType(m_ammoTypes[i]);

		if (!use_item_to_spawn)
			continue;

		if (!inventory_owner().item_to_spawn())
			continue;

		iAmmoCurrent += inventory_owner().ammo_in_box_to_spawn();

		//Msg("[%s] get ammo current [%d]:[%s] weapon [%s] ", m_pCurrentInventory->GetOwner()->Name(), l_count + iAmmoCurrent, m_ammoTypes[i].c_str(), Name_script());
	}
	return l_count + iAmmoCurrent;
}

int CWeapon::GetAmmoCount( u8 ammo_type, u32 max ) const {
  VERIFY( m_pInventory );
  R_ASSERT( ammo_type < m_ammoTypes.size() );

  return GetAmmoCount_forType( m_ammoTypes[ ammo_type ], max );
}

int CWeapon::GetAmmoCount_forType( shared_str const& ammo_type, u32 max ) const {
  u32 res = 0;
  auto callback = [&]( const auto pIItem ) -> bool {
    auto* ammo = smart_cast<CWeaponAmmo*>( pIItem );
    if ( ammo->cNameSect() == ammo_type )
      res += ammo->m_boxCurr;
    return ( max > 0 && res >= max );
  };

  m_pCurrentInventory->IterateAmmo( false, callback );
  if ( max == 0 || res < max )
    if ( !smart_cast<const CActor*>( H_Parent() ) || !psActorFlags.test(AF_AMMO_FROM_BELT) )
      m_pCurrentInventory->IterateAmmo( true, callback );

  return res;
}

float CWeapon::GetConditionMisfireProbability() const
{
	float mis = 0.0f;

	//if (GetCondition() > 0.95f) return 0.0f;

	//float mis = misfireProbability + powf(1.f - GetCondition(), 3.f)*misfireConditionK;

	//вероятность осечки от патрона
	if (!m_magazine.empty())
	{
		const auto& l_cartridge = m_magazine.back();
		mis = l_cartridge.m_misfireProbability;
	}
	//вероятность осечки от состояния оружия
	if (GetCondition() < 0.95f)
		mis += (misfireProbability + powf(1.f - GetCondition(), 3.f) * misfireConditionK);

	clamp(mis, 0.0f, 0.99f);
	return mis;
}

BOOL CWeapon::CheckForMisfire	()
{
	if (OnClient()) return FALSE;

	if ( Core.Features.test( xrCore::Feature::npc_simplified_shooting ) ) {
	  CActor *actor = smart_cast<CActor*>( H_Parent() );
	  if ( !actor ) return FALSE;
	}

	float rnd = ::Random.randF(0.f,1.f);
	float mp = GetConditionMisfireProbability();
	if(rnd < mp)
	{
		FireEnd();

		bMisfire = true;
		SwitchState(eMisfire);		
		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CWeapon::IsMisfire() const
{	
	return bMisfire;
}
void CWeapon::Reload()
{
	OnZoomOut();
}


bool CWeapon::IsGrenadeLauncherAttached() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eGrenadeLauncherStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) || 
			CSE_ALifeItemWeapon::eAddonPermanent == m_eGrenadeLauncherStatus;
}

bool CWeapon::IsScopeAttached() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eScopeStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope)) || 
			CSE_ALifeItemWeapon::eAddonPermanent == m_eScopeStatus;
}

bool CWeapon::IsSilencerAttached() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eSilencerStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer)) || 
			CSE_ALifeItemWeapon::eAddonPermanent == m_eSilencerStatus;
}

bool CWeapon::IsLaserAttached() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eLaserStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonLaser)) ||
		CSE_ALifeItemWeapon::eAddonPermanent == m_eLaserStatus;
}

bool CWeapon::IsFlashlightAttached() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eFlashlightStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonFlashlight)) ||
		CSE_ALifeItemWeapon::eAddonPermanent == m_eFlashlightStatus;
}

bool CWeapon::GrenadeLauncherAttachable() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eGrenadeLauncherStatus);
}
bool CWeapon::ScopeAttachable() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eScopeStatus);
}
bool CWeapon::SilencerAttachable() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eSilencerStatus);
}
bool CWeapon::LaserAttachable() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eLaserStatus);
}
bool CWeapon::FlashlightAttachable() const{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eFlashlightStatus);
}

void CWeapon::UpdateHUDAddonsVisibility()
{
	if (!GetHUDmode())
		return;

	auto pWeaponVisual = smart_cast<IKinematics*>(Visual());

	if (ScopeAttachable())
		HudItemData()->set_bone_visible(m_sHud_wpn_scope_bones, IsScopeAttached());

	if (m_eScopeStatus == ALife::eAddonDisabled)
		HudItemData()->set_bone_visible(m_sHud_wpn_scope_bones, FALSE, TRUE);
	else if (m_eScopeStatus == ALife::eAddonPermanent)
		HudItemData()->set_bone_visible(m_sHud_wpn_scope_bones, TRUE, TRUE);

	if (pWeaponVisual->LL_BoneID(m_sHud_wpn_silencer_bone) != BI_NONE) {
		if (SilencerAttachable()) {
			bool b_show_on_model = READ_IF_EXISTS(pSettings, r_bool, GetSilencerName(), "show_on_model", true);
			HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, IsSilencerAttached() && b_show_on_model);
		}

		if (m_eSilencerStatus == ALife::eAddonDisabled)
			HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, FALSE, TRUE);
		else if (m_eSilencerStatus == ALife::eAddonPermanent)
			HudItemData()->set_bone_visible(m_sHud_wpn_silencer_bone, TRUE, TRUE);
	}

	if (!HudItemData()->has_bone(m_sHud_wpn_launcher_bone) && HudItemData()->has_bone(wpn_launcher_def_bone_cop))
		m_sHud_wpn_launcher_bone = wpn_launcher_def_bone_cop;

	if (GrenadeLauncherAttachable())
		HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, IsGrenadeLauncherAttached());
	
	if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled)
		HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, FALSE, TRUE);
	else if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
		HudItemData()->set_bone_visible(m_sHud_wpn_launcher_bone, TRUE, TRUE);

	if (m_sHud_wpn_laser_bone.size() && IsLaserAttached())
		HudItemData()->set_bone_visible(m_sHud_wpn_laser_bone, IsLaserOn(), TRUE);

	if (m_sHud_wpn_flashlight_bone.size() && IsFlashlightAttached())
		HudItemData()->set_bone_visible(m_sHud_wpn_flashlight_bone, IsFlashlightOn(), TRUE);

	for (const shared_str& bone_name : hud_hidden_bones)
		HudItemData()->set_bone_visible(bone_name, FALSE, TRUE);

	callback(GameObject::eOnUpdateHUDAddonsVisibiility)();
}

void CWeapon::UpdateAddonsVisibility()
{
	auto pWeaponVisual = smart_cast<IKinematics*>(Visual());
	VERIFY(pWeaponVisual);

	UpdateHUDAddonsVisibility();

	///////////////////////////////////////////////////////////////////
	u16 bone_id{};

	for (const auto& sbone : m_sWpn_scope_bones)
	{
		bone_id = pWeaponVisual->LL_BoneID(sbone);

		if (ScopeAttachable())
		{
			if (IsScopeAttached())
			{
				if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
					pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
			}
			else
			{
				if (pWeaponVisual->LL_GetBoneVisible(bone_id))
					pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
			}
		}

		if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
			pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		else if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE && !pWeaponVisual->LL_GetBoneVisible(bone_id))
			pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
	}
	///////////////////////////////////////////////////////////////////

	bone_id = pWeaponVisual->LL_BoneID(m_sWpn_silencer_bone);

	if (SilencerAttachable() && (bone_id != BI_NONE))
	{
		bool b_show_on_model = READ_IF_EXISTS(pSettings, r_bool, GetSilencerName(), "show_on_model", true);
		if (IsSilencerAttached() && b_show_on_model)
		{
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else
		{
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}

	if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
	else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE && !pWeaponVisual->LL_GetBoneVisible(bone_id))
		pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);

	///////////////////////////////////////////////////////////////////

	bone_id = pWeaponVisual->LL_BoneID(m_sWpn_launcher_bone);

	if (GrenadeLauncherAttachable())
	{
		if (IsGrenadeLauncherAttached())
		{
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else
		{
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}

	if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
	else if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE && !pWeaponVisual->LL_GetBoneVisible(bone_id))
		pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);

	///////////////////////////////////////////////////////////////////

	if (m_sWpn_laser_bone.size() && IsLaserAttached())
	{
		bone_id = pWeaponVisual->LL_BoneID(m_sWpn_laser_bone);

		if (bone_id != BI_NONE) {
			const bool laser_on = IsLaserOn();
			if (pWeaponVisual->LL_GetBoneVisible(bone_id) && !laser_on)
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
			else if (!pWeaponVisual->LL_GetBoneVisible(bone_id) && laser_on)
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
	}

	///////////////////////////////////////////////////////////////////

	if (m_sWpn_flashlight_bone.size() && IsFlashlightAttached())
	{
		bone_id = pWeaponVisual->LL_BoneID(m_sWpn_flashlight_bone);

		if (bone_id != BI_NONE) {
			const bool flashlight_on = IsFlashlightOn();
			if (pWeaponVisual->LL_GetBoneVisible(bone_id) && !flashlight_on)
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
			else if (!pWeaponVisual->LL_GetBoneVisible(bone_id) && flashlight_on)
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
	}

	///////////////////////////////////////////////////////////////////

	for (const auto& bone_name : hidden_bones)
	{
		bone_id = pWeaponVisual->LL_BoneID(bone_name);
		if (bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
			pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
	}

	///////////////////////////////////////////////////////////////////

	callback(GameObject::eOnUpdateAddonsVisibiility)();

	pWeaponVisual->CalculateBones_Invalidate();
	pWeaponVisual->CalculateBones();
}


bool CWeapon::Activate( bool now ) 
{
	UpdateAddonsVisibility();
	return inherited::Activate( now );
}

void CWeapon::InitAddons()
{
}

float CWeapon::CurrentZoomFactor()
{
	//if (SecondVPEnabled())
	//	return 1.f;//Core.Features.test(xrCore::Feature::ogse_wpn_zoom_system) ? 1.0f : m_fIronSightZoomFactor; // no change to main fov zoom when use second vp
	//else if (IsScopeAttached())
	//	return m_fScopeZoomFactor;
	//else
	//	return m_fIronSightZoomFactor;

	float res = 1.f;

	if (SecondVPEnabled())
		return res;

	if (IsScopeAttached())
	{
		if (m_bScopeDynamicZoom)
		{
			clamp(m_fRTZoomFactor, m_fMinScopeZoomFactor, m_fScopeZoomFactor);
			res = m_fRTZoomFactor;
			//			Msg("m_fRTZoomFactor res = [%.2f]", res);
		}
		else
		{
			res = IsSecondScopeMode() ? m_fScopeZoomFactorSecond : m_fScopeZoomFactor;
			//			Msg("m_fScopeZoomFactor res = [%.2f]", res);
		}
	}
	else
	{
		res = m_fIronSightZoomFactor;
		//		Msg("m_fIronSightZoomFactor res = [%.2f]", res);
	}

	return res;
}

bool CWeapon::HasScopeSecond() const {
	return IsScopeAttached() && !IsGrenadeMode() && m_bHasScopeSecond;
}

bool CWeapon::IsSecondScopeMode() const {
	return IsScopeAttached() && !IsGrenadeMode() && m_bScopeSecondMode;
}

void CWeapon::OnZoomIn()
{
	m_bZoomMode = true;

	// если в режиме ПГ - не будем давать включать динамический зум
	//if ( m_bScopeDynamicZoom && !IsGrenadeMode() && !SecondVPEnabled())
	//	m_fZoomFactor = m_fRTZoomFactor;
	//else
	m_fZoomFactor = CurrentZoomFactor();

	if (IsScopeAttached() && !IsGrenadeMode()) {
		if (!m_bScopeZoomInertionAllow)
			AllowHudInertion(FALSE);
	}
	else if (!m_bZoomInertionAllow)
		AllowHudInertion(FALSE);

	if(GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(true);

	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if ( pActor )
		pActor->callback(GameObject::eOnActorWeaponZoomIn)(lua_game_object());
}

void CWeapon::OnZoomOut(bool rezoom)
{
	m_fZoomFactor = 1.f;// Core.Features.test(xrCore::Feature::ogse_wpn_zoom_system) ? 1.f : g_fov;

	if ( m_bZoomMode ) {

		m_bZoomMode = false;

		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if ( pActor ) {
			w_states.set( 0.f, 0.f, 0.f, 1.f );
			pActor->callback(GameObject::eOnActorWeaponZoomOut)(lua_game_object());
		}
	}

	AllowHudInertion(TRUE);

	if (GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(false);

	ResetSubStateTime();
}

bool CWeapon::UseScopeTexture() {
	return !SecondVPEnabled() && (m_UIScope || IsSecondScopeMode() && m_UIScopeSecond); // только если есть текстура прицела - для простого создания коллиматоров
}

CUIStaticItem* CWeapon::ZoomTexture()
{
	if (UseScopeTexture())
		return IsSecondScopeMode() ? m_UIScopeSecond : m_UIScope;
	else
		return NULL;
}

void CWeapon::SwitchState(u32 S)
{
	if (OnClient()) return;

	SetNextState		( S );	// Very-very important line of code!!! :)
	if (CHudItem::object().Local() && !CHudItem::object().getDestroy()/* && (S!=NEXT_STATE)*/ 
		&& m_pCurrentInventory && OnServer())	
	{
		// !!! Just single entry for given state !!!
		NET_Packet		P;
		CHudItem::object().u_EventGen		(P,GE_WPN_STATE_CHANGE,CHudItem::object().ID());
		P.w_u8			(u8(S));
		P.w_u8			(u8(m_sub_state));
		P.w_u8			(u8(m_ammoType& 0xff));
		P.w_u8			(u8(iAmmoElapsed & 0xff));
		P.w_u8			(u8(m_set_next_ammoType_on_reload & 0xff));
		CHudItem::object().u_EventSend		(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CWeapon::OnMagazineEmpty	()
{
	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}


void CWeapon::reinit			()
{
	CShootingObject::reinit		();
	CHudItemObject::reinit			();
}

void CWeapon::reload			(LPCSTR section)
{
	CShootingObject::reload		(section);
	CHudItemObject::reload			(section);
	
	m_can_be_strapped			= true;
	m_strapped_mode				= false;
	
	if (pSettings->line_exist(section,"strap_bone0"))
		m_strap_bone0			= pSettings->r_string(section,"strap_bone0");
	else
		m_can_be_strapped		= false;
	
	if (pSettings->line_exist(section,"strap_bone1"))
		m_strap_bone1			= pSettings->r_string(section,"strap_bone1");
	else
		m_can_be_strapped		= false;

	if (m_eScopeStatus == ALife::eAddonAttachable) {
		m_addon_holder_range_modifier	= READ_IF_EXISTS(pSettings,r_float,GetScopeName(),"holder_range_modifier",m_holder_range_modifier);
		m_addon_holder_fov_modifier		= READ_IF_EXISTS(pSettings,r_float,GetScopeName(),"holder_fov_modifier",m_holder_fov_modifier);
	}
	else {
		m_addon_holder_range_modifier	= m_holder_range_modifier;
		m_addon_holder_fov_modifier		= m_holder_fov_modifier;
	}


	{
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"position");
		ypr					= pSettings->r_fvector3		(section,"orientation");
		ypr.mul				(PI/180.f);

		m_Offset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_Offset.translate_over	(pos);
	}

	m_StrapOffset			= m_Offset;
	if (pSettings->line_exist(section,"strap_position") && pSettings->line_exist(section,"strap_orientation")) {
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"strap_position");
		ypr					= pSettings->r_fvector3		(section,"strap_orientation");
		ypr.mul				(PI/180.f);

		m_StrapOffset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_StrapOffset.translate_over	(pos);
	}
	else
		m_can_be_strapped	= false;

	m_ef_main_weapon_type	= READ_IF_EXISTS(pSettings,r_u32,section,"ef_main_weapon_type",u32(-1));
	m_ef_weapon_type		= READ_IF_EXISTS(pSettings,r_u32,section,"ef_weapon_type",u32(-1));
}

void CWeapon::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CWeapon::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CWeapon::setup_physic_shell()
{
	CPhysicsShellHolder::setup_physic_shell();
}

bool CWeapon::can_kill	() const
{
	if (GetAmmoCurrent(true) || m_ammoTypes.empty())
		return				(true);

	return					(false);
}

CInventoryItem *CWeapon::can_kill	(CInventory *inventory) const
{
	if (GetAmmoElapsed() || m_ammoTypes.empty())
		return				(const_cast<CWeapon*>(this));

	for (const auto& item : inventory->m_all) {
		CInventoryItem	*inventory_item = smart_cast<CInventoryItem*>(item);
		if (!inventory_item)
			continue;
		
		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(),m_ammoTypes.end(),inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

const CInventoryItem *CWeapon::can_kill	(const xr_vector<const CGameObject*> &items) const
{
	if (m_ammoTypes.empty())
		return				(this);

	xr_vector<const CGameObject*>::const_iterator I = items.begin();
	xr_vector<const CGameObject*>::const_iterator E = items.end();
	for ( ; I != E; ++I) {
		const CInventoryItem	*inventory_item = smart_cast<const CInventoryItem*>(*I);
		if (!inventory_item)
			continue;

		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(),m_ammoTypes.end(),inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

bool CWeapon::ready_to_kill	() const
{
	return					(
		!IsMisfire() && 
		((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) && 
		GetAmmoElapsed()
	);
}

// Получить индекс текущих координат худа
u8 CWeapon::GetCurrentHudOffsetIdx() const
{
	const bool b_aiming = ((IsZoomed() && m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_fZoomRotationFactor > 0.f));

	if (b_aiming)
	{
		const bool has_gl = GrenadeLauncherAttachable() && IsGrenadeLauncherAttached();
		const bool has_scope = ScopeAttachable() && IsScopeAttached();

		if(IsSecondScopeMode())
			return hud_item_measures::m_hands_offset_type_aim_second;

		if (IsGrenadeMode())
		{
			if (m_bUseScopeGrenadeZoom && has_scope)
				return hud_item_measures::m_hands_offset_type_gl_scope;
			else
				return hud_item_measures::m_hands_offset_type_gl;
		}
		else if (has_gl)
		{
			if (m_bUseScopeZoom && has_scope)
				return hud_item_measures::m_hands_offset_type_gl_normal_scope;
			else
				return hud_item_measures::m_hands_offset_type_aim_gl_normal;
		}
		else
		{
			if (m_bUseScopeZoom && has_scope)
				return hud_item_measures::m_hands_offset_type_aim_scope;
			else
				return hud_item_measures::m_hands_offset_type_aim;
		}
	}

	return hud_item_measures::m_hands_offset_type_normal;
}

void	CWeapon::SetAmmoElapsed	(int ammo_count)
{
	iAmmoElapsed				= ammo_count;

	u32 uAmmo					= u32(iAmmoElapsed);

	if (uAmmo != m_magazine.size())
	{
		if (uAmmo > m_magazine.size())
		{
			CCartridge			l_cartridge; 
			l_cartridge.Load	(*m_ammoTypes[m_ammoType], u8(m_ammoType));
			while (uAmmo > m_magazine.size())
				m_magazine.push_back(l_cartridge);
		}
		else
		{
			while (uAmmo < m_magazine.size())
				m_magazine.pop_back();
		};
	};
}

u32	CWeapon::ef_main_weapon_type	() const
{
	VERIFY	(m_ef_main_weapon_type != u32(-1));
	return	(m_ef_main_weapon_type);
}

u32	CWeapon::ef_weapon_type	() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

bool CWeapon::IsNecessaryItem	    (const shared_str& item_sect)
{
	return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() );
}

void CWeapon::modify_holder_params		(float &range, float &fov) const
{
	if (!IsScopeAttached()) {
		inherited::modify_holder_params	(range,fov);
		return;
	}
	range	*= m_addon_holder_range_modifier;
	fov		*= m_addon_holder_fov_modifier;
}

void CWeapon::OnDrawUI()
{
	if(IsZoomed() && ZoomHideCrosshair()){
		if(ZoomTexture() && !IsRotatingToZoom()){
			ZoomTexture()->SetPos	(0,0);
			ZoomTexture()->SetRect	(0,0,UI_BASE_WIDTH, UI_BASE_HEIGHT);
			ZoomTexture()->Render	();

			if (m_bRangeMeter && !IsSecondScopeMode()) {
				CGameFont* F = HUD().Font().pFontGraffiti19Russian;
				F->SetAligment(CGameFont::alCenter);
				F->OutSetI(m_vRangeMeterOffset.x, m_vRangeMeterOffset.y);
				float range = HUD().GetCurrentRayQuery().range;
				F->SetColor(m_uRangeMeterColor);
				F->OutNext("%4.1f %s", range, CStringTable().translate("st_m").c_str());
			}
//			m_UILens.Draw();
		}
	}
}

bool CWeapon::IsHudModeNow()
{ 
	return (HudItemData() != nullptr);
}

bool CWeapon::unlimited_ammo() 
{ 
	if (m_pCurrentInventory)
		return inventory_owner().unlimited_ammo() && m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited);
	else
		return false;
};

LPCSTR	CWeapon::GetCurrentAmmo_ShortName	()
{
	if (m_magazine.empty()) return ("");
	CCartridge &l_cartridge = m_magazine.back();
	return *(l_cartridge.m_InvShortName);
}

float CWeapon::GetAmmoInMagazineWeight(const decltype(CWeapon::m_magazine)& mag) const
{
    float res = 0;
    const char* last_type = nullptr;
    float last_ammo_weight = 0;
    for (auto& c : mag){
        // Usually ammos in mag have same type, use this fact to improve performance
        if (last_type != c.m_ammoSect.c_str()){
            last_type = c.m_ammoSect.c_str();
            last_ammo_weight = c.Weight();
        }
        res += last_ammo_weight;
    }
    return res;
}

float CWeapon::Weight() const
{
	float res = CInventoryItemObject::Weight();
	if ( GrenadeLauncherAttachable() && IsGrenadeLauncherAttached() )
		res += pSettings->r_float(GetGrenadeLauncherName(),"inv_weight");
	if ( ScopeAttachable() && IsScopeAttached() )
		res += pSettings->r_float(GetScopeName(),"inv_weight");
	if ( SilencerAttachable() && IsSilencerAttached() )
		res += pSettings->r_float(GetSilencerName(),"inv_weight");
	res += GetAmmoInMagazineWeight(m_magazine);

	return res;
}

u32 CWeapon::Cost() const
{
	u32 res = m_cost;
	
	if (Core.Features.test(xrCore::Feature::wpn_cost_include_addons)) {
		if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached())
			res += pSettings->r_u32(GetGrenadeLauncherName(), "cost");
		if (ScopeAttachable() && IsScopeAttached())
			res += pSettings->r_u32(GetScopeName(), "cost");
		if (SilencerAttachable() && IsSilencerAttached())
			res += pSettings->r_u32(GetSilencerName(), "cost");
	}
	return res;
}

float CWeapon::Volume() const
{
	float res = m_volume;

	if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached())
		res += READ_IF_EXISTS(pSettings, r_float, GetGrenadeLauncherName(), "inv_volume", .0f);
	if (ScopeAttachable() && IsScopeAttached())
		res += READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "inv_volume", .0f);
	if (SilencerAttachable() && IsSilencerAttached())
		res += res += READ_IF_EXISTS(pSettings, r_float, GetSilencerName(), "inv_volume", .0f);

	return res;
}

void CWeapon::Hide(bool now)
{
	if (now)
	{
		OnStateSwitch(eHidden, GetState());
		SetState(eHidden);
		StopHUDSounds();
	}
	else
		SwitchState(eHiding);

	OnZoomOut();
}

void CWeapon::Show(bool now)
{
	if (now)
	{
		StopCurrentAnimWithoutCallback();
		OnStateSwitch(eIdle, GetState());
		SetState(eIdle);
		StopHUDSounds();
	}
	else
		SwitchState(eShowing);
}

bool CWeapon::show_crosshair()
{
	return psActorFlags.test(AF_CROSSHAIR_DBG) || !(IsZoomed() && ZoomHideCrosshair());
}

bool CWeapon::show_indicators()
{
	return !(IsZoomed() && !IsRotatingToZoom() && (ZoomTexture() || !m_bScopeShowIndicators));
}

float CWeapon::GetConditionToShow	() const
{
	return	(GetCondition());//powf(GetCondition(),4.0f));
}

BOOL CWeapon::ParentMayHaveAimBullet	()
{
	CObject* O=H_Parent();
	if (!O) return FALSE;
	CEntityAlive* EA=smart_cast<CEntityAlive*>(O);
	return EA->cast_actor()!=0;
}

BOOL CWeapon::ParentIsActor	()
{
	CObject* O=H_Parent();
	if (!O) return FALSE;
	CEntityAlive* EA=smart_cast<CEntityAlive*>(O);
	if (!EA) return FALSE;
	return EA->cast_actor()!=0;
}

const float &CWeapon::hit_probability	() const
{
	VERIFY					((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster)); 
	return					(m_hit_probability[egdNovice]);
}

// Function for callbacks added by Cribbledirge.
void CWeapon::StateSwitchCallback(GameObject::ECallbackType actor_type, GameObject::ECallbackType npc_type)
{
	if (g_actor)
	{
		if (smart_cast<CActor*>(H_Parent()))  // This is an actor.
		{
			Actor()->callback(actor_type)(
				lua_game_object()  // The weapon as a game object.
			);
		}
		else if (smart_cast<CEntityAlive*>(H_Parent()))  // This is an NPC.
		{
			Actor()->callback(npc_type)(
				smart_cast<CEntityAlive*>(H_Parent())->lua_game_object(),       // The owner of the weapon.
				lua_game_object()                                              // The weapon itself.
			);
		}
	}
}

// Обновление необходимости включения второго вьюпорта +SecondVP+
// Вызывается только для активного оружия игрока
void CWeapon::UpdateSecondVP()
{
	// + CActor::UpdateCL();
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (!pActor)
		return;

	CInventoryOwner* inv_owner = pActor->cast_inventory_owner();

	bool b_is_active_item = inv_owner && (inv_owner->m_inventory->ActiveItem() == this);
	R_ASSERT(b_is_active_item); // Эта функция должна вызываться только для оружия в руках нашего игрока

	bool bCond_1 = m_fZoomRotationFactor > 0.05f;    // Мы должны целиться
	bool bCond_3 = pActor->cam_Active() == pActor->cam_FirstEye(); // Мы должны быть от 1-го лица

	Device.m_SecondViewport.SetSVPActive(bCond_1 && bCond_3 && SecondVPEnabled());
}

bool CWeapon::SecondVPEnabled() const
{	
	bool bCond_2 = m_fSecondVPZoomFactor > 0.0f;     // В конфиге должен быть прописан фактор зума (scope_lense_fov_factor) больше чем 0
	bool bCond_4 = !IsGrenadeMode();     // Мы не должны быть в режиме подствольника
	bool bcond_6 = psActorFlags.test(AF_3D_SCOPES);

	return bCond_2 && bCond_4 && bcond_6;
}

// Чувствительность мышкии с оружием в руках во время прицеливания
float CWeapon::GetControlInertionFactor() const
{
	float fInertionFactor = inherited::GetControlInertionFactor();

	if (IsZoomed() && SecondVPEnabled() && !IsRotatingToZoom())
	{
		if (m_bScopeDynamicZoom)
		{
			const float delta_factor_total = 1 - m_fSecondVPZoomFactor;
			float min_zoom_factor = 1 + delta_factor_total * m_fMinZoomK;
			float k = (m_fRTZoomFactor - min_zoom_factor) / (m_fSecondVPZoomFactor - min_zoom_factor);
			return (m_fScopeInertionFactor - fInertionFactor) * k + fInertionFactor;
		}
		else
			return m_fScopeInertionFactor;
	}

	return fInertionFactor;
}

float CWeapon::GetSecondVPFov() const
{
	float fov_factor = m_fSecondVPZoomFactor;
	if (m_bScopeDynamicZoom)
	{
		fov_factor = m_fRTZoomFactor;
	}
	return atanf(tanf(g_fov * (0.5f * PI / 180)) / fov_factor) / (0.5f * PI / 180);
}

// Получить HUD FOV от текущего оружия игрока
float CWeapon::GetHudFov()
{
	const float last_nw_hf = inherited::GetHudFov();

	if (m_fZoomRotationFactor > 0.0f)
	{
		if (SecondVPEnabled() && m_fSecondVPHudFov > 0.0f)
		{
			// В линзе зума
			const float fDiff = last_nw_hf - m_fSecondVPHudFov;
			return m_fSecondVPHudFov + (fDiff * (1 - m_fZoomRotationFactor));
		}
		if ((m_eScopeStatus == CSE_ALifeItemWeapon::eAddonDisabled || IsScopeAttached()) && !IsGrenadeMode() && m_fZoomHudFov > 0.0f)
		{
			// В процессе зума
			const float fDiff = last_nw_hf - m_fZoomHudFov;
			return m_fZoomHudFov + (fDiff * (1 - m_fZoomRotationFactor));
		}
	}

	return last_nw_hf;
}


void CWeapon::OnBulletHit() {
  if ( !fis_zero( conditionDecreasePerShotOnHit ) )
    ChangeCondition( -conditionDecreasePerShotOnHit );
}


bool CWeapon::IsPartlyReloading() {
//  return ( m_set_next_ammoType_on_reload == u32(-1) && GetAmmoElapsed() > 0 && !IsMisfire() );
	return (GetAmmoElapsed() > 0);
}
//
u32 CWeapon::GetNextAmmoType(bool looped)
{
	u32 l_newType = looped ? m_set_next_ammoType_on_reload : m_ammoType;

	for (;;)
	{
		if (++l_newType >= m_ammoTypes.size())
		{
			for (l_newType = 0; l_newType < m_ammoTypes.size(); ++l_newType)
				if (unlimited_ammo() || m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[l_newType].c_str(), ParentIsActor(), HasDetachableMagazine() && !IsSingleReloading()))
					break;
			break;
		}

		if (unlimited_ammo() || m_pCurrentInventory->GetAmmoByLimit(m_ammoTypes[l_newType].c_str(), ParentIsActor(), HasDetachableMagazine() && !IsSingleReloading()))
			break;
	}

	if (l_newType != (looped ? m_set_next_ammoType_on_reload : m_ammoType) && l_newType < m_ammoTypes.size())
		return l_newType;
	else
		return u32(-1);
}
//
int	CWeapon::GetScopeX()
{
	int res = pSettings->r_s32(cNameSect(), "scope_x");

	string1024 scope_sect_x;
	sprintf(scope_sect_x, "%s_x", m_scopes[m_cur_scope].c_str());

	if (pSettings->line_exist(cNameSect(), scope_sect_x))
		res = pSettings->r_s32(cNameSect(), scope_sect_x);

	return res;
}
int	CWeapon::GetScopeY()
{
	int res = pSettings->r_s32(cNameSect(), "scope_y");

	string1024 scope_sect_y;
	sprintf(scope_sect_y, "%s_y", m_scopes[m_cur_scope].c_str());

	if (pSettings->line_exist(cNameSect(), scope_sect_y))
		res = pSettings->r_s32(cNameSect(), scope_sect_y);

	return res;
}

int	CWeapon::GetSilencerX()
{
	int res = pSettings->r_s32(cNameSect(), "silencer_x");

	string1024 silencer_sect_x;
	sprintf(silencer_sect_x, "%s_x", m_silencers[m_cur_silencer].c_str());

	if (pSettings->line_exist(cNameSect(), silencer_sect_x))
		res = pSettings->r_s32(cNameSect(), silencer_sect_x);

	return res;
}
int	CWeapon::GetSilencerY()
{
	int res = pSettings->r_s32(cNameSect(), "silencer_y");

	string1024 silencer_sect_y;
	sprintf(silencer_sect_y, "%s_y", m_silencers[m_cur_silencer].c_str());

	if (pSettings->line_exist(cNameSect(), silencer_sect_y))
		res = pSettings->r_s32(cNameSect(), silencer_sect_y);

	return res;
}

int	CWeapon::GetGrenadeLauncherX()
{
	int res = pSettings->r_s32(cNameSect(), "grenade_launcher_x");

	string1024 glauncher_sect_x;
	sprintf(glauncher_sect_x, "%s_x", m_glaunchers[m_cur_glauncher].c_str());

	if (pSettings->line_exist(cNameSect(), glauncher_sect_x))
		res = pSettings->r_s32(cNameSect(), glauncher_sect_x);

	return res;
}
int	CWeapon::GetGrenadeLauncherY()
{
	int res = pSettings->r_s32(cNameSect(), "grenade_launcher_y");

	string1024 glauncher_sect_y;
	sprintf(glauncher_sect_y, "%s_y", m_glaunchers[m_cur_glauncher].c_str());

	if (pSettings->line_exist(cNameSect(), glauncher_sect_y))
		res = pSettings->r_s32(cNameSect(), glauncher_sect_y);

	return res;
}

int	CWeapon::GetLaserX()
{
	int res = pSettings->r_s32(cNameSect(), "laser_x");

	string1024 laser_sect_x;
	sprintf(laser_sect_x, "%s_x", m_lasers[m_cur_laser].c_str());

	if (pSettings->line_exist(cNameSect(), laser_sect_x))
		res = pSettings->r_s32(cNameSect(), laser_sect_x);

	return res;
}
int	CWeapon::GetLaserY()
{
	int res = pSettings->r_s32(cNameSect(), "laser_y");

	string1024 laser_sect_y;
	sprintf(laser_sect_y, "%s_x", m_lasers[m_cur_laser].c_str());

	if (pSettings->line_exist(cNameSect(), laser_sect_y))
		res = pSettings->r_s32(cNameSect(), laser_sect_y);

	return res;
}

int	CWeapon::GetFlashlightX()
{
	int res = pSettings->r_s32(cNameSect(), "flashlight_x");

	string1024 flashlight_sect_x;
	sprintf(flashlight_sect_x, "%s_x", m_flashlights[m_cur_flashlight].c_str());

	if (pSettings->line_exist(cNameSect(), flashlight_sect_x))
		res = pSettings->r_s32(cNameSect(), flashlight_sect_x);

	return res;
}
int	CWeapon::GetFlashlightY()
{
	int res = pSettings->r_s32(cNameSect(), "flashlight_y");

	string1024 flashlight_sect_y;
	sprintf(flashlight_sect_y, "%s_y", m_flashlights[m_cur_flashlight].c_str());

	if (pSettings->line_exist(cNameSect(), flashlight_sect_y))
		res = pSettings->r_s32(cNameSect(), flashlight_sect_y);

	return res;
}

float CWeapon::GetHitPowerForActor() const {
	return fvHitPower[g_SingleGameDifficulty];
}

bool CWeapon::IsLaserOn() const {
	return m_bIsLaserOn;
}

bool CWeapon::IsFlashlightOn() const {
	return m_bIsFlashlightOn;
}

void CWeapon::DirectReload(CWeaponAmmo* ammo) {
	m_bDirectReload = true;
	m_pAmmo = ammo;
	for (u32 i = 0; i < m_ammoTypes.size(); ++i) {
		if (m_ammoTypes[i] == ammo->cNameSect() && 
			TryToGetAmmo(i) &&
			(iAmmoElapsed < iMagazineSize || IsMisfire() || HasDetachableMagazine())) {
			m_ammoType = i;
			m_set_next_ammoType_on_reload = u32(-1);
			Reload();
			return;
		}
	}
	m_bDirectReload = false;
}

float CWeapon::GetCondDecPerShotToShow() const {
	float silencer_dec_k = IsSilencerAttached() && SilencerAttachable() ? conditionDecreasePerShotSilencer : 0.f;
	return (conditionDecreasePerShot + conditionDecreasePerShot * silencer_dec_k);
}

void CWeapon::SaveAttachableParams()
{
	const char* sect_name = cNameSect().c_str();
	string_path buff;
	FS.update_path(buff, "$logs$", make_string("_world\\%s.ltx", sect_name).c_str());

	CInifile pHudCfg(buff, FALSE, FALSE, TRUE);

	sprintf_s(buff, "%f,%f,%f", m_Offset.c.x, m_Offset.c.y, m_Offset.c.z);
	pHudCfg.w_string(sect_name, "position", buff);

	Fvector ypr;
	m_Offset.getHPB(ypr.x, ypr.y, ypr.z);
	ypr.mul(180.f / PI);
	sprintf_s(buff, "%f,%f,%f", ypr.x, ypr.y, ypr.z);
	pHudCfg.w_string(sect_name, "orientation", buff);

	if (pSettings->line_exist(sect_name, "strap_position") && pSettings->line_exist(sect_name, "strap_orientation"))
	{
		sprintf_s(buff, "%f,%f,%f", m_StrapOffset.c.x, m_StrapOffset.c.y, m_StrapOffset.c.z);
		pHudCfg.w_string(sect_name, "strap_position", buff);
		m_StrapOffset.getHPB(ypr.x, ypr.y, ypr.z);
		ypr.mul(180.f / PI);
		sprintf_s(buff, "%f,%f,%f", ypr.x, ypr.y, ypr.z);
		pHudCfg.w_string(sect_name, "strap_orientation", buff);
	}

	Msg("--[%s] data saved to [%s]", __FUNCTION__, pHudCfg.fname());
}

void CWeapon::ParseCurrentItem(CGameFont* F) {
	F->OutNext("WEAPON IN STRAPPED MODE: [%d]", m_strapped_mode);
}