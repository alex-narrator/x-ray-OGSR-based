#include "stdafx.h"
#include "UICarBodyWnd.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../HUDManager.h"
#include "../level.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIFrameWindow.h"
#include "UIItemInfo.h"
#include "UIPropertiesBox.h"
#include "../ai/monsters/BaseMonster/base_monster.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "../WeaponMagazined.h"
#include "../WeaponMagazinedWGrenade.h"
#include "../Actor.h"
#include "../eatable_item.h"
#include "../alife_registry_wrappers.h"
#include "UI3tButton.h"
#include "UIListBoxItem.h"
#include "../InventoryBox.h"
#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../xr_3da/xr_input.h"

#include "Artifact.h"
#include "CustomOutfit.h"
#include "Warbelt.h"
#include "Backpack.h"
#include "WeaponKnife.h"
#include "string_table.h"

constexpr auto CAR_BODY_XML		= "carbody_new.xml";
constexpr auto CARBODY_ITEM_XML = "carbody_item.xml";

void move_item (u16 from_id, u16 to_id, u16 what_id);

CUICarBodyWnd::CUICarBodyWnd()
{
	Init				();
	Hide				();
	m_b_need_update		= false;
}

CUICarBodyWnd::~CUICarBodyWnd()
{
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);
}

void CUICarBodyWnd::Init()
{
	CUIXml						uiXml;
	uiXml.Init					(CONFIG_PATH, UI_PATH, CAR_BODY_XML);
	
	CUIXmlInit					xml_init;

	xml_init.InitWindow			(uiXml, "main", 0, this);

	m_pUIStaticTop				= xr_new<CUIStatic>(); m_pUIStaticTop->SetAutoDelete(true);
	AttachChild					(m_pUIStaticTop);
	xml_init.InitStatic			(uiXml, "top_background", 0, m_pUIStaticTop);


	m_pUIStaticBottom			= xr_new<CUIStatic>(); m_pUIStaticBottom->SetAutoDelete(true);
	AttachChild					(m_pUIStaticBottom);
	xml_init.InitStatic			(uiXml, "bottom_background", 0, m_pUIStaticBottom);

	m_pUIOurIcon				= xr_new<CUIStatic>(); m_pUIOurIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOurIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 0, m_pUIOurIcon);

	m_pUIOthersIcon				= xr_new<CUIStatic>(); m_pUIOthersIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOthersIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 1, m_pUIOthersIcon);


	m_pUICharacterInfoLeft		= xr_new<CUICharacterInfo>(); m_pUICharacterInfoLeft->SetAutoDelete(true);
	m_pUIOurIcon->AttachChild	(m_pUICharacterInfoLeft);
	m_pUICharacterInfoLeft->Init(0,0, m_pUIOurIcon->GetWidth(), m_pUIOurIcon->GetHeight(), "trade_character.xml");


	m_pUICharacterInfoRight			= xr_new<CUICharacterInfo>(); m_pUICharacterInfoRight->SetAutoDelete(true);
	m_pUIOthersIcon->AttachChild	(m_pUICharacterInfoRight);
	m_pUICharacterInfoRight->Init	(0,0, m_pUIOthersIcon->GetWidth(), m_pUIOthersIcon->GetHeight(), "trade_character.xml");

	m_pUIOurBagWnd					= xr_new<CUIStatic>(); m_pUIOurBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOurBagWnd);
	xml_init.InitStatic				(uiXml, "our_bag_static", 0, m_pUIOurBagWnd);

	m_pUIOurWeightWnd				= xr_new<CUIStatic>(); m_pUIOurWeightWnd->SetAutoDelete(true);
	m_pUIOurBagWnd->AttachChild		(m_pUIOurWeightWnd);
	xml_init.InitStatic				(uiXml, "our_weight_static", 0, m_pUIOurWeightWnd);

	m_pUIOurVolWnd					= xr_new<CUIStatic>(); m_pUIOurVolWnd->SetAutoDelete(true);
	m_pUIOurBagWnd->AttachChild		(m_pUIOurVolWnd);
	xml_init.InitStatic				(uiXml, "our_vol_static", 0, m_pUIOurVolWnd);

	m_pUIOthersBagWnd				= xr_new<CUIStatic>(); m_pUIOthersBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOthersBagWnd);
	xml_init.InitStatic				(uiXml, "others_bag_static", 0, m_pUIOthersBagWnd);

	m_pUIOthersVolWnd				= xr_new<CUIStatic>(); m_pUIOthersVolWnd->SetAutoDelete(true);
	m_pUIOthersBagWnd->AttachChild	(m_pUIOthersVolWnd);
	xml_init.InitStatic				(uiXml, "other_vol_static", 0, m_pUIOthersVolWnd);

	m_pUIOurBagList					= xr_new<CUIDragDropListEx>(); m_pUIOurBagList->SetAutoDelete(true);
	m_pUIOurBagWnd->AttachChild		(m_pUIOurBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_our", 0, m_pUIOurBagList);

	m_pUIOthersBagList				= xr_new<CUIDragDropListEx>(); m_pUIOthersBagList->SetAutoDelete(true);
	m_pUIOthersBagWnd->AttachChild	(m_pUIOthersBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_other", 0, m_pUIOthersBagList);


	//информация о предмете
	m_pUIDescWnd					= xr_new<CUIFrameWindow>(); m_pUIDescWnd->SetAutoDelete(true);
	AttachChild						(m_pUIDescWnd);
	xml_init.InitFrameWindow		(uiXml, "frame_window", 0, m_pUIDescWnd);

	m_pUIStaticDesc					= xr_new<CUIStatic>(); m_pUIStaticDesc->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIStaticDesc);
	xml_init.InitStatic				(uiXml, "descr_static", 0, m_pUIStaticDesc);

	m_pUIItemInfo					= xr_new<CUIItemInfo>(); m_pUIItemInfo->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIItemInfo);
	m_pUIItemInfo->Init				(0,0, m_pUIDescWnd->GetWidth(), m_pUIDescWnd->GetHeight(), CARBODY_ITEM_XML);


	xml_init.InitAutoStatic			(uiXml, "auto_static", this);

	m_pUIPropertiesBox				= xr_new<CUIPropertiesBox>(); m_pUIPropertiesBox->SetAutoDelete(true);
	AttachChild						(m_pUIPropertiesBox);
	m_pUIPropertiesBox->Init		(0,0,300,300);
	m_pUIPropertiesBox->Hide		();

	SetCurrentItem					(NULL);
	m_pUIStaticDesc->SetText		(NULL);

	m_pUITakeAll					= xr_new<CUI3tButton>(); m_pUITakeAll->SetAutoDelete(true);
	AttachChild						(m_pUITakeAll);
	xml_init.Init3tButton			(uiXml, "take_all_btn", 0, m_pUITakeAll);

	m_pUIExitButton					= xr_new<CUI3tButton>(); m_pUIExitButton->SetAutoDelete(true);
	AttachChild						(m_pUIExitButton);
	xml_init.Init3tButton			(uiXml, "exit_button", 0, m_pUIExitButton);

	m_pUIRepackAmmoButton			= xr_new<CUI3tButton>(); m_pUIRepackAmmoButton->SetAutoDelete(true);
	AttachChild						(m_pUIRepackAmmoButton);
	xml_init.Init3tButton			(uiXml, "repack_ammo_button", 0, m_pUIRepackAmmoButton);

	m_pUIMoveAllFromRuckButton		= xr_new<CUI3tButton>(); m_pUIMoveAllFromRuckButton->SetAutoDelete(true);
	AttachChild						(m_pUIMoveAllFromRuckButton);
	xml_init.Init3tButton			(uiXml, "move_all_from_ruck_button", 0, m_pUIMoveAllFromRuckButton);

	BindDragDropListEnents			(m_pUIOurBagList);
	BindDragDropListEnents			(m_pUIOthersBagList);

	//Load sounds
	if (uiXml.NavigateToNode("action_sounds", 0))
	{
		XML_NODE* stored_root = uiXml.GetLocalRoot();
		uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

		::Sound->create		(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0, NULL), st_Effect, sg_SourceType);
		::Sound->create		(sounds[eInvSndClose],		uiXml.Read("snd_close",			0, NULL), st_Effect, sg_SourceType);
		::Sound->create		(sounds[eInvProperties],	uiXml.Read("snd_properties",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create		(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create		(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create		(sounds[eInvMagLoad],		uiXml.Read("snd_mag_load",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create		(sounds[eInvMagUnload],		uiXml.Read("snd_mag_unload",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create		(sounds[eInvMoveItem],		uiXml.Read("snd_move_item",		0, NULL), st_Effect, sg_SourceType);

		uiXml.SetLocalRoot(stored_root);
	}
}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, IInventoryBox* pInvBox)
{
    m_pActorInventoryOwner									= pOur;
	m_pOtherInventoryOwner									= NULL;
	m_pOtherInventoryBox									= pInvBox;
	m_pOtherInventoryBox->m_in_use							= true;

	m_pActorGO = smart_cast<CGameObject*>(m_pActorInventoryOwner);
	m_pOtherGO = smart_cast<CGameObject*>(m_pOtherInventoryBox);

	m_pUICharacterInfoLeft->InitCharacter			(m_pActorGO->ID());
	m_pUIOthersIcon->Show							(false);
	m_pUICharacterInfoRight->ClearInfo				();
	m_pUIPropertiesBox->Hide						();
	EnableAll										();
	UpdateLists										();

	if (auto obj = smart_cast<CInventoryBox*>(pInvBox))
	{
		obj->callback(GameObject::eOnInvBoxOpen)();
	}
}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{
    m_pActorInventoryOwner									= pOur;
	m_pOtherInventoryOwner									= pOthers;
	m_pOtherInventoryBox									= NULL;

	m_pActorGO = smart_cast<CGameObject*>(m_pActorInventoryOwner);
	m_pOtherGO = smart_cast<CGameObject*>(m_pOtherInventoryOwner);
	
	u16 our_id										= m_pActorGO->ID();
	u16 other_id									= m_pOtherGO->ID();

	m_pUICharacterInfoLeft->InitCharacter			(our_id);
	m_pUIOthersIcon->Show							(true);
	
	CBaseMonster *monster = NULL;
	if(m_pOtherInventoryOwner) {
		monster										= smart_cast<CBaseMonster *>(m_pOtherInventoryOwner);
		if (monster || m_pOtherInventoryOwner->use_simplified_visual() ) 
		{
			m_pUICharacterInfoRight->ClearInfo		();
			if(monster)
			{
				shared_str monster_tex_name = pSettings->r_string(monster->cNameSect(),"icon");
				m_pUICharacterInfoRight->UIIcon().InitTexture(monster_tex_name.c_str());
				m_pUICharacterInfoRight->UIIcon().SetStretchTexture(true);
			}
		}else 
		{
			m_pUICharacterInfoRight->InitCharacter	(other_id);
		}
	}

	m_pUIPropertiesBox->Hide						();
	EnableAll										();
	UpdateLists										();

	if(!monster){
		CInfoPortionWrapper	*known_info_registry	= xr_new<CInfoPortionWrapper>();
		known_info_registry->registry().init		(other_id);
		KNOWN_INFO_VECTOR& known_info				= known_info_registry->registry().objects();

		KNOWN_INFO_VECTOR_IT it = known_info.begin();
		for(int i=0;it!=known_info.end();++it,++i){
			NET_Packet		P;
			CGameObject::u_EventGen		(P,GE_INFO_TRANSFER, our_id);
			P.w_u16						(0);//not used
			P.w_stringZ					((*it).info_id);			//сообщение
			P.w_u8						(1);						//добавление сообщения
			CGameObject::u_EventSend	(P);
		}
		known_info.clear	();
		xr_delete			(known_info_registry);
	}
}  

void CUICarBodyWnd::UpdateLists_delayed()
{
		m_b_need_update = true;
}

void CUICarBodyWnd::Hide()
{
	InventoryUtilities::SendInfoToActor			("ui_car_body_hide");
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);
	inherited::Hide								();
	if(m_pOtherInventoryBox)
		m_pOtherInventoryBox->m_in_use				= false;

	if (Actor()){
		if (g_eFreeHands != eFreeHandsOff){
			Actor()->SetWeaponHideState(INV_STATE_INV_WND, false);
		}
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) {
			Actor()->SetRuckAmmoPlacement(false); //сбросим флаг перезарядки из рюкзака
		}
	}

	PlaySnd(eInvSndClose);
}

void CUICarBodyWnd::UpdateLists()
{
	TIItemContainer								ruck_list;
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);

	ruck_list.clear								();
	m_pActorInventoryOwner->inventory().AddAvailableItems	(ruck_list, true);
	std::sort									(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	//Наш рюкзак
	for(const auto& inv_item : ruck_list)
	{
		CUICellItem* itm = create_cell_item(inv_item);
		if (inv_item->m_highlight_equipped)
		{
			itm->m_select_equipped = true;
			itm->SetColor(reinterpret_cast<CInventoryItem*>(itm->m_pData)->ClrEquipped);
		}
		m_pUIOurBagList->SetItem(itm);
	}


	ruck_list.clear									();
	if(m_pOtherInventoryOwner)
		m_pOtherInventoryOwner->inventory().AddAvailableItems	(ruck_list, false);
	else
		m_pOtherInventoryBox->AddAvailableItems			(ruck_list);

	std::sort										(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	//Чужой рюкзак
	for (const auto& inv_item : ruck_list)
	{
		CUICellItem* itm = create_cell_item(inv_item);
		m_pUIOthersBagList->SetItem(itm);
	}

	UpdateWeightVolume();
	m_b_need_update									= false;
}

void CUICarBodyWnd::ActivatePropertiesBox()
{
	m_pUIPropertiesBox->RemoveAll();

	auto pWeapon		= smart_cast<CWeapon*>		(CurrentIItem());
	auto pAmmo			= smart_cast<CWeaponAmmo*>	(CurrentIItem());
	auto pEatableItem	= smart_cast<CEatableItem*> (CurrentIItem());
	auto pWarbelt		= smart_cast<CWarbelt*>		(CurrentIItem());
	auto pBackpack		= smart_cast<CBackpack*>	(CurrentIItem());

	bool b_actor_inv = CurrentItem()->OwnerList() == m_pUIOurBagList;
	auto inv = &m_pActorInventoryOwner->inventory();
	string1024			temp;

	bool b_show = false;

	LPCSTR _action = nullptr;

	if (pAmmo){
		LPCSTR _ammo_sect;

		if (pAmmo->IsBoxReloadable()){
			//unload AmmoBox
			m_pUIPropertiesBox->AddItem("st_unload_magazine", NULL, INVENTORY_UNLOAD_AMMO_BOX);
			b_show = true;
			//reload AmmoBox
			if (pAmmo->m_boxCurr < pAmmo->m_boxSize && b_actor_inv){
				if (inv->GetAmmo(*pAmmo->m_ammoSect, true)){
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoSect, "inv_name_short")));
					_ammo_sect = *pAmmo->m_ammoSect;
					m_pUIPropertiesBox->AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
		else if (pAmmo->IsBoxReloadableEmpty() && b_actor_inv){
			for (u8 i = 0; i < pAmmo->m_ammoTypes.size(); ++i){
				if (inv->GetAmmo(*pAmmo->m_ammoTypes[i], true)){
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoTypes[i], "inv_name_short")));
					_ammo_sect = *pAmmo->m_ammoTypes[i];
					m_pUIPropertiesBox->AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
	}

	if (pWeapon){
		if (inv->InSlot(pWeapon)){
			for (u32 i = 0; i < pWeapon->m_ammoTypes.size(); ++i){
				if (pWeapon->TryToGetAmmo(i)){
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pWeapon->m_ammoTypes[i].c_str(), "inv_name_short")));
					m_pUIPropertiesBox->AddItem(temp, (void*)(__int64)i, INVENTORY_RELOAD_MAGAZINE);
					b_show = true;
				}
			}
		}
		//
		if (pWeapon->GrenadeLauncherAttachable() && pWeapon->IsGrenadeLauncherAttached()){
			sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetGrenadeLauncherName().c_str(), "inv_name_short")).c_str());
			m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetGrenadeLauncherName().c_str(), INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->ScopeAttachable() && pWeapon->IsScopeAttached()){
			sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetScopeName().c_str(), "inv_name_short")).c_str());
			m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetScopeName().c_str(), INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->SilencerAttachable() && pWeapon->IsSilencerAttached()){
			sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetSilencerName().c_str(), "inv_name_short")).c_str());
			m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetSilencerName().c_str(), INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->LaserAttachable() && pWeapon->IsLaserAttached()){
			sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetLaserName().c_str(), "inv_name_short")).c_str());
			m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetLaserName().c_str(), INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->FlashlightAttachable() && pWeapon->IsFlashlightAttached()){
			sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetFlashlightName().c_str(), "inv_name_short")).c_str());
			m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetFlashlightName().c_str(), INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (smart_cast<CWeaponMagazined*>(pWeapon)){
			auto WpnMagazWgl = smart_cast<CWeaponMagazinedWGrenade*>(pWeapon);
			bool b = pWeapon->GetAmmoElapsed() > 0 
				|| WpnMagazWgl && !WpnMagazWgl->m_magazine2.empty() 
				|| smart_cast<CWeaponMagazined*>(pWeapon)->IsMagazineAttached();

			if (!b){
				CUICellItem* itm = CurrentItem();
				for (u32 i = 0; i < itm->ChildsCount(); ++i){
					auto pWeaponChild = static_cast<CWeaponMagazined*>(itm->Child(i)->m_pData);
					auto WpnMagazWglChild = smart_cast<CWeaponMagazinedWGrenade*>(pWeaponChild);
					if (pWeaponChild->GetAmmoElapsed() > 0 
						|| WpnMagazWglChild && !WpnMagazWglChild->m_magazine2.empty() 
						|| pWeaponChild->IsMagazineAttached())
					{
						b = true;
						break;
					}
				}
			}

			if (b) {
				m_pUIPropertiesBox->AddItem("st_unload_magazine", NULL, INVENTORY_UNLOAD_MAGAZINE);
				b_show = true;
			}
		}
	}

	if (pEatableItem)
		_action = pEatableItem->GetUseMenuTip();
	if (_action) {
		m_pUIPropertiesBox->AddItem(_action, NULL, INVENTORY_EAT_ACTION);
		b_show = true;
	}

	bool hasMany = CurrentItem()->ChildsCount() > 0;
	bool can_move_stack = CanTakeStack(CurrentItem(), b_actor_inv ? m_pOtherGO : m_pActorGO);

	if ((pWarbelt || pBackpack) && b_actor_inv && inv->InSlot(CurrentIItem())) {
		bool can_move_content = true;
		if (pWarbelt) {
			for (const auto& belt_item : inv->m_belt) {
				if (!CanMoveToOther(belt_item, m_pOtherGO))
					can_move_content = false;
			}
		}
		if (pBackpack) {
			for (const auto& ruck_item : inv->m_ruck) {
				if (!CanMoveToOther(ruck_item, m_pOtherGO))
					can_move_content = false;
			}
		}
		if (can_move_content) {
			m_pUIPropertiesBox->AddItem("st_move_with_content", NULL, INVENTORY_MOVE_WITH_CONTENT);
			b_show = true;
		}
	}
	else if (CanMoveToOther(CurrentIItem(), b_actor_inv ? m_pOtherGO : m_pActorGO)) {
		m_pUIPropertiesBox->AddItem("st_move", NULL, INVENTORY_MOVE_ACTION);
		if (hasMany && can_move_stack)
			m_pUIPropertiesBox->AddItem("st_move_all", (void*)33, INVENTORY_MOVE_ACTION);
		b_show = true;
	}

	if (CheckMonsterAndKnife() || b_actor_inv) {
		m_pUIPropertiesBox->AddItem("st_drop", NULL, INVENTORY_DROP_ACTION);
		if (hasMany)
			m_pUIPropertiesBox->AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
		b_show = true;
	}


	if (b_show) {
		m_pUIPropertiesBox->AutoUpdateSize();
		m_pUIPropertiesBox->BringAllToTop();

		Fvector2						cursor_pos;
		Frect							vis_rect;

		GetAbsoluteRect(vis_rect);
		cursor_pos = GetUICursor()->GetCursorPosition();
		cursor_pos.sub(vis_rect.lt);
		m_pUIPropertiesBox->Show(vis_rect, cursor_pos);

		PlaySnd(eInvProperties);
	}
}

void CUICarBodyWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if (BUTTON_CLICKED == msg){
		if (m_pUITakeAll == pWnd){
			TakeAll();
		}
		else if (m_pUIExitButton == pWnd){
			GetHolder()->StartStopMenu(this, true);
		}
		else if (m_pUIRepackAmmoButton == pWnd){
			Actor()->RepackAmmo();
			UpdateLists_delayed();
		}
		else if (m_pUIMoveAllFromRuckButton == pWnd){
			MoveItemWithContent(NULL, BACKPACK_SLOT);
		}
	}
	else if(pWnd == m_pUIPropertiesBox && msg == PROPERTY_CLICKED){
		if(m_pUIPropertiesBox->GetClickedItem()){
			switch(m_pUIPropertiesBox->GetClickedItem()->GetTAG()){
				case INVENTORY_EAT_ACTION:	//съесть объект
					EatItem();
					break;
				case INVENTORY_RELOAD_MAGAZINE:
				{
					void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
					auto Wpn = smart_cast<CWeapon*>(CurrentIItem());
					Wpn->m_set_next_ammoType_on_reload = (u32)(__int64)d;
					Wpn->ReloadWeapon();
				}break;
				case INVENTORY_UNLOAD_MAGAZINE:
				{
					auto ProcessUnload = [](void* pWpn) {
						auto WpnMagaz = static_cast<CWeaponMagazined*>(pWpn);
						WpnMagaz->UnloadMagazine();
						WpnMagaz->PullShutter();
						if (auto WpnMagazWgl = smart_cast<CWeaponMagazinedWGrenade*>(WpnMagaz)){
							if (WpnMagazWgl->IsGrenadeLauncherAttached()){
								WpnMagazWgl->PerformSwitchGL();
								WpnMagazWgl->UnloadMagazine();
								WpnMagazWgl->PerformSwitchGL();
							}
						}
					};

					auto itm = CurrentItem();
					ProcessUnload(itm->m_pData);

					for (u32 i = 0; i < itm->ChildsCount(); ++i){
						auto child_itm = itm->Child(i);
						ProcessUnload(child_itm->m_pData);
					}
				}break;
				case INVENTORY_RELOAD_AMMO_BOX:
				{
					//Msg("load %s to %s", (LPCSTR)UIPropertiesBox.GetClickedItem()->GetData(), pAmmo->cNameSect().c_str());
					(smart_cast<CWeaponAmmo*>(CurrentIItem()))->ReloadBox((LPCSTR)m_pUIPropertiesBox->GetClickedItem()->GetData());
					//SetCurrentItem(NULL);
					PlaySnd(eInvMagLoad);
				}break;
				case INVENTORY_UNLOAD_AMMO_BOX:
				{
					auto ProcessUnload = [](void* pAmmo) {
						auto AmmoBox = static_cast<CWeaponAmmo*>(pAmmo);
						AmmoBox->UnloadBox();
					};

					auto itm = CurrentItem();
					ProcessUnload(itm->m_pData);
					for (u32 i = 0; i < itm->ChildsCount(); ++i){
						auto child_itm = itm->Child(i);
						ProcessUnload(child_itm->m_pData);
					}
					//SetCurrentItem(NULL);
					PlaySnd(eInvMagUnload);
				}break;
				case INVENTORY_DETACH_ADDON: {
					CurrentIItem()->cast_weapon()->Detach((const char*)(m_pUIPropertiesBox->GetClickedItem()->GetData()), true);
					PlaySnd(eInvDetachAddon);
				}break;
				case INVENTORY_DROP_ACTION:
				{
					void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
					bool b_all = (d == (void*)33);

					DropItems(b_all);
				}break;
				case INVENTORY_MOVE_ACTION:
				{
					void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
					bool b_all = (d == (void*)33);

					MoveItems(CurrentItem(), b_all);
				}break;
				case INVENTORY_MOVE_WITH_CONTENT:
				{
					auto iitem = CurrentIItem();
					u32 slot = iitem->GetSlot();
					MoveItemWithContent(CurrentItem(), slot);
				}
			}

			// refresh if nessesary
			switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{
				case INVENTORY_EAT_ACTION:
				case INVENTORY_RELOAD_MAGAZINE:
				case INVENTORY_UNLOAD_MAGAZINE:
				case INVENTORY_RELOAD_AMMO_BOX:
				case INVENTORY_UNLOAD_AMMO_BOX:
				case INVENTORY_DETACH_ADDON:
				{
					SetCurrentItem(nullptr);
					UpdateLists_delayed();
				}break;
			}
		}
	}

	inherited::SendMessage(pWnd, msg, pData);
}

