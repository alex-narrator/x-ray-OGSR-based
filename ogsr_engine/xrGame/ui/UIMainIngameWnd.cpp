#include "stdafx.h"

#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "../UIZoneMap.h"


#include <dinput.h>
#include "../actor.h"
#include "../HUDManager.h"
#include "../PDA.h"
#include "CustomOutfit.h"
#include "../character_info.h"
#include "../inventory.h"
#include "../UIGameSP.h"
#include "../weaponmagazined.h"
#include "../missile.h"
#include "../Grenade.h"
#include "../xrServer_objects_ALife.h"
#include "../alife_simulator.h"
#include "../alife_object_registry.h"
#include "../game_cl_base.h"
#include "../level.h"
#include "../seniority_hierarchy_holder.h"

#include "../date_time.h"
#include "../xrServer_Objects_ALife_Monsters.h"
#include "../../xr_3da/LightAnimLibrary.h"

#include "UIInventoryUtilities.h"


#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "../alife_registry_wrappers.h"
#include "../actorcondition.h"

#include "../string_table.h"
#include "clsid_game.h"
#include "UIArtefactPanel.h"
#include "UIMap.h"

#ifdef DEBUG
#	include "../attachable_item.h"
#	include "..\..\xr_3da\xr_input.h"
#endif

#include "UIScrollView.h"
#include "map_hint.h"
#include "UIColorAnimatorWrapper.h"
#include "../game_news.h"

using namespace InventoryUtilities;

constexpr auto DEFAULT_MAP_SCALE = 1.f;
constexpr auto MAININGAME_XML = "maingame.xml";


static CUIMainIngameWnd* GetMainIngameWindow()
{
	if (g_hud)
	{
		CUI *pUI = g_hud->GetUI();
		if (pUI)
			return pUI->UIMainIngameWnd;
	}
	return nullptr;
}

static CUIStatic* warn_icon_list[11]{};

// alpet: для возможности внешнего контроля иконок (используется в NLC6 вместо типичных индикаторов). Никак не влияет на игру для остальных модов.
static bool external_icon_ctrl = false;

// позволяет расцветить иконку или изменить её размер
static bool SetupGameIcon(CUIMainIngameWnd::EWarningIcons icon, u32 cl, float width, float height)
{
	auto window = GetMainIngameWindow();
	if (!window)
	{
		Msg("!![SetupGameIcon] failed due GetMainIngameWindow() returned NULL");
		return false;
	}

	R_ASSERT(icon > 0 && icon < std::size(warn_icon_list), "!!Invalid first arg for setup_game_icon!");

	CUIStatic* sIcon = warn_icon_list[icon];

	if (width > 0 && height > 0)
	{
		sIcon->SetWidth(width);
		sIcon->SetHeight(height);
		sIcon->SetStretchTexture(cl > 0);
	}
	else
		window->SetWarningIconColor(icon, cl);

	external_icon_ctrl = true;
	return true;
}

CUIMainIngameWnd::CUIMainIngameWnd()
{
	m_pActor						= nullptr;
	UIZoneMap						= xr_new<CUIZoneMap>();
	m_pPickUpItem					= nullptr;
	m_artefactPanel					= xr_new<CUIArtefactPanel>();
	m_quickSlotPanel				= xr_new<CUIQuickSlotPanel>();

	warn_icon_list[ewiWeaponJammed]	= &UIWeaponJammedIcon;	
	warn_icon_list[ewiRadiation]	= &UIRadiaitionIcon;
	warn_icon_list[ewiWound]		= &UIWoundIcon;
	warn_icon_list[ewiStarvation]	= &UIStarvationIcon;
	warn_icon_list[ewiPsyHealth]	= &UIPsyHealthIcon;
	warn_icon_list[ewiArmor]		= &UIArmorIcon;
	warn_icon_list[ewiHealth]		= &UIHealthIcon;
	warn_icon_list[ewiPower]		= &UIPowerIcon;
	warn_icon_list[ewiInvincible]	= &UIInvincibleIcon;
	warn_icon_list[ewiThirst]		= &UIThirstIcon;
}

#include "UIProgressShape.h"
extern CUIProgressShape* g_MissileForceShape;

CUIMainIngameWnd::~CUIMainIngameWnd()
{
	DestroyFlashingIcons		();
	xr_delete					(UIZoneMap);
	xr_delete					(m_artefactPanel);
	xr_delete					(m_quickSlotPanel);
	HUD_SOUND::DestroySound		(m_contactSnd);
	xr_delete					(g_MissileForceShape);
}

