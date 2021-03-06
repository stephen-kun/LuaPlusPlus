#pragma once
/*
** $Id: lfunc.h,v 2.15 2015/01/13 15:49:11 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#include <lobject.hpp>

/* test whether thread is in 'twups' list */
#define isintwups(L)    (L->twups != L)

/*
** maximum number of upvalues in a closure (both C and Lua). (Value
** must fit in a VM register.)
*/
#define MAXUPVAL        255

/*
** Upvalues for Lua closures
*/
struct UpVal
{
  TValue *v;  /* points to stack or to its own value */
  lu_mem refcount;  /* reference counter */
  union
  {
    struct    /* (when open) */
    {UpVal *next;  /* linked list */
     int touched;  /* mark to avoid cycles with dead threads */
    } open;
    TValue value;  /* the value (when closed) */
  } u;
};

#define upisopen(up)    ((up)->v != & (up)->u.value)

LUAI_FUNC Proto *luaF_newproto(lua_State *L);
LUAI_FUNC CClosure *luaF_newCclosure(lua_State *L, int nelems);
LUAI_FUNC LClosure *luaF_newLclosure(lua_State *L, int nelems);
LUAI_FUNC void luaF_initupvals(lua_State *L, LClosure *cl);
LUAI_FUNC UpVal *luaF_findupval(lua_State *L, StkId level);
LUAI_FUNC void luaF_close(lua_State *L, StkId level);
LUAI_FUNC const char *luaF_getlocalname(const Proto *func, int local_number,
                                        int pc);
