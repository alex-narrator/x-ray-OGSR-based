#include "stdafx.h"
#include "grenade.h"
#include "PhysicsShell.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "actor.h"
#include "inventory.h"
#include "level.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "xrserver_objects_alife.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "xrServer_Objects_ALife_Items.h"

CGrenade::CGrenade(void) 
{
	m_destroy_callback.clear();
	//m_eSoundCheckout = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
}

CGrenade::~CGrenade(void) 
{
	HUD_SOUND::DestroySound(sndCheckout);
}

void CGrenade::Load(LPCSTR section) 
{
	inherited::Load(section);
	CExplosive::Load(section);

	//HUD_SOUND::LoadSound(section,"snd_checkout",sndCheckout,m_eSoundCheckout);

	m_grenade_detonation_threshold_hit = READ_IF_EXISTS(pSettings, r_float, section, "detonation_threshold_hit", 100.f);
	b_impact_fuze = READ_IF_EXISTS(pSettings, r_bool, section, "impact_fuze", false);
}

void CGrenade::Hit					(SHit* pHDS)
{
	if( ALife::eHitTypeExplosion==pHDS->hit_type && m_grenade_detonation_threshold_hit<pHDS->damage()&&CExplosive::Initiator()==u16(-1)) 
	{
		CExplosive::SetCurrentParentID(pHDS->who->ID());
		Destroy();
	}
	inherited::Hit(pHDS);
}

BOOL CGrenade::net_Spawn(CSE_Abstract* DC) 
{
	BOOL ret= inherited::net_Spawn		(DC);
	Fvector box;BoundingBox().getsize	(box);
	float max_size						= _max(_max(box.x,box.y),box.z);
	box.set								(max_size,max_size,max_size);
	box.mul								(3.f);
	CExplosive::SetExplosionSize		(box);
	m_thrown							= false;
	//
	if (auto se_grenade = smart_cast<CSE_ALifeItemGrenade*>(DC))
		if (se_grenade->m_dwDestroyTimeMax) {	//загружаем значение задержки из серверного объекта
			m_dwDestroyTimeMax = se_grenade->m_dwDestroyTimeMax;
		}else{											//попытаемся сгенерировать задержку
			LPCSTR str = pSettings->r_string(cNameSect(), "destroy_time");
			if (_GetItemCount(str) > 1){				//заданы границы рандомной задержки до взрыва
				Ivector2 m = pSettings->r_ivector2(cNameSect(), "destroy_time");
				m_dwDestroyTimeMax = ::Random.randI(m.x, m.y);
			}
		}
	//
	return								ret;
}

void CGrenade::net_Export(CSE_Abstract* E) {
	auto se_grenade = smart_cast<CSE_ALifeItemGrenade*>(E);
	se_grenade->m_dwDestroyTimeMax = m_dwDestroyTimeMax;
};

void CGrenade::net_Destroy() 
{
	if(m_destroy_callback)
	{
		m_destroy_callback				(this);
		m_destroy_callback = nullptr;
	}

	inherited::net_Destroy				();
	CExplosive::net_Destroy				();
}

void CGrenade::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
}

void CGrenade::OnH_A_Independent() 
{
	inherited::OnH_A_Independent		();	
}

void CGrenade::OnH_A_Chield()
{
	m_dwDestroyTime						= 0xffffffff;
	inherited::OnH_A_Chield				();
}

void CGrenade::State(u32 state, u32 oldState)
{
	switch (state)
	{
	case eThrowStart:
		{
			Fvector						C;
			Center						(C);
			PlaySound					(sndCheckout,C);
		}break;
	case eThrowEnd:
		{
			if(m_thrown)
			{
				if (m_pPhysicsShell)	m_pPhysicsShell->Deactivate();
				xr_delete				(m_pPhysicsShell);
				m_dwDestroyTime			= 0xffffffff;
				
				if(H_Parent())
					PutNextToSlot		();

				if (Local())
				{
					#ifdef DEBUG
					Msg("Destroying local grenade[%d][%d]",ID(),Device.dwFrame);
					#endif
					DestroyObject		();
				}
				
			};
		}break;
	};
	inherited::State(state, oldState);
}