void CUIMainIngameWnd::Init()
{
	CUIXml						uiXml;
	uiXml.Init					(CONFIG_PATH, UI_PATH, MAININGAME_XML);
	
	CUIXmlInit					xml_init;
	CUIWindow::Init				(0,0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

	Enable(false);


	AttachChild					(&UIStaticHealth);
	xml_init.InitStatic			(uiXml, "static_health", 0, &UIStaticHealth);

	AttachChild					(&UIStaticArmor);
	xml_init.InitStatic			(uiXml, "static_armor", 0, &UIStaticArmor);

	AttachChild					(&UIWeaponBack);
	xml_init.InitStatic			(uiXml, "static_weapon", 0, &UIWeaponBack);

	UIWeaponBack.AttachChild	(&UIWeaponSignAmmo);
	xml_init.InitStatic			(uiXml, "static_ammo", 0, &UIWeaponSignAmmo);
	UIWeaponSignAmmo.SetElipsis	(CUIStatic::eepEnd, 2);

	UIWeaponBack.AttachChild	(&UIWeaponIcon);
	xml_init.InitStatic			(uiXml, "static_wpn_icon", 0, &UIWeaponIcon);
	UIWeaponIcon.SetShader		(GetEquipmentIconsShader());
	UIWeaponIcon_rect			= UIWeaponIcon.GetWndRect();
	//---------------------------------------------------------
	AttachChild					(&UIPickUpItemIcon);
	xml_init.InitStatic			(uiXml, "pick_up_item", 0, &UIPickUpItemIcon);
	UIPickUpItemIcon.SetShader	(GetEquipmentIconsShader());
//	UIPickUpItemIcon.ClipperOn	();
	UIPickUpItemIcon.Show(false);

	m_iPickUpItemIconWidth		= UIPickUpItemIcon.GetWidth();
	m_iPickUpItemIconHeight		= UIPickUpItemIcon.GetHeight();
	m_iPickUpItemIconX			= UIPickUpItemIcon.GetWndRect().left;
	m_iPickUpItemIconY			= UIPickUpItemIcon.GetWndRect().top;
	//---------------------------------------------------------


	UIWeaponIcon.Enable			(false);

	//индикаторы 
	UIZoneMap->Init				();
	UIZoneMap->SetScale			(DEFAULT_MAP_SCALE);

		xml_init.InitStatic					(uiXml, "static_pda_online", 0, &UIPdaOnline);
		UIZoneMap->Background().AttachChild	(&UIPdaOnline);


	//Полоса прогресса здоровья
	UIStaticHealth.AttachChild	(&UIHealthBar);
//.	xml_init.InitAutoStaticGroup(uiXml,"static_health", &UIStaticHealth);
	xml_init.InitProgressBar	(uiXml, "progress_bar_health", 0, &UIHealthBar);

	//Полоса прогресса армора
	UIStaticArmor.AttachChild	(&UIArmorBar);
//.	xml_init.InitAutoStaticGroup(uiXml,"static_armor", &UIStaticArmor);
	xml_init.InitProgressBar	(uiXml, "progress_bar_armor", 0, &UIArmorBar);

	

	// Подсказки, которые возникают при наведении прицела на объект
	AttachChild					(&UIStaticQuickHelp);
	xml_init.InitStatic			(uiXml, "quick_info", 0, &UIStaticQuickHelp);

	uiXml.SetLocalRoot			(uiXml.GetRoot());

	m_UIIcons					= xr_new<CUIScrollView>(); m_UIIcons->SetAutoDelete(true);
	xml_init.InitScrollView		(uiXml, "icons_scroll_view", 0, m_UIIcons);
	AttachChild					(m_UIIcons);

// Загружаем иконки 
	xml_init.InitStatic			(uiXml, "starvation_static", 0, &UIStarvationIcon);
	UIStarvationIcon.Show		(false);

	xml_init.InitStatic			(uiXml, "psy_health_static", 0, &UIPsyHealthIcon);
	UIPsyHealthIcon.Show		(false);

	xml_init.InitStatic			(uiXml, "weapon_jammed_static", 0, &UIWeaponJammedIcon);
	UIWeaponJammedIcon.Show		(false);

	xml_init.InitStatic			(uiXml, "radiation_static", 0, &UIRadiaitionIcon);
	UIRadiaitionIcon.Show		(false);

	xml_init.InitStatic			(uiXml, "wound_static", 0, &UIWoundIcon);
	UIWoundIcon.Show			(false);

	xml_init.InitStatic			(uiXml, "invincible_static", 0, &UIInvincibleIcon);
	UIInvincibleIcon.Show		(false);

	if (Core.Features.test(xrCore::Feature::actor_thirst))
	{
		xml_init.InitStatic(uiXml, "thirst_static", 0, &UIThirstIcon);
		UIThirstIcon.Show(false);
	}

	//--
	xml_init.InitStatic         (uiXml, "armor_static", 0, &UIArmorIcon);
	UIArmorIcon.Show            (false);

	xml_init.InitStatic         (uiXml, "health_static", 0, &UIHealthIcon);
	UIHealthIcon.Show           (false);

	xml_init.InitStatic         (uiXml, "power_static", 0, &UIPowerIcon);
	UIPowerIcon.Show(false);
	//--

	constexpr const char* warningStrings[] =
	{
		"jammed",
		"radiation",
		"wounds",
		"starvation",
		"psy",
		"armor",
		"health",
		"power",
		"invincible", // Not used
		"thirst",
	};

	// Загружаем пороговые значения для индикаторов
	EWarningIcons i = ewiWeaponJammed;
	while (i <= (Core.Features.test(xrCore::Feature::actor_thirst) ? ewiThirst: ewiInvincible))
	{
		// Читаем данные порогов для каждого индикатора
		const char* cfgRecord = pSettings->r_string("main_ingame_indicators_thresholds", warningStrings[static_cast<int>(i) - 1]);
		u32 count = _GetItemCount(cfgRecord);

		char	singleThreshold[8];
		float	f = 0;
		for (u32 k = 0; k < count; ++k)
		{
			_GetItem(cfgRecord, k, singleThreshold);
			sscanf(singleThreshold, "%f", &f);

			m_Thresholds[i].push_back(f);
		}

		i = static_cast<EWarningIcons>(i + 1);

		if (i == ewiInvincible)
			i = static_cast<EWarningIcons>(i + 1);
	}


	// Flashing icons initialize
	uiXml.SetLocalRoot						(uiXml.NavigateToNode("flashing_icons"));
	InitFlashingIcons						(&uiXml);

	uiXml.SetLocalRoot						(uiXml.GetRoot());
	
	AttachChild								(&UICarPanel);
	xml_init.InitWindow						(uiXml, "car_panel", 0, &UICarPanel);

	AttachChild								(&UIMotionIcon);
	UIMotionIcon.Init						();

	m_artefactPanel->InitFromXML			(uiXml, "artefact_panel", 0);
	this->AttachChild						(m_artefactPanel);	

	m_quickSlotPanel->Init					();
	m_quickSlotPanel->SetWindowName			("quick_slot_panel");
	this->AttachChild						(m_quickSlotPanel);

	HUD_SOUND::LoadSound					("maingame_ui", "snd_new_contact"		, m_contactSnd		, SOUND_TYPE_IDLE);
}

void CUIMainIngameWnd::Draw()
{
	if(!m_pActor) return;

	CUIWindow::Draw();

	if (IsHUDElementAllowed(ePDA)) 
		UIZoneMap->Render();

	RenderQuickInfos();
}

void CUIMainIngameWnd::SetAmmoIcon (const shared_str& sect_name)
{
	if ( !sect_name.size() )
	{
		UIWeaponIcon.Show			(false);
		return;
	};

	UIWeaponIcon.Show			(true);
	//properties used by inventory menu
	CIconParams icon_params(sect_name);

	icon_params.set_shader( &UIWeaponIcon );

	float iGridWidth = icon_params.grid_width;

	float w = std::clamp(iGridWidth, 1.f, 2.f) * INV_GRID_WIDTH;
	float h = INV_GRID_HEIGHT;
	w *= UI()->get_current_kx();

	float x = UIWeaponIcon_rect.x1;
	if (iGridWidth < 2.f)
		x += w / 2.0f;

	UIWeaponIcon.SetWndPos(x, UIWeaponIcon_rect.y1);

	UIWeaponIcon.SetWidth(w);
	UIWeaponIcon.SetHeight(h);
};

void CUIMainIngameWnd::Update()
{
	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!m_pActor) 
	{
		CUIWindow::Update		();
		return;
	}

	if( !(Device.dwFrame%30) )
	{
			string256				text_str;
			CPda* _pda	= m_pActor->GetPDA();
			bool pda_workable = m_pActor->HasPDAWorkable();
			u32 _cn		= 0;
			if(pda_workable && 0!= (_cn=_pda->ActiveContactsNum()) )
			{
				sprintf_s(text_str, "%d", _cn);
				UIPdaOnline.SetText(text_str);
			}
			else
			{
				UIPdaOnline.SetText("");
			}
	};

	if( !(Device.dwFrame%5) )
	{

		if(!(Device.dwFrame%30))
		{
			bool b_God = (GodMode()||(!Game().local_player)) ? true : Game().local_player->testFlag(GAME_PLAYER_FLAG_INVINCIBLE);
			if(b_God)
				SetWarningIconColor	(ewiInvincible,0xffffffff);
			else
				if (!external_icon_ctrl)
					TurnOffWarningIcon (ewiInvincible);
		}

		// Health bar
		bool show_bar = eHudLaconicOff == g_eHudLaconic;
		UIHealthBar.Show(show_bar);
		UIStaticHealth.Show(show_bar);
		if(show_bar)
			UIHealthBar.SetProgressPos(m_pActor->GetfHealth() * 100.0f);
		// Armor bar
		auto pOutfit = m_pActor->GetOutfit();
		show_bar = pOutfit && eHudLaconicOff == g_eHudLaconic;
		UIArmorBar.Show					(show_bar);
		UIStaticArmor.Show				(show_bar);
		if(show_bar)
			UIArmorBar.SetProgressPos		(pOutfit->GetCondition()*100);

		UpdateActiveItemInfo				();

		bool b_show_icon = eHudLaconicWarning == g_eHudLaconic;
		auto cond = &m_pActor->conditions();

		EWarningIcons i = ewiWeaponJammed;
		while (!external_icon_ctrl && i <= (Core.Features.test(xrCore::Feature::actor_thirst) ? ewiThirst : ewiInvincible))
		{
			float value = 0;
			switch (i)
			{
			case ewiRadiation:
				if (IsHUDElementAllowed(eDetector))
					value = cond->GetRadiation();
				break;
			case ewiWound:
				value = cond->BleedingSpeed();
				break;
			case ewiWeaponJammed:
				if (IsHUDElementAllowed(eActiveItem))
					value = 1 - m_pActor->inventory().ActiveItem()->GetConditionToShow();
				break;
			case ewiStarvation:
				value = 1 - cond->GetSatiety();
				break;
			case ewiThirst:
				value = 1 - cond->GetThirst();
				break;
			case ewiPsyHealth:
				value = 1 - cond->GetPsyHealth();
				break;
				//
			case ewiArmor:
				if (IsHUDElementAllowed(eArmor))
					value = 1 - pOutfit->GetCondition();
				break;
			case ewiHealth:
				if (b_show_icon)
					value = 1 - cond->GetHealth();
				break;
			case ewiPower:
				if (b_show_icon)
					value = 1 - cond->GetPower();
				break;
				//
			default:
				R_ASSERT(!"Unknown type of warning icon");
			}

			// Минимальное и максимальное значения границы
			float min = m_Thresholds[i].front();
			float max = m_Thresholds[i].back();

			if (m_Thresholds[i].size() > 1)
			{
				xr_vector<float>::reverse_iterator	rit;

				// Сначала проверяем на точное соответсвие
				rit  = std::find(m_Thresholds[i].rbegin(), m_Thresholds[i].rend(), value);

				// Если его нет, то берем последнее меньшее значение ()
				if (rit == m_Thresholds[i].rend())
					rit = std::find_if(m_Thresholds[i].rbegin(), m_Thresholds[i].rend(), std::bind(std::less<float>(), std::placeholders::_1, value));

				if (rit != m_Thresholds[i].rend())
				{
					float v = *rit;
					SetWarningIconColor(i, color_argb(0xFF, clampr<u32>(static_cast<u32>(255 * ((v - min) / (max - min) * 2)), 0, 255), 
						clampr<u32>(static_cast<u32>(255 * (2.0f - (v - min) / (max - min) * 2)), 0, 255),
						0));
				}else
					TurnOffWarningIcon(i);
			}
			else
			{
				float val = 1 - value;
				float treshold = 1 - min;
				clamp<float>(treshold, 0.01, 1.f);

				if (val <= treshold)
				{
					float v = val / treshold;
					clamp<float>(v, 0.f, 1.f);
					SetWarningIconColor(i, color_argb(
						0xFF,
						255,
						clampr<u32>(static_cast<u32>(255 * v), 0, 255),
						0
					));
				}
				else
					TurnOffWarningIcon(i);
			}

			i = (EWarningIcons)(i + 1);

			if (i == ewiInvincible)
				i = (EWarningIcons)(i + 1);
		}
	}

	UIZoneMap->UpdateRadar			(Device.vCameraPosition);
	float h,p;
	Device.vCameraDirection.getHP	(h,p);
	UIZoneMap->SetHeading			(-h);

	UpdatePickUpItem				();

	bool show_panels = IsHUDElementAllowed(eGear);
	m_quickSlotPanel->Show	(show_panels);
	m_artefactPanel->Show	(show_panels); //отрисовка панели артефактов

	UpdateFlashingIcons(); //обновляем состояние мигающих иконок

	CUIWindow::Update				();
}

