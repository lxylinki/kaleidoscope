#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "../include/sql_admin.h"
#include "../include/lc_samp.h"
#include "../include/peer_conn.h"

#define SERVER_INSERT_STMT "insert into server(host_name,CPU_scale,avail_RAM,avail_disk,CPU_temp,NIC_load,curr_time,IPv6_addr,core_num,IP_addr) values(?,?,?,?,?,?,?,?,?,?)"

#define GLOBAL_INTERVAL 5

static int server_columns = 10;

// server sampling inner routine
void bind_and_exec_serverinsert( MYSQL* conn, MYSQL_STMT* server_insert ) 
{
    MYSQL_BIND insbind[server_columns];
    server_row* server = collect_server_row();
    printf("%s | %.2fMHz | %ldKB | %ldKB | %.2fC | %.2fKB/s | ? | %s | %d | %s |\n", \
            server->host_name, server->CPU_scale, server->avail_RAM, server->avail_disk, server->CPU_temp, \
            server->NIC_load, server->ip6_addr, server->core_num, server->ip_addr);

    memset((void*)insbind, 0, sizeof(insbind));

    insbind[0].buffer_type = MYSQL_TYPE_STRING; //host_name
    insbind[1].buffer_type = MYSQL_TYPE_FLOAT; //cpu scale
    insbind[2].buffer_type = MYSQL_TYPE_LONG; //avail_ram
    insbind[3].buffer_type = MYSQL_TYPE_LONG; //avail_disk
    insbind[4].buffer_type = MYSQL_TYPE_FLOAT; //cpu_temp
    insbind[5].buffer_type = MYSQL_TYPE_FLOAT; //nic load
    insbind[6].buffer_type = MYSQL_TYPE_DATETIME; // current time
    insbind[7].buffer_type = MYSQL_TYPE_STRING; //ipv6 addr
    insbind[8].buffer_type = MYSQL_TYPE_LONG; //number of cores
    insbind[9].buffer_type = MYSQL_TYPE_STRING; //ipv4 addr

    unsigned long hostname_len;
    unsigned long ipaddr6_len;
    unsigned long ipaddr_len;

    // set buffer values
    hostname_len = strlen(server->host_name);
    insbind[0].buffer_length = hostname_len;
    insbind[0].length = &hostname_len;
    insbind[0].buffer = (void*) server->host_name;

    insbind[1].buffer = &(server->CPU_scale);
    insbind[2].buffer = &(server->avail_RAM);
    insbind[3].buffer = &(server->avail_disk);
    insbind[4].buffer = &(server->CPU_temp);
    insbind[5].buffer = &(server->NIC_load);

    insbind[6].buffer = (void*) server->curr_time;

    ipaddr6_len = strlen(server->ip6_addr);
    ipaddr_len = strlen(server->ip_addr);

    insbind[7].buffer_length = ipaddr6_len;
    insbind[7].length = &ipaddr6_len;
    insbind[7].buffer = (void*) server->ip6_addr;

    insbind[8].buffer = &(server->core_num);
    
    insbind[9].buffer_length = ipaddr_len;
    insbind[9].length = &ipaddr_len;
    insbind[9].buffer = (void*) server->ip_addr;

    // bind parameters
    if( mysql_stmt_bind_param(server_insert, insbind) != 0) {
        fprintf(stderr, "Statement parameter binding failed: %s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    // execute the statement
    if( mysql_stmt_execute(server_insert) != 0 ) {
        fprintf(stderr, "Statement execution failed: %s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }
}

// server sampling outer routine
void prepare_and_exec_serverinsert(MYSQL* conn, int total_samples) {

    static MYSQL_STMT* server_insert;

    // init MYSQL_STMT handle
    server_insert = mysql_stmt_init(conn);

    if (server_insert == NULL) {
        fprintf(stderr, "mysql_stmt_init() failed for server_insert statement\n");
        exit(EXIT_FAILURE);
    }

    // prepare the statement
    int res;
    if ( (res = mysql_stmt_prepare( server_insert, SERVER_INSERT_STMT, strlen(SERVER_INSERT_STMT))) != 0) {
        fprintf(stderr, "mysql_stmt_prepare() failed for server_insert statement with return value %d\n", res);
        perror("mysql_stmt_prepare");
        exit(EXIT_FAILURE);
    }

    int i;
    for(i=0; i<total_samples; i++) {
        bind_and_exec_serverinsert(conn, server_insert);
        sleep(GLOBAL_INTERVAL);
    }

    mysql_stmt_close(server_insert);
}

/* server sampling process */
void local_process(int total_samples) {
    static MYSQL* conn;

    if(mysql_library_init(0, NULL, NULL)) {
        fprintf(stderr, "mysql_library_init() failed\n");
        exit(EXIT_FAILURE);
    } 

    conn = mysql_init(NULL);
    if(conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if(mysql_real_connect(conn, my_host_name, my_user_name, my_password, my_db_name, my_port_num, my_socket_name, my_flags) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed:\n%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    if(mysql_query(conn, "use prototype")) {
        fprintf(stderr, "use database failed: \n%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    // execute the outer routine
    prepare_and_exec_serverinsert(conn, total_samples);

    mysql_close(conn);
    mysql_library_end();

    exit(EXIT_SUCCESS);
}

int main() {
    local_process(20);
    exit(EXIT_SUCCESS);
}
