#include "stdafx.h"
#include "torch.h"
#include "entity.h"
#include "actor.h"
#include "../xr_3da/LightAnimLibrary.h"
#include "PhysicsShell.h"
#include "xrserver_objects_alife_items.h"
#include "ai_sounds.h"

#include "HUDManager.h"
#include "level.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xr_3da/camerabase.h"
#include "inventory.h"
#include "game_base_space.h"

#include "UIGameCustom.h"
#include "actorEffector.h"
#include "CustomOutfit.h"

static const float		TIME_2_HIDE					= 5.f;
static const float		TORCH_INERTION_CLAMP		= PI_DIV_6;
static const float		TORCH_INERTION_SPEED_MAX	= 7.5f;
static const float		TORCH_INERTION_SPEED_MIN	= 0.5f;
static		 Fvector	TORCH_OFFSET				= {-0.2f,+0.1f,-0.3f};
static const Fvector	OMNI_OFFSET					= {-0.2f,+0.1f,-0.1f};
static const float		OPTIMIZATION_DISTANCE		= 100.f;

static bool stalker_use_dynamic_lights	= false;

ENGINE_API int g_current_renderer;

CTorch::CTorch(void) 
{
	light_render				= ::Render->light_create();
	light_render->set_type		(IRender_Light::SPOT);
	light_render->set_shadow	(true);
	light_omni					= ::Render->light_create();
	light_omni->set_type		(IRender_Light::POINT);
	light_omni->set_shadow		(false);

	m_switched_on				= false;
	glow_render					= ::Render->glow_create();
	lanim						= 0;
	time2hide					= 0;
	fBrightness					= 1.f;

	/*m_NightVisionRechargeTime	= 6.f;
	m_NightVisionRechargeTimeMin= 2.f;
	m_NightVisionDischargeTime	= 10.f;
	m_NightVisionChargeTime		= 0.f;*/

	m_prev_hp.set				(0,0);
	m_delta_h					= 0;

	// Disabling shift by x and z axes for 1st render, 
	// because we don't have dynamic lighting in it. 
	if (g_current_renderer == 1)
	{
		TORCH_OFFSET.x = 0;
		TORCH_OFFSET.z = 0;
	}

	SetSlot(ON_HEAD_SLOT);
}

CTorch::~CTorch(void) 
{
	light_render.destroy	();
	light_omni.destroy	();
	glow_render.destroy		();
	HUD_SOUND::DestroySound	(SndTorchOn);
	HUD_SOUND::DestroySound	(SndTorchOff);
}

inline bool CTorch::can_use_dynamic_lights	()
{
	if (!H_Parent())
		return				(true);

	CInventoryOwner			*owner = smart_cast<CInventoryOwner*>(H_Parent());
	if (!owner)
		return				(true);

	return					(owner->can_use_dynamic_lights());
}

void CTorch::Load(LPCSTR section) 
{
	inherited::Load			(section);

	m_light_descr_sect = READ_IF_EXISTS(pSettings, r_string, section, "light_definition", "torch_definition");

	LoadLightDefinitions(m_light_descr_sect);

	light_trace_bone = pSettings->r_string(section, "light_trace_bone");

	if (pSettings->line_exist(section, "snd_torch_on"))
		HUD_SOUND::LoadSound(section, "snd_torch_on", SndTorchOn, SOUND_TYPE_ITEM_USING);
	if (pSettings->line_exist(section, "snd_torch_off"))
		HUD_SOUND::LoadSound(section, "snd_torch_off", SndTorchOff, SOUND_TYPE_ITEM_USING);
}

void CTorch::Switch()
{
	if (OnClient()) return;
	bool bActive			= !m_switched_on;
	Switch					(bActive);
}

void CTorch::Switch	(bool light_on)
{
	bool was_switched_on = m_switched_on;

	m_switched_on			= light_on;
	if (can_use_dynamic_lights())
	{
		light_render->set_active(light_on);
		light_omni->set_active(light_on);
	}
	glow_render->set_active					(light_on);

	if (*light_trace_bone) 
	{
		IKinematics* pVisual				= smart_cast<IKinematics*>(Visual()); VERIFY(pVisual);
		u16 bi								= pVisual->LL_BoneID(light_trace_bone);

		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE);
		pVisual->CalculateBones				(TRUE);
	}

	auto pA = smart_cast<CActor*>(H_Parent());
	if (pA){
		bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());

		if(m_switched_on && !was_switched_on)
			HUD_SOUND::PlaySound(SndTorchOn, pA->Position(), pA, bPlaySoundFirstPerson);
		else if(!m_switched_on && was_switched_on)
			HUD_SOUND::PlaySound(SndTorchOff, pA->Position(), pA, bPlaySoundFirstPerson);
	}
}

bool CTorch::torch_active					() const
{
	return (m_switched_on);
}

