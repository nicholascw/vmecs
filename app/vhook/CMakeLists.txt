# app/vhook

add_lib_batch(vhook-core SHARED "hook.c")
target_link_libraries(vhook-core vmecs-crypto vmecs-proto dl)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVHOOK_CORE=\\\"libvhook-core.so\\\"")

add_exe_batch(vhook STATIC "main.c")
target_link_libraries(vhook)
