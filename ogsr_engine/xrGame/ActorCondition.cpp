#include "stdafx.h"
#include "actorcondition.h"
#include "actor.h"
#include "actorEffector.h"
#include "inventory.h"
#include "level.h"
#include "sleepeffector.h"
#include "game_base_space.h"
#include "xrserver.h"
#include "ai_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "object_broker.h"
#include "weapon.h"
#include "PDA.h"
#include "ai/monsters/BaseMonster/base_monster.h"

#define MAX_SATIETY					1.0f
#define START_SATIETY				0.5f

BOOL	GodMode	()	
{ 
	return psActorFlags.test(AF_GODMODE); 
}

CActorCondition::CActorCondition(CActor *object) :
	inherited	(object)
{
	m_fJumpPower				= 0.f;
	m_fStandPower				= 0.f;
	m_fWalkPower				= 0.f;
	m_fJumpWeightPower			= 0.f;
	m_fWalkWeightPower			= 0.f;
	m_fOverweightWalkK			= 0.f;
	m_fOverweightJumpK			= 0.f;
	m_fAccelK					= 0.f;
	m_fSprintK					= 0.f;
	m_fAlcohol					= 0.f;
	m_fSatiety					= 1.0f;
	m_fThirst					= 1.0f;

	m_bJumpRequirePower			= false;

	VERIFY						(object);
	m_object					= object;
	m_condition_flags.zero		();

	m_f_time_affected = Device.fTimeGlobal;

	monsters_feel_touch  = xr_new<Feel::Touch>();
	monsters_aura_radius = 0.f;

	m_bFlagState = !!psActorFlags.test(AF_SURVIVAL);
}

CActorCondition::~CActorCondition(void)
{
	xr_delete( monsters_feel_touch );
}

