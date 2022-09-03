#pragma once
#include "inventory_item_object.h"
class CPowerBattery :
    public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject	inherited;
public:
					CPowerBattery();
	virtual			~CPowerBattery();

	virtual void	Load		(LPCSTR section);

	virtual void	save		(NET_Packet& output_packet);
	virtual void	load		(IReader& input_packet);

	virtual bool	Useful		() const;

			void	Charge			(CInventoryItem*);
			bool	IsCharger		() const;
	virtual bool	CanBeCharged	() const override;
	virtual float	GetPowerLevel	() const override;
	virtual void	Recharge		() override;
private:
			int				m_uChargeCount{};
			int				m_uChargeCountStart{};
			bool			m_bRechargeable{};
};

