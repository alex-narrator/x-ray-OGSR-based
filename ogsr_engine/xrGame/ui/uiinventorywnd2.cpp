#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "../level.h"
#include "../actor.h"
#include "../ActorCondition.h"
#include "../hudmanager.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"

#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIDragDropListEx.h"
#include "UI3tButton.h"

#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../CustomDetector.h"
#include "../player_hud.h"

CUICellItem* CUIInventoryWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUIInventoryWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUIInventoryWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem				= itm;
	UIItemInfo.InitItem			(CurrentIItem());

	if (m_pCurrentCellItem) {
		m_pCurrentCellItem->m_select_armament = true;
		auto script_obj = CurrentIItem()->object().lua_game_object();
		g_actor->callback(GameObject::eCellItemSelect)(script_obj);
	}
}

void CUIInventoryWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if(pWnd == &UIPropertiesBox &&	msg==PROPERTY_CLICKED)
	{
		ProcessPropertiesBoxClicked	();
	}else 
	if (UIExitButton == pWnd && BUTTON_CLICKED == msg)
	{
		GetHolder()->StartStopMenu			(this,true);
	}
	else
	if (UIRepackAmmoButton == pWnd && BUTTON_CLICKED == msg)
	{
		Actor()->RepackAmmo();
	}

	CUIWindow::SendMessage(pWnd, msg, pData);
}


void CUIInventoryWnd::InitInventory_delayed()
{
	m_b_need_reinit			= true;
	m_b_need_update_stats	= true;
}

void CUIInventoryWnd::InitInventory() 
{
	CInventoryOwner *pInvOwner	= smart_cast<CInventoryOwner*>(Level().CurrentEntity());
	if(!pInvOwner)				return;

	m_pInv						= &pInvOwner->inventory();

	int bag_scroll = m_pUIBagList->ScrollPos();

	UIPropertiesBox.Hide		();
	ClearAllLists				();
	CUIWindow::Reset();
	SetCurrentItem				(NULL);

	UpdateCustomDraw			();

	//Slots
	PIItem	_itm						= m_pInv->m_slots[KNIFE_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIKnifeList->SetItem(itm);
	}

	_itm								= m_pInv->m_slots[ON_SHOULDER_SLOT].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIOnShoulderList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[ON_BACK_SLOT].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIOnBackList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[HOLSTER_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIHolsterList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[GRENADE_SLOT].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item( _itm );
		m_pUIGrenadeList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[ARTEFACT_SLOT].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIArtefactList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[DETECTOR_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIDetectorList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[ON_HEAD_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIOnHeadList->SetItem			(itm);
	}

	_itm								= m_pInv->m_slots[PDA_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIPdaList->SetItem			(itm);
	}

	_itm								= m_pInv->m_slots[QUICK_SLOT_0].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIQuickList_0->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[QUICK_SLOT_1].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIQuickList_1->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[QUICK_SLOT_2].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIQuickList_2->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[QUICK_SLOT_3].m_pIItem;
	if(_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIQuickList_3->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[HELMET_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIHelmetList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[WARBELT_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIWarBeltList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[BACKPACK_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIBackPackList->SetItem		(itm);
	}

	_itm								= m_pInv->m_slots[VEST_SLOT].m_pIItem;
	if (_itm){
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIVestList->SetItem			(itm);
	}
	///vest pouches
	_itm = m_pInv->m_slots[VEST_POUCH_1].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_1->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_2].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_2->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_3].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_3->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_4].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_4->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_5].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_5->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_6].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_6->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_7].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_7->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_8].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_8->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_9].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_9->SetItem(itm);
	}
	_itm = m_pInv->m_slots[VEST_POUCH_10].m_pIItem;
	if (_itm) {
		CUICellItem* itm = create_cell_item(_itm);
		m_pUIVestPouch_10->SetItem(itm);
	}

	PIItem _outfit						= m_pInv->m_slots[OUTFIT_SLOT].m_pIItem;
	CUICellItem* outfit					= (_outfit)?create_cell_item(_outfit):NULL;
	m_pUIOutfitList->SetItem			(outfit);

	TIItemContainer::iterator it, it_e;
	for(it=m_pInv->m_belt.begin(),it_e=m_pInv->m_belt.end(); it!=it_e; ++it) 
	{
		CUICellItem* itm			= create_cell_item(*it);
		m_pUIBeltList->SetItem		(itm);
	}

	ruck_list		= m_pInv->m_ruck;
	std::sort		(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	int i=1;
	for(it=ruck_list.begin(),it_e=ruck_list.end(); it!=it_e; ++it,++i) 
	{
	  if ( !(*it)->m_flags.test( CInventoryItem::FIHiddenForInventory ) ) {
	    CUICellItem* itm = create_cell_item( *it );
	    m_pUIBagList->SetItem( itm );
	  }
	}


	m_pUIBagList->SetScrollPos(bag_scroll);

	UpdateWeightVolume();

	m_b_need_reinit					= false;
}  

