#include "stdafx.h"
#include <dinput.h>
#include "Actor.h"
#include "Torch.h"
#include "NightVisionDevice.h"
#include "trade.h"
#include "../xr_3da/camerabase.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
#include "hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "HudManager.h"
#include "UIGameSP.h"
#include "inventory.h"
#include "level.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "UsableScriptObject.h"
#include "clsid_game.h"
#include "actorcondition.h"
#include "actor_input_handler.h"
#include "string_table.h"
#include "UI/UIStatic.h"
#include "CharacterPhysicsSupport.h"
#include "InventoryBox.h"
#include "player_hud.h"
#include "HudItem.h"
#include "WeaponMagazined.h"
#include "../xr_3da/xr_input.h"
#include "CustomDetector.h"

#include "BottleItem.h"
#include "medkit.h"
#include "antirad.h"
#include "PHCapture.h"

extern int g_bHudAdjustMode;

void CActor::IR_OnKeyboardPress(int cmd){
	if (g_bHudAdjustMode && pInput->iGetAsyncKeyState(DIK_LSHIFT)){
		if (pInput->iGetAsyncKeyState(DIK_RETURN) || pInput->iGetAsyncKeyState(DIK_BACKSPACE) || pInput->iGetAsyncKeyState(DIK_DELETE))
			g_player_hud->tune(Ivector{});

		return;
	}

	if (m_blocked_actions.find((EGameActions)cmd) != m_blocked_actions.end() ) return; // Real Wolf. 14.10.2014

	if (Remote())		return;

//	if (conditions().IsSleeping())	return;
	if (IsTalking())	return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;

//	if (psCallbackFlags.test(CF_KEY_PRESS))
//		callback(GameObject::eOnKeyPress)(cmd);
	
	switch (cmd)
	{
	case kWPN_FIRE:
		{
			//-----------------------------
			if (OnServer())
			{
				NET_Packet P;
				P.w_begin(M_PLAYER_FIRE); 
				P.w_u16(ID());
				u_EventSend(P);
			}
		}break;
	default:
		{
		}break;
	}

	if (!g_Alive()) return;

	if(m_holder && kUSE != cmd)
	{
		m_holder->OnKeyboardPress			(cmd);
		if(m_holder->allowWeapon() && inventory().Action(cmd, CMD_START))		return;
		return;
	}else
		if(inventory().Action(cmd, CMD_START))					return;

	switch(cmd){
	case kJUMP:		
		{
			mstate_wishful |= mcJump;
			{
//				NET_Packet	P;
//				u_EventGen(P, GE_ACTOR_JUMPING, ID());
//				u_EventSend(P);
			}
		}break;
	case kCROUCH:
	{
		if (mstate_wishful & (mcSprint | mcLookout))
			mstate_wishful &= ~(mcSprint | mcLookout);

		if (mstate_wishful & mcCrouch && mstate_wishful & mcAccel)	//глубокий присед
			mstate_wishful &= ~(mcCrouch | mcAccel);
		else if (mstate_wishful & ~mcAccel)						//присед
			mstate_wishful |= (mcCrouch | mcAccel);
		else													//не присед
			mstate_wishful |= mcCrouch;
	}break;
	case kSPRINT_TOGGLE:	
		{
		if (IsZoomAimingMode())
		{
			auto pWeapon = smart_cast<CWeapon*>(inventory().ActiveItem());
			if (pWeapon) pWeapon->OnZoomOut();
		}

		if (mstate_wishful & (mcCrouch | mcAccel | mcLookout))
			mstate_wishful &= ~(mcCrouch | mcAccel | mcLookout);

		if (mstate_wishful & mcSprint)
			mstate_wishful &= ~mcSprint;
		else
			mstate_wishful |= mcSprint;
		}break;
	case kACCEL:
	{
		if (IsZoomAimingMode())
			SetHardHold(!IsHardHold()); //жесткий хват
		else
		{
			if (mstate_wishful & mcCrouch)
				return;
			else if (mstate_wishful & mcAccel)
				mstate_wishful &= ~mcAccel;
			else
				mstate_wishful |= mcAccel;
		}
	}break;
	case kL_LOOKOUT:
	{
		if (mstate_wishful & mcRLookout)
			mstate_wishful &= ~mcRLookout;
		else if (mstate_wishful & mcLLookout)
			mstate_wishful &= ~mcLLookout;
		else
			mstate_wishful |= mcLLookout;
	}break;
	case kR_LOOKOUT:
	{
		if (mstate_wishful & mcLLookout)
			mstate_wishful &= ~mcLLookout;
		else if (mstate_wishful & mcRLookout)
			mstate_wishful &= ~mcRLookout;
		else
			mstate_wishful |= mcRLookout;
	}break;
	case kCAM_1:	cam_Set(eacFirstEye);	UpdateVisorEfects(); break;
	case kCAM_2:	cam_Set(eacLookAt);		UpdateVisorEfects(); break;
	case kCAM_3:	cam_Set(eacFreeLook);	UpdateVisorEfects(); break;
	case kNIGHT_VISION: {
		auto pActiveWeapon = smart_cast<CWeaponMagazined*>(inventory().ActiveItem());

		if (pActiveWeapon && pActiveWeapon->IsZoomed())
			pActiveWeapon->SwitchNightVision();
		else
		{
			if (GetNightVisionDevice())
				GetNightVisionDevice()->SwitchNightVision();
		}
		} break;
	case kTORCH: { 
			if (GetTorch()) {
				GetTorch()->Switch();
			}
		} break;
	case kWPN_8:
	{
		if (auto det = smart_cast<CCustomDetector*>(inventory().ItemFromSlot(DETECTOR_SLOT)))
			det->ToggleDetector(g_player_hud->attached_item(0) != nullptr);
	}
	break;
	case kUSE:
		ActorUse();
		break;
	case kDROP:
		b_DropActivated			= TRUE;
		f_DropPower				= 0;
		break;
	case kNEXT_SLOT:
		{
			OnNextWeaponSlot();
		}break;
	case kPREV_SLOT:
		{
			OnPrevWeaponSlot();
		}break;
	case kUSE_QUICK_SLOT_0:
	case kUSE_QUICK_SLOT_1:
	case kUSE_QUICK_SLOT_2:
	case kUSE_QUICK_SLOT_3:
	{
		if (!GetTrade()->IsInTradeState())
		{	
			PIItem itm = nullptr;
			switch (cmd) {
			case kUSE_QUICK_SLOT_0:
				itm = inventory().m_slots[QUICK_SLOT_0].m_pIItem;
				break;
			case kUSE_QUICK_SLOT_1:
				itm = inventory().m_slots[QUICK_SLOT_1].m_pIItem;
				break;
			case kUSE_QUICK_SLOT_2:
				itm = inventory().m_slots[QUICK_SLOT_2].m_pIItem;
				break;
			case kUSE_QUICK_SLOT_3:
				itm = inventory().m_slots[QUICK_SLOT_3].m_pIItem;
				break;
			}

			if (itm) {
				string1024			str;

				if (itm->cast_eatable_item())
				{
					inventory().TryToHideWeapon(true, false);
					//
					bool SearchRuck = !psActorFlags.test(AF_QUICK_FROM_BELT);
					PIItem iitm = inventory().GetSame(itm, SearchRuck);
					if (iitm)
					{
						inventory().Eat(iitm);
						strconcat(sizeof(str), str, CStringTable().translate("st_item_used").c_str(), ": ", iitm->Name());
					}
					else
					{
						inventory().Eat(itm);
						strconcat(sizeof(str), str, CStringTable().translate("st_item_used").c_str(), ": ", itm->Name());
					}
					HUD().GetUI()->AddInfoMessage("item_usage", str, false);
				}

			}
			else
			{
				HUD().GetUI()->AddInfoMessage("item_usage", "st_quick_slot_empty");
			}
		}
	}break;
	case kLASER_ON:
	{
		if (auto wpn = smart_cast<CWeapon*>(inventory().ActiveItem()))
			wpn->SwitchLaser(!wpn->IsLaserOn());
	}break;
	case kFLASHLIGHT:
	{
		if (auto wpn = smart_cast<CWeapon*>(inventory().ActiveItem()))
			wpn->SwitchFlashlight(!wpn->IsFlashlightOn());
	}break;
	case kKICK:
	{
		ActorKick();
	}break;
	case kQUICK_THROW_GRENADE:
	{
		ActorQuickThrowGrenade();
	}break;
	}
}
void CActor::IR_OnMouseWheel(int direction){
	if (g_bHudAdjustMode)
	{
		g_player_hud->tune(Ivector().set(0, 0, direction));
		return;
	}

//	if (psCallbackFlags.test(CF_MOUSE_WHEEL_ROT))
//		this->callback(GameObject::eOnMouseWheel)(direction);

	if(inventory().Action( (direction>0)? kWPN_ZOOM_DEC:kWPN_ZOOM_INC , CMD_START)) return;


	if (psActorFlags.test(AF_MOUSE_WHEEL_SWITCH_SLOTS)) {
		if (direction > 0)
			OnNextWeaponSlot();
		else
			OnPrevWeaponSlot();
	}
}
void CActor::IR_OnKeyboardRelease(int cmd){
	if (g_bHudAdjustMode && pInput->iGetAsyncKeyState(DIK_LSHIFT))
		return;

	if (m_blocked_actions.find((EGameActions)cmd) != m_blocked_actions.end() ) return; // Real Wolf. 14.10.2014

	if (Remote())		return;

//	if (conditions().IsSleeping())	return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;

	if (g_Alive())	
	{
//		int dik = get_action_dik((EGameActions)cmd);
//		if ((dik != DIK_LALT) && (dik != DIK_RALT) && (dik != DIK_F4) /*&& psCallbackFlags.test(CF_KEY_RELEASE)*/)
//			this->callback(GameObject::eOnKeyRelease)(cmd);
		if (cmd == kUSE) 
			PickupModeOff();

		if(m_holder)
		{
			m_holder->OnKeyboardRelease(cmd);
			
			if(m_holder->allowWeapon() && inventory().Action(cmd, CMD_STOP))		return;
			return;
		}else
			if(inventory().Action(cmd, CMD_STOP))		return;



		switch(cmd)
		{
		case kJUMP:		mstate_wishful &=~mcJump;		break;
		case kDROP:		if(GAME_PHASE_INPROGRESS == Game().Phase()) g_PerformDrop();				break;
//		case kCROUCH:	g_bAutoClearCrouch = true;
		}
	}
}

