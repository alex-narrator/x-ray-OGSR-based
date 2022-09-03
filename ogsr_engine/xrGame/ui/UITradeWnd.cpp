#include "stdafx.h"
#include "UITradeWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "../Entity.h"
#include "../HUDManager.h"
#include "../WeaponAmmo.h"
#include "../Actor.h"
#include "../Trade.h"
#include "../UIGameSP.h"
#include "UIInventoryUtilities.h"
#include "../inventoryowner.h"
#include "../eatable_item.h"
#include "Weapon.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "../inventory.h"
#include "../level.h"
#include "../string_table.h"
#include "../character_info.h"
#include "UIMultiTextStatic.h"
#include "UI3tButton.h"
#include "UIItemInfo.h"

#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIPropertiesBox.h"
#include "UIListBoxItem.h"

#include "string_table.h"

#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../xr_3da/xr_input.h"


#define				TRADE_XML			"trade.xml"
#define				TRADE_CHARACTER_XML	"trade_character.xml"
#define				TRADE_ITEM_XML		"trade_item.xml"


struct CUITradeInternal{
	CUIStatic			UIStaticTop;
	CUIStatic			UIStaticBottom;

	CUIStatic			UIOurBagWnd;
	CUIStatic			UIOurMoneyStatic;
	CUIStatic			UIOthersBagWnd;
	CUIStatic			UIOtherMoneyStatic;
	CUIDragDropListEx	UIOurBagList;
	CUIDragDropListEx	UIOthersBagList;

	CUIStatic			UIOurTradeWnd;
	CUIStatic			UIOthersTradeWnd;
	CUIMultiTextStatic	UIOurPriceCaption;
	CUIMultiTextStatic	UIOthersPriceCaption;
	CUIDragDropListEx	UIOurTradeList;
	CUIDragDropListEx	UIOthersTradeList;

	//кнопки
	CUI3tButton			UIPerformTradeButton;
	CUI3tButton			UIToTalkButton;

	//информация о персонажах 
	CUIStatic			UIOurIcon;
	CUIStatic			UIOthersIcon;
	CUICharacterInfo	UICharacterInfoLeft;
	CUICharacterInfo	UICharacterInfoRight;

	//информация о перетаскиваемом предмете
	CUIStatic			UIDescWnd;
	CUIItemInfo			UIItemInfo;

	SDrawStaticStruct*	UIDealMsg;
};

bool others_zero_trade;
const char* money_name;

CUITradeWnd::CUITradeWnd()
	:	m_bDealControlsVisible	(false),
		m_pTrade(NULL),
		m_pOthersTrade(NULL),
		bStarted(false)
{
	m_uidata = xr_new<CUITradeInternal>();
	Init();
	Hide();
	SetCurrentItem			(NULL);

    others_zero_trade = !!READ_IF_EXISTS( pSettings, r_bool, "trade", "others_zero_trade", false );
	money_name = CStringTable().translate("ui_st_money_regional").c_str();
}

CUITradeWnd::~CUITradeWnd()
{
	m_uidata->UIOurBagList.ClearAll		(true);
	m_uidata->UIOurTradeList.ClearAll	(true);
	m_uidata->UIOthersBagList.ClearAll	(true);
	m_uidata->UIOthersTradeList.ClearAll(true);
	xr_delete							(m_uidata);
}

