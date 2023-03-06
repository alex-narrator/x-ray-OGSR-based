#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "Actor.h"
#include "Addons.h"
#include "Artifact.h"
#include "eatable_item.h"
#include "BottleItem.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "PowerBattery.h"
#include "inventory.h"
#include "game_base.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "UICellItem.h"
#include "UIListBoxItem.h"
#include "CustomOutfit.h"
#include "Vest.h"
#include "InventoryContainer.h"
#include "Warbelt.h"
#include "string_table.h"
#include <regex>

void CUIInventoryWnd::EatItem(PIItem itm)
{
	SetCurrentItem		(nullptr);
	m_b_need_reinit		= true;
	if(!itm->Useful())	return;
	GetInventory()->Eat	(itm);
}

void CUIInventoryWnd::ActivatePropertiesBox()
{
	// Флаг-признак для невлючения пункта контекстного меню: Dreess Outfit, если костюм уже надет
	bool bAlreadyDressed = false; 

	UIPropertiesBox.RemoveAll();

	auto pEatableItem	= smart_cast<CEatableItem*>		(CurrentIItem());
	auto pOutfit		= smart_cast<CCustomOutfit*>	(CurrentIItem());
	auto pContainer		= smart_cast<CInventoryContainer*>(CurrentIItem());
	auto pWarbelt		= smart_cast<CWarbelt*>			(CurrentIItem());
	auto pVest			= smart_cast<CVest*>			(CurrentIItem());
	auto pWeapon		= smart_cast<CWeapon*>			(CurrentIItem());
	auto pAmmo			= smart_cast<CWeaponAmmo*>		(CurrentIItem());
	
	const auto& inv = m_pInv;
	auto pBackpack = pContainer && inv->CanPutInSlot(pContainer);//pContainer->GetSlot() == BACKPACK_SLOT;

	string1024			temp;
    
	bool b_show = false;
	bool b_many = CurrentItem()->ChildsCount();
	LPCSTR _many = b_many ? "•" : "";
	LPCSTR _addon_name{};
	LPCSTR detach_tip = CurrentIItem()->GetDetachMenuTip();
	bool b_wearable = (pOutfit || pVest || pWarbelt || pBackpack);

	bool to_quick{};
	if (!b_wearable && CurrentIItem()->GetSlot() != NO_ACTIVE_SLOT) {
		auto& slots = CurrentIItem()->GetSlots();
		for (u8 i = 0; i < (u8)slots.size(); ++i) {
			auto slot = slots[i];
			if (slot != NO_ACTIVE_SLOT && inv->IsSlotAllowed(slot)) {
				if (!inv->m_slots[slot].m_pIItem || inv->m_slots[slot].m_pIItem != CurrentIItem()) {
					if (slot >= QUICK_SLOT_0 && slot <= QUICK_SLOT_3) {
						to_quick = true;
						continue;
					}
					string128 full_action_text;
					strconcat(sizeof(full_action_text), full_action_text, "st_move_to_slot_", std::to_string(slot).c_str());
					UIPropertiesBox.AddItem(full_action_text, (void*)(__int64)slot, INVENTORY_TO_SLOT_ACTION);
					b_show = true;
				}
			}
		}
	}
	if (to_quick) {
		UIPropertiesBox.AddItem("st_to_quick_slot", NULL, INVENTORY_TO_QUICK);
		b_show = true;
	}

	if (CurrentIItem()->Vest() && inv->CanPutInVest(CurrentIItem())) {
		UIPropertiesBox.AddItem("st_move_to_vest", NULL, INVENTORY_TO_VEST_ACTION);
		b_show = true;
	}

	if (CurrentIItem()->Belt() && inv->CanPutInBelt(CurrentIItem())) {
		UIPropertiesBox.AddItem("st_move_on_belt", NULL, INVENTORY_TO_BELT_ACTION);
		b_show = true;
	}

	if(CurrentIItem()->Ruck() && inv->CanPutInRuck(CurrentIItem()) && 
		(CurrentIItem()->GetSlot() == NO_ACTIVE_SLOT || !inv->m_slots[CurrentIItem()->GetSlot()].m_bPersistent) )
	{
		UIPropertiesBox.AddItem(b_wearable ? "st_undress_outfit" : "st_move_to_bag", NULL, INVENTORY_TO_BAG_ACTION);

		bAlreadyDressed = true;
		b_show			= true;
	}

	if(b_wearable && !bAlreadyDressed ){
		UIPropertiesBox.AddItem("st_dress_outfit",  NULL, INVENTORY_TO_SLOT_ACTION);
		b_show = true;
	}

	const char* _addon_sect{};

	if (pVest && pVest->IsPlateInstalled() && pVest->CanDetach(pVest->GetPlateName().c_str())) {
		_addon_sect = pVest->GetPlateName().c_str();
		_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
		sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
		UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
		b_show = true;
	}

	if (CurrentIItem()->IsPowerSourceAttachable() && CurrentIItem()->IsPowerSourceAttached() && CurrentIItem()->CanDetach(CurrentIItem()->GetPowerSourceName().c_str())) {
		_addon_sect = CurrentIItem()->GetPowerSourceName().c_str();
		_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
		sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
		UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
		b_show = true;
	}

	if (pAmmo){
		LPCSTR _ammo_sect;
		if (pAmmo->IsBoxReloadable()){
			//unload AmmoBox
			sprintf(temp, "%s%s", _many, CStringTable().translate("st_unload_magazine").c_str());
			UIPropertiesBox.AddItem(temp, NULL, INVENTORY_UNLOAD_AMMO_BOX);

			b_show = true;
			//reload AmmoBox
			if (pAmmo->m_boxCurr < pAmmo->m_boxSize){
				_ammo_sect = pAmmo->m_ammoSect.c_str();
				if (inv->GetAmmoByLimit(_ammo_sect, true, false)){
					sprintf(temp, "%s%s %s", _many, 
						CStringTable().translate("st_load_ammo_type").c_str(),
						CStringTable().translate(pSettings->r_string(_ammo_sect, "inv_name_short")).c_str());
					UIPropertiesBox.AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
		else if (pAmmo->IsBoxReloadableEmpty()){
			for (u8 i = 0; i < pAmmo->m_ammoTypes.size(); ++i){
				_ammo_sect = pAmmo->m_ammoTypes[i].c_str();
				if (inv->GetAmmoByLimit(_ammo_sect, true, false)){
					sprintf(temp, "%s%s %s", _many, 
						CStringTable().translate("st_load_ammo_type").c_str(),
						CStringTable().translate(pSettings->r_string(_ammo_sect, "inv_name_short")).c_str());
					UIPropertiesBox.AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
	}
	
	//отсоединение аддонов от вещи
	if(pWeapon){
		if (inv->InSlot(pWeapon) && smart_cast<CWeaponMagazined*>(pWeapon)){
			for (u32 i = 0; i < pWeapon->m_ammoTypes.size(); ++i){
				if (pWeapon->TryToGetAmmo(i)){
					auto ammo_sect = pSettings->r_string(pWeapon->m_ammoTypes[i].c_str(), "inv_name_short");
					sprintf(temp, "%s %s", CStringTable().translate("st_load_ammo_type").c_str(), CStringTable().translate(ammo_sect).c_str());
					UIPropertiesBox.AddItem(temp, (void*)(__int64)i, INVENTORY_RELOAD_MAGAZINE);
					b_show = true;
				}
			}
		}
		//
		if(pWeapon->IsGrenadeLauncherAttached() && pWeapon->GrenadeLauncherAttachable() && pWeapon->CanDetach(pWeapon->GetGrenadeLauncherName().c_str())){
			_addon_sect = pWeapon->GetGrenadeLauncherName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if(pWeapon->IsScopeAttached() && pWeapon->ScopeAttachable() && pWeapon->CanDetach(pWeapon->GetScopeName().c_str())){
			_addon_sect = pWeapon->GetScopeName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if(pWeapon->IsSilencerAttached() && pWeapon->SilencerAttachable() && pWeapon->CanDetach(pWeapon->GetSilencerName().c_str())){
			_addon_sect = pWeapon->GetSilencerName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->IsLaserAttached() && pWeapon->LaserAttachable() && pWeapon->CanDetach(pWeapon->GetLaserName().c_str())){
			_addon_sect = pWeapon->GetLaserName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->IsFlashlightAttached() && pWeapon->FlashlightAttachable() && pWeapon->CanDetach(pWeapon->GetFlashlightName().c_str())){
			_addon_sect = pWeapon->GetFlashlightName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->IsStockAttached() && pWeapon->StockAttachable() && pWeapon->CanDetach(pWeapon->GetStockName().c_str())) {
			_addon_sect = pWeapon->GetStockName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->IsExtenderAttached() && pWeapon->ExtenderAttachable() && pWeapon->CanDetach(pWeapon->GetExtenderName().c_str())) {
			_addon_sect = pWeapon->GetExtenderName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if (pWeapon->IsForendAttached() && pWeapon->ForendAttachable() && pWeapon->CanDetach(pWeapon->GetForendName().c_str())) {
			_addon_sect = pWeapon->GetForendName().c_str();
			_addon_name = pSettings->r_string(_addon_sect, "inv_name_short");
			sprintf(temp, "%s%s %s", _many, CStringTable().translate(detach_tip).c_str(), CStringTable().translate(_addon_name).c_str());
			UIPropertiesBox.AddItem(temp, (void*)_addon_sect, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
		if(smart_cast<CWeaponMagazined*>(pWeapon)){
			auto WpnMagazWgl = smart_cast<CWeaponMagazinedWGrenade*>(pWeapon);
			bool b = pWeapon->GetAmmoElapsed() > 0 || WpnMagazWgl && !WpnMagazWgl->m_magazine2.empty() || smart_cast<CWeaponMagazined*>(pWeapon)->IsMagazineAttached();

			if(!b){
				CUICellItem * itm = CurrentItem();
				for(u32 i=0; i<itm->ChildsCount(); ++i){
					auto pWeaponChild = static_cast<CWeaponMagazined*>(itm->Child(i)->m_pData);
					auto WpnMagazWglChild = smart_cast<CWeaponMagazinedWGrenade*>(pWeaponChild);
					if (pWeaponChild->GetAmmoElapsed() > 0 || ( WpnMagazWglChild && !WpnMagazWglChild->m_magazine2.empty() )){
						b = true;
						break;
					}
				}
			}

			if(b){
				sprintf(temp, "%s%s", _many, CStringTable().translate("st_unload_magazine").c_str());
				UIPropertiesBox.AddItem(temp,  NULL, INVENTORY_UNLOAD_MAGAZINE);
				b_show = true;
			}
		}
	}
	
	//присоединение аддонов к оружиям в слотах
 //       static std::regex addon_re( R"(\{ADDON\})" );
 //       static std::regex wpn_re( R"(\{WPN\})" );
	//for (u8 i = 0; i < SLOTS_TOTAL; ++i) {
	//	PIItem tgt = m_pInv->m_slots[i].m_pIItem;
	//	if (!tgt) continue;
	//	//attach addon
	//	if (tgt->CanAttach(CurrentIItem())) {
	//		string128 trans_str;
	//		strconcat(sizeof(trans_str), trans_str, "st_attach_addon_to_item_in_slot_", std::to_string(i).c_str());
	//		string128 str = { 0 };
	//		// В локализации должно быть что-то типа 'Прикрепить %s к %s в таком-то слоте'
	//		std::snprintf(str, sizeof(str), 
	//			CStringTable().translate(trans_str).c_str(), 
	//			CurrentIItem()->m_nameShort.c_str(), 
	//			tgt->m_nameShort.c_str());
 //                       std::string s( str );
 //                       s = std::regex_replace( s, addon_re, CurrentIItem()->m_nameShort.c_str() );
 //                       s = std::regex_replace( s, wpn_re,   tgt->m_nameShort.c_str() );
	//		UIPropertiesBox.AddItem( s.c_str(), (void*)tgt, INVENTORY_ATTACH_ADDON );
	//		b_show = true;
	//	}
	//	//charge device
	//	if (tgt->CanBeChargedBy(CurrentIItem())) {
	//		sprintf(temp, "%s %s", CStringTable().translate("st_charge").c_str(), tgt->NameShort());
	//		UIPropertiesBox.AddItem(temp, (void*)tgt, INVENTORY_CHARGE_DEVICE);
	//	}
	//}
	for (const auto& slot : inv->m_slots) {
		auto tgt = slot.m_pIItem;
		if (!tgt) continue;
		if (tgt->CanAttach(CurrentIItem())) {
			sprintf(temp, "%s %s", CStringTable().translate(CurrentIItem()->GetAttachMenuTip()).c_str(), tgt->NameShort());
			UIPropertiesBox.AddItem(temp, (void*)tgt, INVENTORY_ATTACH_ADDON);
			b_show = true;
		}
		if (tgt->CanBeRepairedBy(CurrentIItem())) {
			sprintf(temp, "%s %s", CStringTable().translate(CurrentIItem()->GetRepairMenuTip()).c_str(), tgt->NameShort());
			UIPropertiesBox.AddItem(temp, (void*)tgt, INVENTORY_REPAIR_ITEM);
			b_show = true;
		}
	}

	if (pEatableItem){
		UIPropertiesBox.AddItem(pEatableItem->GetUseMenuTip(),  NULL, INVENTORY_EAT_ACTION);
		b_show = true;
	}

	if (CurrentIItem()->CanBeDisassembled()) {
		sprintf(temp, "%s%s", _many, CStringTable().translate(CurrentIItem()->GetDisassembleMenuTip()).c_str());
		UIPropertiesBox.AddItem(temp, NULL, INVENTORY_DISASSEMBLE);
		b_show = true;
	}

	if(!CurrentIItem()->IsQuestItem()){
		sprintf(temp, "%s%s", _many, CStringTable().translate("st_drop").c_str());
		UIPropertiesBox.AddItem(temp, NULL, INVENTORY_DROP_ACTION);
		b_show = true;
	}

	if(b_show){
		UIPropertiesBox.AutoUpdateSize	();
		UIPropertiesBox.BringAllToTop	();

		Fvector2						cursor_pos;
		Frect							vis_rect;
		GetAbsoluteRect					(vis_rect);
		cursor_pos						= GetUICursor()->GetCursorPosition();
		cursor_pos.sub					(vis_rect.lt);
		UIPropertiesBox.Show			(vis_rect, cursor_pos);
		PlaySnd							(eInvProperties);
	}
}

void CUIInventoryWnd::ProcessPropertiesBoxClicked	()
{
	if(UIPropertiesBox.GetClickedItem())
	{
		bool for_all = Level().IR_GetKeyState(get_action_dik(kADDITIONAL_ACTION));
		auto itm = CurrentItem();
		auto item = CurrentIItem();
		switch(UIPropertiesBox.GetClickedItem()->GetTAG())
		{
		case INVENTORY_TO_SLOT_ACTION: 
			{
				// Явно указали слот в меню
				void* d = UIPropertiesBox.GetClickedItem()->GetData();
				if (d) {
				auto slot = (u8)(__int64)d;
				item->SetSlot(slot);
				if (ToSlot(itm, true))
					return;
				}
				// Пытаемся найти свободный слот из списка разрешенных.
				// Если его нету, то принудительно займет первый слот,
				// указанный в списке.
				auto& slots = item->GetSlots();
				for (u8 i = 0; i < (u8)slots.size(); ++i) {
					item->SetSlot( slots[ i ] );
				if (ToSlot(itm, false ))
					return;
				}
		        item->SetSlot( slots.size() ? slots[ 0 ]: NO_ACTIVE_SLOT );
		        ToSlot(itm, true);
	        }break;
		case INVENTORY_TO_QUICK:
		{
			auto& slots = item->GetSlots();
			for (u8 i = 0; i < (u8)slots.size(); ++i) {
				auto slot = slots[i];
				if (slot >= QUICK_SLOT_0 && slot <= QUICK_SLOT_3) {
					item->SetSlot(slot);
					if (ToSlot(itm, false))
						return;
				}
			}
			for (u8 i = 0; i < (u8)slots.size(); ++i) {
				auto slot = slots[i];
				if (slot >= QUICK_SLOT_0 && slot <= QUICK_SLOT_3) {
					item->SetSlot(slot);
					if (ToSlot(itm, true))
						return;
				}
			}
		}break;
		case INVENTORY_TO_BELT_ACTION:	
			ToBelt(itm,false);
			break;
		case INVENTORY_TO_VEST_ACTION:
			ToVest(itm, false);
			break;
		case INVENTORY_TO_BAG_ACTION:	
			ToBag(itm,false);
			break;
		case INVENTORY_DROP_ACTION:
			{
				DropCurrentItem(for_all);
			}break;
		case INVENTORY_DISASSEMBLE:
		{
			DisassembleItem(for_all);
		}break;
		case INVENTORY_EAT_ACTION:
			EatItem(CurrentIItem());
			break;
		case INVENTORY_ATTACH_ADDON:
			AttachAddon((PIItem)(UIPropertiesBox.GetClickedItem()->GetData()));
			break;
		case INVENTORY_DETACH_ADDON:
			DetachAddon((const char*)(UIPropertiesBox.GetClickedItem()->GetData()), for_all);
			break;
		case INVENTORY_RELOAD_MAGAZINE:
		{
			void* d = UIPropertiesBox.GetClickedItem()->GetData();
			auto Wpn = smart_cast<CWeapon*>(item);
			Wpn->m_set_next_ammoType_on_reload = (u32)(__int64)d;
			Wpn->ReloadWeapon();
		}break;
		case INVENTORY_UNLOAD_MAGAZINE:
		{
			auto ProcessUnload = [](void* pWpn) {
				auto WpnMagaz = static_cast<CWeaponMagazined*>(pWpn);
				WpnMagaz->UnloadWeaponFull();
				if (auto WpnMagazWgl = smart_cast<CWeaponMagazinedWGrenade*>(WpnMagaz)){
					if (WpnMagazWgl->IsGrenadeLauncherAttached()){
						WpnMagazWgl->PerformSwitchGL();
						WpnMagazWgl->UnloadWeaponFull();
						WpnMagazWgl->PerformSwitchGL();
					}
				}
			};

			ProcessUnload(itm->m_pData);

			for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i){
				auto child_itm = itm->Child(i);
				ProcessUnload(child_itm->m_pData);
			}
		}break;
		case INVENTORY_RELOAD_AMMO_BOX:
		{
			auto sect_to_load = (LPCSTR)UIPropertiesBox.GetClickedItem()->GetData();

			auto ProcessReload = [](void* pAmmo, LPCSTR sect_to_load) {
				auto AmmoBox = static_cast<CWeaponAmmo*>(pAmmo);
				AmmoBox->ReloadBox(sect_to_load);
			};

			ProcessReload(itm->m_pData, sect_to_load);
			for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i) {
				auto child_itm = itm->Child(i);
				ProcessReload(child_itm->m_pData, sect_to_load);
			}
			InitInventory_delayed();
		}break;
		case INVENTORY_UNLOAD_AMMO_BOX:
		{
			auto ProcessUnload = [](void* pAmmo) {
				auto AmmoBox = static_cast<CWeaponAmmo*>(pAmmo);
				AmmoBox->UnloadBox();
			};
			
			ProcessUnload(itm->m_pData);
			for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i){
				auto child_itm = itm->Child(i);
				ProcessUnload(child_itm->m_pData);
			}
		}break;
		case INVENTORY_REPAIR_ITEM:
			RepairItem((PIItem)(UIPropertiesBox.GetClickedItem()->GetData()));
			break;
		}
	}
}

bool CUIInventoryWnd::TryUseItem(PIItem itm)
{
	if (!itm) return false;

	if (itm->GetSlotsCount() || itm->Belt() || itm->Vest())
		return false;

	if(smart_cast<CEatableItem*>(itm)){
		EatItem(itm);
		return true;
	}

	return false;
}

bool CUIInventoryWnd::DropItem(PIItem itm, CUIDragDropListEx* lst)
{
	if(lst==m_pUIOutfitList)
	{
		return TryUseItem			(itm);
/*
		CCustomOutfit*		pOutfit		= smart_cast<CCustomOutfit*>	(CurrentIItem());
		if(pOutfit)
			ToSlot			(CurrentItem(), true);
		else
			EatItem				(CurrentIItem());

		return				true;
*/
	}
	CUICellItem*	_citem	= lst->ItemsCount() ? lst->GetItemIdx(0) : NULL;
	PIItem _iitem	= _citem ? (PIItem)_citem->m_pData : NULL;

	if(!_iitem)						return	false;
	if(!_iitem->CanAttach(itm))		return	false;
	AttachAddon						(_iitem);

	return							true;
}