void CActor::IR_OnKeyboardHold(int cmd){
	if (g_bHudAdjustMode && pInput->iGetAsyncKeyState(DIK_LSHIFT))
	{
		if (pInput->iGetAsyncKeyState(DIK_UP))
			g_player_hud->tune(Ivector{ 0, -1, 0 });
		else if (pInput->iGetAsyncKeyState(DIK_DOWN))
			g_player_hud->tune(Ivector{ 0, 1, 0 });
		else if (pInput->iGetAsyncKeyState(DIK_LEFT))
			g_player_hud->tune(Ivector{ -1, 0, 0 });
		else if (pInput->iGetAsyncKeyState(DIK_RIGHT))
			g_player_hud->tune(Ivector{ 1, 0, 0 });
		else if (pInput->iGetAsyncKeyState(DIK_PGUP))
			g_player_hud->tune(Ivector{ 0, 0, 1 });
		else if (pInput->iGetAsyncKeyState(DIK_PGDN))
			g_player_hud->tune(Ivector{ 0, 0, -1 });
		return;
	}

	if (m_blocked_actions.find((EGameActions)cmd) != m_blocked_actions.end() ) return; // Real Wolf. 14.10.2014

	if (Remote() || !g_Alive())					return;
//	if (conditions().IsSleeping())				return;
	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;
	if (IsTalking())							return;

//	int dik = get_action_dik((EGameActions)cmd);
//	if ((dik != DIK_LALT) && (dik != DIK_RALT) && (dik != DIK_F4)/* && psCallbackFlags.test(CF_KEY_HOLD)*/)
//		this->callback(GameObject::eOnKeyHold)(cmd);

	if(m_holder)
	{
		m_holder->OnKeyboardHold(cmd);
		return;
	}

	float LookFactor = GetLookFactor();
	switch(cmd)
	{
	case kUP:
	case kDOWN: 
		cam_Active()->Move( (cmd==kUP) ? kDOWN : kUP, 0, LookFactor);									break;
	case kCAM_ZOOM_IN: 
	case kCAM_ZOOM_OUT: 
		cam_Active()->Move(cmd);												break;
	case kLEFT:
	case kRIGHT:
		if (eacFreeLook!=cam_active) cam_Active()->Move(cmd, 0, LookFactor);	break;

//	case kACCEL:	mstate_wishful |= mcAccel;									break;
	case kL_STRAFE:	mstate_wishful |= mcLStrafe;								break;
	case kR_STRAFE:	mstate_wishful |= mcRStrafe;								break;
//	case kL_LOOKOUT:mstate_wishful |= mcLLookout;								break;
//	case kR_LOOKOUT:mstate_wishful |= mcRLookout;								break;
	case kFWD:		mstate_wishful |= mcFwd;									break;
	case kBACK:		mstate_wishful |= mcBack;									break;
//	case kCROUCH:	mstate_wishful |= mcCrouch;									break;
	}
}

