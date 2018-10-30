# app

# add_exe_batch(vmecs-main STATIC "*.c")
# target_link_libraries(vmecs-main vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmess-client STATIC "vmess/client.c")
target_link_libraries(vmess-client vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmess-middle STATIC "vmess/middle.c")
target_link_libraries(vmess-middle vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmess-server STATIC "vmess/server.c")
target_link_libraries(vmess-server vmecs-crypto vmecs-proto pthread)

# add_exe_batch(socks5 STATIC "vmess/socks5.c")
# target_link_libraries(socks5 vmecs-crypto vmecs-proto pthread)

add_exe_batch(socks5-client STATIC "socks5/client.c")
target_link_libraries(socks5-client vmecs-crypto vmecs-proto pthread)

add_exe_batch(socks5-server STATIC "socks5/server.c")
target_link_libraries(socks5-server vmecs-crypto vmecs-proto pthread)