void CUICarBodyWnd::Draw()
{
	inherited::Draw	();
}

void CUICarBodyWnd::Update()
{
	if (m_b_need_update ||
		m_pActorInventoryOwner->inventory().ModifyFrame() == Device.dwFrame ||
		(m_pOtherInventoryOwner&&m_pOtherInventoryOwner->inventory().ModifyFrame() == Device.dwFrame))
	{
		if (m_pUIOurBagList && m_pUIOthersBagList)
		{
			int our_scroll = m_pUIOurBagList->ScrollPos();
			int other_scroll = m_pUIOthersBagList->ScrollPos();
			UpdateLists();
			m_pUIOurBagList->SetScrollPos(our_scroll);
			m_pUIOthersBagList->SetScrollPos(other_scroll);
		}
	}

	if (m_pOtherGO && m_pActorGO->Position().distance_to(m_pOtherGO->Position()) - m_pOtherGO->Radius() - m_pActorGO->Radius() > m_pActorInventoryOwner->inventory().GetTakeDist() + 0.5f)
	{
		GetHolder()->StartStopMenu(this, true);
	}
	CheckForcedWeightVolumeUpdate();
	inherited::Update();
}


void CUICarBodyWnd::Show() 
{ 
	InventoryUtilities::SendInfoToActor		("ui_car_body");
	inherited::Show							();
	SetCurrentItem							(NULL);
	UpdateWeightVolume						();

	if (Actor()){
		if (g_eFreeHands != eFreeHandsOff) {
			Actor()->SetWeaponHideState(INV_STATE_INV_WND, true);
		}
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) {
			Actor()->SetRuckAmmoPlacement(true); //установим флаг перезарядки из рюкзака
		}
	}

	PlaySnd(eInvSndOpen);
}