void CActor::IR_OnMouseMove(int dx, int dy){
	if (g_bHudAdjustMode)
	{
		g_player_hud->tune(Ivector().set(dx, dy, 0));
		return;
	}

	PIItem iitem = inventory().ActiveItem();
	if (iitem && iitem->cast_hud_item())
		iitem->cast_hud_item()->ResetSubStateTime();

	if (Remote())		return;
//	if (conditions().IsSleeping())	return;

//	if (psCallbackFlags.test(CF_MOUSE_MOVE))
//		this->callback(GameObject::eOnMouseMove)(dx, dy);

	if(m_holder) 
	{
		m_holder->OnMouseMove(dx,dy);
		return;
	}

	float LookFactor = GetLookFactor();

	CCameraBase* C	= cameras	[cam_active];
	float scale		= (C->f_fov/g_fov)*psMouseSens * psMouseSensScale/50.f  / LookFactor;
	if (dx){
		float d = float(dx)*scale;
		cam_Active()->Move((d<0)?kLEFT:kRIGHT, _abs(d));
	}
	if (dy){
		float d = ((psMouseInvert.test(1))?-1:1)*float(dy)*scale*3.f/4.f;
		cam_Active()->Move((d>0)?kUP:kDOWN, _abs(d));
	}
}
#include "HudItem.h"
bool CActor::use_Holder				(CHolderCustom* holder){

	if(m_holder){
		bool b = false;
		CGameObject* holderGO			= smart_cast<CGameObject*>(m_holder);
		
		if(smart_cast<CCar*>(holderGO))
			b = use_Vehicle(0);
		else
			if (holderGO->CLS_ID==CLSID_OBJECT_W_MOUNTED ||
				holderGO->CLS_ID==CLSID_OBJECT_W_STATMGUN)
				b = use_MountedWeapon(0);

		if(inventory().ActiveItem()){
			CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
			if(hi) hi->OnAnimationEnd(hi->GetState());
		}

		return b;
	}else{
		bool b = false;
		CGameObject* holderGO			= smart_cast<CGameObject*>(holder);
		if(smart_cast<CCar*>(holder))
			b = use_Vehicle(holder);

		if (holderGO->CLS_ID==CLSID_OBJECT_W_MOUNTED ||
			holderGO->CLS_ID==CLSID_OBJECT_W_STATMGUN)
			b = use_MountedWeapon(holder);
		
		if(b){//used succesfully
			// switch off torch...
			CAttachableItem *I = CAttachmentOwner::attachedItem(CLSID_DEVICE_TORCH);
			if (I){
				CTorch* torch = smart_cast<CTorch*>(I);
				if (torch) torch->Switch(false);
			}
		}

		if(inventory().ActiveItem()){
			CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
			if(hi) hi->OnAnimationEnd(hi->GetState());
		}

		return b;
	}
}

