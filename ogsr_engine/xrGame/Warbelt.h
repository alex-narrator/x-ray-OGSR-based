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
	xr_vector<u8>				m_belt_slots{};

public:
	u32							GetMaxBelt				() const	{ return m_iMaxBelt; }
	bool						HasDropPouch			() const	{ return m_bDropPouch; }

	virtual void				OnMoveToSlot			(EItemPlace prevPlace);
	virtual void				OnMoveToRuck			(EItemPlace prevPlace);
	const       xr_vector<u8>&	GetBeltSlots			()			{ return m_belt_slots; }
	bool						SlotAllowed				(u32) const;
};