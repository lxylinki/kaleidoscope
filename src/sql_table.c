#include<my_global.h>
#include<my_sys.h>
#include<mysql.h>
#include<stdlib.h>
#include"../include/sql_admin.h"

/* create user table */
#define CREATE_USER_TABLE "create table user( username varchar(80), email varchar(80), primary key(email) )" 

/* create task table, type 1: workflow task, type 0: independent task */
#define CREATE_TASK_TABLE "create table task( email varchar(80), task_ID varchar(80),task_type int, sample_interval int, curr_time DATETIME, exec_status float, primary key(task_ID), foreign key(email) references user(email) )" 

/* create program table */
#define CREATE_PROGRAM_TABLE "create table program( prog_ID varchar(80), task_ID varchar(80), curr_time DATETIME, exec_status float, parallism float, primary key(prog_ID), foreign key (task_ID) references task(task_ID) )"

/* create data table  */
#define CREATE_DATA_TABLE "create table data( email varchar(80), srcProg_ID int,dstProg_ID int, label varchar(80), volume int, format varchar(80),curr_time DATETIME, primary key(label, format, curr_time), foreign key(email) references user(email) )"

/* create server table */
#define CREATE_SERVER_TABLE "create table server( host_name varchar(80),CPU_scale float, avail_RAM int, avail_disk int, CPU_temp float,NIC_load float, curr_time DATETIME, IPv6_addr varchar(80),core_num int, IP_addr varchar(80), primary key( host_name, curr_time, IPv6_addr ) )"

/* create exec_on table, relate program to server */
#define CREATE_EXECON_TABLE "create table exec_on( prog_ID int, host_name varchar(80), curr_time DATETIME, primary key( prog_ID, curr_time ), foreign key (host_name) references server (host_name) )"

/* create stored_on table, relate data piece to server */
#define CREATE_STOREDON_TABLE "create table stored_on( label varchar(80),format varchar(80), host_name varchar(80), curr_time DATETIME,primary key( label, format, curr_time), foreign key (host_name) references server(host_name) ) "

/* create peer_connect table */
#define CREATE_PEERCONN_TABLE "create table peer_connect( peer_name varchar(80), peer_ip varchar(80), avail_BW float, curr_time DATETIME, primary key(peer_ip, curr_time))"


void create_tables(char* host_name) {
    /* pointer to connection handler */
    static MYSQL *conn; 

    if( mysql_library_init(0, NULL, NULL) )
    {
        fprintf(stderr, "mysql_library_init() failed\n");
        exit(1);
    }

    /* initialize connection handler */
    conn = mysql_init (NULL);

    if( conn == NULL )
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    /* connect to database */
    if( mysql_real_connect(conn, host_name, my_user_name, my_password, \
        my_db_name, my_port_num, my_socket_name, my_flags ) == NULL )
    {
        fprintf(stderr, "mysql_real_connect() failed: \n%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    /* drop database with same name */
    if(mysql_query(conn, "drop database if exists prototype"))
    {
        fprintf(stderr, "drop database failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* create database */
    if(mysql_query(conn, "create database prototype"))
    {
        fprintf(stderr, "create database failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* use created database */
    if(mysql_query(conn, "use prototype"))
    {
        fprintf(stderr, "use database failed: \n%s\n", mysql_error(conn));
        exit(1);
    }
    
    /* create user table */
    if( mysql_query(conn, CREATE_USER_TABLE) )
    {
        fprintf(stderr, "create user table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }
    
    /* create task table */
    if( mysql_query(conn, CREATE_TASK_TABLE) )
    {
        fprintf(stderr, "create task table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* create program table */
    if( mysql_query(conn, CREATE_PROGRAM_TABLE) )
    {
        fprintf(stderr, "create program table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* create dynamic_data table */
    if( mysql_query(conn, CREATE_DATA_TABLE) )
    {
        fprintf(stderr, "create dynamic_data table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* create server table */
    if( mysql_query(conn, CREATE_SERVER_TABLE) )
    {
        fprintf(stderr, "create server table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }
   
    /* create exec_on table */
    if( mysql_query(conn, CREATE_EXECON_TABLE) )
    {
        fprintf(stderr, "create exec_on table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }
   
    /* create stored_on table */
    if( mysql_query(conn, CREATE_STOREDON_TABLE) )
    {
        fprintf(stderr, "create stored_on table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* create peer_connect table */
    if( mysql_query(conn, CREATE_PEERCONN_TABLE) )
    {
        fprintf(stderr, "create peer_connect table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }
    
    mysql_close(conn);
    mysql_library_end();
    printf("All tables are provisioned.\n");
    exit(EXIT_SUCCESS);
}

int main() {
    create_tables(my_host_name);
    return EXIT_SUCCESS;
}
