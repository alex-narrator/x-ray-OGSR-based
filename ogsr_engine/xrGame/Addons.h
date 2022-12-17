#pragma once

#include "inventory_item_object.h"
#include "script_export_space.h"
///////////////////////////////////////////////////////////////
// Scope.h
// Scope - апгрейд оружия снайперский прицел
///////////////////////////////////////////////////////////////
class CScope : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CScope();
	virtual ~CScope();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScope)
#undef script_type_list
#define script_type_list save_type_list(CScope)
///////////////////////////////////////////////////////////////
// Silencer.h
// Silencer - апгрейд оружия глушитель 
///////////////////////////////////////////////////////////////
class CSilencer : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CSilencer(void){};
	virtual ~CSilencer(void){};
};
///////////////////////////////////////////////////////////////
// GrenadeLauncher.h
// GrenadeLauncher - апгрейд оружия поствольный гранатомет
///////////////////////////////////////////////////////////////
class CGrenadeLauncher : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CGrenadeLauncher(void) {};
	virtual ~CGrenadeLauncher(void) {};

	virtual void Load(LPCSTR section);

	float	GetGrenadeVel() { return m_fGrenadeVel; }

protected:
	//стартовая скорость вылета подствольной гранаты
	float m_fGrenadeVel{};
};
///////////////////////////////////////////////////////////////
// Laser - апгрейд оружия ЛЦУ
///////////////////////////////////////////////////////////////
class CLaser : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CLaser(void){};
	virtual ~CLaser(void){};
};
///////////////////////////////////////////////////////////////
// Flashlight - апгрейд оружия фонарь 
///////////////////////////////////////////////////////////////
class CFlashlight : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CFlashlight(void){};
	virtual ~CFlashlight(void){};
};
///////////////////////////////////////////////////////////////
// Stock - збройовий апгрейд приклад
///////////////////////////////////////////////////////////////
class CStock : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CStock(void){};
	virtual ~CStock(void){};
};

///////////////////////////////////////////////////////////////
// Extender - збройовий апгрейд подовжувач магазину
///////////////////////////////////////////////////////////////
class CExtender : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CExtender(void) {};
	virtual ~CExtender(void) {};
};