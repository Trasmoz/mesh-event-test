/*
	Copyright 2023 Daniel Calatayud 
	#GNUGPLv3
    This file is part of mesh network administration and testing module.
	The mesh network administration and testing module is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
	The mesh network administration and testing module is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>. 
*/

#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_event_base.h"

#include "mesh_admin_cmd.h"
#include "buffer_mesh_msg.h"
#include "esp_mac.h"


#define TAG_ADMIN_MESH_MSGS "ADMIN_MESH_MSGS"
#define NO_MSGS "No hay mensajes disponibles"
#define MSG_SIZE          (1560)
#define OPT_SIZE          (50)



//mesh_opt_t opt_rcv;
//uint8_t buf_data_opt_rcv[50] = { 0, };
//opt_rcv.val =  malloc(sizeof(uint8_t)*10);
typedef struct {
    uint8_t cmd;
    uint8_t id_pkg;
    char * msg_txt;
} msg_t;

ESP_EVENT_DEFINE_BASE(MESH_TEST_EVENTS);

void load_msgs_in_buffer();

void init_msgs_buffer() {

	xTaskCreate(&load_msgs_in_buffer, "load_msgs_in_buffer", 4096, NULL, 5, NULL);
}

void load_msgs_in_buffer() {

	ESP_LOGI(TAG_ADMIN_MESH_MSGS,"********** load_msgs_in_buffer **********");

	mesh_data_t data_rcv;
	static uint8_t buf_data_rcv[MSG_SIZE] = { 0, };

	//data_rcv.data = malloc(sizeof(uint8_t) * MSG_SIZE);

	mesh_addr_t from;
	int flag = 0;

	uint8_t id_cmd = 0;
	mesh_opt_t opt_rcv = {
        .len  = sizeof(id_cmd),
        .val  = (void *) &id_cmd,
        .type = MESH_OPT_RECV_DS_ADDR
    };


	//mesh_opt_t *opt_rcv = malloc(sizeof(mesh_opt_t));
	//opt_rcv.val =  malloc(sizeof(uint8_t) * 10);


	ESP_LOGI(TAG_ADMIN_MESH_MSGS, "****** MEMORIA LIBRE, heap:%d \n",  esp_get_free_heap_size());
    while (1) {

        data_rcv.data = buf_data_rcv;
        data_rcv.size = MSG_SIZE;

        id_cmd = OPT_EMPTY;

/*
		buf_opt_rcv[0] = 5;
        opt_rcv.val = buf_opt_rcv;
        opt_rcv.len = OPT_SIZE;
*/
	    esp_err_t error = esp_mesh_recv(&from, &data_rcv, portMAX_DELAY, &flag, &opt_rcv, 1);  
  	//	ESP_LOGI(TAG_ADMIN_MESH_MSGS, "******** LECTURA HACIA EL BUFFER ******");

	    if(error != ESP_OK) {
			ESP_LOGE(TAG_ADMIN_MESH_MSGS, "ERROR leyendo un nuevo mensaje: %s", esp_err_to_name(error));
	    } else if(*(opt_rcv.val) == OPT_MSG_CMD) { // ¿Puede ser nulo?
			cmd_t * cmd = (cmd_t *) data_rcv.data;
	        // Es un comando, se convertirá en un evento.

	//        printf("*** Recibido el comando %d del paquete %d con tamanyo de parametros %d \n", cmd->id_cmd, cmd->id_pkg, cmd->len_params);
	        if(cmd->id_cmd == MESH_TEST_NODE_SLUGGISH_EVENT) {
				sluggish_t * cmd_slug = (sluggish_t *) cmd->params;
	//			ESP_LOGI(TAG_ADMIN_MESH_MSGS, "PARAMS: laziness %d, total time:%d", cmd_slug->laziness_time_milis, cmd_slug->total_time_sg);
	        } else if(cmd->id_cmd == MESH_TEST_NODE_NOISE_EVENT) {
				noise_t * cmd_noise = (noise_t *) cmd->params;
	//			ESP_LOGI(TAG_ADMIN_MESH_MSGS, "PARAMS: msg2second %d, to_addr: "MACSTR" total_time_sg %d", cmd_noise->msg2second, MAC2STR(cmd_noise->to_addr.addr), cmd_noise->total_time_sg);
	        }
			ESP_LOGI(TAG_ADMIN_MESH_MSGS, "NOTIFICANDO COMANDO %d, llegado en paquete %d\n", cmd->id_cmd, cmd->id_pkg);
	        esp_event_post(MESH_TEST_EVENTS, cmd->id_cmd, cmd->params, cmd->len_params, portMAX_DELAY);
	        //esp_event_post(MESH_TEST_EVENTS, MESH_TEST_NODE_RESTART_EVENT, opt_rcv.val, sizeof(opt_rcv.val[0]), portMAX_DELAY);
	//        ESP_LOGI(TAG_ADMIN_MESH_MSGS, "COMANDO %d, tamanyo de parametros %d\n", cmd->id_cmd, cmd->len_params);

	    } else { // Construcción del mensaje para almacenarlo
	//    	printf("*** Recibido msg de app %d\n", opt_rcv.len);
	//    	ESP_LOGI(TAG_ADMIN_MESH_MSGS, "MNJ_APP %d, VALOR OPT:%p -> %d vs %d", ((msg_t *) data_rcv.data)->id_pkg, opt_rcv.val,  *(opt_rcv.val), OPT_MSG_CMD);

	        mesh_msg_t *msg_to_buff = malloc(sizeof(mesh_msg_t));
	        msg_to_buff->from = from;
	        msg_to_buff->flag = flag;

			msg_to_buff->payload.size = data_rcv.size;
			msg_to_buff->payload.proto = data_rcv.proto;
			msg_to_buff->payload.tos = data_rcv.tos;

			if(data_rcv.size > 0) {
				msg_to_buff->payload.data = malloc(sizeof(uint8_t) * data_rcv.size);
				memcpy(msg_to_buff->payload.data, (uint8_t *) data_rcv.data, data_rcv.size);
			} else {
				msg_to_buff->payload.data = NULL;
			}

			if(opt_rcv.type > 0 || opt_rcv.len > 0) {
				msg_to_buff->opt.type = opt_rcv.type;
				msg_to_buff->opt.len = opt_rcv.len;
				msg_to_buff->opt.val = malloc(sizeof(uint8_t) * opt_rcv.len);
				memcpy(msg_to_buff->opt.val, (uint8_t *) opt_rcv.val, opt_rcv.len); 
	        } else {
	        	msg_to_buff->opt.val = NULL;
	        	msg_to_buff->opt.len = 0;
	        }

	        add_msg(msg_to_buff); 
	    }
	}

}