void CUICarBodyWnd::DisableAll()
{
	m_pUIOurBagWnd->Enable			(false);
	m_pUIOthersBagWnd->Enable		(false);
}

void CUICarBodyWnd::EnableAll()
{
	m_pUIOurBagWnd->Enable			(true);
	m_pUIOthersBagWnd->Enable		(true);
}

CUICellItem* CUICarBodyWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUICarBodyWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUICarBodyWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem		= itm;
	m_pUIItemInfo->InitItem(CurrentIItem());

	if (m_pCurrentCellItem) {
		m_pCurrentCellItem->m_select_armament = true;
		auto script_obj = CurrentIItem()->object().lua_game_object();
		g_actor->callback(GameObject::eCellItemSelect)(script_obj);
	}
}

void CUICarBodyWnd::TakeAll()
{
	u32 cnt				= m_pUIOthersBagList->ItemsCount();
	for(u32 i=0; i<cnt; ++i){
		CUICellItem*	ci = m_pUIOthersBagList->GetItemIdx(i);
		if (!CanTakeStack(ci, m_pActorGO)) continue;
		for (u32 j = 0; j < ci->ChildsCount(); ++j) {
			PIItem _itm = (PIItem)(ci->Child(j)->m_pData);
			TransferItem(_itm, m_pOtherGO, m_pActorGO);
		}
		PIItem itm = (PIItem)(ci->m_pData);
		TransferItem(itm, m_pOtherGO, m_pActorGO);
	}
	PlaySnd(eInvMoveItem);
}