void CActorCondition::LoadCondition(LPCSTR entity_section)
{
	inherited::LoadCondition(entity_section);

	LPCSTR						section = READ_IF_EXISTS(pSettings,r_string,entity_section,"condition_sect",entity_section);

	m_fJumpPower				= pSettings->r_float(section,"jump_power");
	m_fStandPower				= pSettings->r_float(section,"stand_power");
	m_fWalkPower				= pSettings->r_float(section,"walk_power");
	m_fJumpWeightPower			= pSettings->r_float(section,"jump_weight_power");
	m_fWalkWeightPower			= pSettings->r_float(section,"walk_weight_power");
	m_fOverweightWalkK			= pSettings->r_float(section,"overweight_walk_k");
	m_fOverweightJumpK			= pSettings->r_float(section,"overweight_jump_k");
	m_fAccelK					= pSettings->r_float(section,"accel_k");
	m_fSprintK					= pSettings->r_float(section,"sprint_k");

	m_bJumpRequirePower			= READ_IF_EXISTS(pSettings, r_bool, section, "jump_require_power", false);

	//порог силы и здоровья меньше которого актер начинает хромать
	m_fLimpingHealthBegin		= pSettings->r_float(section,	"limping_health_begin");
	m_fLimpingHealthEnd			= pSettings->r_float(section,	"limping_health_end");
	R_ASSERT					(m_fLimpingHealthBegin<=m_fLimpingHealthEnd);

	m_fLimpingPowerBegin		= pSettings->r_float(section,	"limping_power_begin");
	m_fLimpingPowerEnd			= pSettings->r_float(section,	"limping_power_end");
	R_ASSERT					(m_fLimpingPowerBegin<=m_fLimpingPowerEnd);

	m_fCantWalkPowerBegin		= pSettings->r_float(section,	"cant_walk_power_begin");
	m_fCantWalkPowerEnd			= pSettings->r_float(section,	"cant_walk_power_end");
	R_ASSERT					(m_fCantWalkPowerBegin<=m_fCantWalkPowerEnd);

	m_fCantSprintPowerBegin		= pSettings->r_float(section,	"cant_sprint_power_begin");
	m_fCantSprintPowerEnd		= pSettings->r_float(section,	"cant_sprint_power_end");
	R_ASSERT					(m_fCantSprintPowerBegin<=m_fCantSprintPowerEnd);

	m_fPowerLeakSpeed			= pSettings->r_float(section,"max_power_leak_speed");
	
	m_fV_Alcohol				= pSettings->r_float(section,"alcohol_v");
	m_fV_Power = READ_IF_EXISTS(pSettings, r_float, section, "power_v", 0.0f);

	m_fV_Satiety				= pSettings->r_float(section,"satiety_v");		
	m_fV_SatietyPower			= pSettings->r_float(section,"satiety_power_v");
	m_fV_SatietyHealth			= pSettings->r_float(section,"satiety_health_v");

	m_fSatietyLightLimit = READ_IF_EXISTS(pSettings, r_float, section, "satiety_light_limit", 0.0f);
	clamp(m_fSatietyLightLimit, 0.0f, 1.0f);

	m_fSatietyCriticalLimit = READ_IF_EXISTS(pSettings, r_float, section, "satiety_critical_limit", 0.0f);
	clamp(m_fSatietyCriticalLimit, 0.0f, 1.0f);

	if (m_fSatietyCriticalLimit > m_fSatietyLightLimit)
	{
		m_fSatietyCriticalLimit = m_fSatietyLightLimit;
	}

	if (Core.Features.test(xrCore::Feature::actor_thirst))
	{
		m_fThirstLightLimit = READ_IF_EXISTS(pSettings, r_float, section, "thirst_light_limit", 0.0f);
		clamp(m_fThirstLightLimit, 0.0f, 1.0f);

		m_fThirstCriticalLimit = READ_IF_EXISTS(pSettings, r_float, section, "thirst_critical_limit", 0.0f);
		clamp(m_fThirstCriticalLimit, 0.0f, 1.0f);

		if (m_fThirstCriticalLimit > m_fThirstLightLimit)
		{
			m_fThirstCriticalLimit = m_fThirstLightLimit;
		}

		m_fV_Thirst = pSettings->r_float(section, "thirst_v");
		m_fV_ThirstPower = pSettings->r_float(section, "thirst_power_v");
		m_fV_ThirstHealth = pSettings->r_float(section, "thirst_health_v");
	}
	
	//m_MaxWalkWeight					= pSettings->r_float(section,"max_walk_weight");

	m_fBleedingPowerDecrease		= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "bleeding_power_dec", 0.f);
	//
	m_fMinPowerWalkJump				= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "min_power_walk_jump", 1.0f);
	//
	m_fMinHealthRadiation			= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "min_health_radiation", 1.0f);
	m_fMinHealthRadiationTreshold	= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "min_health_radiation_treshold", 0.f);
	//
	m_fAlcoholSatietyIntens			= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "satiety_to_alcohol_effector_intensity", 1.0f);
	//
	m_fExerciseStressFactor			= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "exercise_stress_factor", 1.0f);
	//
	m_fZoomEffectorK				= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "power_to_zoom_effector_k", 10.0f);
	//
	m_fV_HardHoldPower				= READ_IF_EXISTS(pSettings, r_float, section, "hard_hold_power_v", 0.f);
}


//вычисление параметров с ходом времени
#include "UI.h"
#include "HUDManager.h"
#include "CharacterPhysicsSupport.h"
void CActorCondition::UpdateCondition()
{
	if (GodMode())				return;
	if (!object().g_Alive())	return;
	if (!object().Local() && m_object != Level().CurrentViewEntity())		return;	
	//
	if (IsCantWalk() && object().character_physics_support()->movement()->PHCapture())
		object().character_physics_support()->movement()->PHReleaseObject();
	//
	float weight		= object().GetCarryWeight();
	float max_weight	= object().MaxCarryWeight();
	//if (Core.Features.test(xrCore::Feature::condition_jump_weight_mod))
	//	max_weight = object().inventory().GetMaxWeight() + object().ArtefactsAddWeight(false);
	//else
	//max_weight = object().MaxCarryWeight();

	float weight_coef = weight / max_weight;

	if ((object().mstate_real&mcAnyMove)) {
		ConditionWalk(weight_coef, isActorAccelerated(object().mstate_real,object().IsZoomAimingMode()), (object().mstate_real&mcSprint) != 0);
	}
	else {
		ConditionStand(weight_coef);
	}
	
	UpdateAlcohol				();
	UpdateSatiety				();

	if (Core.Features.test(xrCore::Feature::actor_thirst))
		UpdateThirst();

	inherited::UpdateCondition	();

	UpdateTutorialThresholds();

	AffectDamage_InjuriousMaterialAndMonstersInfluence();
}

