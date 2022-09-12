#include "stdafx.h"
#include "UIInventoryWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../string_table.h"

#include "../actor.h"
#include "../uigamesp.h"
#include "../hudmanager.h"

#include "../CustomOutfit.h"

#include "../weapon.h"

#include "../eatable_item.h"
#include "../inventory.h"
#include "Artifact.h"

#include "UIInventoryUtilities.h"
using namespace InventoryUtilities;


#include "../InfoPortion.h"
#include "../level.h"
#include "../game_base_space.h"
#include "../entitycondition.h"

#include "../game_cl_base.h"
#include "../ActorCondition.h"
#include "UIDragDropListEx.h"
#include "UIOutfitSlot.h"
#include "UI3tButton.h"

#define				INVENTORY_ITEM_XML		"inventory_item.xml"
#define				INVENTORY_XML			"inventory_new.xml"



CUIInventoryWnd*	g_pInvWnd = NULL;

CUIInventoryWnd::CUIInventoryWnd() : 
	m_pUIBagList(nullptr), m_pUIBeltList(nullptr), 
	m_pUIOutfitList(nullptr), m_pUIHelmetList(nullptr), m_pUIWarBeltList(nullptr), m_pUIBackPackList(nullptr), 
	m_pUIKnifeList(nullptr), m_pUIOnShoulderList(nullptr), m_pUIOnBackList(nullptr), m_pUIHolsterList(nullptr),
	m_pUIGrenadeList(nullptr), m_pUIArtefactList(nullptr),
	m_pUIDetectorList(nullptr), m_pUIOnHeadList(nullptr), m_pUIPdaList(nullptr),
	m_pUIQuickList_0(nullptr), m_pUIQuickList_1(nullptr), m_pUIQuickList_2(nullptr), m_pUIQuickList_3(nullptr)
{
	m_iCurrentActiveSlot				= NO_ACTIVE_SLOT;
	UIRank								= NULL;
	Init								();
	SetCurrentItem						(NULL);

	g_pInvWnd							= this;	
	m_b_need_reinit						= false;
	m_b_need_update_stats				= false;
	Hide								();	
}

void CUIInventoryWnd::Init()
{
	CUIXml								uiXml;
	bool xml_result						= uiXml.Init(CONFIG_PATH, UI_PATH, INVENTORY_XML);
	R_ASSERT3							(xml_result, "file parsing error ", uiXml.m_xml_file_name);

	CUIXmlInit							xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);

	AttachChild							(&UIBeltSlots);
	xml_init.InitStatic					(uiXml, "belt_slots", 0, &UIBeltSlots);

	AttachChild							(&UIBack);
	xml_init.InitStatic					(uiXml, "back", 0, &UIBack);

	AttachChild							(&UIStaticBottom);
	xml_init.InitStatic					(uiXml, "bottom_static", 0, &UIStaticBottom);

	AttachChild							(&UIBagWnd);
	xml_init.InitStatic					(uiXml, "bag_static", 0, &UIBagWnd);

	UIBagWnd.AttachChild				(&UIWeightWnd);
	xml_init.InitStatic					(uiXml, "weight_static", 0, &UIWeightWnd);

	UIBagWnd.AttachChild				(&UIVolumeWnd);
	xml_init.InitStatic					(uiXml, "volume_static", 0, &UIVolumeWnd);
	
	AttachChild							(&UIMoneyWnd);
	xml_init.InitStatic					(uiXml, "money_static", 0, &UIMoneyWnd);

	AttachChild							(&UIDescrWnd);
	xml_init.InitStatic					(uiXml, "descr_static", 0, &UIDescrWnd);


	UIDescrWnd.AttachChild				(&UIItemInfo);
	UIItemInfo.Init						(0, 0, UIDescrWnd.GetWidth(), UIDescrWnd.GetHeight(), INVENTORY_ITEM_XML);

	AttachChild							(&UIPersonalWnd);
	xml_init.InitFrameWindow			(uiXml, "character_frame_window", 0, &UIPersonalWnd);

	AttachChild							(&UIProgressBack);
	xml_init.InitStatic					(uiXml, "progress_background", 0, &UIProgressBack);

	AttachChild							(&UIProgressBackRadiation);
	xml_init.InitStatic					(uiXml, "progress_background_radiation", 0, &UIProgressBackRadiation);

	UIProgressBack.AttachChild			(&UIProgressBarHealth);
	xml_init.InitProgressBar			(uiXml, "progress_bar_health", 0, &UIProgressBarHealth);
	
	UIProgressBack.AttachChild			(&UIProgressBarPsyHealth);
	xml_init.InitProgressBar			(uiXml, "progress_bar_psy", 0, &UIProgressBarPsyHealth);

	UIProgressBack.AttachChild			(&UIProgressBarSatiety);
	xml_init.InitProgressBar			(uiXml, "progress_bar_satiety", 0, &UIProgressBarSatiety);

	UIProgressBackRadiation.AttachChild	(&UIProgressBarRadiation);
	xml_init.InitProgressBar			(uiXml, "progress_bar_radiation", 0, &UIProgressBarRadiation);

	UIPersonalWnd.AttachChild			(&UIStaticPersonal);
	xml_init.InitStatic					(uiXml, "static_personal",0, &UIStaticPersonal);