void CUITradeWnd::Init()
{
	CUIXml								uiXml;
	bool xml_result						= uiXml.Init(CONFIG_PATH, UI_PATH, TRADE_XML);
	R_ASSERT3							(xml_result, "xml file not found", TRADE_XML);
	CUIXmlInit							xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);

	//статические элементы интерфейса
	AttachChild							(&m_uidata->UIStaticTop);
	xml_init.InitStatic					(uiXml, "top_background", 0, &m_uidata->UIStaticTop);
	AttachChild							(&m_uidata->UIStaticBottom);
	xml_init.InitStatic					(uiXml, "bottom_background", 0, &m_uidata->UIStaticBottom);

	//иконки с изображение нас и партнера по торговле
	AttachChild							(&m_uidata->UIOurIcon);
	xml_init.InitStatic					(uiXml, "static_icon", 0, &m_uidata->UIOurIcon);
	AttachChild							(&m_uidata->UIOthersIcon);
	xml_init.InitStatic					(uiXml, "static_icon", 1, &m_uidata->UIOthersIcon);
	m_uidata->UIOurIcon.AttachChild		(&m_uidata->UICharacterInfoLeft);
	m_uidata->UICharacterInfoLeft.Init	(0,0, m_uidata->UIOurIcon.GetWidth(), m_uidata->UIOurIcon.GetHeight(), TRADE_CHARACTER_XML);
	m_uidata->UIOthersIcon.AttachChild	(&m_uidata->UICharacterInfoRight);
	m_uidata->UICharacterInfoRight.Init	(0,0, m_uidata->UIOthersIcon.GetWidth(), m_uidata->UIOthersIcon.GetHeight(), TRADE_CHARACTER_XML);


	//Списки торговли
	AttachChild							(&m_uidata->UIOurBagWnd);
	xml_init.InitStatic					(uiXml, "our_bag_static", 0, &m_uidata->UIOurBagWnd);
	AttachChild							(&m_uidata->UIOthersBagWnd);
	xml_init.InitStatic					(uiXml, "others_bag_static", 0, &m_uidata->UIOthersBagWnd);

	m_uidata->UIOurBagWnd.AttachChild	(&m_uidata->UIOurMoneyStatic);
	xml_init.InitStatic					(uiXml, "our_money_static", 0, &m_uidata->UIOurMoneyStatic);

	m_uidata->UIOthersBagWnd.AttachChild(&m_uidata->UIOtherMoneyStatic);
	xml_init.InitStatic					(uiXml, "other_money_static", 0, &m_uidata->UIOtherMoneyStatic);

	AttachChild							(&m_uidata->UIOurTradeWnd);
	xml_init.InitStatic					(uiXml, "static", 0, &m_uidata->UIOurTradeWnd);
	AttachChild							(&m_uidata->UIOthersTradeWnd);
	xml_init.InitStatic					(uiXml, "static", 1, &m_uidata->UIOthersTradeWnd);

	m_uidata->UIOurTradeWnd.AttachChild	(&m_uidata->UIOurPriceCaption);
	xml_init.InitMultiTextStatic		(uiXml, "price_mt_static", 0, &m_uidata->UIOurPriceCaption);

	m_uidata->UIOthersTradeWnd.AttachChild(&m_uidata->UIOthersPriceCaption);
	xml_init.InitMultiTextStatic		(uiXml, "price_mt_static", 0, &m_uidata->UIOthersPriceCaption);

	//Списки Drag&Drop
	m_uidata->UIOurBagWnd.AttachChild	(&m_uidata->UIOurBagList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 0, &m_uidata->UIOurBagList);

	m_uidata->UIOthersBagWnd.AttachChild(&m_uidata->UIOthersBagList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 1, &m_uidata->UIOthersBagList);

	m_uidata->UIOurTradeWnd.AttachChild	(&m_uidata->UIOurTradeList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 2, &m_uidata->UIOurTradeList);

	m_uidata->UIOthersTradeWnd.AttachChild(&m_uidata->UIOthersTradeList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 3, &m_uidata->UIOthersTradeList);

	
	AttachChild							(&m_uidata->UIDescWnd);
	xml_init.InitStatic					(uiXml, "desc_static", 0, &m_uidata->UIDescWnd);
	m_uidata->UIDescWnd.AttachChild		(&m_uidata->UIItemInfo);
	m_uidata->UIItemInfo.Init			(0,0, m_uidata->UIDescWnd.GetWidth(), m_uidata->UIDescWnd.GetHeight(), TRADE_ITEM_XML);


	xml_init.InitAutoStatic				(uiXml, "auto_static", this);


	AttachChild							(&m_uidata->UIPerformTradeButton);
	xml_init.Init3tButton				(uiXml, "button", 0, &m_uidata->UIPerformTradeButton);

	AttachChild							(&m_uidata->UIToTalkButton);
	xml_init.Init3tButton				(uiXml, "button", 1, &m_uidata->UIToTalkButton);

	m_pUIPropertiesBox					= xr_new<CUIPropertiesBox>(); m_pUIPropertiesBox->SetAutoDelete(true);
	AttachChild(m_pUIPropertiesBox);
	m_pUIPropertiesBox->Init(0, 0, 300, 300);
	m_pUIPropertiesBox->Hide();

	m_uidata->UIDealMsg					= NULL;

	BindDragDropListEvents				(&m_uidata->UIOurBagList);
	BindDragDropListEvents				(&m_uidata->UIOthersBagList);
	BindDragDropListEvents				(&m_uidata->UIOurTradeList);
	BindDragDropListEvents				(&m_uidata->UIOthersTradeList);

	//Load sounds
	if (uiXml.NavigateToNode("action_sounds", 0))
	{
		XML_NODE* stored_root = uiXml.GetLocalRoot();
		uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

		::Sound->create(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvProperties],		uiXml.Read("snd_properties",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvMoveItem],		uiXml.Read("snd_move_item",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvItemUse],		uiXml.Read("snd_item_use",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvMagLoad],		uiXml.Read("snd_mag_load",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvMagUnload],		uiXml.Read("snd_mag_unload",	0, NULL), st_Effect, sg_SourceType);

		uiXml.SetLocalRoot(stored_root);
	}
}

