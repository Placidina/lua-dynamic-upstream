#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <lauxlib.h>
#include "lua_inet_slab.h"
#include "ngx_http_lua_api.h"

static ngx_http_upstream_srv_conf_t *lua_dynamic_upstream_get_zone(lua_State *L, ngx_str_t *zone);
static ngx_int_t lua_dynamic_upstream_describe_peer(lua_State *L, ngx_http_upstream_rr_peer_t *peer);

static ngx_http_upstream_srv_conf_t *lua_dynamic_upstream_get_zone(lua_State *L, ngx_str_t *zone) {
    ngx_uint_t i;
    ngx_http_upstream_srv_conf_t *uscf, **uscfp;
    ngx_http_upstream_main_conf_t *umcf;
    ngx_http_request_t *r;

    r = ngx_http_lua_get_request(L);
    if (r == NULL) {
        return ngx_http_cycle_get_module_main_conf(ngx_cycle, ngx_http_upstream_module);
    }

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);
    uscfp = umcf->upstreams.elts;

    for (i = 0; i < umcf->upstreams.nelts; i++) {
        uscf = uscfp[i];
        if (uscf->shm_zone != NULL && uscf->shm_zone->shm.name.len == zone->len && ngx_strncmp(uscf->shm_zone->shm.name.data, zone->data, zone->len) == 0) {
            return uscf;
        }
    }

    return NULL;
}

static ngx_int_t lua_dynamic_upstream_describe_peer(lua_State *L, ngx_http_upstream_rr_peer_t *peer) {
    ngx_uint_t n;

    n = 7;
    if (peer->down) {
        n++;
    }

    if (peer->accessed) {
        n++;
    }

    if (peer->checked) {
        n++;
    }

    lua_createtable(L, 0, n);

    lua_pushliteral(L, "name");
    lua_pushlstring(L, (char *)peer->name.data, peer->name.len);
    lua_rawset(L, -3);

    lua_pushliteral(L, "weight");
    lua_pushinteger(L, (lua_Integer)peer->weight);
    lua_rawset(L, -3);

    lua_pushliteral(L, "current_weight");
    lua_pushinteger(L, (lua_Integer)peer->current_weight);
    lua_rawset(L, -3);

    lua_pushliteral(L, "effective_weight");
    lua_pushinteger(L, (lua_Integer)peer->effective_weight);
    lua_rawset(L, -3);

    lua_pushliteral(L, "fails");
    lua_pushinteger(L, (lua_Integer)peer->fails);
    lua_rawset(L, -3);

    lua_pushliteral(L, "max_fails");
    lua_pushinteger(L, (lua_Integer)peer->max_fails);
    lua_rawset(L, -3);

    lua_pushliteral(L, "fail_timeout");
    lua_pushinteger(L, (lua_Integer)peer->fail_timeout);
    lua_rawset(L, -3);

    if (peer->accessed) {
        lua_pushliteral(L, "accessed");
        lua_pushinteger(L, (lua_Integer)peer->accessed);
        lua_rawset(L, -3);
    }

    if (peer->checked) {
        lua_pushliteral(L, "checked");
        lua_pushinteger(L, (lua_Integer)peer->checked);
        lua_rawset(L, -3);
    }

    if (peer->down) {
        lua_pushliteral(L, "down");
        lua_pushboolean(L, 1);
        lua_rawset(L, -3);
    }

    return 1;
}

int lua_dynamic_upstream_list_zones(lua_State *L) {
    ngx_uint_t i;
    ngx_http_upstream_srv_conf_t *uscf, **uscfp;
    ngx_http_upstream_main_conf_t *umcf;
    ngx_http_request_t *r;

    if (lua_gettop(L) != 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "no argument expected\n");
        return 2;
    }

    r = ngx_http_lua_get_request(L);
    if (r == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "get request error \n");
        return 2;
    }

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);
    uscfp = umcf->upstreams.elts;

    lua_createtable(L, umcf->upstreams.nelts, 0);

    for (i = 0; i < umcf->upstreams.nelts; i++) {
        uscf = uscfp[i];
        if (uscf->shm_zone != NULL && uscf->shm_zone->shm.name.len > 0) {
            lua_pushlstring(L, (char *)uscf->shm_zone->shm.name.data, uscf->shm_zone->shm.name.len);

            if (uscf->port) {
                lua_pushfstring(L, ":%d", (int)uscf->port);
                lua_concat(L, 2);
            }

            lua_rawseti(L, -2, i + 1);
        }
    }

    return 1;
}

int lua_dynamic_upstream_describe_zone(lua_State *L) {
    ngx_uint_t i;
    ngx_http_upstream_srv_conf_t *uscf;
    ngx_http_upstream_rr_peers_t *peers;
    ngx_http_upstream_rr_peer_t *peer;
    ngx_str_t zone;

    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushliteral(L, "exactly 1 argument expected\n");
        return 2;
    }

    zone.data = (u_char *)luaL_checklstring(L, 1, &zone.len);
    uscf = lua_dynamic_upstream_get_zone(L, &zone);
    if (uscf == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "zone not found\n");
        return 2;
    }

    peers = uscf->peer.data;
    if (peers == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "no peer data\n");
        return 2;
    }

    lua_createtable(L, peers->number, 0);

    i = 0;
    for (peer = peers->peer; peer; peer = peer->next) {
        lua_dynamic_upstream_describe_peer(L, peer);
        lua_rawseti(L, -2, i + 1);
        i++;
    }

    return 1;
}

