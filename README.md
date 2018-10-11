VMecs
===

VMecs is a proxy implementation written in C, and is based on VMess protocol 
which is designed by [V2Ray](https://v2ray.com) project (a.k.a Project V). 
The original project is licensed in MIT License, with original repository hosted
 on [GitHub](https://github.com/v2ray/v2ray-core).

**VMecs** is licensed in GPLv3.

Usage
---
`$ vmecs CONFIG`

Config File
---
Different with [v2ray-core](https://github.com/v2ray/v2ray-core), VMecs's config
file currently would follow the key-value style for simplicity, shown below:
```
inbound.proto=vmess
inbound.addr=example.org
inbound.port=1080
...
```