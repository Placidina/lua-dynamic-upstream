# lua-dynamic-upstream

`lua_dynamic_upstream` - Nginx C module to expose Lua API to ngx_lua for Nginx upstreams

* [Requirements](#requirements)
* [Compatibility](#compatibility)
* [Status](#status)
* [Installation](#installation)
* [Functions](#functions)
  * [list_zones](#list_zones)
  * [describe_zone](#describe_zone)
  * [add_peer](#add_peer)
  * [remove_peer](#remove_peer)
  * [set_peer_down](#set_peer_down)

## Requirements

* Requires the `zone` directive in the upstream context.
* Requires the `lua-nginx-module` or `OpenResty`.

## Compatibility

The following versions of Nginx should work with this module:

* **1.17.x**
* **1.16.x**
* **1.15.x**
* **1.14.x**
* **1.13.x**
* **1.12.x**
* **1.11.x**
* **1.10.x**
* **1.9.x**

## Status

This module is production ready.

## Installation

1. Grab the nginx source code from [nginx.org](http://nginx.org/).
2. then grab the source code of the [ngx_lua](https://github.com/openresty/lua-nginx-module#installation) as well as its dependencies like [LuaJIT](http://luajit.org/download.html).
3. and finally build the source with this module:

```
curl -fSL http://nginx.org/download/nginx-1.16.0.tar.gz | tar zx
cd nginx-1.16.0/

# assuming your luajit is installed to /opt/luajit:
export LUAJIT_LIB=/opt/luajit/lib

# assuming you are using LuaJIT v2.1:
export LUAJIT_INC=/opt/luajit/include/luajit-2.1

# Here we assume you would install you nginx under /opt/nginx/.
./configure --prefix=/opt/nginx \
    --with-ld-opt="-Wl,-rpath,$LUAJIT_LIB" \
    --add-module=/path/to/lua-nginx-module \
    --add-module=/path/to/lua-dynamic-upstream

make -j2
make install
```

## Functions

- [x] `list_zones`
- [x] `describe_zone`
- [x] `add_peer`
- [x] `remove_peer`
- [x] `set_peer_down`

### list_zones

Return one `array` of all upstream zone names.

_Lua arguments:_

No arguments.

_Example:_

```sh
upstream foo {
    zone dynamic_foo 32k;

   server 127.0.0.1:8030;
}

upstream bar {
    zone dynamic_bar 32k;

   server 127.0.0.1:8031;
}

server {
    listen 80;

    location = /dynamic/list {
        default_type text/plain;
        content_by_lua '
            local dynamic = require "ngx.dynamic"
            local zones, err = dynamic.list_zones()
            if not zones then
                ngx.say("unable to list upstream zone names: " .. err)
            else
                ngx.say("Upstream zone names:")
                for i, z in ipairs(zones) do
                    ngx.say("    ", z)
                end
            end
        ';
    }
}
```

### describe_zone

Return one `array` of `object` for all peers in upstream zone.

_Return `peer` informations:_

* `current_weight`
* `effective_weight`
* `fail_timeout`
* `fails`
* `max_fails`
* `name` - Socket address for the current peer.
* `weight`
* `accessed` - Timestamp for the last access (in seconds since the Epoch).
* `checked` - Timestamp for the last check (in seconds since the Epoch).
* `down` - Holds true if the peer has been marked as "down", otherwise this key is not present.
* `conns` - Number of active connections to the peer.

_Lua arguments:_

Required one argument:

* `upstream zone name`

_Example:_

```sh
upstream foo {
    zone dynamic_foo 32k;

   server 127.0.0.1:8030;
   server 127.0.0.1:8031;
}

server {
    listen 80;

    location = /dynamic/describe {
        default_type text/plain;
        content_by_lua '
            local dynamic = require "ngx.dynamic"
            local zones, err = dynamic.describe_zone("dynamic_foo")
            if not zones then
                ngx.say("unable to describe upstream zone: " .. err)
            else
                for i, zone in ipairs(zones) do
                    ngx.say("Peer")
                    for k, v in pairs(zone) do
                        ngx.say("    ",k, " => ", v)
                    end
                end
            end
        ';
    }
}
```

### add_peer

Add new peer in upstream zone.

TODO:

### remove_peer

Remove peer in upstream zone.

TODO:

### set_peer_down

Set peer down or up.

TODO:
