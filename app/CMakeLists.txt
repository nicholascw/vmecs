# app

add_exe_batch(vmecs-main STATIC "*.c")

target_link_libraries(vmecs-main vmecs-crypto vmecs-proto pthread)
