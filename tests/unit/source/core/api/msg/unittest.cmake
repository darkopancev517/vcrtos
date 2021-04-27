set(unittest-includes ${unittest-includes}
)

set(unittest-sources
    ../../source/core/thread.cpp
    ../../source/core/msg.cpp
    ../../source/core/assert_failure.c
    ../../source/core/api/thread_api.cpp
    ../../source/core/api/msg_api.cpp
    stubs/cpu_stub.c
    stubs/thread_arch_stub.c
)

set(unittest-test-sources
    source/core/api/msg/test_msg_api.cpp
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVCRTOS_PROJECT_CONFIG_FILE='\"vcrtos-unittest-config.h\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVCRTOS_PROJECT_CONFIG_FILE='\"vcrtos-unittest-config.h\"'")
