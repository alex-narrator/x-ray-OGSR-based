#pragma once
#include "inventory_item_object.h"
class CVest :
    public CInventoryItemObject
{
private:
    typedef	CInventoryItemObject inherited;
public:
                                CVest               ();
	virtual                     ~CVest              () = default;
    virtual     void            Load                (LPCSTR section);
    virtual     bool            can_be_attached     () const;

    virtual     void			OnMoveToSlot        (EItemPlace prevPlace);
    virtual     void			OnMoveToRuck        (EItemPlace prevPlace);

    u32							GetVestWidth        () const { return m_iVestWidth; }
    u32							GetVestHeight       () const { return m_iVestHeight; }
protected:
    u32							m_iVestWidth{};
    u32							m_iVestHeight{};
};

