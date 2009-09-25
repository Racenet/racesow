// defines
#define RACESOW_MYSQL
//#define RACESOW_RACENET

// includes
#ifdef RACESOW_MYSQL
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#endif

#ifdef RACESOW_RACENET
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

// cvars
#ifdef RACESOW_MYSQL
extern cvar_t *g_mysql_host;
extern cvar_t *g_mysql_port;
extern cvar_t *g_mysql_user;
extern cvar_t *g_mysql_pass;
extern cvar_t *g_mysql_db;
#endif
