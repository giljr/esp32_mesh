#ifndef __APPS_H__
#define __APPS_H__

void gpios_setup( void );
void task_root_button( void *pvParameter );
void task_mesh_rx ( void *pvParameter );
void task_app_create( void );

#endif