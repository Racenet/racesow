#include "g_local.h"
#include <pthread.h>
#include <mysql.h>
#include <errmsg.h>
#include <dlfcn.h>
#include "g_dynamicmysql.h"

qboolean LoadMySQLFunctions( void )
{
    char* error;
   /*mysql_init_pointer = dlsym(libmysqlclient,"mysql_init");
    if((error = dlerror()) != NULL) {
       Com_Printf("No mysql_init\n");
       return qfalse;
    } else {
      Com_Printf("Got mysql_init\n");    
    }
    mysql_affected_rows_pointer = dlsym(libmysqlclient,"mysql_affected_rows");
    if((error = dlerror()) != NULL) {
       Com_Printf("No mysql_affected_rows\n");
       return qfalse;
    } else {
      Com_Printf("Got mysql_affected_rows\n");    
    }*/

    Com_Printf("\nLoading MySQL Functions:\n\n");

    my_init_pointer = dlsym(libmysqlclient,"my_init");
    if((error = dlerror()) != NULL) {
      Com_Printf("No my_init\n");
      return qfalse;
    } else {
      Com_Printf("Got my_init\n");
    }

    mysql_affected_rows_pointer = dlsym(libmysqlclient,"mysql_affected_rows");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_affected_rows\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_affected_rows\n");
    }

    mysql_autocommit_pointer = dlsym(libmysqlclient,"mysql_autocommit");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_autocommit\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_autocommit\n");
    }

    mysql_change_user_pointer = dlsym(libmysqlclient,"mysql_change_user");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_change_user\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_change_user\n");
    }

    mysql_character_set_name_pointer = dlsym(libmysqlclient,"mysql_character_set_name");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_character_set_name\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_character_set_name\n");
    }

    mysql_close_pointer = dlsym(libmysqlclient,"mysql_close");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_close\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_close\n");
    }

    mysql_commit_pointer = dlsym(libmysqlclient,"mysql_commit");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_commit\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_commit\n");
    }

    /*mysql_connect_pointer = dlsym(libmysqlclient,"mysql_connect");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_connect\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_connect\n");
    }

    mysql_create_db_pointer = dlsym(libmysqlclient,"mysql_create_db");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_create_db\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_create_db\n");
    }*/

    mysql_data_seek_pointer = dlsym(libmysqlclient,"mysql_data_seek");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_data_seek\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_data_seek\n");
    }

    mysql_debug_pointer = dlsym(libmysqlclient,"mysql_debug");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_debug\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_debug\n");
    }

    /*mysql_drop_db_pointer = dlsym(libmysqlclient,"mysql_drop_db");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_drop_db\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_drop_db\n");
    }*/

    mysql_dump_debug_info_pointer = dlsym(libmysqlclient,"mysql_dump_debug_info");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_dump_debug_info\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_dump_debug_info\n");
    }

    mysql_eof_pointer = dlsym(libmysqlclient,"mysql_eof");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_eof\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_eof\n");
    }

    mysql_errno_pointer = dlsym(libmysqlclient,"mysql_errno");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_errno\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_errno\n");
    }

    mysql_error_pointer = dlsym(libmysqlclient,"mysql_error");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_error\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_error\n");
    }

    mysql_fetch_field_pointer = dlsym(libmysqlclient,"mysql_fetch_field");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_fetch_field\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_fetch_field\n");
    }

    mysql_fetch_field_direct_pointer = dlsym(libmysqlclient,"mysql_fetch_field_direct");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_fetch_field_direct\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_fetch_field_direct\n");
    }

    mysql_fetch_fields_pointer = dlsym(libmysqlclient,"mysql_fetch_fields");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_fetch_fields\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_fetch_fields\n");
    }

    mysql_fetch_lengths_pointer = dlsym(libmysqlclient,"mysql_fetch_lengths");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_fetch_lengths\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_fetch_lengths\n");
    }

    mysql_fetch_row_pointer = dlsym(libmysqlclient,"mysql_fetch_row");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_fetch_row\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_fetch_row\n");
    }

    mysql_field_count_pointer = dlsym(libmysqlclient,"mysql_field_count");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_field_count\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_field_count\n");
    }

    mysql_field_seek_pointer = dlsym(libmysqlclient,"mysql_field_seek");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_field_seek\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_field_seek\n");
    }

    mysql_field_tell_pointer = dlsym(libmysqlclient,"mysql_field_tell");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_field_tell\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_field_tell\n");
    }

    mysql_free_result_pointer = dlsym(libmysqlclient,"mysql_free_result");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_free_result\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_free_result\n");
    }

    mysql_get_character_set_info_pointer = dlsym(libmysqlclient,"mysql_get_character_set_info");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_character_set_info\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_character_set_info\n");
    }

    mysql_get_client_info_pointer = dlsym(libmysqlclient,"mysql_get_client_info");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_client_info\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_client_info\n");
    }

    mysql_get_client_version_pointer = dlsym(libmysqlclient,"mysql_get_client_version");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_client_version\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_client_version\n");
    }

    mysql_get_host_info_pointer = dlsym(libmysqlclient,"mysql_get_host_info");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_host_info\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_host_info\n");
    }

    mysql_get_proto_info_pointer = dlsym(libmysqlclient,"mysql_get_proto_info");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_proto_info\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_proto_info\n");
    }

    mysql_get_server_info_pointer = dlsym(libmysqlclient,"mysql_get_server_info");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_server_info\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_server_info\n");
    }

    mysql_get_server_version_pointer = dlsym(libmysqlclient,"mysql_get_server_version");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_server_version\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_server_version\n");
    }

    mysql_get_ssl_cipher_pointer = dlsym(libmysqlclient,"mysql_get_ssl_cipher");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_get_ssl_cipher\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_get_ssl_cipher\n");
    }

    mysql_hex_string_pointer = dlsym(libmysqlclient,"mysql_hex_string");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_hex_string\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_hex_string\n");
    }

    mysql_info_pointer = dlsym(libmysqlclient,"mysql_info");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_info\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_info\n");
    }

    mysql_init_pointer = dlsym(libmysqlclient,"mysql_init");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_init\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_init\n");
    }

    mysql_insert_id_pointer = dlsym(libmysqlclient,"mysql_insert_id");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_insert_id\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_insert_id\n");
    }

    mysql_kill_pointer = dlsym(libmysqlclient,"mysql_kill");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_kill\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_kill\n");
    }

    /*mysql_library_end_pointer = dlsym(libmysqlclient,"mysql_library_end");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_library_end\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_library_end\n");
    }

    mysql_library_init_pointer = dlsym(libmysqlclient,"mysql_library_init");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_library_init\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_library_init\n");
    }

    mysql_list_dbs_pointer = dlsym(libmysqlclient,"mysql_list_dbs");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_list_dbs\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_list_dbs\n");
    }*/

    mysql_list_fields_pointer = dlsym(libmysqlclient,"mysql_list_fields");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_list_fields\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_list_fields\n");
    }

    mysql_list_processes_pointer = dlsym(libmysqlclient,"mysql_list_processes");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_list_processes\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_list_processes\n");
    }

    mysql_list_tables_pointer = dlsym(libmysqlclient,"mysql_list_tables");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_list_tables\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_list_tables\n");
    }

    mysql_more_results_pointer = dlsym(libmysqlclient,"mysql_more_results");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_more_results\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_more_results\n");
    }

    mysql_next_result_pointer = dlsym(libmysqlclient,"mysql_next_result");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_next_result\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_next_result\n");
    }

    mysql_num_fields_pointer = dlsym(libmysqlclient,"mysql_num_fields");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_num_fields\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_num_fields\n");
    }

    mysql_num_rows_pointer = dlsym(libmysqlclient,"mysql_num_rows");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_num_rows\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_num_rows\n");
    }

    mysql_options_pointer = dlsym(libmysqlclient,"mysql_options");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_options\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_options\n");
    }

    mysql_ping_pointer = dlsym(libmysqlclient,"mysql_ping");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_ping\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_ping\n");
    }

    mysql_query_pointer = dlsym(libmysqlclient,"mysql_query");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_query\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_query\n");
    }

    mysql_real_connect_pointer = dlsym(libmysqlclient,"mysql_real_connect");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_real_connect\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_real_connect\n");
    }

    mysql_real_escape_string_pointer = dlsym(libmysqlclient,"mysql_real_escape_string");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_real_escape_string\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_real_escape_string\n");
    }

    mysql_real_query_pointer = dlsym(libmysqlclient,"mysql_real_query");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_real_query\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_real_query\n");
    }

    mysql_refresh_pointer = dlsym(libmysqlclient,"mysql_refresh");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_refresh\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_refresh\n");
    }

    /*mysql_reload_pointer = dlsym(libmysqlclient,"mysql_reload");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_reload\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_reload\n");
    }*/

    mysql_rollback_pointer = dlsym(libmysqlclient,"mysql_rollback");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_rollback\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_rollback\n");
    }

    mysql_row_seek_pointer = dlsym(libmysqlclient,"mysql_row_seek");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_row_seek\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_row_seek\n");
    }

    mysql_row_tell_pointer = dlsym(libmysqlclient,"mysql_row_tell");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_row_tell\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_row_tell\n");
    }

    /*mysql_select_db_pointer = dlsym(libmysqlclient,"mysql_select_db");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_select_db\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_select_db\n");
    }*/

    mysql_server_end_pointer = dlsym(libmysqlclient,"mysql_server_end");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_server_end\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_server_end\n");
    }

    mysql_server_init_pointer = dlsym(libmysqlclient,"mysql_server_init");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_server_init\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_server_init\n");
    }

    mysql_set_character_set_pointer = dlsym(libmysqlclient,"mysql_set_character_set");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_set_character_set\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_set_character_set\n");
    }

    mysql_set_local_infile_default_pointer = dlsym(libmysqlclient,"mysql_set_local_infile_default");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_set_local_infile_default\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_set_local_infile_default\n");
    }

    mysql_set_local_infile_handler_pointer = dlsym(libmysqlclient,"mysql_set_local_infile_handler");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_set_local_infile_handler\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_set_local_infile_handler\n");
    }

    mysql_set_server_option_pointer = dlsym(libmysqlclient,"mysql_set_server_option");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_set_server_option\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_set_server_option\n");
    }

    mysql_sqlstate_pointer = dlsym(libmysqlclient,"mysql_sqlstate");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_sqlstate\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_sqlstate\n");
    }

    mysql_shutdown_pointer = dlsym(libmysqlclient,"mysql_shutdown");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_shutdown\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_shutdown\n");
    }

    mysql_ssl_set_pointer = dlsym(libmysqlclient,"mysql_ssl_set");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_ssl_set\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_ssl_set\n");
    }

    mysql_stat_pointer = dlsym(libmysqlclient,"mysql_stat");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_stat\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_stat\n");
    }

    mysql_store_result_pointer = dlsym(libmysqlclient,"mysql_store_result");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_store_result\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_store_result\n");
    }

    mysql_thread_end_pointer = dlsym(libmysqlclient,"mysql_thread_end");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_thread_end\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_thread_end\n");
    }

    mysql_thread_id_pointer = dlsym(libmysqlclient,"mysql_thread_id");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_thread_id\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_thread_id\n");
    }

    mysql_thread_init_pointer = dlsym(libmysqlclient,"mysql_thread_init");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_thread_init\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_thread_init\n");
    }

    mysql_thread_safe_pointer = dlsym(libmysqlclient,"mysql_thread_safe");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_thread_safe\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_thread_safe\n");
    }

    mysql_use_result_pointer = dlsym(libmysqlclient,"mysql_use_result");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_use_result\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_use_result\n");
    }

    mysql_warning_count_pointer = dlsym(libmysqlclient,"mysql_warning_count");
    if((error = dlerror()) != NULL) {
      Com_Printf("No mysql_warning_count\n");
      return qfalse;
    } else {
      Com_Printf("Got mysql_warning_count\n");
    }

    Com_Printf("\nDone.\n");

    return qtrue;
}

