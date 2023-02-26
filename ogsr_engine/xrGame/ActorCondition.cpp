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

BOOL GodMode(){ 
	return psActorFlags.test(AF_GODMODE); 
}

CActorCondition::CActorCondition(CActor *object) : inherited(object){
	VERIFY						(object);
	m_object					= object;
	m_condition_flags.zero		();
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
	m_fV_Power					= READ_IF_EXISTS(pSettings, r_float, section, "power_v", 0.0f);

	m_fV_Satiety				= pSettings->r_float(section,"satiety_v");		
	m_fV_SatietyPower			= pSettings->r_float(section,"satiety_power_v");
	m_fV_SatietyHealth			= pSettings->r_float(section,"satiety_health_v");

	m_fBleedingPowerDecrease		= READ_IF_EXISTS(pSettings, r_float, section, "bleeding_power_dec", 0.f);
	m_fMinPowerWalkJump				= READ_IF_EXISTS(pSettings, r_float, section, "min_power_walk_jump", 1.0f);
	m_fMinHealthRadiation			= READ_IF_EXISTS(pSettings, r_float, section, "min_health_radiation", 1.0f);
	m_fMinHealthRadiationTreshold	= READ_IF_EXISTS(pSettings, r_float, section, "min_health_radiation_treshold", 0.f);
	m_fAlcoholSatietyIntens			= READ_IF_EXISTS(pSettings, r_float, section, "satiety_to_alcohol_effector_intensity", 1.0f);
	m_fExerciseStressFactor			= READ_IF_EXISTS(pSettings, r_float, section, "exercise_stress_factor", 1.0f);
	m_fZoomEffectorK				= READ_IF_EXISTS(pSettings, r_float, section, "power_to_zoom_effector_k", 10.0f);
	m_fV_HardHoldPower				= READ_IF_EXISTS(pSettings, r_float, section, "hard_hold_power_v", 0.f);
}


//вычисление параметров с ходом времени
#include "CharacterPhysicsSupport.h"
void CActorCondition::UpdateCondition(){
	if (GodMode())
		return;
	if (!object().g_Alive())
		return;
	if (!object().Local() && m_object != Level().CurrentViewEntity())
		return;	
	//
	if (IsCantWalk() && object().character_physics_support()->movement()->PHCapture())
		object().character_physics_support()->movement()->PHReleaseObject();

	float weight_k = object().GetCarryWeight() / object().MaxCarryWeight();

	if ((object().mstate_real&mcAnyMove)) {
		ConditionWalk(weight_k, isActorAccelerated(object().mstate_real,object().IsZoomAimingMode()), (object().mstate_real&mcSprint) != 0);
	}else{
		ConditionStand(weight_k);
	}

	inherited::UpdateCondition();

	UpdateTutorialThresholds();
}

void CActorCondition::UpdateSatiety()
{
	if (m_fSatiety > 0){
		m_fSatiety -= m_fV_Satiety * m_fDeltaTime * GetStress();
		clamp(m_fSatiety, 0.0f, 1.0f);
	}
}


CWound* CActorCondition::ConditionHit(SHit* pHDS)
{
	if (GodMode()) 
		return NULL;
	if (pHDS->hit_type == ALife::eHitTypeTelepatic)
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
	power				/= object().GetExoFactor();
	m_fPower			-=	HitPowerEffect(power);
}
void CActorCondition::ConditionWalk(float weight, bool accel, bool sprint){	
	float power			=	m_fWalkPower;
	power				+=	m_fWalkWeightPower*weight*(weight>1.f?m_fOverweightWalkK:1.f);
	power				*=	m_fDeltaTime*(accel?(sprint?m_fSprintK:m_fAccelK):1.f);
	power				/= object().GetExoFactor();
	m_fPower			-=	HitPowerEffect(power);
}
void CActorCondition::ConditionStand(float weight){	
	float power			= m_fStandPower * GetRegenK();
	power				*= m_fDeltaTime;
	power				/= weight > 1.f ? weight : 1.f;
	m_fPower			+= power;
}


bool CActorCondition::IsCantWalk(){
	if(m_fPower< m_fCantWalkPowerBegin)
		m_condition_flags.set(eCantWalk, TRUE);
	else if(m_fPower > m_fCantWalkPowerEnd)
		m_condition_flags.set(eCantWalk, FALSE);
	return m_condition_flags.test(eCantWalk);
}

