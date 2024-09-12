/* Mesh Network Administration and Testing Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"

#include "mesh_admin_cmd.h"

/*******************************************************
 *                Macros
 *******************************************************/

/*******************************************************
 *                Constants
 *******************************************************/
#define RX_SIZE          (1500)
#define TX_SIZE          (1460)
#define OPT_RX_SIZE      (50)
#define CMD_BUFF_SIZE  (sizeof(sluggish_t) > sizeof(noise_t) ? sizeof(sluggish_t) : sizeof(noise_t))

#define CMD_HOLA_MUNDO 0x01



/*******************************************************
 *                Variable Definitions
 *******************************************************/
static const char *MESH_TAG = "mesh_main";
static const char *MESH_SEND = "mesh_send";
static const char *MESH_RX = "mesh_rx";

static const char * MSG = "Hola Mundo"; 
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};

static const mesh_addr_t BROADCAST_ADDR = {
    .addr = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};


static esp_ip4_addr_t my_current_ip;
static mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
static int route_table_size = 0;

static uint8_t tx_buf[TX_SIZE] = { 0, };
static uint8_t rx_buf[RX_SIZE] = { 0, };
static uint8_t cmd_buf[CMD_BUFF_SIZE] = { 0, };

static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
static esp_netif_t *netif_sta = NULL;

static TaskHandle_t xRxHandle = NULL;
static TaskHandle_t xTxHandle = NULL;



typedef struct {
    uint8_t cmd;
    uint8_t id_pkg;
    char * msg_txt;
} msg_t;


static sluggish_t cmd_sluggish;
static noise_t cmd_noise;
static mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];



/*******************************************************
 *                Function Declarations
 *******************************************************/

/*******************************************************
 *                Function Definitions
 *******************************************************/


void get_command(cmd_t * cmd) {
    cmd->id_cmd = (rand() % 3);
    switch (cmd->id_cmd) {
        case MESH_TEST_NODE_RESTART_EVENT: {
            cmd->len_params = 0;
            memset(cmd->params, 0, sizeof(cmd->params));
        }
        break;
        case MESH_TEST_NODE_SLUGGISH_EVENT: {
            cmd_sluggish.laziness_time_milis = (rand()%1000);
            cmd_sluggish.total_time_sg = (rand()%20);

            cmd->len_params = sizeof(sluggish_t);
            //cmd->params = cmd_buf;
            memcpy(cmd->params, (uint8_t *)&cmd_sluggish, sizeof(sluggish_t));

        }
        break;
        case MESH_TEST_NODE_NOISE_EVENT: {

            cmd_noise.msg2second = (rand()%10);
            cmd_noise.to_addr = route_table[(rand() % route_table_size)];
            cmd_noise.total_time_sg = (rand()%20);

            cmd->len_params = sizeof(noise_t);
            //cmd->params = cmd_buf;
            memcpy(cmd->params, (uint8_t *)&cmd_noise, sizeof(noise_t));
        }
        break;
    }
}