//	UIStaticPersonal.Init				(1, UIPersonalWnd.GetHeight() - 175, 260, 260);

	AttachChild							(&UIOutfitInfo);
	UIOutfitInfo.InitFromXml			(uiXml);
//.	xml_init.InitStatic					(uiXml, "outfit_info_window",0, &UIOutfitInfo);

	//Элементы автоматического добавления
	xml_init.InitAutoStatic				(uiXml, "auto_static", this);

	m_pUIBagList						= xr_new<CUIDragDropListEx>(); UIBagWnd.AttachChild(m_pUIBagList); m_pUIBagList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_bag", 0, m_pUIBagList);
	BindDragDropListEnents				(m_pUIBagList);

	m_pUIBeltList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBeltList); m_pUIBeltList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_belt", 0, m_pUIBeltList);
	BindDragDropListEnents				(m_pUIBeltList);

	m_pUIOutfitList						= xr_new<CUIOutfitDragDropList>(); AttachChild(m_pUIOutfitList); m_pUIOutfitList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_outfit", 0, m_pUIOutfitList);
	BindDragDropListEnents				(m_pUIOutfitList);

	m_pUIHelmetList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIHelmetList); m_pUIHelmetList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_helmet", 0, m_pUIHelmetList);
	BindDragDropListEnents				(m_pUIHelmetList);

	m_pUIWarBeltList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIWarBeltList); m_pUIWarBeltList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_warbelt", 0, m_pUIWarBeltList);
	BindDragDropListEnents				(m_pUIWarBeltList);

	m_pUIBackPackList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBackPackList); m_pUIBackPackList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_backpack", 0, m_pUIBackPackList);
	BindDragDropListEnents				(m_pUIBackPackList);

	m_pUIKnifeList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIKnifeList); m_pUIKnifeList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_knife", 0, m_pUIKnifeList);
	BindDragDropListEnents				(m_pUIKnifeList);

	m_pUIOnShoulderList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIOnShoulderList); m_pUIOnShoulderList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_on_shoulder", 0, m_pUIOnShoulderList);
	BindDragDropListEnents				(m_pUIOnShoulderList);

	m_pUIOnBackList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIOnBackList); m_pUIOnBackList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_on_back", 0, m_pUIOnBackList);
	BindDragDropListEnents				(m_pUIOnBackList);

	m_pUIHolsterList				= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIHolsterList); m_pUIHolsterList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_holster", 0, m_pUIHolsterList);
	BindDragDropListEnents				(m_pUIHolsterList);

	m_pUIGrenadeList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIGrenadeList); m_pUIGrenadeList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_grenade", 0, m_pUIGrenadeList);
	BindDragDropListEnents				(m_pUIGrenadeList);

	m_pUIArtefactList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIArtefactList); m_pUIArtefactList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_artefact", 0, m_pUIArtefactList);
	BindDragDropListEnents				(m_pUIArtefactList);

	m_pUIDetectorList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIDetectorList); m_pUIDetectorList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_detector", 0, m_pUIDetectorList);
	BindDragDropListEnents				(m_pUIDetectorList);

	m_pUIOnHeadList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIOnHeadList); m_pUIOnHeadList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_on_head", 0, m_pUIOnHeadList);
	BindDragDropListEnents				(m_pUIOnHeadList);

	m_pUIPdaList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIPdaList); m_pUIPdaList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_pda", 0, m_pUIPdaList);
	BindDragDropListEnents				(m_pUIPdaList);

	m_pUIQuickList_0					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIQuickList_0); m_pUIQuickList_0->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_quick_0", 0, m_pUIQuickList_0);
	BindDragDropListEnents				(m_pUIQuickList_0);

	m_pUIQuickList_1					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIQuickList_1); m_pUIQuickList_1->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_quick_1", 0, m_pUIQuickList_1);
	BindDragDropListEnents				(m_pUIQuickList_1);

	m_pUIQuickList_2					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIQuickList_2); m_pUIQuickList_2->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_quick_2", 0, m_pUIQuickList_2);
	BindDragDropListEnents				(m_pUIQuickList_2);

	m_pUIQuickList_3					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIQuickList_3); m_pUIQuickList_3->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_quick_3", 0, m_pUIQuickList_3);
	BindDragDropListEnents				(m_pUIQuickList_3);

	for ( u8 i = 0; i < SLOTS_TOTAL; i++ )
		m_slots_array[ i ] = NULL;
	m_slots_array[OUTFIT_SLOT]			= m_pUIOutfitList;
	m_slots_array[HELMET_SLOT]			= m_pUIHelmetList;
	m_slots_array[WARBELT_SLOT]			= m_pUIWarBeltList;
	m_slots_array[BACKPACK_SLOT]		= m_pUIBackPackList;

	m_slots_array[KNIFE_SLOT]			= m_pUIKnifeList;
	m_slots_array[ON_SHOULDER_SLOT]		= m_pUIOnShoulderList;
	m_slots_array[ON_BACK_SLOT]			= m_pUIOnBackList;
	m_slots_array[HOLSTER_SLOT]			= m_pUIHolsterList;

	m_slots_array[GRENADE_SLOT]			= m_pUIGrenadeList;
	m_slots_array[ARTEFACT_SLOT]		= m_pUIArtefactList;

	m_slots_array[DETECTOR_SLOT]		= m_pUIDetectorList;
	m_slots_array[ON_HEAD_SLOT]			= m_pUIOnHeadList;
	m_slots_array[PDA_SLOT]				= m_pUIPdaList;

	m_slots_array[QUICK_SLOT_0]			= m_pUIQuickList_0;
	m_slots_array[QUICK_SLOT_1]			= m_pUIQuickList_1;
	m_slots_array[QUICK_SLOT_2]			= m_pUIQuickList_2;
	m_slots_array[QUICK_SLOT_3]			= m_pUIQuickList_3;

	//pop-up menu
	AttachChild							(&UIPropertiesBox);
	UIPropertiesBox.Init				(0,0,300,300);
	UIPropertiesBox.Hide				();

	AttachChild							(&UIStaticTime);
	xml_init.InitStatic					(uiXml, "time_static", 0, &UIStaticTime);

	UIStaticTime.AttachChild			(&UIStaticTimeString);
	xml_init.InitStatic					(uiXml, "time_static_str", 0, &UIStaticTimeString);

	UIExitButton						= xr_new<CUI3tButton>();UIExitButton->SetAutoDelete(true);
	AttachChild							(UIExitButton);
	xml_init.Init3tButton				(uiXml, "exit_button", 0, UIExitButton);

	UIRepackAmmoButton					= xr_new<CUI3tButton>(); UIRepackAmmoButton->SetAutoDelete(true);
	AttachChild							(UIRepackAmmoButton);
	xml_init.Init3tButton				(uiXml, "repack_ammo_button", 0, UIRepackAmmoButton);