void CActorCondition::AffectDamage_InjuriousMaterialAndMonstersInfluence()
{
	float one = 0.1f;
	float tg  = Device.fTimeGlobal;
	if ( m_f_time_affected + one > tg )
	{
		return;
	}

	clamp( m_f_time_affected, tg - (one * 3), tg );

	float psy_influence					=	0;
	float fire_influence				=	0;
	float radiation_influence			=	GetInjuriousMaterialDamage(); // Get Radiation from Material

	// Add Radiation and Psy Level from Monsters
	monsters_feel_touch->feel_touch_update(
	  object().Position(), monsters_aura_radius,
	  {},
	  [&]( const auto O ) -> bool {
	    const auto monster = smart_cast<CBaseMonster*>( O );
	    return monster ? true : false;
	  }
	);
	for ( const auto& it : monsters_feel_touch->feel_touch ) {
	  const auto monster = smart_cast<CBaseMonster*>( it );
	  if ( !monster ) continue;
	  psy_influence       += monster->get_psy_influence();
	  radiation_influence += monster->get_radiation_influence();
	  fire_influence      += monster->get_fire_influence();
	}
/*
	CPda* const pda						=	m_object->GetPDA();

	if ( pda )
	{
		using monsters = xr_vector<CObject*>;

		for ( monsters::const_iterator	it	=	pda->feel_touch.begin();
										it	!=	pda->feel_touch.end();
										++it )
		{
			CBaseMonster* const	monster		=	smart_cast<CBaseMonster*>(*it);
			if ( !monster || !monster->g_Alive() ) continue;

			psy_influence					+=	monster->get_psy_influence();
			radiation_influence				+=	monster->get_radiation_influence();
			fire_influence					+=	monster->get_fire_influence();
		}
	}
*/

	struct 
	{
		ALife::EHitType	type;
		float			value;

	} hits[]		=	{	{ ALife::eHitTypeRadiation, radiation_influence	*	one },
							{ ALife::eHitTypeTelepatic, psy_influence		*	one }, 
							{ ALife::eHitTypeBurn,		fire_influence		*	one }	};

 	NET_Packet	np;

	while ( m_f_time_affected + one < tg )
	{
		m_f_time_affected			+=	one;

		for (auto & hit : hits)
		{
			float			damage	=	hit.value;
			ALife::EHitType	type	=	hit.type;

			if ( damage > EPS )
			{
				SHit HDS = SHit(damage, 
//.								0.0f, 
								Fvector().set(0,1,0), 
								nullptr, 
								BI_NONE, 
								Fvector().set(0,0,0), 
								0.0f, 
								type, 
								0.0f, 
								false);

				HDS.GenHeader(GE_HIT, m_object->ID());
				HDS.Write_Packet( np );
				CGameObject::u_EventSend( np );
			}

		} // for

	}//while
}

#include "characterphysicssupport.h"
float CActorCondition::GetInjuriousMaterialDamage()
{
/*
	u16 mat_injurios = m_object->character_physics_support()->movement()->injurious_material_idx();

	if(mat_injurios!=GAMEMTL_NONE_IDX)
	{
		const SGameMtl* mtl		= GMLib.GetMaterialByIdx(mat_injurios);
		return					mtl->fInjuriousSpeed;
	}else
*/
		return 0.0f;
}

void CActorCondition::UpdateSatiety()
{
	if (m_fSatiety > 0)
	{
		m_fSatiety -= m_fV_Satiety * m_fDeltaTime;
		clamp(m_fSatiety, 0.0f, 1.0f);
	}

	float satiety_health_koef = 1;
	float satiety_power_koef = 1;

	if (m_fSatietyLightLimit > 0) {
		if (m_fSatiety < m_fSatietyLightLimit) {
			satiety_power_koef = m_fSatiety / m_fSatietyLightLimit;

			const float critical_k = m_fSatietyCriticalLimit / m_fSatietyLightLimit;
			satiety_health_koef = (m_fSatiety / m_fSatietyLightLimit - critical_k) / (m_fSatiety >= m_fSatietyCriticalLimit ? 1 - critical_k : critical_k);
		}
	}
	else {
		if (fis_zero(m_fSatiety))
		{
			satiety_health_koef = -1;
		}
	}

	if (m_bIsBleeding && satiety_health_koef > 0)
	{
		satiety_health_koef = 0;
	}

	m_fDeltaHealth += m_fV_SatietyHealth * satiety_health_koef * m_fDeltaTime * GetRegenKoef();
	m_fDeltaPower += m_fV_SatietyPower * satiety_power_koef * m_fDeltaTime;
}

