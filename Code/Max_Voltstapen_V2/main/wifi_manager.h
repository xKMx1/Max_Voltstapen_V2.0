#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

void wifi_init_softap(void);
void wifi_connect_sta(const char* ssid, const char* password);

#endif
