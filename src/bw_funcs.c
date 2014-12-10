#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../iperf/iperf.h"
#include "../iperf/iperf_api.h"
#include "../include/sql_admin.h"

#define PEER_INSERT_STMT "insert into peer_connect(peer_name,peer_ip,avail_BW,curr_time) values(?,?,?,?)"

/* Duration of an iperf bandwidth test in seconds */
#define GLOBAL_INTERVAL 10
#define MAXLINE 80

static int peer_columns = 4;

/* Static information about localhost's peers */
static int peer_num = 2;
static char* mypeers[] = {"10.0.0.15", "localhost"};
static char* peer_names[] = {"snowy", "kaleidoscope"};

/* convert into mysql time */
MYSQL_TIME* get_mysqltime() {
    static MYSQL_TIME mytime; 
    
    struct tm *nowtime;
    time_t now = time(NULL);
    nowtime = localtime(&now);
    
    mytime.year = nowtime->tm_year + 1900;
    mytime.month = nowtime->tm_mon + 1;
    mytime.day = nowtime->tm_mday;
    mytime.hour = nowtime->tm_hour;
    mytime.minute = nowtime->tm_min;
    mytime.second = nowtime->tm_sec;
    mytime.neg = nowtime->tm_isdst;
    
    return &mytime;
}

/* ref from iperf main */
int run( struct iperf_test *test ){
    int consecutive_errors;
    switch (test->role) {
        case 's':
	    if (test->daemon) {
		int rc = daemon(0, 0);
		if (rc < 0) {
		    i_errno = IEDAEMON;
		    iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
		}
	    }
	    consecutive_errors = 0;
            for (;;) {
                if (iperf_run_server(test) < 0) {
		    iperf_err(test, "error - %s", iperf_strerror(i_errno));
                    fprintf(stderr, "\n");
		    ++consecutive_errors;
		    if (consecutive_errors >= 5) {
		        fprintf(stderr, "too many errors, exiting\n");
			break;
		    }
                } else
		    consecutive_errors = 0;
                iperf_reset_test(test);
            }
            break;
        case 'c':
            if (iperf_run_client(test) < 0)
		iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
            break;
        default:
            usage();
            break;
    }
    return 0;
}

void bw_server() {
    struct iperf_test *mytest;
    mytest = iperf_new_test();

    if(!mytest)
    {
        iperf_errexit( NULL, "%s\n", iperf_strerror(i_errno) );
    }

    iperf_defaults( mytest );
    iperf_set_test_role( mytest,'s' );
    mytest->daemon = 1;
    
    if ( run(mytest) < 0 )
        iperf_errexit( mytest, "error - %s", iperf_strerror(i_errno) );

    iperf_free_test( mytest );
}

float bw_client( const char *peeraddr ) {
    struct iperf_test *mytest;
    double bw;
    mytest = iperf_new_test();

    if(!mytest)
    {
        iperf_errexit( NULL, "%s\n", iperf_strerror(i_errno) );
    }

    iperf_defaults( mytest );
    iperf_set_test_role( mytest, 'c' );
    iperf_set_test_duration( mytest, GLOBAL_INTERVAL );
    iperf_set_test_server_hostname( mytest, peeraddr );
    
    if ( run(mytest) < 0 )
        iperf_errexit( mytest, "error - %s", iperf_strerror(i_errno) );

    bw = iperf_get_bandwidth( mytest );
    //printf("Connection bandwidth: %.2fKB/s\n", (bw/1024));    
    iperf_free_test( mytest );
    
    return (float)(bw/1024);
}

/* get local time as a tm structure */
struct tm* collect_currenttime() {
    static struct tm *nowtime;
    time_t now = time(NULL);
    nowtime = localtime(&now);
    //printf("%s", asctime(nowtime));
    return nowtime;
}

typedef struct peer_link {
    char* pr_name;
    char* pr_ip;
    double pr_bw;
    MYSQL_TIME* curr_time;
} peer_row;

