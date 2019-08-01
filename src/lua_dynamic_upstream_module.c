
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "lua_dynamic_upstream.h"

static ngx_int_t lua_dynamic_upstream_init(ngx_conf_t *cf);
static int lua_dynamic_upstream_create_module(lua_State *L);

static ngx_http_module_t lua_dynamic_upstream_module_ctx = {
    NULL,                      /* preconfiguration */
    lua_dynamic_upstream_init, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL  /* merge location configuration */
};

ngx_module_t lua_dynamic_upstream_module = {
    NGX_MODULE_V1,
    &lua_dynamic_upstream_module_ctx, /* module context */
    NULL,                             /* module directives */
    NGX_HTTP_MODULE,                  /* module type */
    NULL,                             /* init master */
    NULL,                             /* init module */
    NULL,                             /* init process */
    NULL,                             /* init thread */
    NULL,                             /* exit thread */
    NULL,                             /* exit process */
    NULL,                             /* exit master */
    NGX_MODULE_V1_PADDING};

static ngx_int_t lua_dynamic_upstream_init(ngx_conf_t *cf) {
    if (ngx_http_lua_add_package_preload(cf, "ngx.dynamic", lua_dynamic_upstream_create_module) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

static int lua_dynamic_upstream_create_module(lua_State *L) {
    lua_createtable(L, 0, 4);

    lua_pushcfunction(L, lua_dynamic_upstream_list_zones);
    lua_setfield(L, -2, "list_zones");

    lua_pushcfunction(L, lua_dynamic_upstream_describe_zone);
    lua_setfield(L, -2, "describe_zone");

    lua_pushcfunction(L, lua_dynamic_upstream_add_peer);
    lua_setfield(L, -2, "add_peer");

    lua_pushcfunction(L, lua_dynamic_upstream_set_peer_down);
    lua_setfield(L, -2, "set_peer_down");

    return 1;
}
