/*
** $Id: lfunc.c,v 2.45 2014/11/02 19:19:04 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#define lfunc_c
#define LUA_CORE

#include <lprefix.hpp>

#include <cstddef>
#include <lua.hpp>
#include <lfunc.hpp>
#include <lgc.hpp>
#include <lmem.hpp>
#include <lobject.hpp>
#include <lstate.hpp>

CClosure *luaF_newCclosure(lua_State *L, int n)
{
  CClosure* c = LGCFactory::luaC_newobj<CClosure>(L, LuaType::Variant::CFunctionClosure, sizeCClosure(n));
  c->nupvalues = cast_byte(n);
  return c;
}

LClosure *luaF_newLclosure(lua_State *L, int n)
{
  LClosure* c = LGCFactory::luaC_newobj<LClosure>(L, LuaType::Variant::LuaFunctionClosure, sizeLClosure(n));
  c->p = nullptr;
  c->nupvalues = cast_byte(n);
  while (n--)
    c->upvals[n] = nullptr;
  return c;
}

/*
** fill a closure with new closed upvalues
*/
void luaF_initupvals(lua_State *L, LClosure *cl) {
  int i;
  for (i = 0; i < cl->nupvalues; i++)
  {
    UpVal* uv = LMem<UpVal>::luaM_new(L);
    uv->refcount = 1;
    uv->v = &uv->u.value;  /* make it closed */
    setnilvalue(uv->v);
    cl->upvals[i] = uv;
  }
}

UpVal *luaF_findupval(lua_State *L, StkId level) {
  UpVal **pp = &L->openupval;
  UpVal *p;
  UpVal *uv;
  lua_assert(isintwups(L) || L->openupval == NULL);
  while (*pp != nullptr && (p = *pp)->v >= level)
  {
    lua_assert(upisopen(p));
    if (p->v == level) /* found a corresponding upvalue? */
      return p; /* return it */
    pp = &p->u.open.next;
  }
  /* not found: create a new upvalue */
  uv = LMem<UpVal>::luaM_new(L);
  uv->refcount = 0;
  uv->u.open.next = *pp;  /* link it to list of open upvalues */
  uv->u.open.touched = 1;
  *pp = uv;
  uv->v = level;  /* current value lives in the stack */
  if (!isintwups(L))    /* thread not in list of threads with upvalues? */
  {
    L->twups = L->globalState->twups;  /* link it to the list */
    L->globalState->twups = L;
  }
  return uv;
}

void luaF_close(lua_State *L, StkId level) {
  UpVal *uv;
  while (L->openupval != nullptr && (uv = L->openupval)->v >= level)
  {
    lua_assert(upisopen(uv));
    L->openupval = uv->u.open.next;  /* remove from 'open' list */
    if (uv->refcount == 0) /* no references? */
      LMem<UpVal>::luaM_free(L, uv); /* free upvalue */
    else
    {
      setobj(L, &uv->u.value, uv->v);  /* move value to upvalue slot */
      uv->v = &uv->u.value;  /* now current value lives here */
      luaC_upvalbarrier(L, uv);
    }
  }
}

Proto *luaF_newproto(lua_State *L) {
  Proto* f = LGCFactory::luaC_newobj<Proto>(L, LuaType::Variant::FunctionPrototype, sizeof(Proto));
  f->k = nullptr;
  f->sizek = 0;
  f->p = nullptr;
  f->sizep = 0;
  f->code = nullptr;
  f->cache = nullptr;
  f->sizecode = 0;
  f->lineinfo = nullptr;
  f->sizelineinfo = 0;
  f->upvalues = nullptr;
  f->sizeupvalues = 0;
  f->numparams = 0;
  f->is_vararg = 0;
  f->maxstacksize = 0;
  f->locvars = nullptr;
  f->sizelocvars = 0;
  f->linedefined = 0;
  f->lastlinedefined = 0;
  f->source = nullptr;
  return f;
}

/*
** Look for n-th local variable at line 'line' in function 'func'.
** Returns NULL if not found.
*/
const char *luaF_getlocalname(const Proto *f, int local_number, int pc) {
  int i;
  for (i = 0; i < f->sizelocvars && f->locvars[i].startpc <= pc; i++)
    if (pc < f->locvars[i].endpc)    /* is variable active? */
    {
      local_number--;
      if (local_number == 0)
        return getstr(f->locvars[i].varname);
    }
  return nullptr;  /* not found */
}
