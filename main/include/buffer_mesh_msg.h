/*
	Copyright 2023 Daniel Calatayud 
	#GNUGPLv3
    This file is part of mesh network administration and testing module.
	The mesh network administration and testing module is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
	The mesh network administration and testing module is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>. 
*/

#ifndef BUFFER_MESH_MSG_H
#define BUFFER_MESH_MSG_H

#include "esp_err.h"
#include "esp_mesh.h"


typedef struct mesh_msg {
	mesh_addr_t from;
	mesh_data_t payload;
	mesh_opt_t opt;
	int flag;
} mesh_msg_t;


esp_err_t add_msg(mesh_msg_t *msg);
mesh_msg_t * get_msg();
int get_num_msgs();

#endif