extern bool g_bDisableAllInput;
void CActor::ActorUse() {
  if (g_bDisableAllInput || HUD().GetUI()->MainInputReceiver()) return;

  if (m_holder) {
    CGameObject*  GO = smart_cast<CGameObject*>(m_holder);
    NET_Packet    P;
    CGameObject::u_EventGen(P, GEG_PLAYER_DETACH_HOLDER, ID());
    P.w_u32(GO->ID());
    CGameObject::u_EventSend(P);
    return;
  }

  if (character_physics_support()->movement()->PHCapture()) {
	  ActorThrow();
	  character_physics_support()->movement()->PHReleaseObject();
	  return;
  }

  auto pTargetPerson = smart_cast<CEntityAlive*>(m_pPersonWeLookingAt);
  bool looking_at_alive_person = pTargetPerson && pTargetPerson->g_Alive();

  if (m_pUsableObject) {
	  if (looking_at_alive_person || inventory().IsFreeHands()) { //чтобы можно было слышать просьбы убрать оружие при попытке поговорить со сталкерами с оружием в руках
		  m_pUsableObject->use(this);
		  if (g_bDisableAllInput || HUD().GetUI()->MainInputReceiver()) return;
	  }
  }

  if (m_pInvBoxWeLookingAt && m_pInvBoxWeLookingAt->object().nonscript_usable() && m_pInvBoxWeLookingAt->IsOpened()) {
    // если контейнер открыт
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
    if (pGameSP && inventory().IsFreeHands()) pGameSP->StartCarBody(this, m_pInvBoxWeLookingAt);
    return;
  }
  
  else if (!m_pUsableObject || m_pUsableObject->nonscript_usable()) {
    if (m_pPersonWeLookingAt) {
      if (looking_at_alive_person) {
        TryToTalk();
        return;
      }
      //обыск трупа
      else if (!Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION))) {
        //только если находимся в режиме single
        CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
		if (pGameSP && inventory().IsFreeHands()) pGameSP->StartCarBody(this, m_pPersonWeLookingAt);
        return;
      }
    }

    collide::rq_result& RQ      = HUD().GetCurrentRayQuery();
    CPhysicsShellHolder* object = smart_cast<CPhysicsShellHolder*>(RQ.O);
    if (object && inventory().IsFreeHands()) {
      if (Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION))) {
        if (object->ActorCanCapture()) {
			if (!conditions().IsCantWalk())
				//Msg("--[%s] Actor Captured object: [%s]", __FUNCTION__, object->cName().c_str());
				character_physics_support()->movement()->PHCaptureObject(object, (u16)RQ.element);
			else 
				HUD().GetUI()->AddInfoMessage("actor_state", "cant_walk");
        }
        return;
      }
      else if (smart_cast<CHolderCustom*>(object) && RQ.range < inventory().GetTakeDist()) {
        NET_Packet P;
        CGameObject::u_EventGen(P, GEG_PLAYER_ATTACH_HOLDER, ID());
        P.w_u32(object->ID());
        CGameObject::u_EventSend(P);
        return;
      }
    }
  }

  PickupModeOn();
  PickupModeUpdate_COD();
}
BOOL CActor::HUDview				( )const { 
	return IsFocused()
		&&(cam_active==eacFirstEye)		
		&&((!m_holder) || (m_holder && m_holder->allowWeapon() && m_holder->HUDView() ) ); 
}