void CUICarBodyWnd::MoveItems(CUICellItem* itm, bool b_all)
{
	CUIDragDropListEx* owner_list = itm->OwnerList();

	bool actor_to_other = owner_list != m_pUIOthersBagList;

	if (!CanMoveToOther(CurrentIItem(), actor_to_other ? m_pOtherGO : m_pActorGO)) return;

	if (actor_to_other){ //actor -> other
		if (b_all) {
			if (!CanTakeStack(itm, actor_to_other ? m_pOtherGO : m_pActorGO)) return;
			for (u32 j = 0; j < CurrentItem()->ChildsCount(); ++j) {
				PIItem _itm = (PIItem)(CurrentItem()->Child(j)->m_pData);
				TransferItem(_itm, m_pActorGO, m_pOtherGO);
			}
		}
		PIItem itm = (PIItem)(CurrentItem()->m_pData);
		TransferItem(itm, m_pActorGO, m_pOtherGO);
	}
	else{ // other -> actor
		if (b_all) {
			if (!CanTakeStack(itm, actor_to_other ? m_pOtherGO : m_pActorGO)) return;
			for (u32 j = 0; j < CurrentItem()->ChildsCount(); ++j) {
				PIItem _itm = (PIItem)(CurrentItem()->Child(j)->m_pData);
				TransferItem(_itm, m_pOtherGO, m_pActorGO);
			}
		}
		PIItem itm = (PIItem)(CurrentItem()->m_pData);
		TransferItem(itm, m_pOtherGO, m_pActorGO);
	}

	PlaySnd(eInvMoveItem);

	owner_list->RemoveItem(itm, true);

	SetCurrentItem(NULL);
}

