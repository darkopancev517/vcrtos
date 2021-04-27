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

#ifndef CLI_HPP
#define CLI_HPP

#include <stdint.h>

#include <vcrtos/config.h>
#include <vcrtos/cli.h>

namespace vc {
namespace cli {

class Interpreter;
class Server;

class Interpreter
{
public:
    Interpreter();
    void set_user_commands(const cli_command_t *commands, uint8_t length);
    void process_line(char *buf, uint16_t length, Server &server);
    static int parse_long(char *string, long &result);
    static int parse_unsigned_long(char *string, unsigned long &result);
    void output_bytes(const uint8_t *bytes, uint8_t length) const;

private:
    enum
    {
        MAX_ARGS = 32,
    };

    const cli_command_t *_user_commands;
    uint8_t _user_commands_length;
    Server *_server;
};

} // namespace cli
} // namespace vc

#endif /* CLI_HPP */