bool CUIMainIngameWnd::OnKeyboardPress(int dik)
{
	if(Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION)))
	{
		switch(dik)
		{
		case DIK_NUMPADMINUS:
			UIZoneMap->ZoomOut();
			return true;
			break;
		case DIK_NUMPADPLUS:
			UIZoneMap->ZoomIn();
			return true;
			break;
		}
	}
	else
	{
		switch(dik)
		{
		case DIK_NUMPADMINUS:
			//.HideAll();
			HUD().GetUI()->HideGameIndicators();
			return true;
			break;
		case DIK_NUMPADPLUS:
			//.ShowAll();
			HUD().GetUI()->ShowGameIndicators();
			return true;
			break;
		}
	}

	return false;
}


void CUIMainIngameWnd::RenderQuickInfos()
{
	if (!m_pActor)
		return;

	static CGameObject *pObject			= NULL;
	LPCSTR actor_action					= m_pActor->GetDefaultActionForObject();
	UIStaticQuickHelp.Show				(NULL!=actor_action);

	if(NULL!=actor_action){
		if(stricmp(actor_action,UIStaticQuickHelp.GetText()))
			UIStaticQuickHelp.SetTextST				(actor_action);
	}

	if (pObject!=m_pActor->ObjectWeLookingAt())
	{
		UIStaticQuickHelp.SetTextST				(actor_action);
		UIStaticQuickHelp.ResetClrAnimation		();
		pObject	= m_pActor->ObjectWeLookingAt	();
	}
}

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
	VERIFY(news->texture_name.size());
	CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (pActor->HasPDAWorkable())
		HUD().GetUI()->m_pMessagesWnd->AddIconedPdaMessage(*(news->texture_name), news->tex_rect, news->SingleLineText(), news->show_time);
}

