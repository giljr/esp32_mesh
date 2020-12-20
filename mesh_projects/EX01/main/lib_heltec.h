#ifndef __LIB_1306_FONTS__
#define __LIB_1306_FONTS__

#include "lib_ssd1306.h"

/**
 * Logs; 
 */
#include "esp_log.h"
#include "esp_system.h"

#define I2C_SDA_PIN CONFIG_I2C_SDA_PIN        //default 4
#define I2C_SCL_PIN CONFIG_I2C_SCL_PIN        //default 15
#define I2C_CHANNEL CONFIG_I2C_CHANNEL        //default 0 
#define OLED_PIN_RESET CONFIG_OLED_PIN_RESET  //default 16

/**
 * 
* To load an image on the oled display, use the GLCD Bitmap Editor tool from mikroC PRO for PIC compiler:
 * GOTO: https://www.mikroe.com/mikroc-pic
 * 1 ° Open the mikroC PRO for PIC compiler;
 * 2 ° Click on the Tools menu> GLCD Bitmap Editor;
 * 3 ° Check the option 128x64 (kS108) and mikroC PRO;
 * 4 ° Click the Load BMP button and load a monochrome BMP image;
 * 5 ° The BMP image will be converted into a table in C;
 * 6 ° After generating the table in C, click on the "Copy Code To Clipboard" button;
 * 7 ° Replace the table below with the current one.
 * 8 ° Adjust the table name and you're done.
 */

// ------------------------------------------------------  
// GLCD Picture name: J3_mini_logo.bmp            
// GLCD Model: KS0108 128x64            
// ------------------------------------------------------  

const char J3_mini_logo[282] = {
  0,   0,   0,   0,   0,   0, 128,   0, 160, 192,  48, 224, 144, 104, 232,  28, 164, 148,  37, 206, 124, 131,  70, 213, 124, 150, 238, 100, 152, 232, 216,  24, 208, 224,  32, 192,   0, 128,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
  0,   0, 160, 192, 248, 253, 254, 255, 254, 157, 234, 254, 249, 255, 252, 255, 254, 255, 255, 255, 255, 127,  31,  31,  31,  31,  31, 255, 254, 253, 252, 251, 247, 237, 222, 126, 255, 255, 253, 250, 240, 128,   0,   0,   0,   0,   0, 
144, 164, 255, 255, 255, 255, 255,  85, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 135,   0,   1,   0,   0,   0,  16, 224, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 234, 255, 255, 255, 255, 255, 252, 242, 224, 240, 240, 
  0,  22,  71, 127, 255, 255, 255, 254, 103,  63,  63,  63, 127, 127, 127, 127, 127, 255, 255, 252, 252, 124, 124, 126, 126, 126, 127,  63, 127, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  63,  15,   1, 
  0,   0,   0,   0,   5,   3,   7,  31,  88,  48, 112, 240, 240, 248, 240, 240, 248, 248, 232, 252, 236, 156, 188, 236, 124, 252, 252, 252, 254, 255, 255, 255, 255, 127, 127,  63,  63,  15,  15,   7,  15,   7,   3,   1,   0,   0,   0, 
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   1,   1,   5,   7,   3,   3,   7,  10,   6,  11,   4,  11,   3,   3,   3,   5,   3,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};



void ssd1306_start( void )
{
	/**
	 *  Template
	 */
	ssd1306_config( I2C_SDA_PIN, I2C_SCL_PIN , I2C_CHANNEL, OLED_PIN_RESET );
}
#endif