void CActorCondition::UpdateThirst()
{
	if (m_fThirst > 0)
	{
		m_fThirst -= m_fV_Thirst * m_fDeltaTime;
		clamp(m_fThirst, 0.0f, 1.0f);
	}

	float thirst_health_koef = 1;
	float thirst_power_koef = 0;

	if (m_fThirstLightLimit > 0) {
		if (m_fThirst < m_fThirstLightLimit) {
			thirst_power_koef = (1 - m_fThirst / m_fThirstLightLimit) * -1;

			const float critical_k = m_fThirstCriticalLimit / m_fThirstLightLimit;
			thirst_health_koef = (m_fThirst / m_fThirstLightLimit - critical_k) / (m_fThirst >= m_fThirstCriticalLimit ? 1 - critical_k : critical_k);
		}
	}
	else {
		if (fis_zero(m_fThirst))
		{
			thirst_health_koef = -1;
		}
	}

	if (m_bIsBleeding && thirst_health_koef > 0)
	{
		thirst_health_koef = 0;
	}

	m_fDeltaHealth += m_fV_ThirstHealth * thirst_health_koef * m_fDeltaTime;
	m_fDeltaPower += m_fV_ThirstPower * thirst_power_koef * m_fDeltaTime;
}


CWound* CActorCondition::ConditionHit(SHit* pHDS)
{
	if (GodMode()) return NULL;

	if (pHDS->type() == ALife::eHitTypeTelepatic && psActorFlags.test(AF_SURVIVAL))
		pHDS->power *= (1.0f - GetAlcohol());

	return inherited::ConditionHit(pHDS);
}

void CActorCondition::PowerHit(float power, bool apply_outfit)
{
	m_fPower			-=	apply_outfit ? HitPowerEffect(power) : power;
	clamp					(m_fPower, 0.f, 1.f);
}

//weight - "удельный" вес от 0..1
void CActorCondition::ConditionJump(float weight)
{
	float power			=	m_fJumpPower;
	power				+=	m_fJumpWeightPower*weight*(weight>1.f?m_fOverweightJumpK:1.f);
	m_fPower			-=	HitPowerEffect(power);
}
void CActorCondition::ConditionWalk(float weight, bool accel, bool sprint)
{	
	float power			=	m_fWalkPower;
	power				+=	m_fWalkWeightPower*weight*(weight>1.f?m_fOverweightWalkK:1.f);
	power				*=	m_fDeltaTime*(accel?(sprint?m_fSprintK:m_fAccelK):1.f);
	m_fPower			-=	HitPowerEffect(power);
}

void CActorCondition::ConditionStand(float weight)
{	
	float power			= m_fStandPower * GetRegenKoef();
	power				*= m_fDeltaTime;
	m_fPower			+= power;
}


bool CActorCondition::IsCantWalk()
{
	if(m_fPower< m_fCantWalkPowerBegin)
		m_condition_flags.set(eCantWalk, TRUE);
	else if(m_fPower > m_fCantWalkPowerEnd)
		m_condition_flags.set(eCantWalk, FALSE);
	return m_condition_flags.test(eCantWalk);
}

#include "CustomOutfit.h"

bool CActorCondition::IsCantWalkWeight()
{
	if(!GodMode() && !psActorFlags.test(AF_SMOOTH_OVERWEIGHT))
	{
		if( object().GetCarryWeight() > object().MaxCarryWeight())
			return true;
	}
	return false;
}

bool CActorCondition::IsCantSprint()
{
	if (m_fPower < m_fCantSprintPowerBegin)
		m_condition_flags.set(eCantSprint, TRUE);
	else if (m_fPower > m_fCantSprintPowerEnd)
		m_condition_flags.set(eCantSprint, FALSE);
	return m_condition_flags.test(eCantSprint);
}