bool CActorCondition::IsCantSprint(){
	if (m_fPower < m_fCantSprintPowerBegin)
		m_condition_flags.set(eCantSprint, TRUE);
	else if (m_fPower > m_fCantSprintPowerEnd)
		m_condition_flags.set(eCantSprint, FALSE);
	return m_condition_flags.test(eCantSprint);
}

bool CActorCondition::IsCantJump(float weight){
	if (!m_bJumpRequirePower || GodMode()){
		return false;
	}
	float power = m_fJumpPower;
	power += m_fJumpWeightPower * weight*(weight > 1.f ? m_fOverweightJumpK : 1.f);
	return m_fPower < HitPowerEffect(power);
}

bool CActorCondition::IsLimping(){
	if (m_fPower < m_fLimpingPowerBegin || GetHealth() < m_fLimpingHealthBegin)
		m_condition_flags.set(eLimping, TRUE);
	else if (m_fPower > m_fLimpingPowerEnd && GetHealth() > m_fLimpingHealthEnd)
		m_condition_flags.set(eLimping, FALSE);
	return m_condition_flags.test(eLimping);
}

void CActorCondition::save(NET_Packet &output_packet)
{
	inherited::save		(output_packet);
	save_data			(m_fAlcohol, output_packet);
	save_data			(m_condition_flags, output_packet);
	save_data			(m_fSatiety, output_packet);
}

void CActorCondition::load(IReader &input_packet)
{
	inherited::load		(input_packet);
	load_data			(m_fAlcohol, input_packet);
	load_data			(m_condition_flags, input_packet);
	load_data			(m_fSatiety, input_packet);
}

void CActorCondition::reinit	()
{
	inherited::reinit	();
	m_condition_flags.set(eLimping, FALSE);
	m_condition_flags.set(eCantWalk, FALSE);
	m_condition_flags.set(eCantSprint, FALSE);
	m_fSatiety = 1.f;
	m_fAlcohol = 0.f;
}

void CActorCondition::ChangeAlcohol	(float value){
	m_fAlcohol += value;
}

void CActorCondition::ChangeSatiety(float value){
	//влияние поглощённой дозы радиации на насыщение едой
	if(value > 0.f)
		value *= (1.0f - GetRadiation());
	m_fSatiety += value;
	clamp		(m_fSatiety, 0.0f, 1.0f);
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
	switch (pHDS->hit_type)
	{
	case ALife::eHitTypeTelepatic:
	case ALife::eHitTypeChemicalBurn:
	case ALife::eHitTypeBurn:
	case ALife::eHitTypeRadiation:
		return false;
	default:
		return true;
	}
}

float CActorCondition::HitSlowmo(SHit* pHDS){
	float ret{};
	if(pHDS->hit_type==ALife::eHitTypeWound || pHDS->hit_type==ALife::eHitTypeStrike ){
		ret						= pHDS->damage();
		clamp					(ret,0.0f,1.f);
	}
	return ret;	
}

float CActorCondition::GetRegenK() { 
	return (1.0f - GetRadiation()) * GetSatiety(); 
}

float CActorCondition::GetSmoothOwerweightKoef(){
	float val = 1.0f;
	float power_k = m_fMinPowerWalkJump + (1.0f - m_fMinPowerWalkJump) * GetPower();	//коэф влияния выносливости
	float overweight_k = object().GetCarryWeight() > object().MaxCarryWeight() ?		//считаем коэф. только если есть перегруз
		object().MaxCarryWeight() / object().GetCarryWeight() :							//коэф влияния перегруза
		1.0f;
	val = power_k * overweight_k;
	//Msg("SmoothOverweightK = %f", val);
	return val;
}

float CActorCondition::GetStress(){
	float res{1.0f};
	if (object().get_state() & (mcSprint | mcJump))
		res *= m_fExerciseStressFactor;
	if (object().GetCarryWeight() > object().MaxCarryWeight())
		res *= (object().GetCarryWeight() / object().MaxCarryWeight());
	return res;
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

	m_fDeltaHealth += m_fDeltaTime * GetHealthRestore();
	VERIFY(_valid(m_fDeltaHealth));

	ChangeBleeding(GetWoundIncarnation() * m_fDeltaTime);

	//радиация влияет на максимальное здоровье
	if (m_fRadiation > m_fMinHealthRadiationTreshold) //защита от потенциального деления на 0 если m_fRadiationTreshold = 1
		object().SetMaxHealth(m_fMinHealthRadiation + (1.0f - m_fMinHealthRadiation) * (1.0f - m_fRadiation) / (1.0f - m_fMinHealthRadiationTreshold));
	else
		object().SetMaxHealth(1.0f);
}

