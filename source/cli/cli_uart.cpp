/*
 * Copyright (c) 2020, Vertexcom Technologies, Inc.
 * All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains
 * the property of Vertexcom Technologies, Inc. and its suppliers,
 * if any. The intellectual and technical concepts contained
 * herein are proprietary to Vertexcom Technologies, Inc.
 * and may be covered by U.S. and Foreign Patents, patents in process,
 * and protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Vertexcom Technologies, Inc.
 *
 * Authors: Darko Pancev <darko.pancev@vertexcom.com>
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli/cli_uart.hpp"

#include "core/code_utils.h"
#include "core/new.hpp"
#include "core/thread.hpp"

#include "utils/isrpipe.hpp"

namespace vc {
namespace cli {

static const char _command_prompt[] = {'>', ' '};
static const char _erase_string[]   = {'\b', ' ', '\b'};
static const char _newline[]           = {'\r', '\n'};

extern "C" ssize_t vcstdio_read(void *buffer, size_t count);

Uart *Uart::_uart_server;

static DEFINE_ALIGNED_VAR(cli_uart_raw, sizeof(Uart), uint64_t);

extern "C" void *thread_cli_uart_handler(void *arg)
{
    Uart *uart = static_cast<Uart *>(arg);

    uart->output(_command_prompt, sizeof(_command_prompt));

    char c;

    while (1)
    {
        if (vcstdio_read(static_cast<void *>(&c), 1))
        {
            uart->receive_task(reinterpret_cast<const uint8_t *>(&c), 1);
        }
    }

    /* should not reach here */

    return NULL;
}

char _cli_uart_stack[VCRTOS_CONFIG_CLI_UART_THREAD_STACK_SIZE];

extern "C" void vccli_uart_init()
{
    Uart::_uart_server = new (&cli_uart_raw) Uart();

    (void) thread_create(_cli_uart_stack, sizeof(_cli_uart_stack), thread_cli_uart_handler, "uart-cli",
                         VCRTOS_CONFIG_CLI_UART_THREAD_PRIORITY,
                         static_cast<void *>(Uart::_uart_server),
                         THREAD_FLAGS_CREATE_WOUT_YIELD | THREAD_FLAGS_CREATE_STACKMARKER);
}

extern "C" void vccli_set_user_commands(const cli_command_t *user_commands, uint8_t length)
{
    Uart::_uart_server->get_interpreter().set_user_commands(user_commands, length);
}

extern "C" void vccli_output_bytes(const uint8_t *bytes, uint8_t length)
{
    Uart::_uart_server->get_interpreter().output_bytes(bytes, length);
}

extern "C" void vccli_output_format(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Uart::_uart_server->output_format(fmt, ap);
    va_end(ap);
}

extern "C" void vccli_output(const char *string, uint16_t length)
{
    Uart::_uart_server->output(string, length);
}

Uart::Uart()
    : _rx_length(0)
    , _tx_head(0)
    , _tx_length(0)
    , _send_length(0)
    , _interpreter()
{
}

void Uart::receive_task(const uint8_t *buf, uint16_t buf_length)
{
    const uint8_t *end;

    end = buf + buf_length;

    for (; buf < end; buf++)
    {
        switch (*buf)
        {
        case '\r':
        case '\n':
            output(_newline, sizeof(_newline));

            if (_rx_length > 0)
            {
                _rx_buffer[_rx_length] = '\0';
                process_command();
            }

            output(_command_prompt, sizeof(_command_prompt));
            break;

        case '\b':
        case 127:
            if (_rx_length > 0)
            {
                output(_erase_string, sizeof(_erase_string));
                _rx_buffer[--_rx_length] = '\0';
            }
            break;

        default:
            if (_rx_length < RX_BUFFER_SIZE)
            {
                output(reinterpret_cast<const char *>(buf), 1);
                _rx_buffer[_rx_length++] = static_cast<char>(*buf);
            }
            break;
        }
    }
}

int Uart::process_command()
{
    if (_rx_buffer[_rx_length - 1] == '\n')
    {
        _rx_buffer[--_rx_length] = '\0';
    }

    if (_rx_buffer[_rx_length - 1] == '\r')
    {
        _rx_buffer[--_rx_length] = '\0';
    }

    _interpreter.process_line(_rx_buffer, _rx_length, *this);

    _rx_length = 0;

    return 0;
}

int Uart::output(const char *buf, uint16_t buf_length)
{
    uint16_t remaining = TX_BUFFER_SIZE - _tx_length;
    uint16_t tail;

    if (buf_length > remaining)
    {
        buf_length = remaining;
    }

    for (int i = 0; i < buf_length; i++)
    {
        tail = (_tx_head + _tx_length) % TX_BUFFER_SIZE;
        _tx_buffer[tail] = *buf++;
        _tx_length++;
    }

    send();

    return buf_length;
}

int Uart::output_format(const char *fmt, ...)
{
    char buf[MAX_LINE_LENGTH];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return output(buf, static_cast<uint16_t>(strlen(buf)));
}

void Uart::send()
{
    while (_tx_length > 0)
    {
        if (_tx_length > TX_BUFFER_SIZE - _tx_head)
        {
            _send_length = TX_BUFFER_SIZE - _tx_head;
        }
        else
        {
            _send_length = _tx_length;
        }

        if (_send_length > 0)
        {
            for (uint16_t i = 0; i < _send_length; i++)
            {
                putchar(_tx_buffer[_tx_head + i]);
                fflush(stdout);
            }
        }

        _tx_head = (_tx_head + _send_length) % TX_BUFFER_SIZE;
        _tx_length -= _send_length;
        _send_length = 0;
    }
}

} // namespace cli
} // namespace vc
