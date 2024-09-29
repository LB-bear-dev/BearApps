#pragma once

#define debug_(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info_(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn_(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define error_(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)