//void CActor::IR_OnMousePress(int btn)
constexpr u32 SlotsToCheck[] = {
		KNIFE_SLOT		,		// 0
		FIRST_WEAPON_SLOT		,		// 1
		SECOND_WEAPON_SLOT		,		// 2
		GRENADE_SLOT	,		// 3
		APPARATUS_SLOT	,		// 4
		BOLT_SLOT		,		// 5
};

void	CActor::OnNextWeaponSlot(){
	u32 ActiveSlot = inventory().GetActiveSlot();
	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = KNIFE_SLOT;
	
	constexpr u32 NumSlotsToCheck = sizeof(SlotsToCheck)/sizeof(u32);
	u32 CurSlot = 0;
	for (; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	}
	if (CurSlot >= NumSlotsToCheck) return;
	for (u32 i=CurSlot+1; i<NumSlotsToCheck; i++)
	{
		if (inventory().ItemFromSlot(SlotsToCheck[i]))
		{
			IR_OnKeyboardPress(kWPN_1+(i-KNIFE_SLOT));
			return;
		}
	}
}

void	CActor::OnPrevWeaponSlot(){
	u32 ActiveSlot = inventory().GetActiveSlot();
	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT) 
		ActiveSlot = KNIFE_SLOT;

	constexpr u32 NumSlotsToCheck = sizeof(SlotsToCheck)/sizeof(u32);
	u32 CurSlot = 0;
	for (; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	}
	if (CurSlot >= NumSlotsToCheck) return;
	for (s32 i=s32(CurSlot-1); i>=0; i--)
	{
		if (inventory().ItemFromSlot(SlotsToCheck[i]))
		{
			IR_OnKeyboardPress(kWPN_1+(i-KNIFE_SLOT));
			return;
		}
	}
}

float	CActor::GetLookFactor(){
	if (m_input_external_handler) 
		return m_input_external_handler->mouse_scale_factor();

	
	float factor	= 1.f;

	factor += (1.f - conditions().GetPowerKoef());

	PIItem pItem	= inventory().ActiveItem();

	if (pItem)
		factor *= pItem->GetControlInertionFactor();

	VERIFY(!fis_zero(factor));

	return factor;
}