BOOL CTorch::net_Spawn(CSE_Abstract* DC) 
{
	auto torch = smart_cast<CSE_ALifeItemTorch*>(DC);
	R_ASSERT				(torch);
	cNameVisual_set			(torch->get_visual());

	R_ASSERT				(!CFORM());
	R_ASSERT				(smart_cast<IKinematics*>(Visual()));
	collidable.model		= xr_new<CCF_Skeleton>	(this);

	if (!inherited::net_Spawn(DC))
		return				(FALSE);

	//включить/выключить фонарик
	Switch(torch->m_active);
	VERIFY(!torch->m_active || (torch->ID_Parent != 0xffff));

	return					(TRUE);
}

void CTorch::net_Destroy() 
{
	Switch					(false);
	inherited::net_Destroy	();
}

void CTorch::OnH_A_Chield() 
{
	inherited::OnH_A_Chield			();
	m_focus.set						(Position());
	if (smart_cast<CActor*>(H_Parent()) && useVolumetric)
		light_render->set_volumetric(useVolumetricForActor);
}

void CTorch::OnH_B_Independent	(bool just_before_destroy) 
{
	inherited::OnH_B_Independent	(just_before_destroy);
	time2hide						= TIME_2_HIDE;

	Switch						(false);

	HUD_SOUND::StopSound		(SndTorchOn);
	HUD_SOUND::StopSound		(SndTorchOff);
}