esp_err_t read_msg(mesh_addr_t *from, mesh_data_t *payload, int *flag, mesh_opt_t *opts) {
	//ESP_LOGI(TAG_ADMIN_MESH_MSGS,"######## LEYENDO MENSAJE DE APLICACION ########");

	ESP_LOGI(TAG_ADMIN_MESH_MSGS, "- Mensajes disponibles: %d", get_num_msgs());


    ESP_RETURN_ON_FALSE(get_num_msgs() > 0, ESP_ERR_NOT_FOUND, TAG_ADMIN_MESH_MSGS, "No hay mensajes disponibles");
    mesh_msg_t *msg = get_msg();

	*from = msg->from;
	*flag = msg->flag;

	payload->size = msg->payload.size;
	payload->proto = msg->payload.proto;
	payload->tos = msg->payload.tos;
	
	if(msg->payload.size > 0) {
		//ESP_LOGI(TAG_ADMIN_MESH_MSGS, "Recuperado del buffer el mnj: %d", ((msg_t *) msg->payload.data)->id_pkg);
		memcpy(payload->data, (uint8_t *) msg->payload.data, msg->payload.size); 
	} else {
		ESP_LOGI(TAG_ADMIN_MESH_MSGS,"Payload vacío, no se rellena ");
	}

	opts->type = msg->opt.type;
	opts->len = msg->opt.len;

	if(msg->opt.len > 0) {
		//ESP_LOGI(TAG_ADMIN_MESH_MSGS, "Longitud del valor de opt: %d", msg->opt.len);
		memcpy(opts->val, (uint8_t *) msg->opt.val, msg->opt.len);
	} else {
		ESP_LOGI(TAG_ADMIN_MESH_MSGS,"Valor de OPT vacia no se rellena ");
	}

	free(msg->payload.data);
	free(msg->opt.val);
	free(msg);

    return ESP_OK;
    
}

int get_msgs_pending() {
	return get_num_msgs();
}