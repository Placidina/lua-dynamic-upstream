#ifndef LUA_DYNAMIC_UPSTREAM_H
#define LUA_DYNAMIC_UPSTREAM_H

#include <ngx_core.h>
#include "ngx_http_lua_api.h"

int lua_dynamic_upstream_list_zones(lua_State *L);
int lua_dynamic_upstream_describe_zone(lua_State *L);
int lua_dynamic_upstream_add_peer(lua_State *L);
int lua_dynamic_upstream_set_peer_down(lua_State *L);
#endif /* LUA_DYNAMIC_UPSTREAM_H */