void CUIInventoryWnd::DropCurrentItem(bool b_all)
{

	CActor *pActor			= smart_cast<CActor*>(Level().CurrentEntity());
	if(!pActor)				return;

	if (!CurrentIItem() || CurrentIItem()->IsQuestItem()) return;

	if(b_all){
		u32 cnt = CurrentItem()->ChildsCount();
		for(u32 i=0; i<cnt; ++i){
			CUICellItem*	itm				= CurrentItem()->PopChild();
			PIItem			iitm			= (PIItem)itm->m_pData;
			SendEvent_Item_Drop				(iitm);
		}
	}

	SendEvent_Item_Drop		(CurrentIItem());
	SetCurrentItem			(NULL);
	UpdateWeightVolume		();
}

//------------------------------------------
bool CUIInventoryWnd::ToSlot(CUICellItem* itm, u8 _slot_id, bool force_place)
{
	PIItem	iitem = (PIItem)itm->m_pData;
	//Msg("~~[CUIInventoryWnd::ToSlot] Name [%s], slot [%u]", iitem->object().cName().c_str(), _slot_id);
	//LogStackTrace("ST:\n");
	iitem->SetSlot(_slot_id);
	return ToSlot(itm, force_place);
}

bool CUIInventoryWnd::ToSlot(CUICellItem* itm, bool force_place)
{
	CUIDragDropListEx*	old_owner			= itm->OwnerList();
	PIItem	iitem							= (PIItem)itm->m_pData;
	u8 _slot								= iitem->GetSlot();
	bool result{};

	if(GetInventory()->CanPutInSlot(iitem)){
		/*Msg("CUIInventoryWnd::ToSlot [%d]", _slot);*/
		CUIDragDropListEx* new_owner		= GetSlotList(_slot);
		
//		if(_slot==GRENADE_SLOT && !new_owner )return true; //fake, sorry (((

		result = GetInventory()->Slot(iitem);
		VERIFY								(result);

		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );
		
		new_owner->SetItem					(i);
	
		bool b_not_activate = !GetInventory()->activate_slot(_slot) && !iitem->cast_hud_item();
		GetInventory()->Slot(iitem, b_not_activate);
		PlaySnd(eInvItemToSlot);
		m_b_need_update_stats = true;
		
		/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
		// обновляем статик веса в инвентаре
		UpdateWeightVolume();
		/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/
	}else
	{ // in case slot is busy
		if(!force_place ||  _slot==NO_ACTIVE_SLOT || GetInventory()->m_slots[_slot].m_bPersistent || GetInventory()->IsSlotDisabled(_slot)) return false;

		CUIDragDropListEx* slot_list		= GetSlotList(_slot);
		VERIFY								(slot_list->ItemsCount()==1);

		CUICellItem* slot_cell				= slot_list->GetItemIdx(0);
		VERIFY(slot_cell && ((PIItem)slot_cell->m_pData) == GetInventory()->m_slots[_slot].m_pIItem);

#ifdef DEBUG
		bool _result =
#endif
		ToBag(slot_cell, false);
		VERIFY								(_result);

		return ToSlot(itm, false);
	}

	if(result){ 
		switch (_slot)
		{
		case DETECTOR_SLOT:{
			if (auto det = smart_cast<CCustomDetector*>(iitem))
				det->ToggleDetector(g_player_hud->attached_item(0) != nullptr);
		}break;
		case WARBELT_SLOT:
		case BACKPACK_SLOT:
		case OUTFIT_SLOT: 
		case VEST_SLOT:{
			UpdateCustomDraw();
		}break;
		}
	}

	return result;
}

