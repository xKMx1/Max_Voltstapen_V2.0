#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

void init_wifi(void);
void start_web_server(void);
float get_kp(void);
float get_kd(void);
void set_kp(float kp);
void set_kd(float kd);

#endif // REMOTE_CONTROL_H