void CUITradeWnd::InitTrade(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{
	VERIFY								(pOur);
	VERIFY								(pOthers);

	m_pInvOwner							= pOur;
	m_pOthersInvOwner					= pOthers;
	m_uidata->UIOthersPriceCaption.GetPhraseByIndex(0)->SetText(*CStringTable().translate("ui_st_opponent_items"));

	m_uidata->UICharacterInfoLeft.InitCharacter(m_pInvOwner->object_id());
	m_uidata->UICharacterInfoRight.InitCharacter(m_pOthersInvOwner->object_id());

	m_pInv								= &m_pInvOwner->inventory();
	m_pOthersInv						= pOur->GetTrade()->GetPartnerInventory();
		
	m_pTrade							= pOur->GetTrade();
	m_pOthersTrade						= pOur->GetTrade()->GetPartnerTrade();

	m_pUIPropertiesBox->Hide();
    	
	EnableAll							();

	UpdateLists							(eBoth);

	// режим бартерной торговли
	if (!Actor()->HasPDAWorkable())
	{
		m_uidata->UIOurMoneyStatic.SetText(*CStringTable().translate("ui_st_pda_account_unavailable"));   //закроем статиком кол-во денег актора, т.к. оно еще не обновилось и не ноль
		m_uidata->UIOtherMoneyStatic.SetText(*CStringTable().translate("ui_st_pda_account_unavailable")); //закроем статиком кол-во денег контрагента, т.к. оно еще не обновилось и не ---
		m_uidata->UIPerformTradeButton.SetText(*CStringTable().translate("ui_st_barter")); //напишем "бартер" на кнопке, вместо "торговать"
	}
	else
		m_uidata->UIPerformTradeButton.SetText(*CStringTable().translate("ui_st_trade")); //вернём надпись "торговать" на кнопке, вместо "бартер"
//
}  

void CUITradeWnd::ActivatePropertiesBox()
{
	m_pUIPropertiesBox->RemoveAll();

	auto pWeapon		= smart_cast<CWeapon*>		(CurrentIItem());
	auto pAmmo			= smart_cast<CWeaponAmmo*>	(CurrentIItem());
	auto pEatableItem	= smart_cast<CEatableItem*> (CurrentIItem());

	bool b_actor_inv = CurrentItem()->OwnerList() == &m_uidata->UIOurBagList;
	auto inv = m_pInv;

	bool hasMany = CurrentItem()->ChildsCount() > 0;

	string1024			temp;

	bool b_show		= false;
	LPCSTR _action	= nullptr;

	if (b_actor_inv) {
		if (pAmmo) {
			LPCSTR _ammo_sect;

			if (pAmmo->IsBoxReloadable()) {
				//unload AmmoBox
				m_pUIPropertiesBox->AddItem("st_unload_magazine", NULL, INVENTORY_UNLOAD_AMMO_BOX);
				b_show = true;
				//reload AmmoBox
				if (pAmmo->m_boxCurr < pAmmo->m_boxSize && b_actor_inv) {
					if (inv->GetAmmo(*pAmmo->m_ammoSect, true)) {
						strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
							*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoSect, "inv_name_short")));
						_ammo_sect = *pAmmo->m_ammoSect;
						m_pUIPropertiesBox->AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
						b_show = true;
					}
				}
			}
			else if (pAmmo->IsBoxReloadableEmpty() && b_actor_inv) {
				for (u8 i = 0; i < pAmmo->m_ammoTypes.size(); ++i) {
					if (inv->GetAmmo(*pAmmo->m_ammoTypes[i], true)) {
						strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
							*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoTypes[i], "inv_name_short")));
						_ammo_sect = *pAmmo->m_ammoTypes[i];
						m_pUIPropertiesBox->AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
						b_show = true;
					}
				}
			}
		}

		if (pWeapon) {
			if (inv->InSlot(pWeapon)) {
				for (u32 i = 0; i < pWeapon->m_ammoTypes.size(); ++i) {
					if (pWeapon->TryToGetAmmo(i)) {
						strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
							*CStringTable().translate(pSettings->r_string(pWeapon->m_ammoTypes[i].c_str(), "inv_name_short")));
						m_pUIPropertiesBox->AddItem(temp, (void*)(__int64)i, INVENTORY_RELOAD_MAGAZINE);
						b_show = true;
					}
				}
			}
			//
			if (pWeapon->GrenadeLauncherAttachable() && pWeapon->IsGrenadeLauncherAttached()) {
				sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetGrenadeLauncherName().c_str(), "inv_name_short")).c_str());
				m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetGrenadeLauncherName().c_str(), INVENTORY_DETACH_ADDON);
				b_show = true;
			}
			if (pWeapon->ScopeAttachable() && pWeapon->IsScopeAttached()) {
				sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetScopeName().c_str(), "inv_name_short")).c_str());
				m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetScopeName().c_str(), INVENTORY_DETACH_ADDON);
				b_show = true;
			}
			if (pWeapon->SilencerAttachable() && pWeapon->IsSilencerAttached()) {
				sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetSilencerName().c_str(), "inv_name_short")).c_str());
				m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetSilencerName().c_str(), INVENTORY_DETACH_ADDON);
				b_show = true;
			}
			if (pWeapon->LaserAttachable() && pWeapon->IsLaserAttached()) {
				sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetLaserName().c_str(), "inv_name_short")).c_str());
				m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetLaserName().c_str(), INVENTORY_DETACH_ADDON);
				b_show = true;
			}
			if (pWeapon->FlashlightAttachable() && pWeapon->IsFlashlightAttached()) {
				sprintf(temp, "%s %s", CStringTable().translate("st_detach").c_str(), CStringTable().translate(pSettings->r_string(pWeapon->GetFlashlightName().c_str(), "inv_name_short")).c_str());
				m_pUIPropertiesBox->AddItem(temp, (void*)pWeapon->GetFlashlightName().c_str(), INVENTORY_DETACH_ADDON);
				b_show = true;
			}
			if (smart_cast<CWeaponMagazined*>(pWeapon)) {
				auto WpnMagazWgl = smart_cast<CWeaponMagazinedWGrenade*>(pWeapon);
				bool b = pWeapon->GetAmmoElapsed() > 0
					|| WpnMagazWgl && !WpnMagazWgl->m_magazine2.empty()
					|| smart_cast<CWeaponMagazined*>(pWeapon)->IsMagazineAttached();

				if (!b) {
					CUICellItem* itm = CurrentItem();
					for (u32 i = 0; i < itm->ChildsCount(); ++i) {
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

		m_pUIPropertiesBox->AddItem("st_drop", NULL, INVENTORY_DROP_ACTION);
		if (hasMany)
			m_pUIPropertiesBox->AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
	}

	if (CanMoveToOther(CurrentIItem(), b_actor_inv)) {
		m_pUIPropertiesBox->AddItem("st_move", NULL, INVENTORY_MOVE_ACTION);
		if (hasMany)
			m_pUIPropertiesBox->AddItem("st_move_all", (void*)33, INVENTORY_MOVE_ACTION);
	}

	b_show = true;

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

void CUITradeWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if(pWnd == &m_uidata->UIToTalkButton && msg == BUTTON_CLICKED)
	{
		SwitchToTalk();
	}
	else if(pWnd == &m_uidata->UIPerformTradeButton && msg == BUTTON_CLICKED)
	{
		PerformTrade();
	}
	else if (pWnd == m_pUIPropertiesBox && msg == PROPERTY_CLICKED)
	{
		if (m_pUIPropertiesBox->GetClickedItem())
		{
			switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{			
				case INVENTORY_MOVE_ACTION:
				{
					void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
					bool b_all = (d == (void*)33);

					MoveItems(CurrentItem(), b_all);
				}break;
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

					for (u32 i = 0; i < itm->ChildsCount(); ++i)
					{
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
					for (u32 i = 0; i < itm->ChildsCount(); ++i)
					{
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
			}
		}
	}

	CUIWindow::SendMessage(pWnd, msg, pData);
}

void CUITradeWnd::Draw()
{
	inherited::Draw				();
	if(m_uidata->UIDealMsg)		m_uidata->UIDealMsg->Draw();

}

extern void UpdateCameraDirection(CGameObject* pTo);

void CUITradeWnd::Update()
{
	EListType et					= eNone;

	if(m_pInv->ModifyFrame()==Device.dwFrame && m_pOthersInv->ModifyFrame()==Device.dwFrame){
		et = eBoth;
	}else if(m_pInv->ModifyFrame()==Device.dwFrame){
		et = e1st;
	}else if(m_pOthersInv->ModifyFrame()==Device.dwFrame){
		et = e2nd;
	}
	if(et!=eNone)
		UpdateLists					(et);

	inherited::Update				();
	UpdateCameraDirection			(smart_cast<CGameObject*>(m_pOthersInvOwner));

	if(m_uidata->UIDealMsg){
		m_uidata->UIDealMsg->Update();
		if( !m_uidata->UIDealMsg->IsActual()){
			HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money");
			m_uidata->UIDealMsg			= NULL;
		}
	}
}

#include "UIInventoryUtilities.h"
void CUITradeWnd::Show()
{
	InventoryUtilities::SendInfoToActor("ui_trade");
	inherited::Show					(true);
	inherited::Enable				(true);

	SetCurrentItem					(NULL);
	ResetAll						();
	m_uidata->UIDealMsg				= NULL;

	if (auto pActor = Actor())
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) Actor()->SetRuckAmmoPlacement(true); //установим флаг перезарядки из рюкзака
	PlaySnd(eInvSndOpen);
}

void CUITradeWnd::Hide()
{
	InventoryUtilities::SendInfoToActor("ui_trade_hide");
	inherited::Show					(false);
	inherited::Enable				(false);
	if(bStarted)
		StopTrade					();
	
	m_uidata->UIDealMsg				= NULL;

	if(HUD().GetUI()->UIGame()){
		HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money");
	}

	m_uidata->UIOurBagList.ClearAll		(true);
	m_uidata->UIOurTradeList.ClearAll	(true);
	m_uidata->UIOthersBagList.ClearAll	(true);
	m_uidata->UIOthersTradeList.ClearAll(true);

	if (auto pActor = Actor())
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) Actor()->SetRuckAmmoPlacement(false); //сбросим флаг перезарядки из рюкзака
}

void CUITradeWnd::StartTrade()
{
	if (m_pTrade)					m_pTrade->TradeCB(true);
	if (m_pOthersTrade)				m_pOthersTrade->TradeCB(true);
	bStarted						= true;
}

void CUITradeWnd::StopTrade()
{
	if (m_pTrade)					m_pTrade->TradeCB(false);
	if (m_pOthersTrade)				m_pOthersTrade->TradeCB(false);
	bStarted						= false;
}

#include "../trade_parameters.h"
bool CUITradeWnd::CanMoveToOther( PIItem pItem, bool our )
{
	if (pItem->m_flags.test(CInventoryItem::FIAlwaysUntradable))
		return false;
	if ( !our ) return true;

	float r1				= CalcItemsWeight(&m_uidata->UIOurTradeList);	// our
	float r2				= CalcItemsWeight(&m_uidata->UIOthersTradeList);	// other

	float itmWeight			= pItem->Weight();
	float otherInvWeight	= m_pOthersInv->CalcTotalWeight();
	float otherMaxWeight	= m_pOthersInv->GetMaxWeight();

	if (!m_pOthersInvOwner->trade_parameters().enabled(
			CTradeParameters::action_buy(0),
			pItem->object().cNameSect()
		))
		return				(false);

	if ( pItem->GetCondition() < m_pOthersInvOwner->trade_parameters().factors( CTradeParameters::action_buy( 0 ), pItem->object().cNameSect() ).min_condition() )
	  return false;

	if(otherInvWeight-r2+r1+itmWeight > otherMaxWeight)
		return				false;

	return true;
}

void move_item(CUICellItem* itm, CUIDragDropListEx* from, CUIDragDropListEx* to)
{
	CUICellItem* _itm		= from->RemoveItem	(itm, false);
	to->SetItem				(_itm);
}

bool CUITradeWnd::ToOurTrade(CUICellItem* itm)
{
	if ( !CanMoveToOther((PIItem)m_pCurrentCellItem->m_pData, true ) ) return false;

	move_item				(itm, &m_uidata->UIOurBagList, &m_uidata->UIOurTradeList);
	UpdatePrices			();
	return					true;
}

bool CUITradeWnd::ToOthersTrade(CUICellItem* itm)
{
	if ( !CanMoveToOther((PIItem)m_pCurrentCellItem->m_pData, false ) ) return false;

	move_item				(itm, &m_uidata->UIOthersBagList, &m_uidata->UIOthersTradeList);
	UpdatePrices			();

	return					true;
}

bool CUITradeWnd::ToOurBag(CUICellItem* itm)
{
	move_item				(itm, &m_uidata->UIOurTradeList, &m_uidata->UIOurBagList);
	UpdatePrices			();
	
	return					true;
}

bool CUITradeWnd::ToOthersBag(CUICellItem* itm)
{
	move_item				(itm, &m_uidata->UIOthersTradeList, &m_uidata->UIOthersBagList);
	UpdatePrices			();

	return					true;
}

float CUITradeWnd::CalcItemsWeight(CUIDragDropListEx* pList)
{
	float res = 0.0f;

	for(u32 i=0; i<pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx	(i);
		PIItem	iitem		= (PIItem)itm->m_pData;
		res					+= iitem->Weight();
		for(u32 j=0; j<itm->ChildsCount(); ++j){
			PIItem	jitem		= (PIItem)itm->Child(j)->m_pData;
			res					+= jitem->Weight();
		}
	}
	return res;
}

u32 CUITradeWnd::CalcItemsPrice(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying)
{
	u32 iPrice				= 0;
	
	for(u32 i=0; i<pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx(i);
		PIItem iitem		= (PIItem)itm->m_pData;
		iPrice				+= pTrade->GetItemPrice(iitem, bBuying);

		for(u32 j=0; j<itm->ChildsCount(); ++j){
			PIItem jitem	= (PIItem)itm->Child(j)->m_pData;
			iPrice			+= pTrade->GetItemPrice(jitem, bBuying);
		}

	}

	return					iPrice;
}

void CUITradeWnd::PerformTrade()
{

	if (m_uidata->UIOurTradeList.ItemsCount()==0 && m_uidata->UIOthersTradeList.ItemsCount()==0) 
		return;

	int our_money			= (int)m_pInvOwner->get_money();
	int others_money		= (int)m_pOthersInvOwner->get_money();

	int delta_price			= int(m_iOurTradePrice-m_iOthersTradePrice);

	our_money				+= delta_price;
	others_money			-= delta_price;

	if(our_money>=0 && others_money>=0 && (m_iOurTradePrice>=0 || m_iOthersTradePrice>0))
	{
		m_pOthersTrade->OnPerformTrade(m_iOthersTradePrice, m_iOurTradePrice);
		
		TransferItems		(&m_uidata->UIOurTradeList,		&m_uidata->UIOthersBagList, m_pOthersTrade,	true);
		TransferItems		(&m_uidata->UIOthersTradeList,	&m_uidata->UIOurBagList,	m_pOthersTrade,	false);
                if ( others_zero_trade ) {
                  m_pOthersTrade->pThis.inv_owner->set_money( others_money, true );
                  m_pOthersTrade->pPartner.inv_owner->set_money( our_money, true );
                }
	}else
	{
		string256				deal_refuse_text; //строка с текстом сообщения-отказа при невозмжности совершить торговую сделку
		//условия для формирования текста
		LPCSTR                  trader_name = others_money < 0 ? m_pOthersInvOwner->Name() : m_pInvOwner->Name(); //от чьего имени выдаётся сообщение
		STRING_ID               refusal_text = Actor()->HasPDAWorkable() ? "st_not_enough_money_to_trade" : "st_not_enough_money_to_barter"; //текст сообщения отказа в зависимости от торговля/бартер
		//показываем статик с текстом отказа
		m_uidata->UIDealMsg = HUD().GetUI()->UIGame()->AddCustomStatic("not_enough_money", true); //показать статик
		strconcat(sizeof(deal_refuse_text), deal_refuse_text, trader_name, ": ", *CStringTable().translate(refusal_text)); //сформировать текст
		m_uidata->UIDealMsg->wnd()->SetText(deal_refuse_text); //задать текст статику

		m_uidata->UIDealMsg->m_endTime	= Device.fTimeGlobal+1.0f;// sec
	}
	SetCurrentItem			(NULL);
}

#include "../xr_level_controller.h"

bool CUITradeWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if (m_pUIPropertiesBox->GetVisible())
		m_pUIPropertiesBox->OnKeyboard(dik, keyboard_action);

	if (inherited::OnKeyboard(dik, keyboard_action))return true;

	return false;
}

