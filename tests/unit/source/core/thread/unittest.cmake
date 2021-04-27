set(unittest-includes ${unittest-includes}
)

set(unittest-sources
    ../../source/core/thread.cpp
    ../../source/core/api/event_api.cpp
    ../../source/core/assert_failure.c
    stubs/cpu_stub.c
    stubs/thread_arch_stub.c
)

set(unittest-test-sources
    source/core/thread/test_thread.cpp
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVCRTOS_PROJECT_CONFIG_FILE='\"vcrtos-unittest-config.h\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVCRTOS_PROJECT_CONFIG_FILE='\"vcrtos-unittest-config.h\"'")
