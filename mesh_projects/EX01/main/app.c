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
   $ make erase_flash        -> To erase the flash; before take the final two digit MAC Address reported here ( as NODE_NAME)
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
   Espressif SDK-IDF: v3.3.2
   Lectures: IoT Formation Online Course Using ESP32
   Link: https://www.microgenios.com.br/formacao-iot-esp32/
 *  --------------------------------------------------------------------------
   
   Edited by j3
   date: Dez, 17/2020
*/
   
/**
 * Lib C BASE
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
 * Drivers;
 */
#include "driver/gpio.h"

/**
 * GPIOs Config;
 */
#include "app.h"

/**
 * Standard configurations loaded
 */
#include "sys_config.h"

/**
 * Logs;
 */
#include "esp_log.h"

/**
 * Rede mesh;
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
* Lib Display SSD1306 Oled;
* Note: Include this file just once;
* Otherwise, you must include "lib_ss1306.h"
*/
#include "lib_heltec.h"

/**
 * Gloabal Variables; 
 */
extern EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_BIT;
extern mesh_addr_t route_table[];
extern char mac_address_root_str[];

/**
 * Constants;
 */
static const char *TAG = "app: ";

#define RX_SIZE          (100)
static uint8_t rx_buf[RX_SIZE] = { 0, };

#define TX_SIZE          (100)
static uint8_t tx_buf[TX_SIZE] = { 0, };

#define NODE_NAME "NODE_D4"

/**
 * Configure the ESP32 gpios (lLED & button );
 */
void gpios_setup( void )
{
    /**
     * Configure the GPIO LED BUILDING
     */
    gpio_config_t io_conf_output;
    io_conf_output.intr_type = GPIO_INTR_DISABLE;
    io_conf_output.mode = GPIO_MODE_OUTPUT;
    io_conf_output.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf_output.pull_down_en = 0;
    io_conf_output.pull_up_en = 1;
    gpio_config(&io_conf_output);

    /**
     * LED Off in the startup
     * Level 0 -> off 
     * Level 1 -> on 
     */
    gpio_set_level( LED_BUILDING, 0 ); 
   
    /**
     * Configure the GPIO BUTTON
    */
    gpio_config_t io_conf_input;
    io_conf_input.intr_type = GPIO_PIN_INTR_DISABLE; 
    io_conf_input.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf_input.mode = GPIO_MODE_INPUT;
    io_conf_input.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_input.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf_input);
}

void init_oled( void )
{
    /**
    * Initializes Oled display 128x64 SSD1306;
    * Oled pinout settings are found
    * in "lib_heltec.h";
    */
   ssd1306_start();


    /**
    * Print using font 8x16;
    * signature: ssd1306_out16( line, column, string , font_color );
    */
   
    ssd1306_out16( 0, 0, "ESP32 Mesh:Hi o/", WHITE );
     ssd1306_out16( 3, 4, NODE_NAME, WHITE );
      
}

/**
 * Button Manipulation Task
 */
void task_root_button( void *pvParameter )
{
    int counter = 0;
    
    char mac_str[30];
    int route_table_size = 0;
    
    esp_err_t err;

    mesh_data_t data;
    data.data = tx_buf;
    data.size = TX_SIZE;

    /**
     *  Defines the format of the message to be sent;
     * 'MESH_PROTO_BIN' is the standard binary format, but there are others:
     * 'MESH_PROTO_JSON'
     * 'MESH_PROTO_MQTT'
     * 'MESH_PROTO_HTTP'
     */
    data.proto = MESH_PROTO_BIN;

    for( ;; ) 
    {
        /**
         * If this device is the root, then create the socket server connection;
         */
        if( esp_mesh_is_root() )
        {
        
            ssd1306_out16( 6, 3, "ROOT NODE", WHITE );
            /**
             * Is it root node? Then turn on the led building;
             */
            gpio_set_level( LED_BUILDING, 1 );

            /**
             * The button was pressed?
             */
            if( gpio_get_level( BUTTON ) == 0 ) 
            {       
                if( DEBUG )
                    ESP_LOGI( TAG, "Button %d Pressed.\r\n", BUTTON );
                
                counter++;
                snprintf( (char*)tx_buf, TX_SIZE, "%d", (counter) ); 

                /**
                 * Calculating the size of the data type buffer
                 * pointed by esp_mesh_send() method
                 */
                data.size = strlen((char*)tx_buf) + 1;
                        
                /**
                * Updates the reading of mac addresses of devices on the mesh network
                 */
                esp_mesh_get_routing_table((mesh_addr_t *) &route_table,
                               CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);

                for( int i = 0; i < route_table_size; i++ ) 
                {
                    /**
                     * Convert address binary to string;
                     */
                    sprintf(mac_str, MACSTR, MAC2STR(route_table[i].addr));
                    
                    
                    /**
                      * Routing routine for sending the message quoted by the button. 
                      * Here the ROOT sends a message to all node but himself:)
                      */
                    if( strcmp( mac_address_root_str, mac_str) != 0 )
                    {   
                        /**
                         * Actual sending of datatype already loaded
                         */
                        err = esp_mesh_send(&route_table[i], &data, MESH_DATA_P2P, NULL, 0);
                        if (err) 
                        {
                            /**
                             * Error child node message
                             */
                            if( DEBUG )
                                ESP_LOGI( TAG, "ERROR : Sending Message!\r\n" ); 
                        } else {
                    
                            /**
                             * Mensagem Enviada com sucesso pelo ROOT;
                             */
                            if( DEBUG )
                                ESP_LOGI( TAG, "\r\nROOT sends (%s) (%s) to NON-ROOT (%s)\r\n", mac_address_root_str, tx_buf, mac_str );                         
                        }
                    }                    

                }                        

            }


            /**
             * Good practices by using FreeRTOs libs;)
             * For more info, Please visit: 
             * https://freertos.org/index.html 
             */
            vTaskDelay( 300/portTICK_PERIOD_MS );   
        } 

        /**
         * Up here, the device is NON-ROOT = CHILD NODE
         * For ESP32 Mesh Concepts, please, visit:
         * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/mesh.html#mesh-concepts
         */
         else
        {
            /**
             * Printing OLED 
            */
            ssd1306_out16( 6, 3, "CHILD NODE", WHITE );           
            
            /**
             * Good practices by using FreeRTOs libs;)
             * For more info, Please visit: 
             * https://freertos.org/index.html 
             */
            vTaskDelay( 100/portTICK_PERIOD_MS );

        }


    }
}