bool CUITradeWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if (mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if (m_pUIPropertiesBox->IsShown())
		{
			m_pUIPropertiesBox->Hide();
			return						true;
		}
	}

	if (m_pUIPropertiesBox->IsShown())
	{
		switch (mouse_action)
		{
		case WINDOW_MOUSE_WHEEL_DOWN:
		case WINDOW_MOUSE_WHEEL_UP:
			return true;
			break;
		}
	}

	CUIWindow::OnMouse(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

void CUITradeWnd::DisableAll()
{
	m_uidata->UIOurBagWnd.Enable			(false);
	m_uidata->UIOthersBagWnd.Enable			(false);
	m_uidata->UIOurTradeWnd.Enable			(false);
	m_uidata->UIOthersTradeWnd.Enable		(false);
}

void CUITradeWnd::EnableAll()
{
	m_uidata->UIOurBagWnd.Enable			(true);
	m_uidata->UIOthersBagWnd.Enable			(true);
	m_uidata->UIOurTradeWnd.Enable			(true);
	m_uidata->UIOthersTradeWnd.Enable		(true);
}

void CUITradeWnd::UpdatePrices()
{
	m_iOurTradePrice	= CalcItemsPrice	(&m_uidata->UIOurTradeList,		m_pOthersTrade, true);
	m_iOthersTradePrice = CalcItemsPrice	(&m_uidata->UIOthersTradeList,	m_pOthersTrade, false);

	if ( !m_pOthersInvOwner->InfinitiveMoney() ) {
          u32 others_money = m_pOthersInvOwner->get_money();
          if ( others_zero_trade && m_iOurTradePrice > others_money )
            m_iOurTradePrice = others_money;
	}

	string256				buf;
	sprintf_s( buf, "%d %s", m_iOurTradePrice, money_name);
	m_uidata->UIOurPriceCaption.GetPhraseByIndex(2)->str = buf;
	sprintf_s					(buf, "%d %s", m_iOthersTradePrice, money_name);
	m_uidata->UIOthersPriceCaption.GetPhraseByIndex(2)->str = buf;

	sprintf_s					(buf, "%d %s", m_pInvOwner->get_money(), money_name);
	m_uidata->UIOurMoneyStatic.SetText(buf);

	if(!m_pOthersInvOwner->InfinitiveMoney()){
          sprintf_s( buf, "%d %s", (int)m_pOthersInvOwner->get_money(), money_name);
          m_uidata->UIOtherMoneyStatic.SetText(buf);
	}//else
	if (m_pOthersInvOwner->InfinitiveMoney() || (!m_pOthersInvOwner->InfinitiveMoney() && !Actor()->HasPDAWorkable())) //закроем --- счетчик денег контрагента, если в режиме бартера
	{
		m_uidata->UIOtherMoneyStatic.SetText("---");
	}
}

void CUITradeWnd::TransferItems(CUIDragDropListEx* pSellList,
								CUIDragDropListEx* pBuyList,
								CTrade* pTrade,
								bool bBuying)
{
	while(pSellList->ItemsCount())
	{
		CUICellItem* itm	=	pSellList->RemoveItem(pSellList->GetItemIdx(0),false);
		auto InvItm = (PIItem)itm->m_pData;
		if (!bBuying)		InvItm->OnMoveOut(InvItm->m_eItemPlace);
		InvItm->m_highlight_equipped = false; //Убираем подсветку после продажи предмета
		itm->m_select_equipped = false;
		pTrade->TransferItem( InvItm, bBuying, !others_zero_trade );
		pBuyList->SetItem		(itm);
	}

        if ( !others_zero_trade ) {
          pTrade->pThis.inv_owner->set_money ( pTrade->pThis.inv_owner->get_money(), true );
          pTrade->pPartner.inv_owner->set_money( pTrade->pPartner.inv_owner->get_money(), true );
        }
}

void CUITradeWnd::UpdateLists(EListType mode)
{
	if(mode==eBoth||mode==e1st){
		m_uidata->UIOurBagList.ClearAll(true);
		m_uidata->UIOurTradeList.ClearAll(true);
	}

	if(mode==eBoth||mode==e2nd){
		m_uidata->UIOthersBagList.ClearAll(true);
		m_uidata->UIOthersTradeList.ClearAll(true);
	}

	UpdatePrices						();


	if(mode==eBoth||mode==e1st){
		ruck_list.clear					();
   		m_pInv->AddAvailableItems		(ruck_list, true);
		std::sort						(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);
		FillList						(ruck_list, m_uidata->UIOurBagList, true);
	}

	if(mode==eBoth||mode==e2nd){
		ruck_list.clear					();
		m_pOthersInv->AddAvailableItems	(ruck_list, true);
		std::sort						(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);
		FillList						(ruck_list, m_uidata->UIOthersBagList, false);
	}
}

void CUITradeWnd::FillList	(TIItemContainer& cont, CUIDragDropListEx& dragDropList, bool our)
{
	TIItemContainer::iterator it	= cont.begin();
	TIItemContainer::iterator it_e	= cont.end();

	for(; it != it_e; ++it)
	{
		CInventoryItem* item = *it;
		CUICellItem* itm = create_cell_item( item );
		if (item->m_highlight_equipped)
			itm->m_select_equipped = true;
		bool canTrade = CanMoveToOther( item, our );
		ColorizeItem( itm, canTrade, itm->m_select_equipped );
		dragDropList.SetItem( itm );
	}
}

bool CUITradeWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUITradeWnd::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem(itm);
	itm->ColorizeItems( { &m_uidata->UIOurTradeList, &m_uidata->UIOthersTradeList, &m_uidata->UIOurBagList, &m_uidata->UIOthersBagList } );
	return false;
}

