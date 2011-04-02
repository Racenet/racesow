#include <dlfcn.h>
void* libmysqlclient;
qboolean RS_LoadMySQL( void ); //Loads the dynamic library and calls LoadMySQLFunctions
qboolean LoadMySQLFunctions( void ); //Loads the functions from the dynamic mysql(client) library
void UnLoadMySQLFunctions( void ); //Sets the function pointers to NULL and undefines them "off" the normal mysql functions

//MySQL Function pointers
MYSQL* (*my_init_pointer)(MYSQL *mysql);
my_ulonglong (*mysql_affected_rows_pointer)(MYSQL *mysql);
my_bool (*mysql_autocommit_pointer)(MYSQL *mysql, my_bool mode);
my_bool (*mysql_change_user_pointer)(MYSQL *mysql, const char *user, const char *password, const char *db);
const char *(*mysql_character_set_name_pointer)(MYSQL *mysql);
void (*mysql_close_pointer)(MYSQL *mysql);
my_bool (*mysql_commit_pointer)(MYSQL *mysql);
MYSQL *(*mysql_connect_pointer)(MYSQL *mysql, const char *host, const char *user, const char *passwd);
int (*mysql_create_db_pointer)(MYSQL *mysql, const char *db);
void (*mysql_data_seek_pointer)(MYSQL_RES *result, my_ulonglong offset);
void (*mysql_debug_pointer)(const char *debug);
int (*mysql_drop_db_pointer)(MYSQL *mysql, const char *db);
int (*mysql_dump_debug_info_pointer)(MYSQL *mysql);
my_bool (*mysql_eof_pointer)(MYSQL_RES *result);
unsigned int (*mysql_errno_pointer)(MYSQL *mysql);
const char *(*mysql_error_pointer)(MYSQL *mysql);
MYSQL_FIELD *(*mysql_fetch_field_pointer)(MYSQL_RES *result);
MYSQL_FIELD *(*mysql_fetch_field_direct_pointer)(MYSQL_RES *result, unsigned int fieldnr);
MYSQL_FIELD *(*mysql_fetch_fields_pointer)(MYSQL_RES *result);
unsigned long *(*mysql_fetch_lengths_pointer)(MYSQL_RES *result);
MYSQL_ROW (*mysql_fetch_row_pointer)(MYSQL_RES *result);
unsigned int (*mysql_field_count_pointer)(MYSQL *mysql);
MYSQL_FIELD_OFFSET (*mysql_field_seek_pointer)(MYSQL_RES *result, MYSQL_FIELD_OFFSET offset);
MYSQL_FIELD_OFFSET (*mysql_field_tell_pointer)(MYSQL_RES *result);
void (*mysql_free_result_pointer)(MYSQL_RES *result);
void (*mysql_get_character_set_info_pointer)(MYSQL *mysql, MY_CHARSET_INFO *cs);
const char *(*mysql_get_client_info_pointer)(void);
unsigned long (*mysql_get_client_version_pointer)(void);
const char *(*mysql_get_host_info_pointer)(MYSQL *mysql);
unsigned int (*mysql_get_proto_info_pointer)(MYSQL *mysql);
const char *(*mysql_get_server_info_pointer)(MYSQL *mysql);
unsigned long (*mysql_get_server_version_pointer)(MYSQL *mysql);
const char *(*mysql_get_ssl_cipher_pointer)(MYSQL *mysql);
unsigned long (*mysql_hex_string_pointer)(char *to, const char *from, unsigned long length);
const char *(*mysql_info_pointer)(MYSQL *mysql);
MYSQL *(*mysql_init_pointer)(MYSQL *mysql);
my_ulonglong (*mysql_insert_id_pointer)(MYSQL *mysql);
int (*mysql_kill_pointer)(MYSQL *mysql, unsigned long pid);
void (*mysql_library_end_pointer)(void);
int (*mysql_library_init_pointer)(int argc, char **argv, char **groups);
MYSQL_RES *(*mysql_list_dbs_pointer)(MYSQL *mysql, const char *wild);
MYSQL_RES *(*mysql_list_fields_pointer)(MYSQL *mysql, const char *table, const char *wild);
MYSQL_RES *(*mysql_list_processes_pointer)(MYSQL *mysql);
MYSQL_RES *(*mysql_list_tables_pointer)(MYSQL *mysql, const char *wild);
my_bool (*mysql_more_results_pointer)(MYSQL *mysql);
int (*mysql_next_result_pointer)(MYSQL *mysql);
unsigned int (*mysql_num_fields_pointer)(MYSQL_RES *result);
my_ulonglong (*mysql_num_rows_pointer)(MYSQL_RES *result);
int (*mysql_options_pointer)(MYSQL *mysql, enum mysql_option option, const char *arg);
int (*mysql_ping_pointer)(MYSQL *mysql);
int (*mysql_query_pointer)(MYSQL *mysql, const char *stmt_str);
MYSQL *(*mysql_real_connect_pointer)(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long client_flag);
unsigned long (*mysql_real_escape_string_pointer)(MYSQL *mysql, char *to, const char *from, unsigned long length);
int (*mysql_real_query_pointer)(MYSQL *mysql, const char *stmt_str, unsigned long length);
int (*mysql_refresh_pointer)(MYSQL *mysql, unsigned int options);
int (*mysql_reload_pointer)(MYSQL *mysql);
my_bool (*mysql_rollback_pointer)(MYSQL *mysql);
MYSQL_ROW_OFFSET (*mysql_row_seek_pointer)(MYSQL_RES *result, MYSQL_ROW_OFFSET offset);
MYSQL_ROW_OFFSET (*mysql_row_tell_pointer)(MYSQL_RES *result);
int (*mysql_select_db_pointer)(MYSQL *mysql, const char *db);
void (*mysql_server_end_pointer)(void);
int (*mysql_server_init_pointer)(int argc, char **argv, char **groups);
int (*mysql_set_character_set_pointer)(MYSQL *mysql, const char *csname);
void (*mysql_set_local_infile_default_pointer)(MYSQL *mysql);
void (*mysql_set_local_infile_handler_pointer)(MYSQL *mysql, int (*local_infile_init)(void **, const char *, void *), int (*local_infile_read)(void *, char *, unsigned int), void (*local_infile_end)(void *), int (*local_infile_error)(void *, char*, unsigned int), void *userdata);
int (*mysql_set_server_option_pointer)(MYSQL *mysql, enum enum_mysql_set_option option);
const char *(*mysql_sqlstate_pointer)(MYSQL *mysql);
int (*mysql_shutdown_pointer)(MYSQL *mysql, enum mysql_enum_shutdown_level shutdown_level);
my_bool (*mysql_ssl_set_pointer)(MYSQL *mysql, const char *key, const char *cert, const char *ca, const char *capath, const char *cipher);
const char *(*mysql_stat_pointer)(MYSQL *mysql);
MYSQL_RES *(*mysql_store_result_pointer)(MYSQL *mysql);
void (*mysql_thread_end_pointer)(void);
unsigned long (*mysql_thread_id_pointer)(MYSQL *mysql);
my_bool (*mysql_thread_init_pointer)(void);
unsigned int (*mysql_thread_safe_pointer)(void);
MYSQL_RES *(*mysql_use_result_pointer)(MYSQL *mysql);
unsigned int (*mysql_warning_count_pointer)(MYSQL *mysql);

