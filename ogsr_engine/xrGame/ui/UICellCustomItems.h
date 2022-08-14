#pragma once
#include "UICellItem.h"
#include "Weapon.h"
#include "eatable_item.h"
#include "Artifact.h"
#include "Warbelt.h"
#include "Backpack.h"

class CUIInventoryCellItem :public CUICellItem
{
	typedef  CUICellItem	inherited;
protected:
	bool						b_auto_drag_childs;

	CUIStatic*					m_text_add{};
	void						init_add();
public:
								CUIInventoryCellItem		(CInventoryItem* itm);
	virtual		void			Update						();
	virtual		bool			EqualTo						(CUICellItem* itm);
	virtual		CUIDragItem*	CreateDragItem				();
				CInventoryItem* object						() {return (CInventoryItem*)m_pData;}

	// Real Wolf: Для коллбеков. 25.07.2014.
	virtual void			OnFocusReceive				();
	virtual void			OnFocusLost					();
	virtual bool			OnMouse						(float, float, EUIMessages);
	// Real Wolf: Для метода get_cell_item(). 25.07.2014.
	virtual					~CUIInventoryCellItem		();
};

class CUIWarbeltCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
protected:
	virtual		void			UpdateItemText				();
public:
								CUIWarbeltCellItem			(CWarbelt* itm);
	virtual		void			Update						();
				CWarbelt*		object						() {return (CWarbelt*)m_pData;}
};

class CUICBackpackCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
protected:
	virtual		void			UpdateItemText				();
public:
								CUICBackpackCellItem		(CBackpack* itm);
	virtual		void			Update						();
				CBackpack*		object						() {return (CBackpack*)m_pData;}
};

class CUIEatableCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
protected:
	virtual		void			UpdateItemText			();
public:
								CUIEatableCellItem			(CEatableItem* itm);
	virtual		void			Update						();
	virtual		bool			EqualTo						(CUICellItem* itm);
				CEatableItem*	object						() {return (CEatableItem*)m_pData;}
};

class CUIArtefactCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
public:
								CUIArtefactCellItem			(CArtefact* itm);
	virtual		bool			EqualTo						(CUICellItem* itm);
				CArtefact*		object						() { return (CArtefact*)m_pData; }
};

class CUIAmmoCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
protected:
	virtual		void			UpdateItemText				();
public:
								CUIAmmoCellItem				(CWeaponAmmo* itm);
	virtual		void			Update						();
	virtual		bool			EqualTo						(CUICellItem* itm);
	CWeaponAmmo*				object						() { return (CWeaponAmmo*)m_pData; }
};

class CUIWeaponCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
public:
	enum eAddonType{	eSilencer=0, eScope, eLauncher, eMaxAddon};
	CUIStatic*					m_addons					[eMaxAddon];
protected:
	virtual		void			UpdateItemText();
	Fvector2					m_addon_offset				[eMaxAddon];
	void						CreateIcon					(eAddonType, CIconParams &params);
	void						DestroyIcon					(eAddonType);
	CUIStatic*					GetIcon						(eAddonType);
	void						InitAddon					(CUIStatic* s, CIconParams &params, Fvector2 offset, bool b_rotate);
	void						InitAllAddons				(CUIStatic* s_silencer, CUIStatic* s_scope, CUIStatic* s_launcher, bool b_vertical);
	bool						is_scope					();
	bool						is_silencer					();
	bool						is_launcher					();
public:
								CUIWeaponCellItem			(CWeapon* itm);
				virtual			~CUIWeaponCellItem			();
	virtual		void			Update						();
				CWeapon*		object						() {return (CWeapon*)m_pData;}
	virtual		void			OnAfterChild				(CUIDragDropListEx* parent_list);
				CUIDragItem*	CreateDragItem				();
	virtual		bool			EqualTo						(CUICellItem* itm);
	CUIStatic*					get_addon_static			(u32 idx)				{return m_addons[idx];}
	Fvector2					get_addon_offset			(u32 idx)				{ return m_addon_offset[idx]; }
};

class CBuyItemCustomDrawCell :public ICustomDrawCell
{
	CGameFont*			m_pFont;
	string16			m_string;
public:
						CBuyItemCustomDrawCell	(LPCSTR str, CGameFont* pFont);
	virtual void		OnDraw					(CUICellItem* cell);

};
