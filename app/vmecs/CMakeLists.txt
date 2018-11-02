# app/vmecs

add_exe_batch(vmecs STATIC "*.c")
target_link_libraries(vmecs toml vmecs-crypto vmecs-proto pthread)