bool CUIInventoryWnd::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem	iitem						= (PIItem)itm->m_pData;

	if(GetInventory()->CanPutInRuck(iitem))
	{
		CUIDragDropListEx*	old_owner		= itm->OwnerList();
		CUIDragDropListEx*	new_owner		= NULL;
		if(b_use_cursor_pos){
				new_owner					= CUIDragDropListEx::m_drag_item->BackList();
				VERIFY						(new_owner==m_pUIBagList);
		}else
				new_owner					= m_pUIBagList;

#ifdef DEBUG
		bool result =
#endif
		GetInventory()->Ruck(iitem);
		PlaySnd(eInvItemToRuck);
		m_b_need_update_stats = true;
		VERIFY								(result);
		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );

		/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
		// обновляем статик веса в инвентаре
		UpdateWeightVolume();
		/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/
		
		if(b_use_cursor_pos)
			new_owner->SetItem				(i,old_owner->GetDragItemPosition());
		else
			new_owner->SetItem				(i);

		switch (iitem->GetSlot())
		{
		case WARBELT_SLOT:
		case BACKPACK_SLOT:
		case OUTFIT_SLOT: 
		case VEST_SLOT:{
			UpdateCustomDraw();
		}break;
		}

		return true;
	}
	return false;
}

bool CUIInventoryWnd::ToBelt(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem	iitem						= (PIItem)itm->m_pData;

	if(GetInventory()->CanPutInBelt(iitem))
	{
		CUIDragDropListEx*	old_owner		= itm->OwnerList();
		CUIDragDropListEx*	new_owner		= NULL;
		if(b_use_cursor_pos){
				new_owner					= CUIDragDropListEx::m_drag_item->BackList();
				VERIFY						(new_owner==m_pUIBeltList);
		}else
				new_owner					= m_pUIBeltList;
#ifdef DEBUG
		bool result =
#endif
		GetInventory()->Belt(iitem);
		PlaySnd(eInvItemToBelt);
		m_b_need_update_stats = true;
		VERIFY								(result);
		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );
		
	//.	UIBeltList.RearrangeItems();
		if(b_use_cursor_pos)
			new_owner->SetItem				(i,old_owner->GetDragItemPosition());
		else
			new_owner->SetItem				(i);

		/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
		// обновляем статик веса в инвентаре
		UpdateWeightVolume();
		/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/

		return								true;
	}
	return									false;
}

void CUIInventoryWnd::AddItemToBag(PIItem pItem)
{
	CUICellItem* itm						= create_cell_item(pItem);
	m_pUIBagList->SetItem					(itm);
}

bool CUIInventoryWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUIInventoryWnd::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem(itm);

	itm->ColorizeItems( { 
		m_pUIBagList, 
		m_pUIBeltList, 
		//
		m_pUIOutfitList,
		m_pUIHelmetList,
		m_pUIWarBeltList,
		m_pUIBackPackList,
		m_pUIVestList,
		//
		m_pUIKnifeList,
		m_pUIOnShoulderList, 
		m_pUIOnBackList, 
		m_pUIHolsterList,
		// 
		m_pUIGrenadeList,
		m_pUIArtefactList,
		//
		m_pUIDetectorList, 
		m_pUIOnHeadList, 
		m_pUIPdaList,
		//
		m_pUIQuickList_0,
		m_pUIQuickList_1,
		m_pUIQuickList_2,
		m_pUIQuickList_3,

		m_pUIVestPouch_1,
		m_pUIVestPouch_2,
		m_pUIVestPouch_3,
		m_pUIVestPouch_4,
		m_pUIVestPouch_5,
		m_pUIVestPouch_6,
		m_pUIVestPouch_7,
		m_pUIVestPouch_8,
		m_pUIVestPouch_9,
		m_pUIVestPouch_10
		} );
	return false;
}

bool CUIInventoryWnd::OnItemDrop(CUICellItem* itm)
{
	auto old_owner = itm->OwnerList();
	auto new_owner = CUIDragDropListEx::m_drag_item->BackList();
	if (old_owner == new_owner || !old_owner || !new_owner)
		return false;

	auto t_new = GetType(new_owner);
	auto t_old = GetType(old_owner);

	if (t_new == t_old && t_new != iwSlot)
		return true;

	switch (t_new) {
	case iwSlot:
	{
          auto item = CurrentIItem();

          bool     can_put  = false;
          Ivector2 max_size = new_owner->CellSize();

          LPCSTR name = item->object().Name_script();
          int item_w  = item->GetGridWidth();
          int item_h  = item->GetGridHeight();

          if ( new_owner->GetVerticalPlacement() )
            std::swap( max_size.x, max_size.y );

          if ( item_w <= max_size.x && item_h <= max_size.y ) {
            for ( u8 i = 0; i < SLOTS_TOTAL; i++ )
              if ( new_owner == GetSlotList( i ) ) {
                if ( item->IsPlaceable( i, i ) ) {
                  item->SetSlot( i );
                  can_put = true;
                }
                else {
                  if ( !DropItem( CurrentIItem(), new_owner ) && item->GetSlotsCount() > 0 )
                    Msg( "! cannot put item %s into slot %d, allowed slots {%s}", name, i, item->GetSlotsSect() );
                }
                break;
              }   // for-if
          }
          else
            Msg( "!#ERROR: item %s to large for slot: (%d x %d) vs (%d x %d) ", name, item_w, item_h, max_size.x, max_size.y );

          // при невозможности поместить в выбранный слот
          if ( !can_put ) {
            // восстановление не требуется, слот не был назначен
            return true;
          }

          if( GetSlotList( item->GetSlot() ) == new_owner )
            ToSlot( itm, true );
	}break;
	case iwBag: {
		ToBag(itm, true);
	}break;
	case iwBelt: {
		ToBelt(itm, true);
	}break;
	};

	DropItem(CurrentIItem(), new_owner);

	return true;
}