bool CActorCondition::IsCantJump(float weight)
{
	if (!m_bJumpRequirePower || GodMode())
	{
		return false;
	}

	float power = m_fJumpPower;
	power += m_fJumpWeightPower * weight*(weight > 1.f ? m_fOverweightJumpK : 1.f);
	return m_fPower < HitPowerEffect(power);
}

bool CActorCondition::IsLimping()
{
	if (m_fPower < m_fLimpingPowerBegin || GetHealth() < m_fLimpingHealthBegin)
		m_condition_flags.set(eLimping, TRUE);
	else if (m_fPower > m_fLimpingPowerEnd && GetHealth() > m_fLimpingHealthEnd)
		m_condition_flags.set(eLimping, FALSE);
	return m_condition_flags.test(eLimping);
}
extern bool g_bShowHudInfo;

void CActorCondition::save(NET_Packet &output_packet)
{
	inherited::save		(output_packet);
	save_data			(m_fAlcohol, output_packet);
	save_data			(m_condition_flags, output_packet);
	save_data			(m_fSatiety, output_packet);
	save_data			(m_fThirst, output_packet);
}

#include "alife_registry_wrappers.h"
#include "alife_simulator_header.h"

void CActorCondition::load(IReader &input_packet)
{
	inherited::load		(input_packet);
	load_data			(m_fAlcohol, input_packet);
	load_data			(m_condition_flags, input_packet);
	load_data			(m_fSatiety, input_packet);
	if (ai().get_alife()->header().version() > 8)
	{
		load_data(m_fThirst, input_packet);
	}
}

void CActorCondition::reinit	()
{
	inherited::reinit	();
	m_condition_flags.set(eLimping, FALSE);
	m_condition_flags.set(eCantWalk, FALSE);
	m_condition_flags.set(eCantSprint, FALSE);
	m_fSatiety = 1.f;
	m_fAlcohol = 0.f;
	m_fThirst = 1.f;
}

void CActorCondition::ChangeAlcohol	(float value)
{
	m_fAlcohol += value;
}

void CActorCondition::ChangeSatiety(float value)
{
	//влияние поглощённой дозы радиации на насыщение едой
	float radiation_k = value > 0 && psActorFlags.test(AF_SURVIVAL) ?
		(1.0f - GetRadiation()) : 1.0f;
	value *= radiation_k;

	m_fSatiety += value;
	clamp		(m_fSatiety, 0.0f, 1.0f);
}

void CActorCondition::ChangeThirst(float value)
{
	m_fThirst += value;
	clamp(m_fThirst, 0.0f, 1.0f);
}


