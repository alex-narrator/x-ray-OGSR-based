#include "stdafx.h"
#include "StalkerOutfit.h"

using namespace luabind;

#pragma optimize("s",on)
void CStalkerOutfit::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CStalkerOutfit,CGameObject>("CStalkerOutfit")
			.def(constructor<>())
	];
}
