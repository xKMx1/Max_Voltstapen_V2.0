// wifi_manager.h
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

void wifi_init_sta(void);
void wifi_init_softap(void);
void wifi_connect_sta(const char* ssid, const char* password);
bool is_wifi_connected(void);

#endif