void CUIMainIngameWnd::SetWarningIconColor(CUIStatic* s, const u32 cl)
{
	int bOn = (cl>>24);
	bool bIsShown = s->IsShown();

	if(bOn)
		s->SetColor	(cl);

	if(bOn&&!bIsShown){
		m_UIIcons->AddWindow	(s, false);
		s->Show					(true);
	}

	if(!bOn&&bIsShown){
		m_UIIcons->RemoveWindow	(s);
		s->Show					(false);
	}
}

void CUIMainIngameWnd::SetWarningIconColor(EWarningIcons icon, const u32 cl)
{
	// Задаем цвет требуемой иконки
	switch(icon)
	{
	case ewiAll:
		break;
	case ewiWeaponJammed:
		SetWarningIconColor		(&UIWeaponJammedIcon, cl);
		break;
	case ewiRadiation:
		SetWarningIconColor		(&UIRadiaitionIcon, cl);
		break;
	case ewiWound:
		SetWarningIconColor		(&UIWoundIcon, cl);
		break;
	case ewiStarvation:
		SetWarningIconColor		(&UIStarvationIcon, cl);
		break;
	case ewiPsyHealth:
		SetWarningIconColor		(&UIPsyHealthIcon, cl);
		break;
		//
	case ewiArmor:
		SetWarningIconColor		(&UIArmorIcon, cl);
		break;
	case ewiHealth:
		SetWarningIconColor		(&UIHealthIcon, cl);
		break;
	case ewiPower:
		SetWarningIconColor		(&UIPowerIcon, cl);
		break;
		//
	case ewiInvincible:
		SetWarningIconColor		(&UIInvincibleIcon, cl);
		break;

	case ewiThirst:
		SetWarningIconColor(&UIThirstIcon, cl);
		break;

	default:
		R_ASSERT(!"Unknown warning icon type");
		break;
	}
}

void CUIMainIngameWnd::TurnOffWarningIcon(EWarningIcons icon)
{
	SetWarningIconColor(icon, 0x00ffffff);
}


void CUIMainIngameWnd::SetFlashIconState_(EFlashingIcons type, bool enable)
{
	// Включаем анимацию требуемой иконки
	FlashingIcons_it icon = m_FlashingIcons.find(type);
	R_ASSERT2(icon != m_FlashingIcons.end(), "Flashing icon with this type not existed");
	icon->second->Show(enable);
}

