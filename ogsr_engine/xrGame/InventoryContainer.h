////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_container.h
//	Created 	: 12.11.2014
//  Modified 	: 11.12.2014
//	Author		: Alexander Petrov
//	Description : Mobile container class, based on inventory item 
////////////////////////////////////////////////////////////////////////////
#pragma once
#include "InventoryBox.h"
#include "inventory_item.h"
#include "inventory_item_object.h"

// CInventoryContainer
class CInventoryContainer : public CCustomInventoryBox <CInventoryItemObject>
{
private:
	typedef CCustomInventoryBox		<CInventoryItemObject>				inherited;
public:			
									CInventoryContainer					() {};
	virtual							~CInventoryContainer				() {};
	virtual void					Load								(LPCSTR section);
	virtual bool					CanTrade							() const;
	virtual	u32						Cost								() const;
	virtual	float					Weight								() const;
	virtual	DLL_Pure*				_construct							();
	virtual	void					OnEvent								(NET_Packet& P, u16 type);
	virtual	BOOL					net_Spawn							(CSE_Abstract* DC);

	virtual	void					UpdateCL							();

	virtual float					GetItemEffect						(int) const;
	virtual bool					NeedForcedDescriptionUpdate			() const;
	//окремий підрахунок діючих параметрів від артефактів у контейнері
	virtual float					GetContainmentArtefactEffect		(int) const;
	virtual float					GetContainmentArtefactProtection	(int) const;

	virtual	float					MaxCarryVolume						() const;

	virtual bool					can_be_attached						() const override;

	virtual void					Hit									(SHit* pHDS);
	void							HitItemsInBackPack					(SHit* pHDS);
	void							HitItemsInContainer					(SHit* pHDS);

	virtual void					OnMoveToSlot						(EItemPlace prevPlace);
	virtual void					OnMoveOut							(EItemPlace prevPlace);

	virtual bool					HasQuickDrop						() const;

	virtual u32						GetSameItemCount					(shared_str) const;

			void					AddUniqueItems						(TIItemContainer& items_container) const;

virtual		bool					IsVolumeUnlimited					() const;
			void					UpdateVolumeDropOut					();

protected:
			void					UpdateDropTasks						();
			void					UpdateDropItem						(PIItem pIItem);
			bool					m_bQuickDrop{};
};
