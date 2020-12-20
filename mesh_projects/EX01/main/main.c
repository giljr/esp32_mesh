/* main.c
   
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

   Directories:
   Base: C:\msys32\home\USER_PROFILE\esp\esp-idf\examples\mesh_projects
   EX01
     build
     
     main:
       app.c
       app.h
       component.mk
       libs:
       lib_heltec.h
       main.c
       mesh.c
       mesh.h
       
     Makefile
     sys_config.h
     sdkconfig

   
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

/**
 * C library
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/**
 * WiFi
 */
#include "esp_wifi.h"

/**
 * Logs
 */
#include "esp_log.h"

/**
 * Callback
 */
#include "esp_event_loop.h"

/**
 * Drivers
 */
#include "nvs_flash.h"

/**
 * Aplications (App);
 */
#include "app.h"

/**
 * Mesh APP
 */
 #include "mesh.h"

/**
 * PINOUT; 
 */
#include "sys_config.h"

/**
 * Constants;
 */
static const char *TAG = "main: ";

/**
 * Prototypes Functions;
 */
void app_main( void );


void init_oled( void );


/**
 * Program begins here:)
 */


void app_main( void )
{
     
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /**
     * Inicializa GPIOs;
     */
	gpios_setup();

    /**
     * Inicializa o stack mesh;
     */
	mesh_app_start();

    /**
     * Initialize OLED Print Services at app.c;
     */
    init_oled(); 

}