void CUIMainIngameWnd::InitFlashingIcons(CUIXml* node)
{
	const char * const flashingIconNodeName = "flashing_icon";
	int staticsCount = node->GetNodesNum("", 0, flashingIconNodeName);

	CUIXmlInit xml_init;
	CUIStatic *pIcon = NULL;
	// Пробегаемся по всем нодам и инициализируем из них статики
	for (int i = 0; i < staticsCount; ++i)
	{
		pIcon = xr_new<CUIStatic>();
		xml_init.InitStatic(*node, flashingIconNodeName, i, pIcon);
		shared_str iconType = node->ReadAttrib(flashingIconNodeName, i, "type", "none");

		// Теперь запоминаем иконку и ее тип
		EFlashingIcons type = efiPdaTask;

		if		(iconType == "pda")		type = efiPdaTask;
		else if (iconType == "mail")	type = efiMail;
		else	R_ASSERT(!"Unknown type of mainingame flashing icon");

		R_ASSERT2(m_FlashingIcons.find(type) == m_FlashingIcons.end(), "Flashing icon with this type already exists");

		CUIStatic* &val	= m_FlashingIcons[type];
		val			= pIcon;

		AttachChild(pIcon);
		pIcon->Show(false);
	}
}

void CUIMainIngameWnd::DestroyFlashingIcons()
{
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		DetachChild(it->second);
		xr_delete(it->second);
	}

	m_FlashingIcons.clear();
}

void CUIMainIngameWnd::UpdateFlashingIcons()
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		if (pActor->HasPDAWorkable())
			it->second->Update();
		else
			it->second->Show(false);
	}
}

void CUIMainIngameWnd::AnimateContacts(bool b_snd)
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor->HasPDAWorkable()) return;

	UIPdaOnline.ResetClrAnimation	();

	if(b_snd)
		HUD_SOUND::PlaySound	(m_contactSnd, Fvector().set(0,0,0), 0, true );

}


void CUIMainIngameWnd::SetPickUpItem	(CInventoryItem* PickUpItem)
{
//	m_pPickUpItem = PickUpItem;
	if (m_pPickUpItem != PickUpItem)
	{
		m_pPickUpItem = PickUpItem;
		UIPickUpItemIcon.Show(false);
		UIPickUpItemIcon.DetachAll();
	}
};

#include "UICellCustomItems.h"
#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../Actor.h"

typedef CUIWeaponCellItem::eAddonType eAddonType;

CUIStatic* init_addon(
	CUIWeaponCellItem *cell_item,
	LPCSTR sect,
	float scale,
	float scale_x,
	eAddonType idx)
{
	CUIStatic *addon = xr_new<CUIStatic>();
	addon->SetAutoDelete(true);
	
	auto pos = cell_item->get_addon_offset(idx);
	pos.x	  *= scale *scale_x; 
	pos.y	  *= scale;
	
	CIconParams     params(sect);
	Frect rect = params.original_rect();			
	params.set_shader( addon );
	addon->SetWndRect(pos.x, pos.y, rect.width()*scale*scale_x, rect.height()*scale);
	addon->SetColor(color_rgba(255, 255, 255, 192));

	return addon;
}

void CUIMainIngameWnd::UpdatePickUpItem	()
{
	if (!m_pPickUpItem || !Level().CurrentViewEntity() || Level().CurrentViewEntity()->CLS_ID != CLSID_OBJECT_ACTOR) 
	{
		if (UIPickUpItemIcon.IsShown())
		{
			UIPickUpItemIcon.Show(false);
		}

		return;
	};
	if (UIPickUpItemIcon.IsShown()) return;

	//properties used by inventory menu
	CIconParams &params = m_pPickUpItem->m_icon_params;
	Frect rect = params.original_rect();

	float scale_x = m_iPickUpItemIconWidth / rect.width();

	float scale_y = m_iPickUpItemIconHeight / rect.height();

	scale_x = (scale_x>1) ? 1.0f : scale_x;
	scale_y = (scale_y>1) ? 1.0f : scale_y;

	float scale = scale_x<scale_y?scale_x:scale_y;

	
	params.set_shader( &UIPickUpItemIcon );

	UIPickUpItemIcon.SetWidth(rect.width()*scale*UI()->get_current_kx());
	UIPickUpItemIcon.SetHeight(rect.height()*scale);

	UIPickUpItemIcon.SetWndPos(m_iPickUpItemIconX + 
		(m_iPickUpItemIconWidth - UIPickUpItemIcon.GetWidth())/2,
		m_iPickUpItemIconY + 
		(m_iPickUpItemIconHeight - UIPickUpItemIcon.GetHeight())/2);

	UIPickUpItemIcon.SetColor(color_rgba(255,255,255,192));
	if (auto wpn = m_pPickUpItem->cast_weapon())
	{
		auto cell_item = xr_new<CUIWeaponCellItem>(wpn);
		
		if (wpn->SilencerAttachable() && wpn->IsSilencerAttached())
		{
			auto sil = init_addon(cell_item, *wpn->GetSilencerName(), scale, UI()->get_current_kx(), eAddonType::eSilencer);
			UIPickUpItemIcon.AttachChild(sil);
		}
		
		if (wpn->ScopeAttachable() && wpn->IsScopeAttached())
		{
			auto scope = init_addon(cell_item, *wpn->GetScopeName(), scale, UI()->get_current_kx(), eAddonType::eScope);
			UIPickUpItemIcon.AttachChild(scope);
		}
		
		if (wpn->GrenadeLauncherAttachable() && wpn->IsGrenadeLauncherAttached())
		{
			auto launcher = init_addon(cell_item, *wpn->GetGrenadeLauncherName(), scale, UI()->get_current_kx(), eAddonType::eLauncher);
			UIPickUpItemIcon.AttachChild(launcher);
		}
		delete_data(cell_item);
	}

	// Real Wolf: Колбек для скриптового добавления своих иконок. 10.08.2014.
	g_actor->callback(GameObject::eUIPickUpItemShowing)(m_pPickUpItem->object().lua_game_object(), &UIPickUpItemIcon);

	UIPickUpItemIcon.Show(true);
};