void task_mesh_rx ( void *pvParameter )
{
    esp_err_t err;
    mesh_addr_t from;

    mesh_data_t data;
    data.data = rx_buf;
    data.size = RX_SIZE;

    char mac_address_str[30];
    int flag = 0;
    
    static uint8_t buffer_s[10];
    mesh_data_t data_s;
    data_s.data = buffer_s;
    data_s.size = sizeof(buffer_s);    

    for( ;; )
    {
        data.size = RX_SIZE;

       /**
        * Waits for message reception
        */
        err = esp_mesh_recv( &from, &data, portMAX_DELAY, &flag, NULL, 0 );
        if( err != ESP_OK || !data.size ) 
        {
            if( DEBUG )
                ESP_LOGI( TAG, "err:0x%x, size:%d", err, data.size );
            continue;
        }

        /**
         * Is it routed for ROOT Node?
         */
        if( esp_mesh_is_root() ) 
        {
            /**
             * Which NON_ROOT node sends this message?
             */
            if( DEBUG ) {
                ESP_LOGI( TAG,"ROOT(MAC:%s) - Msg: %s, ", mac_address_root_str, data.data );
                /**
                 * Log message to console
                 */
                snprintf( mac_address_str, sizeof(mac_address_str), ""MACSTR"", MAC2STR(from.addr) );
                ESP_LOGI( TAG, "send by NON-ROOT: %s\r\n", mac_address_str );
            }

        } 

        /**
         * By pressing the button the message are broadcasted to all NON-ROOTS nodes. 
         */
        else 
        {
            /**
             * Finds out which MAC Address NON-ROOT node gets the message 
             */
            uint8_t mac_address[20];
            esp_efuse_mac_get_default( mac_address );
            snprintf( mac_address_str, sizeof( mac_address_str ), ""MACSTR"", MAC2STR( mac_address ) );

            if( DEBUG ) {
                ESP_LOGI( TAG, "NON-ROOT(MAC:%s)- Msg: %s, ", mac_address_str, data.data  );  
                /**
                 * Resgata e imprime no console o mac_address de quem enviou a mensagem;
                 */
                snprintf( mac_address_str, sizeof(mac_address_str), ""MACSTR"", MAC2STR(from.addr) );
                ESP_LOGI( TAG, "send by ROOT: %s\r\n", mac_address_str );
            }

            /**
             * Toggle the LED_BUILDING at each button increment
             */
            if( data.size > 0 )
            {
                gpio_set_level( LED_BUILDING, atoi((char*)data.data) % 2 );
            }
            /**
             * Feedback ROOT with OK acknowledgment message
             */
            snprintf( (char*)buffer_s, sizeof(buffer_s), "%s", "OK" );
            data_s.size = strlen( (char*) buffer_s ) + 1; 

            /**
             * Where: 
             *  NULL send message to the ROOT;
             *  data_s content the message itself;
             *  0 Flag that signalize the ROOT is the objective;
             *  NULL indicating broadcasting to NON-ROOT
             *  0 NOT IMPLEMENTED YET;
             */
            err = esp_mesh_send( NULL, &data_s, 0, NULL, 0 );
            if(err != ESP_OK ) 
            {
                if( DEBUG )
                    ESP_LOGI( TAG, "err:0x%x, size:%d", err, data_s.size );
            }  
                           
        }

    }

    vTaskDelete(NULL);
}



void task_app_create( void )
{
    if( DEBUG )
        ESP_LOGI( TAG, "task_app_create() called" );

    /**
     * Creates a Task to control the message routing;
     */
    if( xTaskCreate( task_mesh_rx, "task_mesh_rx", 1024 * 5, NULL, 2, NULL) != pdPASS )
    {
        if( DEBUG )
          ESP_LOGI( TAG, "ERROR - task_mesh_rx NOT ALLOCATED :/\r\n" );  
        return;   
    }

    /**
     *  Creates a Task to control the hardware button
     */
    if( xTaskCreate( task_root_button, "task_root_button", 1024 * 8, NULL, 1, NULL ) != pdPASS )
    {
        if( DEBUG )
          ESP_LOGI( TAG, "ERROR - task_root_button NOT ALLOCATED :/\r\n" );  
        return;   
    }     
}
