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

#ifndef CLI_UART_HPP
#define CLI_UART_HPP

#include <vcrtos/config.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"

namespace vc {

namespace cli {

class Uart : public Server
{
public:
    Uart();
    virtual int output(const char *buf, uint16_t buf_length);
    virtual int output_format(const char *fmt, ...);
    Interpreter &get_interpreter() { return _interpreter; }
    void receive_task(const uint8_t *buf, uint16_t buf_length);
    static Uart *_uart_server;

private:
    enum
    {
        RX_BUFFER_SIZE = VCRTOS_CONFIG_CLI_UART_RX_BUFFER_SIZE,
        TX_BUFFER_SIZE = VCRTOS_CONFIG_CLI_UART_TX_BUFFER_SIZE,
        MAX_LINE_LENGTH = VCRTOS_CONFIG_CLI_MAX_LINE_LENGTH,
    };

    int process_command();
    void send();

    char _rx_buffer[RX_BUFFER_SIZE];
    uint16_t _rx_length;

    char _tx_buffer[TX_BUFFER_SIZE];
    uint16_t _tx_head;
    uint16_t _tx_length;

    uint16_t _send_length;

    Interpreter _interpreter;

    friend class Interpreter;
};

} // namespace cli
} // namespace vc

#endif /* CLI_UART_HPP */
