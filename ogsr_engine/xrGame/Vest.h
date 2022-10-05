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
    const       xr_vector<u8>&  GetVestSlots        () { return m_vest_slots; }
                bool            SlotAllowed         (u32) const;
    virtual     bool            can_be_attached     () const;

    virtual     void			OnMoveToSlot        (EItemPlace prevPlace);
    virtual     void			OnMoveToRuck        (EItemPlace prevPlace);
protected:
    xr_vector<u8>       m_vest_slots{};
};

