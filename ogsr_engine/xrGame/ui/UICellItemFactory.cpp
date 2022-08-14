#include "stdafx.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"



CUICellItem*	create_cell_item(CInventoryItem* itm)
{

	CWeaponAmmo* pAmmo					= smart_cast<CWeaponAmmo*>(itm);
	if(pAmmo)							return xr_new<CUIAmmoCellItem>(pAmmo);

	CWeapon* pWeapon					= smart_cast<CWeapon*>(itm);
	if(pWeapon)							return xr_new<CUIWeaponCellItem>(pWeapon);

	CEatableItem* pEatable				= smart_cast<CEatableItem*>(itm);
	if (pEatable)						return xr_new<CUIEatableCellItem>(pEatable);

	CArtefact* pArtefact				= smart_cast<CArtefact*>(itm);
	if (pArtefact)						return xr_new<CUIArtefactCellItem>(pArtefact);

	CWarbelt* pWarbelt					= smart_cast<CWarbelt*>(itm);
	if (pWarbelt)						return xr_new<CUIWarbeltCellItem>(pWarbelt);

	CBackpack* pBackpack				= smart_cast<CBackpack*>(itm);
	if (pBackpack)						return xr_new<CUICBackpackCellItem>(pBackpack);

	return xr_new<CUIInventoryCellItem>(itm);
}