void CUIMainIngameWnd::UpdateActiveItemInfo()
{
	PIItem item		=  m_pActor->inventory().ActiveItem();
	bool show_info = item && item->NeedBriefInfo() && (IsHUDElementAllowed(eActiveItem) || IsHUDElementAllowed(eGear));

	UIWeaponBack.Show			(show_info);
	UIWeaponSignAmmo.Show		(show_info);

	if (show_info) {
		xr_string					str_name;
		xr_string					icon_sect_name;
		xr_string					str_count;

		item->GetBriefInfo(str_name, icon_sect_name, str_count);

		UIWeaponBack.SetText(str_name.c_str());
		UIWeaponSignAmmo.SetText(str_count.c_str());
		SetAmmoIcon(icon_sect_name.c_str());
	}
}

void CUIMainIngameWnd::OnConnected()
{
	UIZoneMap->SetupCurrentMap		();
}

void CUIMainIngameWnd::reset_ui()
{
	m_pActor						= NULL;
	//m_pWeapon						= NULL;
	//m_pGrenade						= NULL;
	//m_pItem							= NULL;
	m_pPickUpItem					= NULL;
	UIMotionIcon.ResetVisibility	();
}

bool CUIMainIngameWnd::IsHUDElementAllowed(EHUDElement element)
{
	if (Device.Paused() || m_pActor && !m_pActor->g_Alive()) return false;

	bool allow_devices_hud = eHudLaconicOff == g_eHudLaconic || OnKeyboardHold(get_action_dik(kSCORES)) || m_pActor->inventory().GetActiveSlot() == BOLT_SLOT;

	switch (element)
	{
	case ePDA: //ПДА
	{
		return allow_devices_hud && m_pActor->GetPDA();
	}break;
	case eDetector: //Детектор (иконка радиационного заражения)
	{
		return allow_devices_hud && m_pActor->HasDetectorWorkable();
	}break;
	case eActiveItem: //Информация об предмете в руках (для оружия - кол-во/тип заряженных патронов, режим огня)
	{
		return m_pActor->inventory().ActiveItem() && (eHudLaconicOff == g_eHudLaconic || OnKeyboardHold(get_action_dik(kCHECKACTIVEITEM)));
	}break;
	case eGear: //Информация о снаряжении - панель артефактов, наполнение квикслотов, общее кол-во патронов к оружию в руках
	{
		return eHudLaconicOff == g_eHudLaconic || OnKeyboardHold(get_action_dik(kCHECKGEAR));
	}break;
	case eArmor: //Иконка состояния брони
	{
		return eHudLaconicOff != g_eHudLaconic && m_pActor->GetOutfit() && OnKeyboardHold(get_action_dik(kCHECKGEAR));
	}break;
	default:
		Msg("! unknown hud element");
		return false;
		break;
	}
}
#include "../xr_3da/XR_IOConsole.h"
bool CUIMainIngameWnd::OnKeyboardHold(int cmd){
	if (Console->bVisible) return false;
	return Level().IR_GetKeyState(cmd);
}

using namespace luabind::detail;			

template <typename T>
bool test_push_window(lua_State *L, CUIWindow *wnd)
{
	T* derived = smart_cast<T*>(wnd);
	if (derived)
	{		
		convert_to_lua<T*>(L, derived);
		return true;
	}
	return false;
}


void GetStaticRaw(CUIMainIngameWnd *wnd, lua_State *L)
{
	// wnd->GetChildWndList();
	shared_str name = lua_tostring(L, 2);
	CUIWindow *child = wnd->FindChild(name, 2); 	
	if (!child)
	{
		CUIStatic *src = &wnd->GetUIZoneMap()->Background();		
		child = src->FindChild(name, 5);
		
		if (!child)
		{
			src = &wnd->GetUIZoneMap()->ClipFrame();
			child = src->FindChild(name, 5);
		}
		if (!child)
		{
			src = &wnd->GetUIZoneMap()->Compass();
			child = src->FindChild(name, 5);
		}
	}

	if (child)
	{	
		// if (test_push_window<CUIMotionIcon>  (L, child)) return;		
		if (test_push_window<CUIProgressBar> (L, child)) return;		
		if (test_push_window<CUIStatic>		 (L, child)) return;
		if (test_push_window<CUIWindow>	     (L, child)) return;						
	}
	lua_pushnil(L);
}


using namespace luabind;

#pragma optimize("s",on)
void CUIMainIngameWnd::script_register(lua_State *L)
{

	module(L)
		[

			class_<CUIMainIngameWnd, CUIWindow>("CUIMainIngameWnd")
			.def("GetStatic",		 &GetStaticRaw, raw<2>()),
			def("get_main_window",   &GetMainIngameWindow) // get_mainingame_window better??
			, def("setup_game_icon", &SetupGameIcon)
		];

}


using namespace InventoryUtilities;

CUIQuickSlotPanel::CUIQuickSlotPanel(){}

CUIQuickSlotPanel::~CUIQuickSlotPanel(){}

