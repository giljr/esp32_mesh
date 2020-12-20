/* app.c
   
   Project 00 - Wireless Mesh Networking w/ ESP32
   
   Simple Mesh ESP32 Mesh Net
   
   Description 
   This program automatically chooses what node becomes root.
   TO MAKE IT WORKS, We include the OLED library and set up all the links at 
   Directory C:\msys32\home\USER_PROFILE\esp\esp-idf\components;
   To test disconnect one of the root and in approximately 30 seconds 
   a new root will be chosen. Return the node and you will see it associate 
   with the new root. Nice & cool experiment!
   Test with at least three ESP32 boards. 
   Enjoy / 

   Commands:
   $ make menuconfig         -> Set your project port, location, etc
   $ make erase_flash        -> To erase the flash; take the final two digit MAC Address reported here
   $ make flash -j5 monitor  -> Flash the ESP32 and run the Serial Monitor at the end :)
 
   To Get started at Flashing ESP-32 Tutorial:
   https://medium.com/jungletronics/esp-idf-programming-guide-wifi-lora-32-v2-53f89e12c96e

   ESP-MESH is a networking protocol built atop the Wi-Fi protocol.
   
   Features:
   Easy and Secure Setup;
   Self-forming and Self-healing;
   No Extra Gateways Required;
   IP Connectivity;
   Secure by Design;
   
   Applications:
   Smart Lighting: smart lights, lighting network;
   Smart Home: smart switches, sockets, plugs, etc;
   Automation: big parking lots, small factories, shared offices;
   
   Official Expressif & Heltec guide:
   ESP32 Mesh: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/mesh.html
   API Mesh ESP32 REF: https://lang-ship.com/reference/ESP-IDF/latest/esp__mesh_8h_source.html
   Quick Start: https://heltec-automation-docs.readthedocs.io/en/latest/esp32+arduino/quick_start.html

   Credits
 *  --------------------------------------------------------------------------
   Author: Prof° Fernando Simplicio;
   Hardware: WiFi LoRa 32 (v2) 
   A classic IoT dev-board designed & produced by Heltec Automation(TM)
   Espressif SDK-IDF: v3.3
   Lectures: IoT Formation Online Course Using ESP32
   Link: https://www.microgenios.com.br/formacao-iot-esp32/
 *  --------------------------------------------------------------------------
   
   Edited by j3
   date: Dez, 17/2020
*/
//
/**
 * Lib C
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/**
 * ESP hall;
 */

#include "esp_wifi.h"
#include "esp_system.h"

/**
 * Callback do WiFi e MQTT;
 */
#include "esp_event_loop.h"

/**
 * Logs;
 */
#include "esp_log.h"

/**
 * Mesh Net;
 */
#include "esp_mesh.h"
#include "esp_mesh_internal.h"

/**
 * Lwip
 */
#include "lwip/err.h"
#include "lwip/sys.h"
#include <lwip/sockets.h>

/**
 * Drivers
 */
#include "nvs_flash.h"
#include "driver/gpio.h"

/**
 * PINOUT; 
 */
#include "sys_config.h"

/**
 * App;
 */
#include "app.h"

/**
 * Overloads sdkconfig file;
 * What Crypto algorithm to use in Mesh Network?
 */
#define CONFIG_MESH_IE_CRYPTO_FUNCS  1
#define CONFIG_MESH_IE_CRYPTO_KEY    "chave de criptografia"

/**
 * Global defs
 */
char mac_address_root_str[50];
mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];

static const char *TAG = "mesh";
static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77 };

static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;

EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

/**
 * Function prototypes
 */
static void esp_mesh_rx_start( void );
void mesh_event_handler( mesh_event_t event );
void mesh_app_start( void );

/**
 * Calls reception thread to change messages
 */
static void esp_mesh_rx_start( void )
{
    static bool is_esp_mesh_rx_started = false;
    if( !is_esp_mesh_rx_started )
    {
        is_esp_mesh_rx_started = true;
        task_app_create();
    }
}

/**
 * Callback function
 */
