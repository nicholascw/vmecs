#ifndef _VHOOK_HOOK_H_
#define _VHOOK_HOOK_H_

#define REAL_FUNC(name) real_ ## name
#define REAL_TYPE(name) real_ ## name ## _t
#define LOAD_REAL(name) (*(void **)(&REAL_FUNC(name)) = load_libc(#name))
#define DEF_REAL(name) static REAL_TYPE(name) REAL_FUNC(name)

#define PROXY_ENV_VAR "VHOOK_PROXY"
#define PORT_ENV_VAR "VHOOK_PORT"

#endif