void CUIQuickSlotPanel::Init()
{
	CUIXml uiXml;
	bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, "quick_slot_wnd.xml");
	R_ASSERT2(xml_result, "xml file not found 'quick_slot_wnd.xml'");

	CUIXmlInit	xml_init;

	xml_init.InitWindow(uiXml, "quick_slot_panel", 0, this);
	//
	m_QuickSlotPanelBackground = xr_new<CUIStatic>();
	m_QuickSlotPanelBackground->SetAutoDelete(true);
	AttachChild(m_QuickSlotPanelBackground);
	xml_init.InitStatic(uiXml, "quick_slot_panel:quick_slot_panel_background", 0, m_QuickSlotPanelBackground);
	//
	m_QuickSlot_0_Icon = xr_new<CUIStatic>();
	m_QuickSlot_0_Icon->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_QuickSlot_0_Icon);
	xml_init.InitStatic(uiXml, "quick_slot_panel:image_static_quick_slot_0", 0, m_QuickSlot_0_Icon);
	m_QuickSlot_0_Icon->TextureAvailable(true);
	m_QuickSlot_0_Icon->TextureOff();
	m_QuickSlot_0_Icon->ClipperOn();
	m_QuickSlot_0_Icon_Size.set(m_QuickSlot_0_Icon->GetWidth(), m_QuickSlot_0_Icon->GetHeight());
	//
	m_QuickSlot_1_Icon = xr_new<CUIStatic>();
	m_QuickSlot_1_Icon->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_QuickSlot_1_Icon);
	xml_init.InitStatic(uiXml, "quick_slot_panel:image_static_quick_slot_1", 0, m_QuickSlot_1_Icon);
	m_QuickSlot_1_Icon->TextureAvailable(true);
	m_QuickSlot_1_Icon->TextureOff();
	m_QuickSlot_1_Icon->ClipperOn();
	m_QuickSlot_1_Icon_Size.set(m_QuickSlot_1_Icon->GetWidth(), m_QuickSlot_1_Icon->GetHeight());
	//
	m_QuickSlot_2_Icon = xr_new<CUIStatic>();
	m_QuickSlot_2_Icon->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_QuickSlot_2_Icon);
	xml_init.InitStatic(uiXml, "quick_slot_panel:image_static_quick_slot_2", 0, m_QuickSlot_2_Icon);
	m_QuickSlot_2_Icon->TextureAvailable(true);
	m_QuickSlot_2_Icon->TextureOff();
	m_QuickSlot_2_Icon->ClipperOn();
	m_QuickSlot_2_Icon_Size.set(m_QuickSlot_2_Icon->GetWidth(), m_QuickSlot_2_Icon->GetHeight());
	//
	m_QuickSlot_3_Icon = xr_new<CUIStatic>();
	m_QuickSlot_3_Icon->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_QuickSlot_3_Icon);
	xml_init.InitStatic(uiXml, "quick_slot_panel:image_static_quick_slot_3", 0, m_QuickSlot_3_Icon);
	m_QuickSlot_3_Icon->TextureAvailable(true);
	m_QuickSlot_3_Icon->TextureOff();
	m_QuickSlot_3_Icon->ClipperOn();
	m_QuickSlot_3_Icon_Size.set(m_QuickSlot_3_Icon->GetWidth(), m_QuickSlot_3_Icon->GetHeight());
	//
	m_CountItemQuickSlot_0_Text = xr_new<CUIStatic>();
	m_CountItemQuickSlot_0_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_CountItemQuickSlot_0_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:count_item_quick_slot_0_text", 0, m_CountItemQuickSlot_0_Text);
	//
	m_CountItemQuickSlot_1_Text = xr_new<CUIStatic>();
	m_CountItemQuickSlot_1_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_CountItemQuickSlot_1_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:count_item_quick_slot_1_text", 0, m_CountItemQuickSlot_1_Text);
	//
	m_CountItemQuickSlot_2_Text = xr_new<CUIStatic>();
	m_CountItemQuickSlot_2_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_CountItemQuickSlot_2_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:count_item_quick_slot_2_text", 0, m_CountItemQuickSlot_2_Text);
	//
	m_CountItemQuickSlot_3_Text = xr_new<CUIStatic>();
	m_CountItemQuickSlot_3_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_CountItemQuickSlot_3_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:count_item_quick_slot_3_text", 0, m_CountItemQuickSlot_3_Text);
	//
	m_UseQuickSlot_0_Text = xr_new<CUIStatic>();
	m_UseQuickSlot_0_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_UseQuickSlot_0_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:use_quick_slot_0_text", 0, m_UseQuickSlot_0_Text);
	//
	m_UseQuickSlot_1_Text = xr_new<CUIStatic>();
	m_UseQuickSlot_1_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_UseQuickSlot_1_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:use_quick_slot_1_text", 0, m_UseQuickSlot_1_Text);
	//
	m_UseQuickSlot_2_Text = xr_new<CUIStatic>();
	m_UseQuickSlot_2_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_UseQuickSlot_2_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:use_quick_slot_2_text", 0, m_UseQuickSlot_2_Text);
	//
	m_UseQuickSlot_3_Text = xr_new<CUIStatic>();
	m_UseQuickSlot_3_Text->SetAutoDelete(true);
	m_QuickSlotPanelBackground->AttachChild(m_UseQuickSlot_3_Text);
	xml_init.InitStatic(uiXml, "quick_slot_panel:use_quick_slot_3_text", 0, m_UseQuickSlot_3_Text);
}