void bind_and_exec_peerinsert(MYSQL* conn, MYSQL_STMT* peer_insert, int peer_rank) {
    if (peer_rank > peer_num-1) {
        printf("Error: no such peer.\n");
        exit (EXIT_FAILURE);
    }
    
    MYSQL_BIND prbind[peer_columns];

    float bw = bw_client(mypeers[peer_rank]);
    printf("Connection bandwidth: %.2fKB/s\n", (bw/1024));    
    struct tm* my_time;
    my_time = collect_currenttime();

    memset((void*)prbind, 0, sizeof(prbind));

    prbind[0].buffer_type = MYSQL_TYPE_STRING;
    prbind[1].buffer_type = MYSQL_TYPE_STRING;
    prbind[2].buffer_type = MYSQL_TYPE_FLOAT;
    prbind[3].buffer_type = MYSQL_TYPE_DATETIME;
    
    unsigned long hostname_len;
    unsigned long ipaddr_len;

    hostname_len = strlen(mypeers[peer_rank]);
    prbind[0].buffer_length = hostname_len;
    prbind[0].length = &hostname_len;
    prbind[0].buffer = (void*) (mypeers[peer_rank]);

    ipaddr_len = strlen(peer_names[peer_rank]);
    prbind[1].buffer_length = ipaddr_len;
    prbind[1].length = &ipaddr_len;
    prbind[1].buffer = (void*) (peer_names[peer_rank]);

    prbind[2].buffer = &bw;
    prbind[3].buffer = (void*) get_mysqltime(my_time);

    // bind parameters
    if (mysql_stmt_bind_param(peer_insert, prbind) != 0) {
        fprintf(stderr, "Statement parameter binding failed: %s\n", mysql_error(conn));
        exit (EXIT_FAILURE);
    }
    
    // execute the statement
    if( mysql_stmt_execute(peer_insert) != 0 ) {
        fprintf(stderr, "Statement execution failed: %s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }
}

void prepare_and_exec_peerinsert(MYSQL* conn, int total_iterations) {
    static MYSQL_STMT* peer_insert;

    peer_insert = mysql_stmt_init(conn);
    if (peer_insert == NULL) {
        fprintf(stderr, "mysql_stmt_init() failed for peer_insert statement\n");
        exit(EXIT_FAILURE);
    }

    // prepare the statement
    int res;
    if ((res=mysql_stmt_prepare(peer_insert, PEER_INSERT_STMT, strlen(PEER_INSERT_STMT))) != 0) {
        fprintf(stderr, "mysql_stmt_prepare() failed for peer_insert statement with return value %d\n", res);
        perror ("mysql_stmt_prepare");
        exit (EXIT_FAILURE);
    }

    int i, j;
    for (i=0; i<total_iterations; i++) {
        for (j=0; j<peer_num; j++) {
            // prob j-th peer
            bind_and_exec_peerinsert(conn, peer_insert, j);
            sleep(GLOBAL_INTERVAL);
        }
    }

    mysql_stmt_close(peer_insert);
}


void conn_process(int total_iterations) {
    static MYSQL* conn;
    
    if (mysql_library_init(0, NULL, NULL)) {
        fprintf(stderr, "mysql_library_init() failed\n");
        exit(EXIT_FAILURE);
    }  

    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(conn, my_host_name, my_user_name, my_password, my_db_name, my_port_num, my_socket_name, my_flags) == NULL) {
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    if (mysql_query(conn, "use prototype")) {
        fprintf(stderr, "use database failed: \n%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    } 

    // execute the outer routine
    prepare_and_exec_peerinsert(conn, total_iterations);

    mysql_close(conn);
    mysql_library_end();

    exit (EXIT_SUCCESS);
}

/* testing iperf functions */
int main( int argc, char *argv[] ){
    if(fork() == 0)
    {
        /* start the server daemon,this is not a concurrent server */
        bw_server();
    }

    /* run the client side alg iterate through each remote peer recorded */
    //TODO: the peer hostname is selected from peer_connect table
    conn_process(10);
    exit (EXIT_SUCCESS);
}

