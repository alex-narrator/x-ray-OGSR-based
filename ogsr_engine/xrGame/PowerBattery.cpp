#include "stdafx.h"
#include "PowerBattery.h"
#include "xrserver.h"

CPowerBattery::CPowerBattery() {
}

CPowerBattery::~CPowerBattery() {
}

void CPowerBattery::Load(LPCSTR section) {
	inherited::Load(section);
	m_uChargeCountStart = READ_IF_EXISTS(pSettings, r_s8, section, "charge_count", 1);
	m_uChargeCount		= m_uChargeCountStart;
	m_bRechargeable		= READ_IF_EXISTS(pSettings, r_bool, section, "rechargeable", false);
}

void CPowerBattery::save(NET_Packet& packet){
	inherited::save(packet);
	packet.w_s8(m_uChargeCount);
}

void CPowerBattery::load(IReader& packet){
	inherited::load(packet);
	m_uChargeCount = packet.r_s8();
}

bool CPowerBattery::Useful() const{
	return m_uChargeCount || m_bRechargeable;
}

void CPowerBattery::Charge(CInventoryItem* item) {
	if (!item->CanBeCharged() || !m_uChargeCount) return;
	item->Recharge();
	--m_uChargeCount;
	if (!Useful())
		DestroyObject();
}

bool CPowerBattery::CanBeCharged() const {
	return IsCharger() && !fis_zero(GetCondition()) && m_uChargeCount < m_uChargeCountStart;
}

bool CPowerBattery::IsCharger() const {
	return m_bRechargeable;
}

float CPowerBattery::GetPowerLevel() const {
	return ((float)m_uChargeCount / (float)m_uChargeCountStart);
}

void CPowerBattery::Recharge() {
	m_uChargeCount = m_uChargeCountStart;
}