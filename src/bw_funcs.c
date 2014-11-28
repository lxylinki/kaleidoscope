/* This is for using iperf functions */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../iperf/iperf.h"
#include "../iperf/iperf_api.h"

/* Duration of an iperf bandwidth test in seconds */
#define BW_TEST_DUR 5
#define MAXLINE 80

/*ref from iperf main*************************************************************************/
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

void bw_server(){
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

double bw_client( const char *peeraddr ){
    struct iperf_test *mytest;
    double bw;
    mytest = iperf_new_test();

    if(!mytest)
    {
        iperf_errexit( NULL, "%s\n", iperf_strerror(i_errno) );
    }

    iperf_defaults( mytest );
    iperf_set_test_role( mytest, 'c' );
    iperf_set_test_duration( mytest, BW_TEST_DUR );
    iperf_set_test_server_hostname( mytest, peeraddr );
    
    if ( run(mytest) < 0 )
        iperf_errexit( mytest, "error - %s", iperf_strerror(i_errno) );

    bw = iperf_get_bandwidth( mytest );
    printf("Connection bandwidth: %.2fKB/s\n", (bw/1024));    
    iperf_free_test( mytest );
    return bw;
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
    bw_client("10.0.0.15");
    bw_client("localhost");
    return 0;
}