void CActorCondition::UpdateTutorialThresholds()
{
	string256						cb_name;
	static float _cPowerThr = pSettings->r_float("tutorial_conditions_thresholds", "power");
	static float _cPowerMaxThr = pSettings->r_float("tutorial_conditions_thresholds", "max_power");
	static float _cBleeding = pSettings->r_float("tutorial_conditions_thresholds", "bleeding");
	static float _cSatiety = pSettings->r_float("tutorial_conditions_thresholds", "satiety");
	static float _cRadiation = pSettings->r_float("tutorial_conditions_thresholds", "radiation");
	static float _cWpnCondition = pSettings->r_float("tutorial_conditions_thresholds", "weapon_jammed");
	static float _cPsyHealthThr = pSettings->r_float("tutorial_conditions_thresholds", "psy_health");
	static float _cThirst = 0.0f;

	if (Core.Features.test(xrCore::Feature::actor_thirst))
	{
		_cThirst = pSettings->r_float("tutorial_conditions_thresholds", "thirst");
	}

	bool b = true;
	if (b && !m_condition_flags.test(eCriticalPowerReached) && GetPower() < _cPowerThr) {
		m_condition_flags.set(eCriticalPowerReached, TRUE);
		b = false;
		strcpy_s(cb_name, "_G.on_actor_critical_power");
	}

	if (b && !m_condition_flags.test(eCriticalMaxPowerReached) && GetMaxPower() < _cPowerMaxThr) {
		m_condition_flags.set(eCriticalMaxPowerReached, TRUE);
		b = false;
		strcpy_s(cb_name, "_G.on_actor_critical_max_power");
	}

	if (b && !m_condition_flags.test(eCriticalBleedingSpeed) && BleedingSpeed() > _cBleeding) {
		m_condition_flags.set(eCriticalBleedingSpeed, TRUE);
		b = false;
		strcpy_s(cb_name, "_G.on_actor_bleeding");
	}

	if (b && !m_condition_flags.test(eCriticalSatietyReached) && GetSatiety() < _cSatiety) {
		m_condition_flags.set(eCriticalSatietyReached, TRUE);
		b = false;
		strcpy_s(cb_name, "_G.on_actor_satiety");
	}

	if (Core.Features.test(xrCore::Feature::actor_thirst))
	{
		if (b && !m_condition_flags.test(eCriticalThirstReached) && GetThirst() < _cThirst) {
			m_condition_flags.set(eCriticalThirstReached, true);
			b = false;
			xr_strcpy(cb_name, "_G.on_actor_thirst");
		}
	}

	if (b && !m_condition_flags.test(eCriticalRadiationReached) && GetRadiation() > _cRadiation) {
		m_condition_flags.set(eCriticalRadiationReached, TRUE);
		b = false;
		strcpy_s(cb_name, "_G.on_actor_radiation");
	}

	if (b && !m_condition_flags.test(ePhyHealthMinReached) && GetPsyHealth() < _cPsyHealthThr) {
		m_condition_flags.set(ePhyHealthMinReached, TRUE);
		b = false;
		strcpy_s(cb_name, "_G.on_actor_psy");
	}

	if (b && !m_condition_flags.test(eCantWalkWeight) && IsCantWalkWeight()) {
		m_condition_flags.set(eCantWalkWeight, TRUE);
		b = false;
		strcpy_s(cb_name, "_G.on_actor_cant_walk_weight");
	}

	if (b && !m_condition_flags.test(eWeaponJammedReached) && m_object->inventory().GetActiveSlot() != NO_ACTIVE_SLOT) {
		PIItem item = m_object->inventory().ItemFromSlot(m_object->inventory().GetActiveSlot());
		CWeapon* pWeapon = smart_cast<CWeapon*>(item);
		if (pWeapon&&pWeapon->GetCondition() < _cWpnCondition) {
			m_condition_flags.set(eWeaponJammedReached, TRUE); b = false;
			strcpy_s(cb_name, "_G.on_actor_weapon_jammed");
		}
	}

	if (!b) {
		luabind::functor<LPCSTR>			fl;
		R_ASSERT(ai().script_engine().functor<LPCSTR>(cb_name, fl));
		fl();
	}
}

bool CActorCondition::DisableSprint(SHit* pHDS)
{
	return	(pHDS->hit_type != ALife::eHitTypeTelepatic)	&& 
			(pHDS->hit_type != ALife::eHitTypeChemicalBurn)	&&
			(pHDS->hit_type != ALife::eHitTypeBurn)			&&
			(pHDS->hit_type != ALife::eHitTypeRadiation)	;
}

float CActorCondition::HitSlowmo(SHit* pHDS)
{
	float ret;
	if(pHDS->hit_type==ALife::eHitTypeWound || pHDS->hit_type==ALife::eHitTypeStrike )
	{
		ret						= pHDS->damage();
		clamp					(ret,0.0f,1.f);
	}else
		ret						= 0.0f;

	return ret;	
}


void CActorCondition::net_Relcase( CObject* O ) {
  if ( Level().is_removing_objects() ) return;
  monsters_feel_touch->feel_touch_relcase( O );
}

float CActorCondition::GetPowerKoef() 
{ 
	return psActorFlags.test(AF_SURVIVAL) ? GetPower() : 1.0f; 
};

float CActorCondition::GetRegenKoef() 
{ 
	return psActorFlags.test(AF_SURVIVAL) ? (1.0f - GetRadiation()) * GetSatiety() : 1.0f; 
};

float CActorCondition::GetSmoothOwerweightKoef()
{
	float val = 1.0f;

	if (psActorFlags.test(AF_SMOOTH_OVERWEIGHT))
	{
		float power_k = m_fMinPowerWalkJump + (1.0f - m_fMinPowerWalkJump) * GetPower();				//коэф влияния выносливости

		float overweight_k = object().GetCarryWeight() > object().MaxCarryWeight() ?	//считаем коэф. только если есть перегруз
			object().MaxCarryWeight() / object().GetCarryWeight() :	//коэф влияния перегруза
			1.0f;

		val = power_k * overweight_k;
	}
	//Msg("SmoothOverweightK = %f", val);
	return val;
}