void CActor::set_input_external_handler(CActorInputHandler *handler) {
	// clear state
	if (handler) 
		mstate_wishful			= 0;

	// release fire button
	if (handler)
		IR_OnKeyboardRelease	(kWPN_FIRE);

	// set handler
	m_input_external_handler	= handler;
}

void CActor::ActorThrow(){
	CPHCapture* capture = character_physics_support()->movement()->PHCapture();
	if (!capture->taget_object())
		return;

	if (conditions().IsCantWalk())
	{
		HUD().GetUI()->AddInfoMessage("actor_state", "cant_walk");
		return;
	}

	float mass_f = GetTotalMass(capture->taget_object());

	bool drop_not_throw = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)); //отпустить или отбросить предмет

	float throw_impulse = drop_not_throw ?
		0.5f :											//отпустить
		m_fThrowImpulse * conditions().GetPowerKoef();	//бросить

	Fvector dir = Direction();	//направлении взгляда актора
	if (drop_not_throw)
		dir = { 0, -1, 0 };		//если отпускаем а не бросаем то вектор просто вниз

	dir.normalize();

	float real_imp = throw_impulse * mass_f;

	capture->taget_object()->PPhysicsShell()->applyImpulse(dir, real_imp); //придадим предмету импульс в заданном направлении
	//Msg("throw_impulse [%f], real_imp [%f]", throw_impulse, real_imp);

	if (!GodMode())
		if (!drop_not_throw) conditions().ConditionJump(mass_f / 50);
	//Msg("power decreased on [%f]", mass_f / 50);
}
//
void CActor::ActorKick(){
	CGameObject* O = ObjectWeLookingAt();
	if (!O)
		return;

	if (conditions().IsCantWalk())
	{
		HUD().GetUI()->AddInfoMessage("actor_state", "cant_walk");
		return;
	}

	float mass_f = GetTotalMass(O);

	CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
	if (EA && EA->g_Alive() && mass_f > 20.0f) //ability to kick tuskano and rat
		return;

	float kick_impulse = m_fKickImpulse * conditions().GetPowerKoef();
	Fvector dir = Direction();
	dir.y = sin(15.f * PI / 180.f);
	dir.normalize();

	u16 bone_id = 0;
	collide::rq_result& RQ = HUD().GetCurrentRayQuery();
	if (RQ.O == O && RQ.element != 0xffff)
		bone_id = (u16)RQ.element;

	clamp<float>(mass_f, 1.0f, 100.f); // ограничить параметры хита

	// The smaller the mass, the more damage given capped at 60 mass. 60+ mass take 0 damage
	float hit_power = (100.f * ((mass_f / 100.f) - 0.6f) / (0.f - 0.6f)) * conditions().GetPowerKoef();
	clamp<float>(hit_power, 0.f, 100.f);
	hit_power /= 100;

	//shell->applyForce(dir, kick_power * conditions().GetPower());
	Fvector h_pos = O->Position();
	SHit hit = SHit(hit_power, dir, this, bone_id, h_pos, kick_impulse, ALife::eHitTypeStrike, 0.f, false);
	O->Hit(&hit);
	if (EA)
	{
		static float alive_kick_power = 3.f;
		float real_imp = kick_impulse / mass_f;
		dir.mul(pow(real_imp, alive_kick_power));
		if (EA->character_physics_support())
		{
			EA->character_physics_support()->movement()->AddControlVel(dir);
			EA->character_physics_support()->movement()->ApplyImpulse(dir.normalize(), kick_impulse * alive_kick_power);
		}
	}

	if (!GodMode())
		conditions().ConditionJump(mass_f / 50);
}
#include "Grenade.h"
void CActor::ActorQuickThrowGrenade(){
	auto &inv = inventory();
	auto pGrenade = smart_cast<CGrenade*>(inv.m_slots[GRENADE_SLOT].m_pIItem);
	if (!pGrenade) return;

	if (inv.GetActiveSlot() != GRENADE_SLOT) {
		inv.SetPrevActiveSlot(inv.GetActiveSlot());
		pGrenade->m_bIsQuickThrow = true;
		inv.Activate(GRENADE_SLOT, eGeneral, true, true);
	}
	else
		pGrenade->Action(kWPN_FIRE, CMD_START);
}