bool CUITradeWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox();
	return						false;
}


bool CUITradeWnd::OnItemDrop(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= CUIDragDropListEx::m_drag_item->BackList();
	if(old_owner==new_owner || !old_owner || !new_owner)
		return false;

	bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
	MoveItems(itm, b_all);

	return true;
}

bool CUITradeWnd::OnItemDbClick(CUICellItem* itm)
{
	SetCurrentItem						(itm);

	bool b_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
	MoveItems(itm, b_all);

	return true;
}

void CUITradeWnd::MoveItems(CUICellItem* itm, bool b_all)
{
	if (!itm) return;

	auto old_owner = itm->OwnerList();

	auto MoveItemFromCell = [&](void* cellitm) {

		auto itm = static_cast<CUICellItem*>(cellitm);

		if (old_owner == &m_uidata->UIOurBagList)
			ToOurTrade(itm);
		else if (old_owner == &m_uidata->UIOurTradeList)
			ToOurBag(itm);
		else if (old_owner == &m_uidata->UIOthersBagList)
			ToOthersTrade(itm);
		else if (old_owner == &m_uidata->UIOthersTradeList)
			ToOthersBag(itm);
	};

	u32 cnt = itm->ChildsCount();
	for (u32 i = 0; i < cnt && b_all; ++i)
		MoveItemFromCell(itm);

	MoveItemFromCell(itm);

	PlaySnd(eInvMoveItem);

	UpdatePrices();

	SetCurrentItem(NULL);
}

