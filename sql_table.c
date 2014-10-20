#include<my_sys.h>
#include<mysql.h>
#include<stdlib.h>
#include"sql_admin.h"

/* create user table */
#define CREATE_USER_TABLE "create table user( username varchar(80), email varchar(80), primary key(email) )" 

/* create task table, type 1: workflow task, type 0: independent task */
#define CREATE_TASK_TABLE "create table task( task_ID varchar(80),task_type int, timeslot_len int, curr_time DATETIME, exec_status float, primary key(task_ID) )" 

/* create program table */
#define CREATE_PROGRAM_TABLE "create table program( prog_ID varchar(80), curr_time DATETIME, exec_status float, parallism float, primary key(prog_ID) )"

/* create contains table, relate program to task */
#define CREATE_CONTAINS_TABLE "create table contains(task_ID varchar(80), prog_ID varchar(80), primary key(prog_ID) )"

/* create owns table, relate task to user */
#define CREATE_OWNS_TABLE "create table owns( task_ID varchar(80),email varchar(80), primary key(task_ID) )"

/* create dynamic_data table  */
#define CREATE_DDATA_TABLE "create table dynamic_data( srcProg_ID int,dstProg_ID int, label varchar(80), volume int, format varchar(80),curr_time DATETIME, primary key(label, format, curr_time) )"

/* create signed_by table, relate data to user */
#define CREATE_SIGNEDBY_TABLE "create table signed_by( label varchar(80),format varchar(80), email varchar(80), primary key(label, format) )"

/* create server table */
#define CREATE_SERVER_TABLE "create table server( host_name varchar(80),CPU_scale float, avail_RAM int, avail_disk int, CPU_temp float,NIC_load float, curr_time DATETIME, MAC_addr varchar(80),CPU_capacity float,core_num int, IP_addr varchar(80), primary key( curr_time, MAC_addr ) )"

/* create exec_on table, relate program to server */
#define CREATE_EXECON_TABLE "create table exec_on( prog_ID int,host_name varchar(80), curr_time DATETIME, primary key( prog_ID, curr_time ) )"

/* create stored_on table */
#define CREATE_STOREDON_TABLE "create table stored_on( label varchar(80),format varchar(80), host_name varchar(80), curr_time DATETIME,primary key( label, format, curr_time) ) "

/* create peer_connect table */
#define CREATE_PEERCONN_TABLE "create table peer_connect( host1_name varchar(80), host1_ip varchar(80), host2_name varchar(80), host2_ip varchar(80), avail_BW float, curr_time DATETIME, primary key(host1_ip, host2_ip, curr_time))"


void createtables(char *host_name) {
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

    /* create contains table */
    if( mysql_query(conn, CREATE_CONTAINS_TABLE) )
    {
        fprintf(stderr, "create contains table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* create owns table */
    if( mysql_query(conn, CREATE_OWNS_TABLE) )
    {
        fprintf(stderr, "create owns table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }


    /* create dynamic_data table */
    if( mysql_query(conn, CREATE_DDATA_TABLE) )
    {
        fprintf(stderr, "create dynamic_data table failed: \n%s\n", mysql_error(conn));
        exit(1);
    }

    /* create signed_by table */
    if( mysql_query(conn, CREATE_SIGNEDBY_TABLE) )
    {
        fprintf(stderr, "create signed_by table failed: \n%s\n", mysql_error(conn));
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
    createtables(my_host_name);
    return EXIT_SUCCESS;
}
