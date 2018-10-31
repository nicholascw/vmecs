# app/hook

add_lib_batch(vhook SHARED "hook.c")
target_link_libraries(vhook vmecs-crypto vmecs-proto dl)

add_exe_batch(hookv STATIC "main.c")
target_link_libraries(hookv vmecs-crypto vmecs-proto)