CUICellItem* CUITradeWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUITradeWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUITradeWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem				= itm;
	m_uidata->UIItemInfo.InitItem	(CurrentIItem());
	
	if(!m_pCurrentCellItem)		return;

	m_pCurrentCellItem->m_select_armament = true;

	CUIDragDropListEx* owner	= itm->OwnerList();
	bool bBuying				= (owner==&m_uidata->UIOurBagList) || (owner==&m_uidata->UIOurTradeList);

	if(m_uidata->UIItemInfo.UICost){

		string256			str;

		sprintf_s				(str, "%d %s", m_pOthersTrade->GetItemPrice(CurrentIItem(), bBuying), money_name);
		m_uidata->UIItemInfo.UICost->SetText (str);
	}

	auto script_obj = CurrentIItem()->object().lua_game_object();
	g_actor->callback(GameObject::eCellItemSelect)(script_obj);
}

void CUITradeWnd::SwitchToTalk()
{
	GetMessageTarget()->SendMessage		(this, TRADE_WND_CLOSED);
}

void CUITradeWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemDrop);
	lst->m_f_item_start_drag = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemStartDrag);
	lst->m_f_item_db_click = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemDbClick);
	lst->m_f_item_selected = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemSelected);
	lst->m_f_item_rbutton_click = fastdelegate::MakeDelegate(this, &CUITradeWnd::OnItemRButtonClick);
}

