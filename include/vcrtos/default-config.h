#ifndef VCRTOS_DEFAULT_CONFIG_H
#define VCRTOS_DEFAULT_CONFIG_H

#ifndef VCRTOS_CONFIG_PACKAGE_NAME
#define VCRTOS_CONFIG_PACKAGE_NAME "vcrtos"
#endif

#ifndef VCRTOS_CONFIG_PACKAGE_VERSION
#define VCRTOS_CONFIG_PACKAGE_VERSION "0.0.1"
#endif

#ifndef VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
#define VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE 0
#endif

#ifndef VCRTOS_CONFIG_THREAD_PRIORITY_LEVELS
#define VCRTOS_CONFIG_THREAD_PRIORITY_LEVELS 16
#endif

#ifndef VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
#define VCRTOS_CONFIG_THREAD_FLAGS_ENABLE 0
#endif

#ifndef VCRTOS_CONFIG_THREAD_EVENT_ENABLE
#define VCRTOS_CONFIG_THREAD_EVENT_ENABLE 0
#endif

#ifndef VCRTOS_CONFIG_UTILS_UART_TSRB_ISRPIPE_SIZE
#define VCRTOS_CONFIG_UTILS_UART_TSRB_ISRPIPE_SIZE 128
#endif

#ifndef VCRTOS_CONFIG_MAIN_THREAD_STACK_SIZE
#define VCRTOS_CONFIG_MAIN_THREAD_STACK_SIZE 1024
#endif

#ifndef VCRTOS_CONFIG_IDLE_THREAD_STACK_SIZE
#define VCRTOS_CONFIG_IDLE_THREAD_STACK_SIZE 1024
#endif

#ifndef VCRTOS_CONFIG_CLI_UART_RX_BUFFER_SIZE
#define VCRTOS_CONFIG_CLI_UART_RX_BUFFER_SIZE 128
#endif

#ifndef VCRTOS_CONFIG_CLI_UART_TX_BUFFER_SIZE
#define VCRTOS_CONFIG_CLI_UART_TX_BUFFER_SIZE 128
#endif

#ifndef VCRTOS_CONFIG_CLI_MAX_LINE_LENGTH
#define VCRTOS_CONFIG_CLI_MAX_LINE_LENGTH 128
#endif

#ifndef VCRTOS_CONFIG_CLI_UART_STACK_SIZE
#define VCRTOS_CONFIG_CLI_UART_STACK_SIZE 1024
#endif

#ifndef VCRTOS_CONFIG_CLI_UART_THREAD_PRIORITY
#define VCRTOS_CONFIG_CLI_UART_THREAD_PRIORITY KERNEL_THREAD_PRIORITY_MAIN
#endif

#ifndef VCRTOS_CONFIG_TIMER_BACKOFF
#define VCRTOS_CONFIG_TIMER_BACKOFF 30
#endif

#ifndef VCRTOS_CONFIG_TIMER_OVERHEAD
#define VCRTOS_CONFIG_TIMER_OVERHEAD 20
#endif

#ifndef VCRTOS_CONFIG_TIMER_ISR_BACKOFF
#define VCRTOS_CONFIG_TIMER_ISR_BACKOFF 20
#endif

#ifndef VCRTOS_CONFIG_TIMER_PERIODIC_RELATIVE
#define VCRTOS_CONFIG_TIMER_PERIODIC_RELATIVE 512
#endif

#ifndef VCRTOS_CONFIG_TIMER_DEV
#define VCRTOS_CONFIG_TIMER_DEV 0
#define VCRTOS_CONFIG_TIMER_CHAN 0
#endif

#ifndef VCRTOS_CONFIG_TIMER_WIDTH
#define VCRTOS_CONFIG_TIMER_WIDTH 32
#endif

#ifndef VCRTOS_CONFIG_TIMER_HZ_BASE
#define VCRTOS_CONFIG_TIMER_HZ_BASE (1000000)
#endif

#ifndef VCRTOS_CONFIG_TIMER_MASK
#if (VCRTOS_CONFIG_TIMER_WIDTH != 32)
#define VCRTOS_CONFIG_TIMER_MASK ((0xffffffff >> VCRTOS_CONFIG_TIMER_WIDTH) << VCRTOS_CONFIG_TIMER_WIDTH)
#else
#define VCRTOS_CONFIG_TIMER_MASK (0)
#endif
#endif

#ifndef VCRTOS_CONFIG_TIMER_HZ
#define VCRTOS_CONFIG_TIMER_HZ VCRTOS_CONFIG_TIMER_HZ_BASE
#endif

#ifndef VCRTOS_CONFIG_TIMER_SHIFT
#if (VCRTOS_CONFIG_TIMER_HZ == 32768)
#define VCRTOS_CONFIG_TIMER_SHIFT 0
#elif (VCRTOS_CONFIG_TIMER_HZ == VCRTOS_CONFIG_TIMER_HZ_BASE)
#define VCRTOS_CONFIG_TIMER_SHIFT 0
#elif (VCRTOS_CONFIG_TIMER_HZ >> 1 == VCRTOS_CONFIG_TIMER_HZ_BASE) || \
    (VCRTOS_CONFIG_TIMER_HZ << 1 == VCRTOS_CONFIG_TIMER_HZ_BASE)
#define VCRTOS_CONFIG_TIMER_SHIFT 1
#elif (VCRTOS_CONFIG_TIMER_HZ >> 2 == VCRTOS_CONFIG_TIMER_HZ_BASE) || \
    (VCRTOS_CONFIG_TIMER_HZ << 2 == VCRTOS_CONFIG_TIMER_HZ_BASE)
#define VCRTOS_CONFIG_TIMER_SHIFT 2
#elif (VCRTOS_CONFIG_TIMER_HZ >> 3 == VCRTOS_CONFIG_TIMER_HZ_BASE) || \
    (VCRTOS_CONFIG_TIMER_HZ << 3 == VCRTOS_CONFIG_TIMER_HZ_BASE)
#define VCRTOS_CONFIG_TIMER_SHIFT 3
#elif (VCRTOS_CONFIG_TIMER_HZ >> 4 == VCRTOS_CONFIG_TIMER_HZ_BASE) || \
    (VCRTOS_CONFIG_TIMER_HZ << 4 == VCRTOS_CONFIG_TIMER_HZ_BASE)
#define VCRTOS_CONFIG_TIMER_SHIFT 4
#elif (VCRTOS_CONFIG_TIMER_HZ >> 5 == VCRTOS_CONFIG_TIMER_HZ_BASE) || \
    (VCRTOS_CONFIG_TIMER_HZ << 5 == VCRTOS_CONFIG_TIMER_HZ_BASE)
#define VCRTOS_CONFIG_TIMER_SHIFT 5
#elif (VCRTOS_CONFIG_TIMER_HZ >> 6 == VCRTOS_CONFIG_TIMER_HZ_BASE) || \
    (VCRTOS_CONFIG_TIMER_HZ << 6 == VCRTOS_CONFIG_TIMER_HZ_BASE)
#define VCRTOS_CONFIG_TIMER_SHIFT 6
#else
#error "VCRTOS_CONFIG_TIMER_SHIFT cannot be derived from given VCRTOS_CONFIG_TIMER_HZ!"
#endif
#endif

#endif /* VCRTOS_DEFAULT_CONFIG_H */