void CGrenade::Throw() 
{
	if (!m_fake_missile || m_thrown)
		return;

	CGrenade					*pGrenade = smart_cast<CGrenade*>(m_fake_missile);
	VERIFY						(pGrenade);
	
	if (pGrenade) {
		pGrenade->set_destroy_time(m_dwDestroyTimeMax);
		//установить ID того кто кинул гранату
		pGrenade->SetInitiator( H_Parent()->ID() );
	}
	inherited::Throw			();

	if (m_pCurrentInventory->GetOwner())
	{
		CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			Actor()->set_state_wishful(Actor()->get_state_wishful() & (~mcSprint));
		}
	}

	m_fake_missile->processing_activate();//@sliph
	m_thrown = true;
	
	// Real Wolf.Start.18.12.14
	auto parent = smart_cast<CGameObject*>(H_Parent());
	auto obj	= smart_cast<CGameObject*>(m_fake_missile);
	if (parent && obj)
	{
		parent->callback(GameObject::eOnThrowGrenade)(obj->lua_game_object());
	}
	// Real Wolf.End.18.12.14
}



void CGrenade::Destroy() 
{
	//Generate Expode event
	Fvector						normal;

	if(m_destroy_callback)
	{
		m_destroy_callback		(this);
		m_destroy_callback = nullptr;
	}

	FindNormal					(normal);
	Fvector C; Center( C );
	CExplosive::GenExplodeEvent( C, normal );
}



bool CGrenade::Useful() const
{

	bool res = (/* !m_throw && */ m_dwDestroyTime == 0xffffffff && CExplosive::Useful() && TestServerFlag(CSE_ALifeObject::flCanSave));

	return res;
}

void CGrenade::OnEvent(NET_Packet& P, u16 type) 
{
	inherited::OnEvent			(P,type);
	CExplosive::OnEvent			(P,type);
}

void CGrenade::PutNextToSlot()
{
	if (OnClient()) return;
	VERIFY									(!getDestroy());

	//выкинуть гранату из инвентаря
	if (m_pCurrentInventory)
	{
		auto& inv = m_pCurrentInventory;
		auto _grenade_slot = this->GetSlot();

		inv->Ruck(this, true);

		if(psActorFlags.test(AF_ITEMS_FROM_BELT)) 
			inv->Belt(this);

		CGrenade *pNext					= smart_cast<CGrenade*>(inv->Same(this, !psActorFlags.test(AF_ITEMS_FROM_BELT)));
		if (!pNext)
			pNext						= smart_cast<CGrenade*>(inv->SameGrenade(this, !psActorFlags.test(AF_ITEMS_FROM_BELT)));

		if (pNext) pNext->SetSlot(_grenade_slot);

		VERIFY							(pNext != this);

		CActor* pActor = smart_cast<CActor*>(inv->GetOwner());
		if (!pNext || !inv->Slot(pNext, m_bIsQuickThrow) || m_bIsQuickThrow) {
			if (pActor) 
				inv->Activate(inv->GetPrevActiveSlot(), eGeneral, m_bIsQuickThrow, m_bIsQuickThrow);
		}
		if (m_bIsQuickThrow) {
			m_bIsQuickThrow = false;
		}
	}
}

void CGrenade::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eThrowEnd: SwitchState(eHidden);	break;
	default: inherited::OnAnimationEnd(state);
	}
}

void CGrenade::UpdateCL() 
{
	inherited::UpdateCL			();
	CExplosive::UpdateCL		();
}

