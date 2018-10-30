# proto

add_lib_batch(vmecs-proto STATIC "*.c" "vmess/*.c" "native/*.c" "router/*.c" "socks5/*.c")

target_link_libraries(vmecs-proto vmecs-crypto vmecs-pub)