void CActorCondition::UpdatePower(){
	float weight{ object().GetCarryWeight() }, base_w{ object().MaxCarryWeight() };

	float k_max_power = 1.0f + _min(weight, base_w) / base_w + _max(0.0f, (weight - base_w) / 10.0f);

	ChangeMaxPower(-m_fPowerLeakSpeed * m_fDeltaTime * k_max_power); //кажется это таки "сонливость" - постоянное уменьшение максимальной выносливости

	float bleeding_power_dec = BleedingSpeed() * m_fBleedingPowerDecrease;

	m_fDeltaPower += m_fV_SatietyPower *
		m_fDeltaTime * GetRegenK() - bleeding_power_dec;

	//задержка дыхания
	if (object().IsHardHold() && !object().is_actor_creep() && GetPower() > m_fCantWalkPowerEnd){
		float inertion_factor = object().inventory().ActiveItem()->GetControlInertionFactor();
		m_fDeltaPower -= m_fDeltaTime * (m_fV_HardHoldPower / object().GetExoFactor()) * inertion_factor;
	}
	else
		object().SetHardHold(false);
}

void CActorCondition::UpdatePsyHealth(){
	if (!fis_zero(m_fPsyHealth)){
		m_fDeltaPsyHealth += GetPsyHealthRestore() * m_fDeltaTime;
		CEffectorPP* ppe = object().Cameras().GetPPEffector((EEffectorPPType)effPsyHealth);

		string64 pp_sect_name;
		shared_str ln = Level().name();
		strconcat(sizeof(pp_sect_name), pp_sect_name, "effector_psy_health", "_", ln.c_str());
		if (!pSettings->section_exist(pp_sect_name))
			strcpy_s(pp_sect_name, "effector_psy_health");
		if (!fsimilar(GetPsyHealth(), 1.0f, 0.05f)) {
			if (!ppe) {
				AddEffector(m_object, effPsyHealth, pp_sect_name, GET_KOEFF_FUNC(this, &CActorCondition::GetPsy));
			}
		}
		else {
			if (ppe)
				RemoveEffector(m_object, effPsyHealth);
		}
	}else
		health() = 0.0f;
}

void CActorCondition::UpdateRadiation(){
	if (m_fRadiation > 0.f){
		m_fDeltaRadiation -= m_change_v.m_fV_Radiation * m_fDeltaTime;
	}
}

void CActorCondition::UpdateAlcohol(){
	//смерть при максимальном опьянении
	if (fsimilar(GetAlcohol(), 1.0f))
		health() = 0.0f;
	else {
		m_fAlcohol += m_fV_Alcohol * m_fDeltaTime * GetStress();
		clamp(m_fAlcohol, 0.0f, 1.0f);
		CEffectorCam* ce = Actor()->Cameras().GetCamEffector((ECamEffectorType)effAlcohol);
		if (!fis_zero(m_fAlcohol)) {
			if (!ce) {
				AddEffector(m_object, effAlcohol, "effector_alcohol", GET_KOEFF_FUNC(this, &CActorCondition::AlcoholSatiety));
			}
		}else{
			if (ce) RemoveEffector(m_object, effAlcohol);
		}
	}
}

float CActorCondition::BleedingSpeed(){
	return inherited::BleedingSpeed() * GetStress();
}

float CActorCondition::GetWoundIncarnation(){
	return inherited::GetWoundIncarnation() * GetRegenK();
}

float CActorCondition::GetHealthRestore(){
	return (inherited::GetHealthRestore() + m_fV_SatietyHealth) * GetRegenK();
}

float CActorCondition::GetPsyHealthRestore(){
	return inherited::GetPsyHealthRestore() * GetRegenK();
}

float CActorCondition::GetPowerRestore(){
	return (m_fStandPower + m_fV_SatietyPower) * GetRegenK();
}

float CActorCondition::GetMaxPowerRestore() {
	return 1.f;
}

float CActorCondition::GetSatietyRestore(){
	return m_fV_Satiety * GetStress();
}

float CActorCondition::GetAlcoholRestore(){
	return m_fV_Alcohol * GetStress();
}