void mesh_event_handler( mesh_event_t event )
{
    mesh_addr_t id = {0,};
    static uint8_t last_layer = 0;
    ESP_LOGD(TAG, "esp_event_handler:%d", event.id);

    switch (event.id) {
    case MESH_EVENT_STARTED:
        esp_mesh_get_id(&id);
        ESP_LOGI(TAG, "<MESH_EVENT_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_STOPPED:
        ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;
    case MESH_EVENT_CHILD_CONNECTED:
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 event.info.child_connected.aid,
                 MAC2STR(event.info.child_connected.mac));
        break;
    case MESH_EVENT_CHILD_DISCONNECTED:
        ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 event.info.child_disconnected.aid,
                 MAC2STR(event.info.child_disconnected.mac));
        break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
        ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;
    case MESH_EVENT_NO_PARENT_FOUND:
        ESP_LOGI(TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 event.info.no_parent.scan_times);
        /* TODO handler for the failure */
        break;
    case MESH_EVENT_PARENT_CONNECTED:
        esp_mesh_get_id(&id);
        mesh_layer = event.info.connected.self_layer;
        memcpy(&mesh_parent_addr.addr, event.info.connected.connected.bssid, 6);
        ESP_LOGI(TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        is_mesh_connected = true;
        if (esp_mesh_is_root()) 
        {
            /**
             * FIXED IP?
             */
            #if !FIXED_IP
                tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
            #endif
        }

        /**
         * Initialize the message reception thread 
         */
        esp_mesh_rx_start();

        break;
    /**
     * Parent desconnection event
     */
    case MESH_EVENT_PARENT_DISCONNECTED:
        ESP_LOGI(TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 event.info.disconnected.reason);
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
        break;

    /**
     * Layer change event
     */
    case MESH_EVENT_LAYER_CHANGE:
        mesh_layer = event.info.layer_change.new_layer;
        ESP_LOGI(TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
        break;

    /**
     * Root address event
     */
    case MESH_EVENT_ROOT_ADDRESS:
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(event.info.root_addr.addr));

        /**
         * Storage ROOT Address event
         */
        if(esp_mesh_is_root()) 
        {
            uint8_t chipid[20];
            esp_efuse_mac_get_default( chipid );
            snprintf( mac_address_root_str, sizeof( mac_address_root_str ), ""MACSTR"", MAC2STR( chipid ) );
        }
        break;
    /**
     * Root got ip event
     */
    case MESH_EVENT_ROOT_GOT_IP:
        /* root starts to connect to server */
        ESP_LOGI(TAG,
                 "<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 IP2STR(&event.info.got_ip.ip_info.ip),
                 IP2STR(&event.info.got_ip.ip_info.netmask),
                 IP2STR(&event.info.got_ip.ip_info.gw));
        break;
    /**
     * ROOT lost IP event
     */
    case MESH_EVENT_ROOT_LOST_IP:
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_LOST_IP>");
        break;
    /**
     * Init vote routine event
     */
    case MESH_EVENT_VOTE_STARTED:
        ESP_LOGI(TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 event.info.vote_started.attempts,
                 event.info.vote_started.reason,
                 MAC2STR(event.info.vote_started.rc_addr.addr));
        break;
    case MESH_EVENT_VOTE_STOPPED:
        ESP_LOGI(TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    /**
     * Software forced request for root exchange 
     */
    case MESH_EVENT_ROOT_SWITCH_REQ:
        ESP_LOGI(TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 event.info.switch_req.reason,
                 MAC2STR( event.info.switch_req.rc_addr.addr));
        break;
    /**
     * Callback acknowledgemnts
     */
    case MESH_EVENT_ROOT_SWITCH_ACK:
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
        break;
    /**
     * Messages sent by root can be addressed to an external IP.
     * When we use this mesh stack feature, this event will be used
     * in notification of states (toDS - for DS (distribute system))
     */
    case MESH_EVENT_TODS_STATE:
        ESP_LOGI(TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d",
                 event.info.toDS_state);
        break;
    /**
     * MESH_EVENT_ROOT_FIXED forces the child device to maintain the same settings as the
     * parent device on the mesh network; 
     */
    case MESH_EVENT_ROOT_FIXED:
        ESP_LOGI(TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 event.info.root_fixed.is_fixed ? "fixed" : "not fixed");
        break;
    /**
     * Event called when there is another and best possible candidate to be root of the network; 
     * The current root passes control to the new root to take over the network;
     */
    case MESH_EVENT_ROOT_ASKED_YIELD:
        ESP_LOGI(TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(event.info.root_conflict.addr),
                 event.info.root_conflict.rssi,
                 event.info.root_conflict.capacity);
        break;
    /**
     * Channel switch event
     */
    case MESH_EVENT_CHANNEL_SWITCH:
        ESP_LOGI(TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", event.info.channel_switch.channel);
        break;

    /**
     * Scan is done event
     */
    case MESH_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 event.info.scan_done.number);
        break;
    /**
     * Mesh state event
     */
    case MESH_EVENT_NETWORK_STATE:
        ESP_LOGI(TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 event.info.network_state.is_rootless);
        break;
    /**
     * Event called when the root device stops connecting to the router and
     * the child device stops connecting to the parent device;
     */
    case MESH_EVENT_STOP_RECONNECTION:
        ESP_LOGI(TAG, "<MESH_EVENT_STOP_RECONNECTION>");
        break;
    /**
     * Event called when the device encounters a mesh network to be paired
     */
    case MESH_EVENT_FIND_NETWORK:
        ESP_LOGI(TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 event.info.find_network.channel, MAC2STR(event.info.find_network.router_bssid));
        break;
    /**
     * Event called when the device finds and exchanges for another router 
     * (linksys, dlink ...) with the same SSID;
     */
    case MESH_EVENT_ROUTER_SWITCH:
        ESP_LOGI(TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 event.info.router_switch.ssid, event.info.router_switch.channel, MAC2STR(event.info.router_switch.bssid));
        break;
    default:
        ESP_LOGI(TAG, "unknown id:%d", event.id);
        break;
    }
}


/**
 * Mesh stack init
 */
void mesh_app_start( void )
{
    /*  tcpip stack init */
    tcpip_adapter_init();

    
    /* for mesh
     * stop DHCP server on softAP interface by default
     * stop DHCP client on station interface by default
     * */
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));

