#pragma once

#include "UIDialogWnd.h"
#include "UIEditBox.h"
#include "../inventory_space.h"

class CUIDragDropListEx;
class CUIItemInfo;
class CUICharacterInfo;
class CUIPropertiesBox;
class CUI3tButton;
class CUICellItem;
class IInventoryBox;
class CInventoryOwner;
class CGameObject;

class CUICarBodyWnd: public CUIDialogWnd
{
private:
	typedef CUIDialogWnd	inherited;
	bool					m_b_need_update;
public:
							CUICarBodyWnd				();
	virtual					~CUICarBodyWnd				();

	virtual void			Init						();
	virtual bool			StopAnyMove					(){return true;}

	virtual void			SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);

	void					InitCarBody					(CInventoryOwner* pOurInv, CInventoryOwner* pOthersInv);
	void					InitCarBody					(CInventoryOwner* pOur, IInventoryBox* pInvBox);
	virtual void			Draw						();
	virtual void			Update						();
		
	virtual void			Show						();
	virtual void			Hide						();

	void					DisableAll					();
	void					EnableAll					();
	virtual bool			OnKeyboard					(int dik, EUIMessages keyboard_action);
	virtual bool			OnMouse						(float x, float y, EUIMessages mouse_action);

	void					UpdateLists_delayed			();
	void					CheckForcedWeightVolumeUpdate();
	bool					CheckMonsterAndKnife		() const;
	void					TryPlayStabbing				(PIItem itm, CGameObject* owner_from);

protected:
	CInventoryOwner*		m_pActorInventoryOwner{};
	
	CUIDragDropListEx*		m_pUIOurBagList;
	CUIDragDropListEx*		m_pUIOthersBagList;

	CUIStatic*				m_pUIStaticTop;
	CUIStatic*				m_pUIStaticBottom;

	CUIFrameWindow*			m_pUIDescWnd;
	CUIStatic*				m_pUIStaticDesc;
	CUIItemInfo*			m_pUIItemInfo;

	CUIStatic*				m_pUIOurBagWnd;
	CUIStatic*				m_pUIOthersBagWnd;

	CUIStatic*				m_pUIOurVolWnd;
	CUIStatic*				m_pUIOthersVolWnd;

	CUIStatic*				m_pUIOurWeightWnd;

	//информация о персонажах 
	CUIStatic*				m_pUIOurIcon;
	CUIStatic*				m_pUIOthersIcon;
	CUICharacterInfo*		m_pUICharacterInfoLeft;
	CUICharacterInfo*		m_pUICharacterInfoRight;
	CUIPropertiesBox*		m_pUIPropertiesBox;
	CUI3tButton*			m_pUITakeAll;
	CUI3tButton*			m_pUIExitButton;
	CUI3tButton*			m_pUIRepackAmmoButton;
	CUI3tButton*			m_pUIMoveAllFromRuckButton;

public:
	CUICellItem*			m_pCurrentCellItem;

	CInventoryOwner*		m_pOtherInventoryOwner{};
	IInventoryBox*			m_pOtherInventoryBox{};

	CGameObject*			m_pActorGO{};
	CGameObject*			m_pOtherGO{};

protected:
	void					UpdateLists					();

	void					ActivatePropertiesBox		();
	void					EatItem						();
	
	void					SetCurrentItem				(CUICellItem* itm);
	CUICellItem*			CurrentItem					();
	PIItem					CurrentIItem				();

	// Взять все
	void					TakeAll						();
	void					MoveItems					(CUICellItem* itm, bool b_all);
	void					DropItems					(bool b_all);
	void					SendEvent_Item_Drop			(PIItem	pItem);
	void					MoveItemWithContent			(CUICellItem* itm, u32 slot);


	bool					OnItemDrop					(CUICellItem* itm);
	bool					OnItemStartDrag				(CUICellItem* itm);
	bool					OnItemDbClick				(CUICellItem* itm);
	bool					OnItemSelected				(CUICellItem* itm);
	bool					OnItemRButtonClick			(CUICellItem* itm);

	bool					TransferItem				(PIItem itm, CGameObject* owner_from, CGameObject* owner_to);
	void					BindDragDropListEnents		(CUIDragDropListEx* lst);

	enum eInventorySndAction {
		eInvSndOpen = 0,
		eInvSndClose,
		eInvProperties,
		eInvDropItem,
		eInvDetachAddon,
		eInvMoveItem,
		eInvSndMax
	};

	ref_sound					sounds[eInvSndMax];
	void						PlaySnd(eInventorySndAction a);

	bool						CanMoveToOther		(PIItem pItem, CGameObject* owner_to) const;
	void						UpdateWeightVolume	(bool only_for_actor = false);
	float						GetStackVolume		(CUICellItem* ci) const;
	bool						CanTakeStack		(CUICellItem* ci, CGameObject* owner_to) const;
};