bool CUIInventoryWnd::OnItemDbClick(CUICellItem* itm)
{
	auto __item = (PIItem)itm->m_pData;
	auto old_owner = itm->OwnerList();

	if (TryUseItem(__item))
		return true;

	auto t_old = GetType(old_owner);

	switch (t_old) {
	case iwSlot: {
		ToBag(itm, false);
	}break;

	case iwBag:
	{
          // Пытаемся найти свободный слот из списка разрешенных.
          // Если его нету, то принудительно займет первый слот,
          // указанный в списке.
          auto& slots = __item->GetSlots();
          for (const auto& slot : slots) {
            __item->SetSlot(slot);
            if ( ToSlot( itm, false ) )
              return true;
          }
          //__item->SetSlot( slots.size() ? slots[ 0 ]: NO_ACTIVE_SLOT );
		  for (const auto& slot : slots) {
			  if (!m_pInv->IsSlotDisabled(slot)) {
				  __item->SetSlot(slot);
				  break;
			  }
		  }
          if ( !ToSlot( itm, false ) )
            if ( !ToBelt( itm, false ) )
              ToSlot( itm, true );
	}break;

	case iwBelt: {
		ToBag(itm, false);
	}break;
	};

	return true;
}


bool CUIInventoryWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox		();
	return						false;
}

CUIDragDropListEx* CUIInventoryWnd::GetSlotList( u8 slot_idx ) {
  if( slot_idx == NO_ACTIVE_SLOT || GetInventory()->m_slots[ slot_idx ].m_bPersistent )
    return NULL;
  return m_slots_array[ slot_idx ];
}

void CUIInventoryWnd::ClearAllLists()
{
	m_pUIBagList->ClearAll					(true);
	m_pUIBeltList->ClearAll					(true);
	//
	m_pUIOutfitList->ClearAll				(true);
	m_pUIHelmetList->ClearAll				(true);
	m_pUIWarBeltList->ClearAll				(true);
	m_pUIBackPackList->ClearAll				(true);
	m_pUIVestList->ClearAll					(true);
	//
	m_pUIKnifeList->ClearAll				(true);
	m_pUIOnShoulderList->ClearAll			(true);
	m_pUIOnBackList->ClearAll				(true);
	m_pUIHolsterList->ClearAll				(true);
	//
	m_pUIGrenadeList->ClearAll				(true);
	m_pUIArtefactList->ClearAll				(true);
	//
	m_pUIDetectorList->ClearAll				(true);
	m_pUIOnHeadList->ClearAll				(true);
	m_pUIPdaList->ClearAll					(true);
	//
	m_pUIQuickList_0->ClearAll				(true);
	m_pUIQuickList_1->ClearAll				(true);
	m_pUIQuickList_2->ClearAll				(true);
	m_pUIQuickList_3->ClearAll				(true);

	m_pUIVestPouch_1->ClearAll				(true);
	m_pUIVestPouch_2->ClearAll				(true);
	m_pUIVestPouch_3->ClearAll				(true);
	m_pUIVestPouch_4->ClearAll				(true);
	m_pUIVestPouch_5->ClearAll				(true);
	m_pUIVestPouch_6->ClearAll				(true);
	m_pUIVestPouch_7->ClearAll				(true);
	m_pUIVestPouch_8->ClearAll				(true);
	m_pUIVestPouch_9->ClearAll				(true);
	m_pUIVestPouch_10->ClearAll				(true);
}


void CUIInventoryWnd::UpdateWeightVolume() {
  InventoryUtilities::UpdateWeight(UIWeightWnd, true);
  InventoryUtilities::UpdateVolume(Actor()->cast_game_object(), UIVolumeWnd, true);
}