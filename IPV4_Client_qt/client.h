#ifndef CLIENT_H
#define CLIENT_H
#define DEFAULT_PALYERCMD "/usr/bin/mpg123 - > /dev/null"

#include <stdint.h>

typedef struct client_conf_t
{
    char *mgroup;
    char *revport;
    char *playercmd;
} client_conf_t;

#endif // CLIENT_H
