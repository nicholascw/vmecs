# app/test/vmess

add_exe_batch(vmess-client STATIC "client.c")
target_link_libraries(vmess-client vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmess-middle STATIC "middle.c")
target_link_libraries(vmess-middle vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmess-server STATIC "server.c")
target_link_libraries(vmess-server vmecs-crypto vmecs-proto pthread)