//Redefine mysql functions with the pointers from the dynamic loaded library
#define my_init(x) my_init_pointer(x)
#define mysql_affected_rows(x) mysql_affected_rows_pointer(x)
#define mysql_autocommit(x,y) mysql_autocommit_pointer(x,y)
#define mysql_change_user(x,y,z,db) mysql_change_user_pointer(x,y,z,db)
#define mysql_character_set_name(x) mysql_character_set_name_pointer(x)
#define mysql_close(x) mysql_close_pointer(x)
#define mysql_commit(x) mysql_commit_pointer(x)
#define mysql_connect(x,y,z,passwd) mysql_connect_pointer(x,y,z,passwd)
#define mysql_create_db(x,y) mysql_create_db_pointer(x,y)
#define mysql_data_seek(x,y) mysql_data_seek_pointer(x,y)
#define mysql_debug(x) mysql_debug_pointer(x)
#define mysql_drop_db(x,y) mysql_drop_db_pointer(x,y)
#define mysql_dump_debug_info(x) mysql_dump_debug_info_pointer(x)
#define mysql_eof(x) mysql_eof_pointer(x)
#define mysql_errno(x) mysql_errno_pointer(x)
#define mysql_error(x) mysql_error_pointer(x)
#define mysql_fetch_field(x) mysql_fetch_field_pointer(x)
#define mysql_fetch_field_direct(x,y) mysql_fetch_field_direct_pointer(x,y)
#define mysql_fetch_fields(x) mysql_fetch_fields_pointer(x)
#define mysql_fetch_lengths(x) mysql_fetch_lengths_pointer(x)
#define mysql_fetch_row(x) mysql_fetch_row_pointer(x)
#define mysql_field_count(x) mysql_field_count_pointer(x)
#define mysql_field_seek(x,y) mysql_field_seek_pointer(x,y)
#define mysql_field_tell(x) mysql_field_tell_pointer(x)
#define mysql_free_result(x) mysql_free_result_pointer(x)
#define mysql_get_character_set_info(x,y) mysql_get_character_set_info_pointer(x,y)
#define mysql_get_client_info() mysql_get_client_info_pointer()
#define mysql_get_client_version() mysql_get_client_version_pointer()
#define mysql_get_host_info(x) mysql_get_host_info_pointer(x)
#define mysql_get_proto_info(x) mysql_get_proto_info_pointer(x)
#define mysql_get_server_info(x) mysql_get_server_info_pointer(x)
#define mysql_get_server_version(x) mysql_get_server_version_pointer(x)
#define mysql_get_ssl_cipher(x) mysql_get_ssl_cipher_pointer(x)
#define mysql_hex_string(x,y,z) mysql_hex_string_pointer(x,y,z)
#define mysql_info(x) mysql_info_pointer(x)
#define mysql_init(x) mysql_init_pointer(x)
#define mysql_insert_id(x) mysql_insert_id_pointer(x)
#define mysql_kill(x,y) mysql_kill_pointer(x,y)
#undef mysql_library_end //Fixes compiler warning
#define mysql_library_end() mysql_library_end_pointer()
#undef mysql_library_init //Fixes compiler warning
#define mysql_library_init(x,y,z) mysql_library_init_pointer(x,y,z)
#define mysql_list_dbs(x,y) mysql_list_dbs_pointer(x,y)
#define mysql_list_fields(x,y,z) mysql_list_fields_pointer(x,y,z)
#define mysql_list_processes(x) mysql_list_processes_pointer(x)
#define mysql_list_tables(x,y) mysql_list_tables_pointer(x,y)
#define mysql_more_results(x) mysql_more_results_pointer(x)
#define mysql_next_result(x) mysql_next_result_pointer(x)
#define mysql_num_fields(x) mysql_num_fields_pointer(x)
#define mysql_num_rows(x) mysql_num_rows_pointer(x)
#define mysql_options(x,y,z) mysql_options_pointer(x,y,z)
#define mysql_ping(x) mysql_ping_pointer(x)
#define mysql_query(x,y) mysql_query_pointer(x,y)
#define mysql_real_connect(x,y,z,passwd,db,port,unix_socket,client_flag) mysql_real_connect_pointer(x,y,z,passwd,db,port,unix_socket,client_flag)
#define mysql_real_escape_string(x,y,z,length) mysql_real_escape_string_pointer(x,y,z,length)
#define mysql_real_query(x,y,z) mysql_real_query_pointer(x,y,z)
#define mysql_refresh(x,y) mysql_refresh_pointer(x,y)
#undef mysql_reload //Fixes compiler warning
#define mysql_reload(x) mysql_reload_pointer(x)
#define mysql_rollback(x) mysql_rollback_pointer(x)
#define mysql_row_seek(x,y) mysql_row_seek_pointer(x,y)
#define mysql_row_tell(x) mysql_row_tell_pointer(x)
#define mysql_select_db(x,y) mysql_select_db_pointer(x,y)
#define mysql_server_end() mysql_server_end_pointer()
#define mysql_server_init(x,y,z) mysql_server_init_pointer(x,y,z)
#define mysql_set_character_set(x,y) mysql_set_character_set_pointer(x,y)
#define mysql_set_local_infile_default(x) mysql_set_local_infile_default_pointer(x)
#define mysql_set_local_infile_handler(x,y,z,local_infile_end,local_infile_error,userdata) mysql_set_local_infile_handler_pointer(xy,z,local_infile_end,local_infile_error,userdata)
#define mysql_set_server_option(x,y) mysql_set_server_option_pointer(x,y)
#define mysql_sqlstate(x) mysql_sqlstate_pointer(x)
#define mysql_shutdown(x,y) mysql_shutdown_pointer(x,y)
#define mysql_ssl_set(x,y,z,ca,capath,cipher) mysql_ssl_set_pointer(x,y,z,ca,capath,cipher)
#define mysql_stat(x) mysql_stat_pointer(x)
#define mysql_store_result(x) mysql_store_result_pointer(x)
#define mysql_thread_end() mysql_thread_end_pointer()
#define mysql_thread_id(x) mysql_thread_id_pointer(x)
#define mysql_thread_init() mysql_thread_init_pointer()
#define mysql_thread_safe() mysql_thread_safe_pointer()
#define mysql_use_result(x) mysql_use_result_pointer(x)
#define mysql_warning_count(x) mysql_warning_count_pointer(x)
