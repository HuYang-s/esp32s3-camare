#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <stddef.h>

void time_service_init(void);
void time_service_get_beijing_time_string(char* time_str, size_t max_len);
bool time_service_is_time_synchronized(void);

#endif // TIME_SERVICE_H
