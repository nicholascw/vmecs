# crypto

add_lib_batch(vmecs-crypto STATIC "*.c")

target_link_libraries(vmecs-crypto ssl crypto)
