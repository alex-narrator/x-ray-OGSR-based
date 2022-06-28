////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine.h
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_storage.h"
#include "script_export_space.h"

class CScriptEngine : public CScriptStorage {
private:
	bool						m_reload_modules;

protected:
	int							m_stack_level;

private:
	string128					m_last_no_file;
	u32						m_last_no_file_cnt;
	u32							m_last_no_file_length;

	bool				no_file_exists(const char* file_name, u32 string_length);
	void				add_no_file(const char* file_name, u32 string_length);

public:
	CScriptEngine();
	virtual						~CScriptEngine() = default;
	void init();
	virtual	void				unload();
	static	int					lua_panic(lua_State *L);
#ifdef LUABIND_NO_EXCEPTIONS
	static	void				lua_error(lua_State *L);
#endif
	static	int					lua_pcall_failed(lua_State *L);
	void				setup_auto_load();
	bool				process_file_if_exists(const char* file_name, bool warn_if_not_exist);
	bool				process_file(const char* file_name);
	bool				process_file(const char* file_name, bool reload_modules);
	bool				function_object(const char* function_to_call, luabind::object &object, int type = LUA_TFUNCTION);
	void				register_script_classes();
	void				parse_script_namespace(const char *name, char *ns, u32 nsSize, char *func, u32 funcSize);

	void				collect_all_garbage();

	template <typename TResult>
	IC		bool				functor(const char* function_to_call, luabind::functor<TResult> &lua_function);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptEngine)
#undef script_type_list
#define script_type_list save_type_list(CScriptEngine)

template <typename TResult>
IC bool CScriptEngine::functor(const char* function_to_call, luabind::functor<TResult> &lua_function)
{
	luabind::object object;
	if (!function_object(function_to_call, object))
		return false;
	try {
#ifdef LUABIND_09
		lua_function = luabind::functor<TResult>(object);
#else
		lua_function = luabind::object_cast<luabind::functor<TResult>>(object);
#endif
	}
	catch (...) {
		return false;
	}
	return true;
}
