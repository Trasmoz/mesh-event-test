
/*
	Copyright 2023 Daniel Calatayud 
	#GNUGPLv3
    This file is part of mesh network administration and testing module.
	The mesh network administration and testing module is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
	The mesh network administration and testing module is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>. 
*/

#include "esp_err.h"
#include "buffer_mesh_msg.h"

typedef struct mesh_buff_msg {
	mesh_msg_t *msg;
	struct mesh_buff_msg *next;
} mesh_buff_msg_t;

typedef struct cola_msg {
	mesh_buff_msg_t *first;
	mesh_buff_msg_t *last;
} cola_msg_t;


static cola_msg_t buff = { NULL, NULL };

static uint8_t num_msgs = 0; 

esp_err_t add_msg(mesh_msg_t *msg) {

	//TODO Hacer copia de todos los punteros de las estructuras
	mesh_buff_msg_t *node = malloc(sizeof(mesh_buff_msg_t));
	node->msg = msg;

	if(buff.last != NULL) {
		buff.last->next = node;
		buff.last = node;
	} else {
		buff.last = node;
		buff.first = node;
	}
	num_msgs++;

	return ESP_OK;
}

mesh_msg_t * get_msg() {
	mesh_msg_t *msg = NULL;
	mesh_buff_msg_t *node = buff.first;

	if (node != NULL) {
		msg = node->msg;
		buff.first = node->next;
		if(buff.last == node) buff.last = NULL;
		free(node);
		num_msgs--;
	}

	return msg;
}

int get_num_msgs() {
	return num_msgs;
}