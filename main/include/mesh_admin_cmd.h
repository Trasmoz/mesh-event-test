/*
	Copyright 2023 Daniel Calatayud 
	#GNUGPLv3
    This file is part of mesh network administration and testing module.
	The mesh network administration and testing module is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
	The mesh network administration and testing module is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>. 
*/

#ifndef MESH_ADMIN_CMD_H
#define MESH_ADMIN_CMD_H

#include "esp_event.h"

#define OPT_MSG_CMD 0xffu
#define OPT_EMPTY 0x00u

ESP_EVENT_DECLARE_BASE(MESH_TEST_EVENTS);         

enum {
    MESH_TEST_NODE_RESTART_EVENT,
    MESH_TEST_NODE_SLUGGISH_EVENT,
    MESH_TEST_NODE_NOISE_EVENT                     
};

typedef struct {
	uint8_t laziness_time_milis;
    uint8_t total_time_sg;
} sluggish_t;

typedef struct {
	uint8_t msg2second;
	mesh_addr_t to_addr;
    uint8_t total_time_sg;
} noise_t;

typedef struct {
	uint8_t id_cmd;
	uint8_t id_pkg;
	uint8_t len_params;
    uint8_t params[50];
} cmd_t;


void init_msgs_buffer();

esp_err_t read_msg(mesh_addr_t *from, mesh_data_t *data, int *flag, mesh_opt_t *opts);

int get_msgs_pending();

#endif