void CTorch::UpdateCL() 
{
	inherited::UpdateCL			();
	
	if (!m_switched_on)			return;

	CBoneInstance			&BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);
	Fmatrix					M;

	if (H_Parent()) 
	{
		CActor*			actor = smart_cast<CActor*>(H_Parent());
		if (actor)
		{
			smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones_Invalidate();
#pragma todo("KRodin: переделать под новый рендер!")
			//light_render->set_actor_torch(true);
		}

		if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition)<_sqr(OPTIMIZATION_DISTANCE)) {
			// near camera
			smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones	();
			M.mul_43				(XFORM(),BI.mTransform);
		} else {
			// approximately the same
			M		= H_Parent()->XFORM		();
			H_Parent()->Center				(M.c);
			M.c.y	+= H_Parent()->Radius	()*2.f/3.f;
		}

		if (actor) 
		{
			if (actor->active_cam() == eacLookAt)
			{
				m_prev_hp.x = angle_inertion_var(m_prev_hp.x, -actor->cam_Active()->yaw, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
				m_prev_hp.y = angle_inertion_var(m_prev_hp.y, -actor->cam_Active()->pitch, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
			}
			else
			{
				m_prev_hp.x = angle_inertion_var(m_prev_hp.x, -actor->cam_FirstEye()->yaw, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
				m_prev_hp.y = angle_inertion_var(m_prev_hp.y, -actor->cam_FirstEye()->pitch, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
			}

			Fvector			dir,right,up;	
			dir.setHP		(m_prev_hp.x+m_delta_h,m_prev_hp.y);
			Fvector::generate_orthonormal_basis_normalized(dir,up,right);


			if (true)
			{
				Fvector offset				= M.c; 
				offset.mad					(M.i,TORCH_OFFSET.x);
				offset.mad					(M.j,TORCH_OFFSET.y);
				offset.mad					(M.k,TORCH_OFFSET.z);
				light_render->set_position	(offset);

				if(true /*false*/)
				{
					offset						= M.c; 
					offset.mad					(M.i,OMNI_OFFSET.x);
					offset.mad					(M.j,OMNI_OFFSET.y);
					offset.mad					(M.k,OMNI_OFFSET.z);
					light_omni->set_position	(offset);
				}
			}//if (true)
			glow_render->set_position	(M.c);

			if (true)
			{
				light_render->set_rotation	(dir, right);
				
				if(true /*false*/)
				{
					light_omni->set_rotation	(dir, right);
				}
			}//if (true)
			glow_render->set_direction	(dir);

		}// if(actor)
		else 
		{
			if (can_use_dynamic_lights()) 
			{
				light_render->set_position	(M.c);
				light_render->set_rotation	(M.k,M.i);

				Fvector offset				= M.c; 
				offset.mad					(M.i,OMNI_OFFSET.x);
				offset.mad					(M.j,OMNI_OFFSET.y);
				offset.mad					(M.k,OMNI_OFFSET.z);
				light_omni->set_position	(M.c);
				light_omni->set_rotation	(M.k,M.i);
			}//if (can_use_dynamic_lights()) 

			glow_render->set_position	(M.c);
			glow_render->set_direction	(M.k);
		}
	}//if(HParent())
	else 
	{
#pragma todo("KRodin: переделать под новый рендер!")
		//light_render->set_actor_torch(false);
		if (getVisible() && m_pPhysicsShell) 
		{
			M.mul						(XFORM(),BI.mTransform);

			//. what should we do in case when 
			// light_render is not active at this moment,
			// but m_switched_on is true?
//			light_render->set_rotation	(M.k,M.i);
//			light_render->set_position	(M.c);
//			glow_render->set_position	(M.c);
//			glow_render->set_direction	(M.k);
//
//			time2hide					-= Device.fTimeDelta;
//			if (time2hide<0)
			{
				m_switched_on			= false;
				light_render->set_active(false);
				light_omni->set_active(false);
				glow_render->set_active	(false);
			}
		}//if (getVisible() && m_pPhysicsShell)  
	}

	if (!m_switched_on)					return;

	// calc color animator
	if (!lanim)							return;

	int						frame;
	// возвращает в формате BGR
	u32 clr					= lanim->CalculateBGR(Device.fTimeGlobal,frame); 

	Fcolor					fclr;
	fclr.set( (float)color_get_B( clr ) / 255.f, (float)color_get_G( clr ) / 255.f, (float)color_get_R( clr ) / 255.f, 1.f );
	fclr.mul_rgb( fBrightness );
	if (can_use_dynamic_lights())
	{
		light_render->set_color	(fclr);
		light_omni->set_color	(fclr);
	}
	glow_render->set_color		(fclr);
}


void CTorch::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CTorch::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CTorch::setup_physic_shell	()
{
	CPhysicsShellHolder::setup_physic_shell();
}

void CTorch::net_Export( CSE_Abstract* E ) {
  inherited::net_Export( E );
  CSE_ALifeItemTorch* torch = smart_cast<CSE_ALifeItemTorch*>( E );
  torch->m_active = m_switched_on;
  const CActor *pA = smart_cast<const CActor*>( H_Parent() );
  torch->m_attached = ( pA && pA->attached( this ) );
}

bool  CTorch::can_be_attached		() const{
	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	return pA ? (pA->GetTorch() == this) : true;
}

void CTorch::afterDetach			()
{
	inherited::afterDetach	();
	Switch					(false);
}
void CTorch::renderable_Render()
{
	inherited::renderable_Render();
}

void CTorch::calc_m_delta_h( float range ) {
  m_delta_h = PI_DIV_2 - atan( ( range * 0.5f ) / _abs( TORCH_OFFSET.x ) );
}

float CTorch::get_range() const {
  return light_render->get_range();
}

void CTorch::LoadLightDefinitions(shared_str light_sect) {

	bool b_r2 = !!psDeviceFlags.test(rsR2);
	b_r2 |= !!psDeviceFlags.test(rsR3);
	b_r2 |= !!psDeviceFlags.test(rsR4);

	IKinematics* K = smart_cast<IKinematics*>(Visual());
	CInifile* pUserData = K->LL_UserData();
	R_ASSERT3(pUserData, "Empty Torch user data!"/*, torch->get_visual()*/);
	lanim = LALib.FindItem(pUserData->r_string(light_sect, "color_animator"));
	guid_bone = K->LL_BoneID(pUserData->r_string(light_sect, "guide_bone"));	VERIFY(guid_bone != BI_NONE);

	m_color = pUserData->r_fcolor(light_sect, b_r2 ? "color_r2" : "color");
	fBrightness = m_color.intensity();
	float range = pUserData->r_float(light_sect, (b_r2) ? "range_r2" : "range");
	light_render->set_color(m_color);
	light_render->set_range(range);

	if (b_r2)
	{
		useVolumetric = READ_IF_EXISTS(pUserData, r_bool, light_sect, "volumetric_enabled", false);
		useVolumetricForActor = READ_IF_EXISTS(pUserData, r_bool, light_sect, "volumetric_for_actor", false);
		light_render->set_volumetric(useVolumetric);
		if (useVolumetric)
		{
			float volQuality = READ_IF_EXISTS(pUserData, r_float, light_sect, "volumetric_quality", 1.0f);
			volQuality = std::clamp(volQuality, 0.f, 1.f);
			light_render->set_volumetric_quality(volQuality);

			float volIntensity = READ_IF_EXISTS(pUserData, r_float, light_sect, "volumetric_intensity", 0.15f);
			volIntensity = std::clamp(volIntensity, 0.f, 10.f);
			light_render->set_volumetric_intensity(volIntensity);

			float volDistance = READ_IF_EXISTS(pUserData, r_float, light_sect, "volumetric_distance", 0.45f);
			volDistance = std::clamp(volDistance, 0.f, 1.f);
			light_render->set_volumetric_distance(volDistance);
		}
	}

	Fcolor clr_o = pUserData->r_fcolor(light_sect, (b_r2) ? "omni_color_r2" : "omni_color");
	float range_o = pUserData->r_float(light_sect, (b_r2) ? "omni_range_r2" : "omni_range");
	light_omni->set_color(clr_o);
	light_omni->set_range(range_o);

	light_render->set_cone(deg2rad(pUserData->r_float(light_sect, "spot_angle")));
	light_render->set_texture(pUserData->r_string(light_sect, "spot_texture"));

	glow_render->set_texture(pUserData->r_string(light_sect, "glow_texture"));
	glow_render->set_color(m_color);
	glow_render->set_radius(pUserData->r_float(light_sect, "glow_radius"));

	calc_m_delta_h(range);
}