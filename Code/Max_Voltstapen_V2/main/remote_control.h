#ifndef WIFI_HTTP_SERVER_H
#define WIFI_HTTP_SERVER_H

void init_wifi(void);
void start_server(void);

float get_kp(void);
float get_kd(void);
void set_kp(float new_kp);
void set_kd(float new_kd);

#endif // WIFI_HTTP_SERVER_H