void CUICarBodyWnd::SendEvent_Item_Drop(PIItem	pItem)
{
	pItem->OnMoveOut(pItem->m_eItemPlace);
	pItem->SetDropManual(TRUE);

	if (OnClient())
	{
		NET_Packet P;
		pItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pItem->object().H_Parent()->ID());
		P.w_u16(pItem->object().ID());
		pItem->object().u_EventSend(P);
	}
}

void CUICarBodyWnd::DropItems(bool b_all)
{
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor){
		return;
	}

	CUICellItem* ci = CurrentItem();
	if (!ci) {
		return;
	}

	CUIDragDropListEx* old_owner = ci->OwnerList();

	if (b_all){
		u32 cnt = ci->ChildsCount();
		for (u32 i = 0; i < cnt; ++i){
			CUICellItem* itm = ci->PopChild();
			PIItem			iitm = (PIItem)itm->m_pData;
			SendEvent_Item_Drop(iitm);
		}
	}
	TryPlayStabbing(CurrentIItem(), m_pOtherGO);
	SendEvent_Item_Drop(CurrentIItem());
	old_owner->RemoveItem(ci, b_all);

	SetCurrentItem(NULL);

	UpdateWeightVolume();

	PlaySnd(eInvDropItem);
}

#include "../xr_level_controller.h"

