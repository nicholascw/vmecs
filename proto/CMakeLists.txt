# proto

add_lib_batch(vmecs-proto STATIC "*.c")

target_link_libraries(vmecs-proto vmecs-crypto vmecs-pub)
