# app/test/socks5

add_exe_batch(socks5-client STATIC "client.c")
target_link_libraries(socks5-client vmecs-crypto vmecs-proto pthread)

add_exe_batch(socks5-server STATIC "server.c")
target_link_libraries(socks5-server vmecs-crypto vmecs-proto pthread)