void UnLoadMySQLFunctions( void )
{
my_init_pointer=NULL;
mysql_affected_rows_pointer=NULL;
mysql_autocommit_pointer=NULL;
mysql_change_user_pointer=NULL;
mysql_character_set_name_pointer=NULL;
mysql_close_pointer=NULL;
mysql_commit_pointer=NULL;
mysql_connect_pointer=NULL;
mysql_create_db_pointer=NULL;
mysql_data_seek_pointer=NULL;
mysql_debug_pointer=NULL;
mysql_drop_db_pointer=NULL;
mysql_dump_debug_info_pointer=NULL;
mysql_eof_pointer=NULL;
mysql_errno_pointer=NULL;
mysql_error_pointer=NULL;
mysql_fetch_field_pointer=NULL;
mysql_fetch_field_direct_pointer=NULL;
mysql_fetch_fields_pointer=NULL;
mysql_fetch_lengths_pointer=NULL;
mysql_fetch_row_pointer=NULL;
mysql_field_count_pointer=NULL;
mysql_field_seek_pointer=NULL;
mysql_field_tell_pointer=NULL;
mysql_free_result_pointer=NULL;
mysql_get_character_set_info_pointer=NULL;
mysql_get_client_info_pointer=NULL;
mysql_get_client_version_pointer=NULL;
mysql_get_host_info_pointer=NULL;
mysql_get_proto_info_pointer=NULL;
mysql_get_server_info_pointer=NULL;
mysql_get_server_version_pointer=NULL;
mysql_get_ssl_cipher_pointer=NULL;
mysql_hex_string_pointer=NULL;
mysql_info_pointer=NULL;
mysql_init_pointer=NULL;
mysql_insert_id_pointer=NULL;
mysql_kill_pointer=NULL;
mysql_library_end_pointer=NULL;
mysql_library_init_pointer=NULL;
mysql_list_dbs_pointer=NULL;
mysql_list_fields_pointer=NULL;
mysql_list_processes_pointer=NULL;
mysql_list_tables_pointer=NULL;
mysql_more_results_pointer=NULL;
mysql_next_result_pointer=NULL;
mysql_num_fields_pointer=NULL;
mysql_num_rows_pointer=NULL;
mysql_options_pointer=NULL;
mysql_ping_pointer=NULL;
mysql_query_pointer=NULL;
mysql_real_connect_pointer=NULL;
mysql_real_escape_string_pointer=NULL;
mysql_real_query_pointer=NULL;
mysql_refresh_pointer=NULL;
mysql_reload_pointer=NULL;
mysql_rollback_pointer=NULL;
mysql_row_seek_pointer=NULL;
mysql_row_tell_pointer=NULL;
mysql_select_db_pointer=NULL;
mysql_server_end_pointer=NULL;
mysql_server_init_pointer=NULL;
mysql_set_character_set_pointer=NULL;
mysql_set_local_infile_default_pointer=NULL;
mysql_set_local_infile_handler_pointer=NULL;
mysql_set_server_option_pointer=NULL;
mysql_sqlstate_pointer=NULL;
mysql_shutdown_pointer=NULL;
mysql_ssl_set_pointer=NULL;
mysql_stat_pointer=NULL;
mysql_store_result_pointer=NULL;
mysql_thread_end_pointer=NULL;
mysql_thread_id_pointer=NULL;
mysql_thread_init_pointer=NULL;
mysql_thread_safe_pointer=NULL;
mysql_use_result_pointer=NULL;
mysql_warning_count_pointer=NULL;

#undef my_init
#undef mysql_affected_rows
#undef mysql_autocommit
#undef mysql_change_user
#undef mysql_character_set_name
#undef mysql_close
#undef mysql_commit
#undef mysql_connect
#undef mysql_create_db
#undef mysql_data_seek
#undef mysql_debug
#undef mysql_drop_db
#undef mysql_dump_debug_info
#undef mysql_eof
#undef mysql_errno
#undef mysql_error
#undef mysql_fetch_field
#undef mysql_fetch_field_direct
#undef mysql_fetch_fields
#undef mysql_fetch_lengths
#undef mysql_fetch_row
#undef mysql_field_count
#undef mysql_field_seek
#undef mysql_field_tell
#undef mysql_free_result
#undef mysql_get_character_set_info
#undef mysql_get_client_info
#undef mysql_get_client_version
#undef mysql_get_host_info
#undef mysql_get_proto_info
#undef mysql_get_server_info
#undef mysql_get_server_version
#undef mysql_get_ssl_cipher
#undef mysql_hex_string
#undef mysql_info
#undef mysql_init
#undef mysql_insert_id
#undef mysql_kill
#undef mysql_library_end
#undef mysql_library_init
#undef mysql_list_dbs
#undef mysql_list_fields
#undef mysql_list_processes
#undef mysql_list_tables
#undef mysql_more_results
#undef mysql_next_result
#undef mysql_num_fields
#undef mysql_num_rows
#undef mysql_options
#undef mysql_ping
#undef mysql_query
#undef mysql_real_connect
#undef mysql_real_escape_string
#undef mysql_real_query
#undef mysql_refresh
#undef mysql_reload
#undef mysql_rollback
#undef mysql_row_seek
#undef mysql_row_tell
#undef mysql_select_db
#undef mysql_server_end
#undef mysql_server_init
#undef mysql_set_character_set
#undef mysql_set_local_infile_default
#undef mysql_set_local_infile_handler
#undef mysql_set_server_option
#undef mysql_sqlstate
#undef mysql_shutdown
#undef mysql_ssl_set
#undef mysql_stat
#undef mysql_store_result
#undef mysql_thread_end
#undef mysql_thread_id
#undef mysql_thread_init
#undef mysql_thread_safe
#undef mysql_use_result
#undef mysql_warning_count
}