bool CGrenade::Action(s32 cmd, u32 flags) 
{
	if (inherited::Action(cmd, flags))
		return true;


	switch(cmd) 
	{
	//переключение типа гранаты
	case kWPN_NEXT:
		{
            if(flags&CMD_START) 
			{
				const u32 state = GetState();
				if (state == eHidden
					|| state == eIdle
					|| state == eBore)
				{
					if (m_pCurrentInventory){
						const auto& inv = m_pCurrentInventory;
						// (c) NanoBot
						xr_vector<shared_str>    types_sect_grn;        // текущий список секций гранат
						// Находим список секций гранат разных типов в активе
						// в m_belt или m_ruck нет гранаты которую актор держит в руках, т.е. this
						types_sect_grn.push_back(this->cNameSect());
						int        count_types = 1;    // текущие количество типов гранат в активе
						//GRENADE_FROM_BELT
						if (!psActorFlags.test(AF_ITEMS_FROM_BELT)) {
							for (const auto& item : inv->m_ruck) {
								CGrenade* pGrenade = smart_cast<CGrenade*>(item);
								if (pGrenade) {
									// составляем список типов гранат (с) НаноБот
									bool    new_type = true;
									for (const auto& sect : types_sect_grn) {
										if (!xr_strcmp(pGrenade->cNameSect(), sect)) // если совпадают
											new_type = false;
									}
									if (new_type) {    // новый тип гранаты?, добавляем
										types_sect_grn.push_back(pGrenade->cNameSect());
										count_types++;
									}
								}
							}
						} else {
							for (const auto& item : inv->m_vest) {
								CGrenade* pGrenade = smart_cast<CGrenade*>(item);
								if (pGrenade) {
									// составляем список типов гранат (с) НаноБот
									bool    new_type = true;
									for (const auto& sect : types_sect_grn) {
										if (!xr_strcmp(pGrenade->cNameSect(), sect)) // если совпадают
											new_type = false;
									}
									if (new_type) {    // новый тип гранаты?, добавляем
										types_sect_grn.push_back(pGrenade->cNameSect());
										count_types++;
									}
								}
							}
							for (const auto& item : inv->m_belt) {
								CGrenade* pGrenade = smart_cast<CGrenade*>(item);
								if (pGrenade) {
									// составляем список типов гранат (с) НаноБот
									bool    new_type = true;
									for (const auto& sect : types_sect_grn) {
										if (!xr_strcmp(pGrenade->cNameSect(), sect)) // если совпадают
											new_type = false;
									}
									if (new_type) {    // новый тип гранаты?, добавляем
										types_sect_grn.push_back(pGrenade->cNameSect());
										count_types++;
									}
								}
							}
							for (u32 i = 0; i < inv->m_slots.size(); ++i) {
								const auto _item = inv->m_slots[i].m_pIItem;
								if (!_item) continue;
								CGrenade* pGrenade = smart_cast<CGrenade*>(_item);
								if (!pGrenade) continue;

								bool    new_type = true;
								for (const auto& sect : types_sect_grn) {
									if (!xr_strcmp(pGrenade->cNameSect(), sect)) // если совпадают
										new_type = false;
								}
								if (new_type) {    // новый тип гранаты?, добавляем
									types_sect_grn.push_back(pGrenade->cNameSect());
									count_types++;
								}
							}
						}
						// Если типов больше 1 то, сортируем список по алфавиту
						// и находим номер текущей гранаты в списке.
						if (count_types > 1){
							int        curr_num = 0;        // номер типа текущей гранаты
							std::sort(types_sect_grn.begin(), types_sect_grn.end());
							for (const auto& sect : types_sect_grn)
							{
								if (!xr_strcmp(this->cNameSect(), *sect)) // если совпадают
									break;
								curr_num++;
							}
							int        next_num = curr_num + 1;    // номер секции следующей гранаты
							if (next_num >= count_types)    next_num = 0;
							shared_str    sect_next_grn = types_sect_grn[next_num];    // секция следущей гранаты
							// Ищем в активе гранату с секцией следущего типа
							//GRENADE_FROM_BELT
							auto _slot = this->GetSlot();
							if (!psActorFlags.test(AF_ITEMS_FROM_BELT)) {
								for (const auto& item : inv->m_ruck) {
									CGrenade* pGrenade = smart_cast<CGrenade*>(item);
									if (pGrenade && !xr_strcmp(pGrenade->cNameSect(), sect_next_grn)) {
										pGrenade->SetSlot(_slot);
										inv->Ruck(this, true);
										inv->SetActiveSlot(NO_ACTIVE_SLOT);
										inv->Slot(pGrenade);
										//GRENADE_FROM_BELT
										if (psActorFlags.test(AF_ITEMS_FROM_BELT)) {
											if (!inv->Vest(this))						//поточну гранату до розгрузки
												if (!inv->Belt(this))					//якщо ні то у пояс
													if (inv->CanPutInSlot(this, true))	//перевіримо так щоб вільний слот був призначено автоматично
														inv->Slot(this);				//та пхнемо у слот якщо нікуди не лізе
										}
										return true;
									}
								}
							} else {
								for (const auto& item : inv->m_vest) {
									CGrenade* pGrenade = smart_cast<CGrenade*>(item);
									if (pGrenade && !xr_strcmp(pGrenade->cNameSect(), sect_next_grn)) {
										pGrenade->SetSlot(_slot);
										inv->Ruck(this, true);
										inv->SetActiveSlot(NO_ACTIVE_SLOT);
										inv->Slot(pGrenade);
										//GRENADE_FROM_BELT
										if (psActorFlags.test(AF_ITEMS_FROM_BELT)) {
											if (!inv->Vest(this))						//поточну гранату до розгрузки
												if (!inv->Belt(this))					//якщо ні то у пояс
													if (inv->CanPutInSlot(this, true))	//перевіримо так щоб вільний слот був призначено автоматично
														inv->Slot(this);				//та пхнемо у слот якщо нікуди не лізе
										}
										return true;
									}
								}
								for (const auto& item : inv->m_belt) {
									CGrenade* pGrenade = smart_cast<CGrenade*>(item);
									if (pGrenade && !xr_strcmp(pGrenade->cNameSect(), sect_next_grn)) {
										pGrenade->SetSlot(_slot);
										inv->Ruck(this, true);
										inv->SetActiveSlot(NO_ACTIVE_SLOT);
										inv->Slot(pGrenade);
										//GRENADE_FROM_BELT
										if (psActorFlags.test(AF_ITEMS_FROM_BELT)) {
											if (!inv->Vest(this))						//поточну гранату до розгрузки
												if (!inv->Belt(this))					//якщо ні то у пояс
													if (inv->CanPutInSlot(this, true))	//перевіримо так щоб вільний слот був призначено автоматично
														inv->Slot(this);				//та пхнемо у слот якщо нікуди не лізе
										}
										return true;
									}
								}
								for (u32 i = 0; i < inv->m_slots.size(); ++i) {
									const auto _item = inv->m_slots[i].m_pIItem;
									CGrenade* pGrenade = smart_cast<CGrenade*>(_item);
									if (pGrenade && !xr_strcmp(pGrenade->cNameSect(), sect_next_grn)) {
										pGrenade->SetSlot(_slot);
										inv->Ruck(this, true);
										inv->SetActiveSlot(NO_ACTIVE_SLOT);
										inv->Slot(pGrenade);
										//GRENADE_FROM_BELT
										if (!inv->Vest(this))						//поточну гранату до розгрузки
											if(!inv->Belt(this))					//якщо ні то у пояс
												if(inv->CanPutInSlot(this, true))	//перевіримо так щоб вільний слот був призначено автоматично
													inv->Slot(this);				//та пхнемо у слот якщо нікуди не лізе
										return true;
									}
								}
							}
						}
						return true;
					}
				}
			}
			return true;
		};
	}
	return false;
}