//Load sounds

	XML_NODE* stored_root				= uiXml.GetLocalRoot		();
	uiXml.SetLocalRoot					(uiXml.NavigateToNode		("action_sounds",0));
	::Sound->create						(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvSndClose],		uiXml.Read("snd_close",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToSlot],	uiXml.Read("snd_item_to_slot",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToBelt],	uiXml.Read("snd_item_to_belt",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToRuck],	uiXml.Read("snd_item_to_ruck",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvProperties],	uiXml.Read("snd_properties",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvAttachAddon],	uiXml.Read("snd_attach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemUse],		uiXml.Read("snd_item_use",		0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvMagLoad],		uiXml.Read("snd_mag_load",		0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvMagUnload],		uiXml.Read("snd_mag_unload",	0,	NULL),st_Effect,sg_SourceType);

	uiXml.SetLocalRoot					(stored_root);
}

EListType CUIInventoryWnd::GetType(CUIDragDropListEx* l)
{
	if(l==m_pUIBagList)			return iwBag;
	if(l==m_pUIBeltList)		return iwBelt;

        for ( u8 i = 0; i < SLOTS_TOTAL; i++ )
          if ( m_slots_array[ i ] == l )
            return iwSlot;

	NODEFAULT;
#ifdef DEBUG
	return iwSlot;
#endif // DEBUG
}

void CUIInventoryWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
        sounds[a].play					(NULL, sm_2D);
}

CUIInventoryWnd::~CUIInventoryWnd()
{
//.	ClearDragDrop(m_vDragDropItems);
	ClearAllLists						();
}