void esp_mesh_p2p_tx_main(void *arg) {
    esp_err_t err;
    int send_count = 0;
    mesh_rx_pending_t msg_pending;


    is_running = true;

    msg_t msg_to_send = {
        .cmd = CMD_HOLA_MUNDO,
        .msg_txt = MSG
    };

    mesh_data_t data;
    data.proto = MESH_PROTO_BIN;
    data.tos = MESH_TOS_P2P;
    data.data = tx_buf;
    data.size = sizeof(tx_buf);

    mesh_opt_t opts = {
        .len  = sizeof(uint8_t),
        .val  = malloc(sizeof(uint8_t)),
        .type = MESH_OPT_RECV_DS_ADDR,
    };
    opts.val[0] = (uint8_t) OPT_MSG_CMD;

    cmd_t command;
    bool is_cmd;

    while (is_running) {


        if(esp_mesh_is_root()) {
            esp_mesh_get_routing_table((mesh_addr_t *) &route_table, CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);

            is_cmd = (rand() % 2) < 1;

/*            
           for (int i = 0; i < route_table_size; i++) {

                if (route_table[i].mip.ip4.addr ==  my_current_ip.addr) {
                    // 0 == memcmp(route_table[i].mip.ip4, my_current_ip, sizeof(uint32_t))
                    ESP_LOGI(MESH_TAG, "Mi IP es la misma que la del envío. Envio cancelado"); 
                    continue;
                }

                ESP_LOGI(MESH_TAG, "Enviando paquete a " MACSTR, MAC2STR(route_table[i].addr));
*/
            if(is_cmd) {

                get_command(&command);
                command.id_pkg = send_count;
                
                memcpy(tx_buf, (uint8_t *)&command, sizeof(command));
                data.data = tx_buf;
                data.size = sizeof(tx_buf);

                ESP_LOGI(MESH_SEND, " ROOT ENVIANDO COMANDO %d con paquete %d", command.id_cmd, command.id_pkg);
                //err = esp_mesh_send(&route_table[i], &data, MESH_DATA_P2P, opts, 1);  //MESH_DATA_FROMDS | MESH_DATA_P2P Probar la diferencia 
                err = esp_mesh_send(&BROADCAST_ADDR, &data, MESH_DATA_P2P, &opts, 1);
            } else {

                msg_to_send.id_pkg = send_count;
                memcpy(tx_buf, (uint8_t *)&msg_to_send, sizeof(msg_to_send));
                ESP_LOGI(MESH_SEND, "ROOT ENVIANDO MSG DE APLIACION con paquete %d", msg_to_send.id_pkg);

                data.data = tx_buf;
                data.size = sizeof(tx_buf);
                //err = esp_mesh_send(&route_table[i], &data, MESH_DATA_P2P, NULL, 0); 
                err = esp_mesh_send(&BROADCAST_ADDR, &data, MESH_DATA_P2P, NULL, 0); 
            }

            esp_mesh_get_rx_pending(&msg_pending); 
            ESP_LOGI(MESH_RX, "BUFFER: datos internos: %d, externos: %d", msg_pending.toSelf, msg_pending.toDS);

            if (err) {
                ESP_LOGE(MESH_SEND, "--- ENVIO ERRONEO --- err:0x%x", err);
                /*ESP_LOGE(MESH_SEND,
                         "--- ENVIO ERRONEO --- [ROOT-2-UNICAST:%d][L:%d]parent:"MACSTR" to "MACSTR", heap:%d[err:0x%x, proto:%d, tos:%d]",
                         send_count, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                         MAC2STR(BROADCAST_ADDR.addr), esp_get_minimum_free_heap_size(),
                         err, data.proto, data.tos);*/
            } //else {
                //ESP_LOGI(MESH_SEND, "--- ROOT ENVIANDO ---> %s: %d", MSG, send_count); 
                /*ESP_LOGI(MESH_SEND,
                         "--- ROOT ENVIANDO --- [ROOT-2-UNICAST:%d][L:%d][rtableSize:%d]parent:"MACSTR" to "MACSTR", heap:%d[err:0x%x, proto:%d, tos:%d] data: %s",
                         send_count, mesh_layer,
                         esp_mesh_get_routing_table_size(),
                         MAC2STR(mesh_parent_addr.addr),
                         MAC2STR(BROADCAST_ADDR.addr), esp_get_minimum_free_heap_size(),
                         err, data.proto, data.tos, MSG);*/
            //}

            
            send_count++;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        /* if route_table_size is less than 10, add delay to avoid watchdog in this task. */
        if (route_table_size < 10) { }
    }
    vTaskDelete(NULL);
}


void esp_mesh_p2p_rx_main(void *arg) {
    esp_err_t err;
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;
    is_running = true;
    //mesh_rx_pending_t msg_pending;
    msg_t * msg_rx;
    mesh_opt_t opt;
    uint8_t rx_opt_buf = 0;
    opt.val = &rx_opt_buf;
    opt.len = sizeof(uint8_t);


    
    ESP_LOGI(MESH_RX,"********** VOY A INICIAR EL BUFFER **********");
    init_msgs_buffer();

    while (is_running) {
        //esp_mesh_get_rx_pending(&msg_pending); 
        //ESP_LOGI(MESH_RX, "BUFFER: datos internos: %d, externos: %d", msg_pending.toSelf, msg_pending.toDS);
        /* if(msg_pending.toSelf > 0 || msg_pending.toDS > 0) {

            if(msg_pending.toSelf < 10 && msg_pending.toDS < 10) {
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            } else {
                vTaskDelay(500 / portTICK_PERIOD_MS);
            } */
            vTaskDelay(10000 / portTICK_PERIOD_MS);


            data.data = rx_buf;
            data.size = RX_SIZE;

            rx_opt_buf = 0;

            err = read_msg(&from, &data, &flag, &opt);
            if (err != ESP_OK || !data.size) {
                ESP_LOGE(MESH_RX, "ERROR: 0x%x, size:%d", err, data.size);
                continue;
            } else {
                msg_rx = (msg_t *) data.data;
                ESP_LOGI(MESH_RX, "LEIDO MENSAJE APLIACION [Paquete %d] con msg:  %s", msg_rx->id_pkg, msg_rx->msg_txt);
            }
            
        //}
        //vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void mesh_test_cmd_handler(void *arg, esp_event_base_t event_base, 
                            int32_t event_id, void *event_data) {

    sluggish_t * cmd_rcv_sluggish;
    noise_t * cmd_rcv_noise;

    switch (event_id) {
        case MESH_TEST_NODE_RESTART_EVENT: {
            ESP_LOGI(MESH_RX, "+++ COMANDO DE REINICIO RECIBIDO +++ ");
        }
        break;
        case MESH_TEST_NODE_SLUGGISH_EVENT: {

            cmd_rcv_sluggish = (sluggish_t *) event_data;
            ESP_LOGI(MESH_RX, "+++ COMANDO DE INACTIVIDAD RECIBIDO: estaré %d milisegundos inactivo durante %d segundos +++ ", cmd_rcv_sluggish->laziness_time_milis, cmd_rcv_sluggish->total_time_sg);
        }
        break;
        case MESH_TEST_NODE_NOISE_EVENT: {
            cmd_rcv_noise = (noise_t *) event_data;
            ESP_LOGI(MESH_RX, "+++ COMANDO DE RUIDO RECIBIDO: enviaré mensajes cada %d milisegundos durante %d segundos a "MACSTR"", cmd_rcv_noise->msg2second, cmd_rcv_noise->total_time_sg, MAC2STR(cmd_rcv_noise->to_addr.addr));
        }
        break; 
    }

}

esp_err_t esp_mesh_comm_p2p_start() {
    static bool is_comm_p2p_started = false;
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;
        if(esp_mesh_is_root()) xTaskCreate(esp_mesh_p2p_tx_main, "MPTX", 3072, NULL, 5, NULL);
        xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
    }
    return ESP_OK;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
    mesh_addr_t id = {0,};
    static uint16_t last_layer = 0;

    switch (event_id) {
        case MESH_EVENT_STARTED: {
            esp_mesh_get_id(&id);
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
            is_mesh_connected = false;
            mesh_layer = esp_mesh_get_layer();
        }
        break;
        case MESH_EVENT_STOPPED: {
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
            is_mesh_connected = false;
            mesh_layer = esp_mesh_get_layer();
        }
        break;
        case MESH_EVENT_CHILD_CONNECTED: {
            mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                     child_connected->aid,
                     MAC2STR(child_connected->mac));
        }
        break;
        case MESH_EVENT_CHILD_DISCONNECTED: {
            mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                     child_disconnected->aid,
                     MAC2STR(child_disconnected->mac));
        }
        break;
        case MESH_EVENT_ROUTING_TABLE_ADD: {
            mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
            ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new, mesh_layer);
        }
        break;
        case MESH_EVENT_ROUTING_TABLE_REMOVE: {
            mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
            ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new, mesh_layer);
        }
        break;
        case MESH_EVENT_NO_PARENT_FOUND: {
            mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                     no_parent->scan_times);
        }
        /* TODO handler for the failure */
        break;
        case MESH_EVENT_PARENT_CONNECTED: {
            mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
            esp_mesh_get_id(&id);
            mesh_layer = connected->self_layer;
            memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
            ESP_LOGI(MESH_TAG,
                     "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR", duty:%d",
                     last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                     esp_mesh_is_root() ? "<ROOT>" :
                     (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr), connected->duty);
            last_layer = mesh_layer;
            is_mesh_connected = true;
            esp_mesh_comm_p2p_start();
            if (esp_mesh_is_root()) {
                esp_netif_dhcpc_stop(netif_sta);
                esp_netif_dhcpc_start(netif_sta);

                /*
                if(xRxHandle != NULL) {
                    vTaskDelete(xRxHandle);
                } */
            } else {
                if(xTxHandle != NULL) {
                    vTaskDelete(xTxHandle);
                } 
                /*if(xRxHandle == NULL) {
                    xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, &xRxHandle);
                } */
            }
            
        }
        break;
        case MESH_EVENT_PARENT_DISCONNECTED: {
            mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
            ESP_LOGI(MESH_TAG,
                     "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                     disconnected->reason);
            is_mesh_connected = false;
            mesh_layer = esp_mesh_get_layer();
        }
        break;
        case MESH_EVENT_LAYER_CHANGE: {
            mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
            mesh_layer = layer_change->new_layer;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                     last_layer, mesh_layer,
                     esp_mesh_is_root() ? "<ROOT>" :
                     (mesh_layer == 2) ? "<layer2>" : "");
            last_layer = mesh_layer;
        }
        break;
        case MESH_EVENT_ROOT_ADDRESS: {
            mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                     MAC2STR(root_addr->addr));
        }
        break;
        case MESH_EVENT_VOTE_STARTED: {
            mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
            ESP_LOGI(MESH_TAG,
                     "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                     vote_started->attempts,
                     vote_started->reason,
                     MAC2STR(vote_started->rc_addr.addr));
        }
        break;
        case MESH_EVENT_VOTE_STOPPED: {
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
            break;
        }
        case MESH_EVENT_ROOT_SWITCH_REQ: {
            mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
            ESP_LOGI(MESH_TAG,
                     "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                     switch_req->reason,
                     MAC2STR( switch_req->rc_addr.addr));
        }
        break;
        case MESH_EVENT_ROOT_SWITCH_ACK: {
            /* new root */
            mesh_layer = esp_mesh_get_layer();
            esp_mesh_get_parent_bssid(&mesh_parent_addr);
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
        }
        break;
        case MESH_EVENT_TODS_STATE: {
            mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
        }
        break;
        case MESH_EVENT_ROOT_FIXED: {
            mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                     root_fixed->is_fixed ? "fixed" : "not fixed");
        }
        break;
        case MESH_EVENT_ROOT_ASKED_YIELD: {
            mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
            ESP_LOGI(MESH_TAG,
                     "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                     MAC2STR(root_conflict->addr),
                     root_conflict->rssi,
                     root_conflict->capacity);
        }
        break;
        case MESH_EVENT_CHANNEL_SWITCH: {
            mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
        }
        break;
        case MESH_EVENT_SCAN_DONE: {
            mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                     scan_done->number);
        }
        break;
        case MESH_EVENT_NETWORK_STATE: {
            mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                     network_state->is_rootless);
        }
        break;
        case MESH_EVENT_STOP_RECONNECTION: {
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
        }
        break;
        case MESH_EVENT_FIND_NETWORK: {
            mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                     find_network->channel, MAC2STR(find_network->router_bssid));
        }
        break;
        case MESH_EVENT_ROUTER_SWITCH: {
            mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                     router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
        }
        break;
        case MESH_EVENT_PS_PARENT_DUTY: {
            mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
        }
        break;
        case MESH_EVENT_PS_CHILD_DUTY: {
            mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
            ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, "MACSTR", duty:%d", ps_duty->child_connected.aid-1,
                    MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
        }
        break;
        default: {
            ESP_LOGI(MESH_TAG, "unknown id:%d", event_id);
            break;
        }
    }
}

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
    my_current_ip.addr = event->ip_info.ip.addr;

}

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    /*  tcpip initialization */
    ESP_ERROR_CHECK(esp_netif_init());
    /*  event initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /*  create network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));
    /*  wifi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());
    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));


    // Registro event handler de comandos
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_TEST_EVENTS, ESP_EVENT_ANY_ID, &mesh_test_cmd_handler, NULL));

    /*  set mesh topology */
    ESP_ERROR_CHECK(esp_mesh_set_topology(CONFIG_MESH_TOPOLOGY));
    /*  set mesh max layer according to the topology */
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));
#ifdef CONFIG_MESH_ENABLE_PS
    /* Enable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_enable_ps());
    /* better to increase the associate expired time, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(60));
    /* better to increase the announce interval to avoid too much management traffic, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_announce_interval(600, 3300));
#else
    /* Disable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
#endif
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    /* router */
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(CONFIG_MESH_ROUTER_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, CONFIG_MESH_ROUTER_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, CONFIG_MESH_ROUTER_PASSWD,
           strlen(CONFIG_MESH_ROUTER_PASSWD));
    /* mesh softAP */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
           strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
#ifdef CONFIG_MESH_ENABLE_PS
    /* set the device active duty cycle. (default:10, MESH_PS_DEVICE_DUTY_REQUEST) */
    ESP_ERROR_CHECK(esp_mesh_set_active_duty_cycle(CONFIG_MESH_PS_DEV_DUTY, CONFIG_MESH_PS_DEV_DUTY_TYPE));
    /* set the network active duty cycle. (default:10, -1, MESH_PS_NETWORK_DUTY_APPLIED_ENTIRE) */
    ESP_ERROR_CHECK(esp_mesh_set_network_duty_cycle(CONFIG_MESH_PS_NWK_DUTY, CONFIG_MESH_PS_NWK_DUTY_DURATION, CONFIG_MESH_PS_NWK_DUTY_RULE));
#endif
    ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s<%d>%s, ps:%d\n",  esp_get_minimum_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
             esp_mesh_get_topology(), esp_mesh_get_topology() ? "(chain)":"(tree)", esp_mesh_is_ps_enabled());
}
