# proto

add_lib_batch(vmecs-proto STATIC "*.c" "vmess/*.c" "native/*.c" "relay/*.c" "socks/*.c" "dokodemo/*.c")

target_link_libraries(vmecs-proto vmecs-crypto vmecs-pub)