int lua_dynamic_upstream_add_peer(lua_State *L) {
    ngx_http_upstream_srv_conf_t *uscf;
    ngx_http_upstream_rr_peer_t *peer, *last;
    ngx_http_upstream_rr_peers_t *peers;
    ngx_slab_pool_t *shpool;
    ngx_str_t zone;
    ngx_url_t u;
    u_char *p;
    ngx_int_t weight, max_fails;
    time_t fail_timeout;

    if (lua_gettop(L) != 6) {
        lua_pushnil(L);
        lua_pushliteral(L, "exactly 6 arguments expected\n");
        return 2;
    }

    zone.data = (u_char *)luaL_checklstring(L, 1, &zone.len);
    uscf = lua_dynamic_upstream_get_zone(L, &zone);
    if (uscf == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "zone not found\n");
        return 2;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));
    p = (u_char *)luaL_checklstring(L, 2, &u.url.len);

    shpool = (ngx_slab_pool_t *)uscf->shm_zone->shm.addr;
    ngx_shmtx_lock(&shpool->mutex);

    u.url.data = ngx_slab_alloc_locked(shpool, u.url.len);
    if (u.url.data == NULL) {
        ngx_shmtx_unlock(&shpool->mutex);

        lua_pushnil(L);
        lua_pushliteral(L, "failed to allocate memory from slab\n");
        return 2;
    }

    ngx_cpystrn(u.url.data, p, u.url.len + 1);
    u.url.len = u.url.len;
    u.default_port = 80;

    peers = uscf->peer.data;
    for (peer = peers->peer, last = peer; peer; peer = peer->next) {
        if (u.url.len == peer->name.len && ngx_strncmp(u.url.data, peer->name.data, peer->name.len) == 0) {
            ngx_shmtx_unlock(&shpool->mutex);

            lua_pushnil(L);
            lua_pushliteral(L, "the peer is exist\n");
            return 2;
        }

        last = peer;
    }

    if (ngx_parse_url_slab(shpool, &u) != NGX_OK) {
        ngx_shmtx_unlock(&shpool->mutex);

        lua_pushnil(L);
        lua_pushliteral(L, "the peer is exist\n");
        return 2;
    }

    last->next = ngx_slab_calloc_locked(shpool, sizeof(ngx_http_upstream_rr_peer_t));
    if (last->next == NULL) {
        ngx_shmtx_unlock(&shpool->mutex);

        lua_pushnil(L);
        lua_pushliteral(L, "failed to allocate memory from slab\n");
        return 2;
    }

    weight = (ngx_int_t)luaL_checkint(L, 3);
    max_fails = (ngx_int_t)luaL_checkint(L, 4);
    fail_timeout = (time_t)luaL_checklong(L, 5);

    last->next->name = u.url;
    last->next->server = u.url;
    last->next->sockaddr = u.addrs[0].sockaddr;
    last->next->socklen = u.addrs[0].socklen;
    last->next->weight = weight;
    last->next->effective_weight = weight;
    last->next->current_weight = 0;
    last->next->max_fails = max_fails;
    last->next->fail_timeout = fail_timeout;
    last->next->down = lua_toboolean(L, 6);

    peers->number++;
    peers->total_weight += last->next->weight;
    peers->single = (peers->number == 1);
    peers->weighted = (peers->total_weight != peers->number);

    ngx_shmtx_unlock(&shpool->mutex);
    lua_pushboolean(L, 1);

    return 1;
}

int lua_dynamic_upstream_set_peer_down(lua_State *L) {
    ngx_http_upstream_srv_conf_t *uscf;
    ngx_http_upstream_rr_peer_t *peer, *target;
    ngx_http_upstream_rr_peers_t *peers;
    ngx_str_t zone, server;

    if (lua_gettop(L) != 3) {
        lua_pushnil(L);
        lua_pushliteral(L, "exactly 3 arguments expected\n");
        return 2;
    }

    zone.data = (u_char *)luaL_checklstring(L, 1, &zone.len);
    uscf = lua_dynamic_upstream_get_zone(L, &zone);
    if (uscf == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "zone not found\n");
        return 2;
    }

    peers = uscf->peer.data;
    if (peers == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "no peer data\n");
        return 2;
    }

    target = NULL;
    server.data = (u_char *)luaL_checklstring(L, 2, &server.len);

    for (peer = peers->peer; peer; peer = peer->next) {
        if (server.len == peer->name.len && ngx_strncmp(server.data, peer->name.data, peer->name.len) == 0) {
            target = peer;
            break;
        }
    }

    if (target == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "server not found\n");
        return 2;
    }

    target->down = lua_toboolean(L, 3);
    return 1;
}
