vmecs
===

vmecs is a proxy implementation written in C, and is based on VMess protocol 
which is designed by [V2Ray](https://v2ray.com) project (a.k.a Project V). 
The original project is licensed in MIT License, with original repository hosted
 on [GitHub](https://github.com/v2ray/v2ray-core).

**vmecs** is licensed in GPLv3.

Usage
---
`$ vmecs <config>`

Config File
---
Different from [v2ray-core](https://github.com/v2ray/v2ray-core), vmecs uses a (slightly modified) version
of TOML(Tom's Obvious, Minimal Language). An example is shown below.
```
[inbound]
    proto = "vmess"
    local = "127.0.0.1"
    port = "3133"
    pass = "123456"

[outbound]
    proto = "socks5"
    proxy = "1.2.3.4"
    port = "4321"
```
