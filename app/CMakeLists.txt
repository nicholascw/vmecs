# app

# add_exe_batch(vmecs-main STATIC "*.c")
# target_link_libraries(vmecs-main vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmecs-client STATIC "client.c")
target_link_libraries(vmecs-client vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmecs-middle STATIC "middle.c")
target_link_libraries(vmecs-middle vmecs-crypto vmecs-proto pthread)

add_exe_batch(vmecs-server STATIC "server.c")
target_link_libraries(vmecs-server vmecs-crypto vmecs-proto pthread)