bool CUICarBodyWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if (m_b_need_update)
		return true;

	if (keyboard_action == WINDOW_KEY_PRESSED){
		if (m_pUIPropertiesBox->GetVisible())
			m_pUIPropertiesBox->OnKeyboard(dik, keyboard_action);
	}

	if(keyboard_action==WINDOW_KEY_PRESSED && is_binded(kUSE, dik)) {
			GetHolder()->StartStopMenu(this,true);
			return true;
	}

	if (inherited::OnKeyboard(dik, keyboard_action))return true;

	return false;
}

bool CUICarBodyWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if (m_b_need_update)
		return true;

	if (mouse_action == WINDOW_RBUTTON_DOWN){
		if (m_pUIPropertiesBox->IsShown()){
			m_pUIPropertiesBox->Hide();
			return						true;
		}
	}

	if (m_pUIPropertiesBox->IsShown()){
		switch (mouse_action)
		{
		case WINDOW_MOUSE_WHEEL_DOWN:
		case WINDOW_MOUSE_WHEEL_UP:
			return true;
			break;
		}
	}

	if (CUIWindow::OnMouse(x, y, mouse_action)){
		return  true;
	}

	return false;
}

void CUICarBodyWnd::EatItem()
{
	CActor *pActor				= smart_cast<CActor*>(Level().CurrentEntity());
	if(!pActor)					return;
	m_pActorInventoryOwner->inventory().Eat(CurrentIItem(), m_pActorInventoryOwner);
	SetCurrentItem(NULL);
}

bool CUICarBodyWnd::OnItemStartDrag(CUICellItem* itm)
{
	return	false; //default behaviour
}

bool CUICarBodyWnd::OnItemDrop(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= CUIDragDropListEx::m_drag_item->BackList();
	
	if(old_owner==new_owner || !old_owner || !new_owner)
		return false;

	bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
	MoveItems(itm, b_all);

	return true;
}

bool CUICarBodyWnd::OnItemDbClick(CUICellItem* itm)
{
	bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
	MoveItems(itm, b_all);
	return true;
}

bool CUICarBodyWnd::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem(itm);
	itm->ColorizeItems( { m_pUIOurBagList, m_pUIOthersBagList } );
	return false;
}

bool CUICarBodyWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox		();
	return						false;
}

void move_item (u16 from_id, u16 to_id, u16 what_id)
{
	NET_Packet P;
	CGameObject::u_EventGen					(	P,
												GE_TRANSFER_REJECT,
												from_id
											);

	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

	//другому инвентарю - взять вещь 
	CGameObject::u_EventGen					(	P,
												GE_TRANSFER_TAKE,
												to_id
											);
	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

}

bool CUICarBodyWnd::TransferItem(PIItem itm, CGameObject* owner_from, CGameObject* owner_to)
{
	if (!CanMoveToOther(itm, owner_to)) return false;

	TryPlayStabbing(itm, owner_from);
	//
	if (smart_cast<CInventoryOwner*>(owner_from))
		itm->OnMoveOut(itm->m_eItemPlace);

	move_item(owner_from->ID(), owner_to->ID(), itm->object().ID());

	/*Msg("~ item [%s] transfered from [%s] to [%s]", itm->Name(), owner_from->Name(), owner_to->Name());*/

	return true;
}

void CUICarBodyWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemDrop);
	lst->m_f_item_start_drag = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemStartDrag);
	lst->m_f_item_db_click = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemDbClick);
	lst->m_f_item_selected = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemSelected);
	lst->m_f_item_rbutton_click = fastdelegate::MakeDelegate(this, &CUICarBodyWnd::OnItemRButtonClick);
}

void CUICarBodyWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
		sounds[a].play(NULL, sm_2D);
}