#if FIXED_IP
    /**
     * The ESP32 ROOT of the Mesh network is one that receives the IP address of the Router;
     * Do you want to work with the fixed IP address on the network? 
     * That is, you want to configure; ROOT with Static IP?
     */
    tcpip_adapter_ip_info_t sta_ip;
    sta_ip.ip.addr = ipaddr_addr( IP_ADDRESS );
    sta_ip.gw.addr = ipaddr_addr( GATEWAY_ADDRESS );
    sta_ip.netmask.addr = ipaddr_addr( NETMASK_ADDRESS );
    tcpip_adapter_set_ip_info(WIFI_IF_STA, &sta_ip);
#endif

    /**
     * WiFi Init
     */
    ESP_ERROR_CHECK( esp_event_loop_init( NULL, NULL ) );
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &config ) );
    ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_FLASH ) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    /**
     * Mesh init
     */
    ESP_ERROR_CHECK( esp_mesh_init() );
    ESP_ERROR_CHECK( esp_mesh_set_max_layer( CONFIG_MESH_MAX_LAYER ));
    ESP_ERROR_CHECK( esp_mesh_set_vote_percentage(1) );
    ESP_ERROR_CHECK( esp_mesh_set_ap_assoc_expire(10) );

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    
    /**
     * Register the Mesh network ID. All non-root who wish to participate 
     * in this mesh network must have the same ID and the login and password  
     * to access the network (informed further down in the code);
     */
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);
    
    /**
     * Registers the callback function of the Mesh network;
     * The callback function is responsible for signaling to you, the user
     * the states of the internal operations of the Mesh network;
     */
    cfg.event_cb = &mesh_event_handler;
    
    /**
     * Define channel frequency
     */
    cfg.channel = CONFIG_MESH_CHANNEL;

    /**
     * Defines the ssid and password that will be used for communication between nodes
     * Mesh network; This SSID and PASSWORD is that of YOUR ROUTER FROM YOUR HOME or COMPANY;
     */
    cfg.router.ssid_len = strlen(WIFI_SSID);
    memcpy((uint8_t *) &cfg.router.ssid, WIFI_SSID, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, WIFI_PASSWORD, strlen(WIFI_PASSWORD));

    /**
     * The Mesh network requires authentication and will be configured as an access point;
     */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));

    /**
     * Defines the maximum number of non-root (node) in each node of the network;
     * If 'max_connection' is equal to 1, then only one node per layer will be allowed;
     * Example: 3x ESP32 would be: A (root) -> B (non-root) -> C (non-root); So there would be
     * 3 layers the Mesh network;
     */
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;

    /**
     * SSID and Password for network access BETWEEN Mesh network nodes;
     * This SSID and PASSWORD is used only by devices on the network;
     */
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD, strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));

    /**
     * Mesh start;
     */
    ESP_ERROR_CHECK(esp_mesh_start());

}
