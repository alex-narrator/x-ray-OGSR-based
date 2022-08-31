#include "stdafx.h"
#include "Addons.h"

CScope::CScope(){
}
CScope::~CScope(){
}
using namespace luabind;
#pragma optimize("s",on)
void CScope::script_register(lua_State* L){
	module(L)
		[
			class_<CScope, CGameObject>("CScope")
			.def(constructor<>())
		];
}

CSilencer::CSilencer(){
}
CSilencer::~CSilencer(){
}

CGrenadeLauncher::CGrenadeLauncher(){
}
CGrenadeLauncher::~CGrenadeLauncher(){
}
void CGrenadeLauncher::Load(LPCSTR section){
	m_fGrenadeVel = pSettings->r_float(section, "grenade_vel");
	inherited::Load(section);
}

CLaser::CLaser() {
}
CLaser::~CLaser() {
}

CFlashlight::CFlashlight() {
}
CFlashlight::~CFlashlight() {
}