void CUICarBodyWnd::MoveItemWithContent(CUICellItem* itm, u32 slot)
{
	EItemPlace move_from = eItemPlaceUndefined;

	switch (slot)
	{
	case WARBELT_SLOT:
		move_from = eItemPlaceBelt;
		break;
	case BACKPACK_SLOT:
		move_from = eItemPlaceRuck;
		break;
	}

	auto inv_all = m_pActorInventoryOwner->inventory().m_all;

	for (const auto& item : inv_all){
		if (item->m_eItemPlace == move_from){
			TransferItem(item, m_pActorGO, m_pOtherGO);
		}
	}

	if (itm) MoveItems(itm, false);
}

bool CUICarBodyWnd::CanMoveToOther(PIItem pItem, CGameObject* owner_to) const {
	if (smart_cast<CBaseMonster*>(owner_to)) return false;
	if (!CheckMonsterAndKnife()) return false;
	bool can_move{};
	auto owner	= smart_cast<CInventoryOwner*>(owner_to);
	auto box	= smart_cast<IInventoryBox*>(owner_to);
	if (owner) {
		can_move = owner->inventory().CanTakeItem(pItem);
	}
	else if (box) {
		can_move = box->CanTakeItem(pItem);
	}
	return can_move;
}

void CUICarBodyWnd::UpdateWeightVolume(bool only_for_actor) {
	InventoryUtilities::UpdateWeight(*m_pUIOurWeightWnd, true);
	InventoryUtilities::UpdateVolume(m_pActorGO, *m_pUIOurVolWnd, true);
	if (only_for_actor) return;
	InventoryUtilities::UpdateVolume(m_pOtherGO, *m_pUIOthersVolWnd, true);
}

float CUICarBodyWnd::GetStackVolume(CUICellItem* ci) const {
	float total_vol{};
	if (!ci) return total_vol;
	u32 cnt = ci->ChildsCount();
	PIItem item = (PIItem)ci->m_pData;
	total_vol += item->Volume();
	for (u32 i = 0; i < cnt; ++i) {
		CUICellItem* _ci = ci->Child(i);
		PIItem		_item = (PIItem)_ci->m_pData;
		total_vol += _item->Volume();
	}
	return total_vol;
}

bool CUICarBodyWnd::CanTakeStack(CUICellItem* ci, CGameObject* owner_to) const {
	bool can_take{};
	float stack_volume = GetStackVolume(ci);
	auto owner = smart_cast<CInventoryOwner*>(owner_to);
	auto box = smart_cast<IInventoryBox*>(owner_to);
	if (owner) {
		can_take = owner->IsVolumeUnlimited() ? true : owner->GetCarryVolume() + stack_volume <= owner->MaxCarryVolume();
	}
	else if (box) {
		can_take = box->IsVolumeUnlimited() ? true : box->GetCarryVolume() + stack_volume <= box->MaxCarryVolume();
	}
	/*Msg("%s: [%s]", __FUNCTION__, can_take ? "true" : "false");*/
	return can_take;
}

void CUICarBodyWnd::CheckForcedWeightVolumeUpdate() {
	bool need_update{};
	auto place_to_search = psActorFlags.test(AF_ARTEFACTS_FROM_ALL) ? m_pActorInventoryOwner->inventory().m_all : m_pActorInventoryOwner->inventory().m_belt;
	for (const auto& item : place_to_search) {
		auto artefact = smart_cast<CArtefact*>(item);
		if (artefact && !fis_zero(artefact->m_fTTLOnDecrease) && !fis_zero(artefact->GetCondition()) &&
			(!fis_zero(artefact->GetItemEffect(CInventoryItem::eAdditionalWeight)) || !fis_zero(artefact->GetItemEffect(CInventoryItem::eAdditionalVolume)))) {
			need_update = true;
			break;
		}
	}
	if (need_update)
		UpdateWeightVolume(true);
}

bool CUICarBodyWnd::CheckMonsterAndKnife() const {
	return !psActorFlags.test(AF_KNIFE_TO_CUT_PART)
		|| !smart_cast<CBaseMonster*>(m_pOtherInventoryOwner)
		|| smart_cast<CWeaponKnife*>(m_pActorInventoryOwner->inventory().ActiveItem());
}

void CUICarBodyWnd::TryPlayStabbing(PIItem itm, CGameObject* owner_from) {
	auto monster = smart_cast<CBaseMonster*>(owner_from);
	if (!monster) return;
	bool owner_is_monster = (itm->object().H_Parent() == owner_from);
	if (psActorFlags.test(AF_KNIFE_TO_CUT_PART) && monster && owner_is_monster) {
		auto knife = smart_cast<CWeaponKnife*>(m_pActorInventoryOwner->inventory().ActiveItem());
		if (knife) {
			knife->Fire2Start();                                         //нанесём удар ножом
			itm->ChangeCondition(-(1 - knife->GetCondition()));        //уменьшим Condition части монстра на величину износа ножа (1 - Knife->GetCondition())
			knife->ChangeCondition(-knife->GetCondDecPerShotOnHit() * monster->m_fSkinDensityK); //уменьшим Condition ножа износ за удар * коэф плотности кожи монстра
		}
	}
}