float CActorCondition::GetStress()
{
	float exercise_stress = psActorFlags.test(AF_SURVIVAL) && object().get_state() & (mcSprint | mcJump) ?
		m_fExerciseStressFactor :
		1.0f;

	float overweight_stress = psActorFlags.test(AF_SMOOTH_OVERWEIGHT) && object().GetCarryWeight() > object().MaxCarryWeight() ?
		object().GetCarryWeight() / object().MaxCarryWeight() :
		1.0f;

	/*if (object().get_state()&mcSprint) Msg("mcSprint!");
	else
		if (object().get_state()&mcJump) Msg("Jump!");
	Msg("overweight_stress = %f | exercise_stress = %f | m_fStressK = %f", overweight_stress, exercise_stress, m_fStressK);*/

	return overweight_stress * exercise_stress;
}

float CActorCondition::GetZoomEffectorKoef() 
{ 
	return m_fZoomEffectorK; 
};

void CActorCondition::UpdateHealth()
{
	float bleeding_speed = BleedingSpeed() * m_fDeltaTime * m_change_v.m_fV_Bleeding;
	m_bIsBleeding = fis_zero(bleeding_speed) ? false : true;
	m_fDeltaHealth -= CanBeHarmed() ? bleeding_speed : 0;

	m_fDeltaHealth += m_fDeltaTime * m_change_v.m_fV_HealthRestore * GetRegenKoef();
	VERIFY(_valid(m_fDeltaHealth));

	ChangeBleeding(m_change_v.m_fV_WoundIncarnation * m_fDeltaTime * GetRegenKoef());

	//радиация влияет на максимальное здоровье
	if (psActorFlags.test(AF_SURVIVAL) && m_fRadiation > m_fMinHealthRadiationTreshold) //защита от потенциального деления на 0 если m_fRadiationTreshold = 1
		object().SetMaxHealth(m_fMinHealthRadiation + (1.0f - m_fMinHealthRadiation) * (1.0f - m_fRadiation) / (1.0f - m_fMinHealthRadiationTreshold));
	else
		object().SetMaxHealth(1.0f);

	//debug
	/*Msg("Max_Health [%f]", GetMaxHealth());
	Msg("regen_koef [%f]", GetRegenKoef());
	Msg("stress [%f]", GetStress());
	Msg("bleeding_speed [%f]", BleedingSpeed());
	Msg("ChangeBleeding [%f]", m_change_v.m_fV_WoundIncarnation * m_fDeltaTime * GetRegenKoef());*/
}

void CActorCondition::UpdatePower()
{
	float k_max_power = 1.0f;

	if (true)
	{
		//float weight = object().inventory().TotalWeight();
		float weight = object().GetCarryWeight();

		float base_w = object().MaxCarryWeight();
		/*
		CCustomOutfit* outfit	= m_object->GetOutfit();
		if(outfit)
		base_w += outfit->m_additional_weight2;
		*/

		k_max_power = 1.0f + _min(weight, base_w) / base_w + _max(0.0f, (weight - base_w) / 10.0f);

	}
	else
		k_max_power = 1.0f;

	SetMaxPower(GetMaxPower() - m_fPowerLeakSpeed * m_fDeltaTime * k_max_power); //кажется это таки "сонливость" - постоянное уменьшение максимальной выносливости

	//коэффициенты уменьшения восстановления силы от сытоти и радиации
	/*float radiation_power_k = 1.f;
	float satiety_power_k = 1.f;*/
	float bleeding_power_dec = BleedingSpeed() * /*m_fDeltaTime * m_change_v.m_fV_Bleeding */ m_fBleedingPowerDecrease;

	m_fDeltaPower += m_fV_SatietyPower *
		//radiation_power_k*
		//satiety_power_k*
		m_fDeltaTime * GetRegenKoef() - bleeding_power_dec;

	//задержка дыхания
	if (object().IsHardHold() && !object().is_actor_creep() && GetPower() > m_fCantWalkPowerEnd)
	{
		float inertion_factor = object().inventory().ActiveItem()->GetControlInertionFactor();
		m_fDeltaPower -= m_fDeltaTime * m_fV_HardHoldPower * inertion_factor;
	}
	else
		object().SetHardHold(false);
}