bool CUIInventoryWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if(m_b_need_reinit)
		return true;

	if(mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if(UIPropertiesBox.IsShown())
		{
			UIPropertiesBox.Hide		();
			return						true;
		}
	}

	if (UIPropertiesBox.IsShown())
	{
		switch (mouse_action)
		{
		case WINDOW_MOUSE_WHEEL_DOWN:
		case WINDOW_MOUSE_WHEEL_UP:
			return true;
			break;
		}
	}

	CUIWindow::OnMouse					(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

void CUIInventoryWnd::Draw()
{
	CUIWindow::Draw						();
}


void CUIInventoryWnd::Update()
{
	if(m_b_need_reinit)
		InitInventory					();


	CEntityAlive *pEntityAlive			= smart_cast<CEntityAlive*>(Level().CurrentEntity());

	if(pEntityAlive) 
	{
		auto cond = &pEntityAlive->conditions();

		float v = cond->GetHealth()*100.0f;
		UIProgressBarHealth.SetProgressPos		(v);

		v = cond->GetPsyHealth()*100.0f;
		UIProgressBarPsyHealth.SetProgressPos	(v);

		v = cond->GetSatiety() * 100.0f;
		UIProgressBarSatiety.SetProgressPos(v);

		v = cond->GetRadiation()*100.0f;
		if (Actor()->HasDetectorWorkable()) //удаляем шкалу радиации для прогрессбара в инвентаре если не экипирован детектор -- NO_RAD_UI_WITHOUT_DETECTOR_IN_SLOT
		{
			UIProgressBackRadiation.Show(true);
			UIProgressBarRadiation.Show(true);
			UIProgressBarRadiation.SetProgressPos(v);
		}
		else
		{
			UIProgressBackRadiation.Show(false);
		}

		CInventoryOwner* pOurInvOwner	= smart_cast<CInventoryOwner*>(pEntityAlive);
		u32 _money						= pOurInvOwner->get_money();

		// update money
		string64						sMoney;
		sprintf_s						(sMoney,"%d %s", _money, CStringTable().translate("ui_st_money_regional").c_str());
		UIMoneyWnd.SetText				(Actor()->HasPDAWorkable() ? sMoney : "");

		if (m_b_need_update_stats){
			// update outfit parameters
			UIOutfitInfo.Update();
			m_b_need_update_stats = false;
		}

		CheckForcedWeightVolumeUpdate();
	}

	UIStaticTimeString.SetText(*InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes));

	CUIWindow::Update					();
}

void CUIInventoryWnd::Show() 
{ 
	InitInventory			();
	inherited::Show			();

	SendInfoToActor						("ui_inventory");

	Update								();
	PlaySnd								(eInvSndOpen);

	m_b_need_update_stats = true;

	if (Actor()){
		if (g_eFreeHands != eFreeHandsOff) {
			Actor()->SetWeaponHideState(INV_STATE_INV_WND, true);
		}
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) {
			Actor()->SetRuckAmmoPlacement(true); //установим флаг перезарядки из рюкзака
		}
	}
}

void CUIInventoryWnd::Hide()
{
	PlaySnd								(eInvSndClose);
	inherited::Hide						();

	SendInfoToActor						("ui_inventory_hide");
	ClearAllLists						();

	//достать вещь в активный слот
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && m_iCurrentActiveSlot != NO_ACTIVE_SLOT && 
		pActor->inventory().m_slots[m_iCurrentActiveSlot].m_pIItem)
	{
		pActor->inventory().Activate(m_iCurrentActiveSlot);
		m_iCurrentActiveSlot = NO_ACTIVE_SLOT;
	}

	if (pActor){
		if (g_eFreeHands != eFreeHandsOff) {
			pActor->SetWeaponHideState(INV_STATE_INV_WND, false);
		}
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) {
			pActor->SetRuckAmmoPlacement(false); //сбросим флаг перезарядки из рюкзака
		}
	}

	HideSlotsHighlight();
}


void CUIInventoryWnd::HideSlotsHighlight()
{
	m_pUIBeltList->enable_highlight(false);
	for (const auto& DdList : m_slots_array)
		if (DdList)
			DdList->enable_highlight(false);
}

void CUIInventoryWnd::ShowSlotsHighlight(PIItem InvItem)
{
	if (InvItem->m_flags.test(CInventoryItem::Fbelt) && !Actor()->inventory().InBelt(InvItem))
		m_pUIBeltList->enable_highlight(true);

	for (const u8 slot : InvItem->GetSlots())
		if (auto DdList = m_slots_array[slot]; DdList && (!Actor()->inventory().InSlot(InvItem) || InvItem->GetSlot() != slot))
			DdList->enable_highlight(true);
}


