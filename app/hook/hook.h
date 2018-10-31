#ifndef _HOOK_H_
#define _HOOK_H_

#define REAL_FUNC(name) real_ ## name
#define REAL_TYPE(name) real_ ## name ## _t
#define LOAD_REAL(name) (*(void **)(&REAL_FUNC(name)) = load_libc(#name))
#define DEF_REAL(name) static REAL_TYPE(name) REAL_FUNC(name)

#endif