void CActorCondition::UpdatePsyHealth()
{
	if (!fis_zero(m_fPsyHealth))
	{
		m_fDeltaPsyHealth += m_change_v.m_fV_PsyHealth * m_fDeltaTime * GetRegenKoef();
	}

	CEffectorPP* ppe = object().Cameras().GetPPEffector((EEffectorPPType)effPsyHealth);

	string64			pp_sect_name;
	shared_str ln = Level().name();
	strconcat(sizeof(pp_sect_name), pp_sect_name, "effector_psy_health", "_", *ln);
	if (!pSettings->section_exist(pp_sect_name))
		strcpy_s(pp_sect_name, "effector_psy_health");

	if (!fsimilar(GetPsyHealth(), 1.0f, 0.05f))
	{
		if (!ppe)
		{
			AddEffector(m_object, effPsyHealth, pp_sect_name, GET_KOEFF_FUNC(this, &CActorCondition::GetPsy));
		}
	}
	else
	{
		if (ppe)
			RemoveEffector(m_object, effPsyHealth);
	}
	//смерть при нулевом пси-здоровье
	if (fis_zero(GetPsyHealth()))
		health() = 0.0f;
}

void CActorCondition::UpdateRadiation()
{
	if (m_fRadiation > 0)
	{
		m_fDeltaRadiation -= m_change_v.m_fV_Radiation * m_fDeltaTime;
		//радиация постоянно отнимает здоровье только если выкючена опция взаимозависимости параметров
		m_fDeltaHealth -= CanBeHarmed() && !psActorFlags.test(AF_SURVIVAL) ? m_change_v.m_fV_RadiationHealth * m_fRadiation * m_fDeltaTime : 0.0f;
		//Msg("CActorCondition m_fDeltaHealth [%f]", m_fDeltaHealth);
	}
}

void CActorCondition::UpdateAlcohol()
{
	m_fAlcohol += m_fV_Alcohol * m_fDeltaTime * GetStress();
	clamp(m_fAlcohol, 0.0f, 1.0f);

	bool flag_state = !!psActorFlags.test(AF_SURVIVAL);

	CEffectorCam* ce = Actor()->Cameras().GetCamEffector((ECamEffectorType)effAlcohol);
	if (!fis_zero(m_fAlcohol))
	{
		if (!ce)
		{
			AddEffector(m_object, effAlcohol, "effector_alcohol", GET_KOEFF_FUNC(this, flag_state ?
				&CActorCondition::AlcoholSatiety :
				&CActorCondition::GetAlcohol));
		}
		else if (m_bFlagState != flag_state) //удалим эффектор для его передобавления если опцию изменили в процессе
		{
			RemoveEffector(m_object, effAlcohol);
			m_bFlagState = flag_state;
			//Msg("Restart effector");
		}
	}
	else
	{
		if (ce) RemoveEffector(m_object, effAlcohol);
	}
	//смерть при максимальном опьянении
	if (fsimilar(GetAlcohol(), 1.0f))
		health() = 0.0f;
	//Msg("Alcohol [%f], Satiety [%f], AlcoSat [%f]", GetAlcohol(), GetSatiety(), AlcoholSatiety());
}

float CActorCondition::BleedingSpeed()
{
	return inherited::BleedingSpeed() * GetStress();
}

float CActorCondition::GetWoundIncarnation()
{
	return m_change_v.m_fV_WoundIncarnation * GetRegenKoef();
}

float CActorCondition::GetHealthRestore()
{
	return m_change_v.m_fV_HealthRestore * GetRegenKoef();
}

float CActorCondition::GetRadiationRestore()
{
	return m_change_v.m_fV_Radiation;
}

float CActorCondition::GetPsyHealthRestore()
{
	return m_change_v.m_fV_PsyHealth * GetRegenKoef();
}

float CActorCondition::GetPowerRestore()
{
	return (m_fStandPower + m_fV_SatietyPower) * GetRegenKoef();
}

float CActorCondition::GetSatietyRestore()
{
	return m_fV_Satiety * GetStress();
}

float CActorCondition::GetAlcoholRestore()
{
	return m_fV_Alcohol * GetStress();
}

float CActorCondition::GetThirstRestore()
{
	return m_fV_Thirst * GetStress();
}