void CUIInventoryWnd::AttachAddon(PIItem item_to_upgrade)
{
	PlaySnd										(eInvAttachAddon);
	R_ASSERT									(item_to_upgrade);
	if (OnClient())
	{
		NET_Packet								P;
		item_to_upgrade->object().u_EventGen	(P, GE_ADDON_ATTACH, item_to_upgrade->object().ID());
		P.w_u32									(CurrentIItem()->object().ID());
		item_to_upgrade->object().u_EventSend	(P);
	};

	item_to_upgrade->Attach						(CurrentIItem(), true);


	//спрятать вещь из активного слота в инвентарь на время вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && item_to_upgrade == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
	SetCurrentItem								(NULL);
}

void CUIInventoryWnd::DetachAddon(const char* addon_name)
{
	PlaySnd										(eInvDetachAddon);
	if (OnClient())
	{
		NET_Packet								P;
		CurrentIItem()->object().u_EventGen		(P, GE_ADDON_DETACH, CurrentIItem()->object().ID());
		P.w_stringZ								(addon_name);
		CurrentIItem()->object().u_EventSend	(P);
	};
	CurrentIItem()->Detach						(addon_name, true);

	//спрятать вещь из активного слота в инвентарь на время вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && CurrentIItem() == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
}

void	CUIInventoryWnd::SendEvent_Item_Drop(PIItem	pItem)
{
	pItem->SetDropManual			(TRUE);

	if( OnClient() )
	{
		NET_Packet					P;
		pItem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, pItem->object().H_Parent()->ID());
		P.w_u16						(pItem->object().ID());
		pItem->object().u_EventSend(P);
	}
	g_pInvWnd->PlaySnd				(eInvDropItem);
	m_b_need_update_stats = true;
};

void CUIInventoryWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemDrop);
	lst->m_f_item_start_drag = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemStartDrag);
	lst->m_f_item_db_click = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemDbClick);
	lst->m_f_item_selected = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemSelected);
	lst->m_f_item_rbutton_click = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemRButtonClick);
}


#include "../xr_level_controller.h"
#include <dinput.h>

bool CUIInventoryWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if(m_b_need_reinit)
		return true;

	if (UIPropertiesBox.GetVisible())
		UIPropertiesBox.OnKeyboard(dik, keyboard_action);

	if ( is_binded(kDROP, dik) )
	{
		if(WINDOW_KEY_PRESSED==keyboard_action)
			DropCurrentItem(false);
		return true;
	}

	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
#ifdef DEBUG
		if(DIK_NUMPAD7 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(-0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
		else if(DIK_NUMPAD8 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
#endif
	}
	if( inherited::OnKeyboard(dik,keyboard_action) )return true;

	return false;
}

void CUIInventoryWnd::UpdateCustomDraw()
{
	if (!smart_cast<CActor*>(Level().CurrentEntity())) 
		return;

	auto& inv = Actor()->inventory();
	u32 belt_size = inv.BeltSize();

	m_pUIBeltList->SetCellsAvailable(belt_size);
	/*m_pUIBeltList->SetCellsCapacity({ (int)belt_size, 1 });*/

	if (!Actor()->GetBackpack()) {
		m_pUIBagList->SetCellsAvailable(0);
	}
	else {
		m_pUIBagList->ResetCellsAvailable();
	}

	for (u8 i = 0; i < SLOTS_TOTAL; ++i) {
		auto list = GetSlotList(i);
		if (!list) 
			continue;
		if (inv.IsSlotDisabled(i)) {
			if(i==HELMET_SLOT)
				list->SetCellsAvailable(0);
			else
				list->SetCellsCapacity({});
		}else{
			if (i == HELMET_SLOT)
				list->ResetCellsAvailable();
			else
				list->ResetCellsCapacity();
		}
	}

	InitInventory_delayed();
}

void CUIInventoryWnd::CheckForcedWeightVolumeUpdate() {
	bool need_update{};
	auto place_to_search = psActorFlags.test(AF_ARTEFACTS_FROM_ALL) ? GetInventory()->m_all : GetInventory()->m_belt;
	for (const auto& item : place_to_search) {
		auto artefact = smart_cast<CArtefact*>(item);
		if (artefact && !fis_zero(artefact->m_fTTLOnDecrease) && !fis_zero(artefact->GetCondition()) &&
			(!fis_zero(artefact->GetItemEffect(CInventoryItem::eAdditionalWeight)) || !fis_zero(artefact->GetItemEffect(CInventoryItem::eAdditionalVolume)))) {
			need_update = true;
			break;
		}
	}
	if (need_update)
		UpdateWeightVolume();
}