BOOL CGrenade::UsedAI_Locations		()
{
#pragma todo("Dima to Yura : It crashes, because on net_Spawn object doesn't use AI locations, but on net_Destroy it does use them")
	return TRUE;//m_dwDestroyTime == 0xffffffff;
}

void CGrenade::net_Relcase(CObject* O )
{
	CExplosive::net_Relcase(O);
	inherited::net_Relcase(O);
}

void CGrenade::Deactivate( bool now )
{
	//Drop grenade if primed
	StopCurrentAnimWithoutCallback();
	CEntityAlive* entity = smart_cast<CEntityAlive*>( m_pCurrentInventory->GetOwner() );
	if ( !entity->g_Alive() && !GetTmpPreDestroy() && Local() && ( GetState() == eThrowStart || GetState() == eReady || GetState() == eThrow ) )
	{
		if (m_fake_missile)
		{
			CGrenade*		pGrenade	= smart_cast<CGrenade*>(m_fake_missile);
			if (pGrenade)
			{
				if (m_pCurrentInventory->GetOwner())
				{
					CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
					if (pActor)
					{
						if (!pActor->g_Alive())
						{
							m_constpower			= false;
							m_fThrowForce			= 0;
						}
					}
				}				
				Throw					();
			};
		};
	};

	inherited::Deactivate( now || ( GetState() == eThrowStart || GetState() == eReady || GetState() == eThrow) );
}
#include "hudmanager.h"
#include "ui/UIMainIngameWnd.h"
void CGrenade::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
	str_name = NameShort();
	bool SearchRuck = !psActorFlags.test(AF_ITEMS_FROM_BELT);
	u32 ThisGrenadeCount = m_pCurrentInventory->GetSameItemCount(cNameSect().c_str(), SearchRuck);
	string16				stmp{};
	auto CurrentHUD		= HUD().GetUI()->UIMainIngameWnd;

	if (CurrentHUD->IsHUDElementAllowed(eGear))
		sprintf_s(stmp, "%d", ThisGrenadeCount);
	else if (CurrentHUD->IsHUDElementAllowed(eActiveItem))
		sprintf_s(stmp, "");

	str_count = stmp;
	icon_sect_name = cNameSect().c_str();
}

#include "ai_object_location.h"
void CGrenade::Contact(CPhysicsShellHolder* obj) {
	inherited::Contact(obj);
	if (Initiator() == u16(-1) || !b_impact_fuze) return;
	if (m_dwDestroyTime <= Level().timeServer()) {
		VERIFY(!m_pCurrentInventory);
		Destroy();
		return;
	}
	//recreate usable grenade
	Fvector pos{ Position() };
	u32 lvid = UsedAI_Locations() ? ai_location().level_vertex_id() : ai().level_graph().vertex(pos);
	CSE_Abstract* object = Level().spawn_item(cNameSect().c_str(), pos, lvid, 0xffff, true);
	NET_Packet P;
	object->Spawn_Write(P, TRUE);
	Level().Send(P, net_flags(TRUE));
	F_entity_Destroy(object);
	DestroyObject();
}