void CUIQuickSlotPanel::DrawItemInSlot(const PIItem itm, CUIStatic* m_QuickSlot_Icon, Fvector2 m_QuickSlot_Icon_Size)
{
	PIItem iitm = itm;

	m_QuickSlot_Icon->SetShader(GetEquipmentIconsShader());
	CIconParams icon_params = iitm->m_icon_params;
	icon_params.set_shader(m_QuickSlot_Icon);

	int iGridWidth = iitm->GetGridWidth();
	int iGridHeight = iitm->GetGridHeight();
	int iXPos = iitm->GetXPos();
	int iYPos = iitm->GetYPos();

	m_QuickSlot_Icon->GetUIStaticItem().SetOriginalRect(float(iXPos * INV_GRID_WIDTH), float(iYPos * INV_GRID_HEIGHT),
		float(iGridWidth * INV_GRID_WIDTH), float(iGridHeight * INV_GRID_HEIGHT));
	m_QuickSlot_Icon->TextureOn();
	m_QuickSlot_Icon->ClipperOn();
	m_QuickSlot_Icon->SetStretchTexture(true);

	Frect v_r = { 0.0f,
											0.0f,
											float(iGridWidth * INV_GRID_WIDTH),
											float(iGridHeight * INV_GRID_HEIGHT)
	};
	if (UI()->is_widescreen())
		v_r.x2 /= 1.328f;

	m_QuickSlot_Icon->GetUIStaticItem().SetRect(v_r);
	m_QuickSlot_Icon->SetWidth(_min(v_r.width(), m_QuickSlot_Icon_Size.x));
	m_QuickSlot_Icon->SetHeight(_min(v_r.height(), m_QuickSlot_Icon_Size.y));
	m_QuickSlot_Icon->Show(true);
}


void CUIQuickSlotPanel::Update()
{
	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());

	if (!pActor) return;

	string16	slot_use{};
	string32	str;
	shared_str itm_name;
	u32 count;
	bool SearchRuck = !psActorFlags.test(AF_QUICK_FROM_BELT);

	u8 slot = QUICK_SLOT_0;
	sprintf_s(slot_use, "ui_use_slot_%d", slot);
	auto itm = pActor->inventory().m_slots[slot].m_pIItem;

	if (itm){
		sprintf_s(str, "%s", CStringTable().translate(slot_use).c_str());
		m_UseQuickSlot_0_Text->SetText(str);
		m_UseQuickSlot_0_Text->Show(itm->cast_eatable_item() || itm->cast_hud_item());

		itm_name = itm->object().cNameSect();
		count = pActor->inventory().GetSameItemCount(itm_name.c_str(), SearchRuck);
		sprintf(str, "x%d", count);
		m_CountItemQuickSlot_0_Text->SetText(str);
		m_CountItemQuickSlot_0_Text->Show(SearchRuck);
		DrawItemInSlot(itm, m_QuickSlot_0_Icon, m_QuickSlot_0_Icon_Size);
	}else{
		m_UseQuickSlot_0_Text->Show(false);
		m_CountItemQuickSlot_0_Text->Show(false);
		m_QuickSlot_0_Icon->Show(false);
	}

	slot = QUICK_SLOT_1;
	sprintf_s(slot_use, "ui_use_slot_%d", slot);
	itm = pActor->inventory().m_slots[slot].m_pIItem;

	if (itm){
		sprintf_s(str, "%s", CStringTable().translate(slot_use).c_str());
		m_UseQuickSlot_1_Text->SetText(str);
		m_UseQuickSlot_1_Text->Show(itm->cast_eatable_item() || itm->cast_hud_item());

		itm_name = itm->object().cNameSect();
		count = pActor->inventory().GetSameItemCount(itm_name.c_str(), SearchRuck);
		sprintf(str, "x%d", count);
		m_CountItemQuickSlot_1_Text->SetText(str);
		m_CountItemQuickSlot_1_Text->Show(SearchRuck);
		DrawItemInSlot(itm, m_QuickSlot_1_Icon, m_QuickSlot_1_Icon_Size);
	}else{
		m_UseQuickSlot_1_Text->Show(false);
		m_CountItemQuickSlot_1_Text->Show(false);
		m_QuickSlot_1_Icon->Show(false);
	}

	slot = QUICK_SLOT_2;
	sprintf_s(slot_use, "ui_use_slot_%d", slot);
	itm = pActor->inventory().m_slots[slot].m_pIItem;

	if (itm){
		sprintf_s(str, "%s", CStringTable().translate(slot_use).c_str());
		m_UseQuickSlot_2_Text->SetText(str);
		m_UseQuickSlot_2_Text->Show(itm->cast_eatable_item() || itm->cast_hud_item());

		itm_name = itm->object().cNameSect();
		count = pActor->inventory().GetSameItemCount(itm_name.c_str(), SearchRuck);
		sprintf(str, "x%d", count);
		m_CountItemQuickSlot_2_Text->SetText(str);
		m_CountItemQuickSlot_2_Text->Show(SearchRuck);
		DrawItemInSlot(itm, m_QuickSlot_2_Icon, m_QuickSlot_2_Icon_Size);
	}else{
		m_UseQuickSlot_2_Text->Show(false);
		m_CountItemQuickSlot_2_Text->Show(false);
		m_QuickSlot_2_Icon->Show(false);
	}

	slot = QUICK_SLOT_3;
	sprintf_s(slot_use, "ui_use_slot_%d", slot);
	itm = pActor->inventory().m_slots[slot].m_pIItem;

	if (itm){
		sprintf_s(str, "%s", CStringTable().translate(slot_use).c_str());
		m_UseQuickSlot_3_Text->SetText(str);
		m_UseQuickSlot_3_Text->Show(itm->cast_eatable_item() || itm->cast_hud_item());

		itm_name = itm->object().cNameSect();
		count = pActor->inventory().GetSameItemCount(itm_name.c_str(), SearchRuck);
		sprintf(str, "x%d", count);
		m_CountItemQuickSlot_3_Text->SetText(str);
		m_CountItemQuickSlot_3_Text->Show(SearchRuck);
		DrawItemInSlot(itm, m_QuickSlot_3_Icon, m_QuickSlot_3_Icon_Size);
	}else{
		m_UseQuickSlot_3_Text->Show(false);
		m_CountItemQuickSlot_3_Text->Show(false);
		m_QuickSlot_3_Icon->Show(false);
	}
}