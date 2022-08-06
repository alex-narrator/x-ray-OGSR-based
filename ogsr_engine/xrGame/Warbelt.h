#pragma once
#include "inventory_item_object.h"
class CWarbelt :
	public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;
public:
								CWarbelt();
	virtual						~CWarbelt();

	virtual void				Load(LPCSTR section);

protected:
	u32							m_iMaxBelt{};
	bool						m_bDropPouch{};

public:
	u32							GetMaxBelt				() const	{ return m_iMaxBelt; }
	bool						HasDropPouch			() const	{ return m_bDropPouch; }

	virtual void				OnMoveToSlot			(EItemPlace previous_place);
	virtual void				OnMoveToRuck			(EItemPlace previous_place);
};