void CUITradeWnd::ColorizeItem(CUICellItem* itm, bool canTrade, bool highlighted)
{
	static const bool highlight_cop_enabled = !Core.Features.test(xrCore::Feature::colorize_untradable); //Это опция для Dsh, не убирать!

	if (!canTrade) {
		if ( highlight_cop_enabled )
			itm->m_select_untradable = true;
		itm->SetColor(reinterpret_cast<CInventoryItem*>(itm->m_pData)->ClrUntradable);
	}
	else {
		if ( highlight_cop_enabled )
			itm->m_select_untradable = false;
		if ( highlighted )
			itm->SetColor(reinterpret_cast<CInventoryItem*>(itm->m_pData)->ClrEquipped);
	}
}

void CUITradeWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
		sounds[a].play(NULL, sm_2D);
}

void CUITradeWnd::EatItem()
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor)					return;

	m_pInv->Eat(CurrentIItem(), m_pInvOwner);

	PlaySnd(eInvItemUse);

	SetCurrentItem(NULL);
}

void CUITradeWnd::SendEvent_Item_Drop(PIItem	pItem)
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

void CUITradeWnd::DropItems(bool b_all)
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor)
	{
		return;
	}

	CUICellItem* ci = CurrentItem();
	if (!ci)
	{
		return;
	}

	CUIDragDropListEx* owner_list = ci->OwnerList();

	if (b_all){
		u32 cnt = ci->ChildsCount();
		for (u32 i = 0; i < cnt; ++i){
			CUICellItem* itm = ci->PopChild();
			PIItem			iitm = (PIItem)itm->m_pData;
			SendEvent_Item_Drop(iitm);
		}
	}

	SendEvent_Item_Drop(CurrentIItem());

	owner_list->RemoveItem(ci, b_all);

	SetCurrentItem(NULL);
	PlaySnd(eInvDropItem);
}