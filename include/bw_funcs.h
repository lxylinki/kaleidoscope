#ifndef _BW_FUNCS_H_
#define _BW_FUNCS_H_

/* run a bandwidth test */
int run( struct iperf_test *test );

/* start bandwidth measuring daemon */
void bw_server();

/* start bandwidth measuring client towards a peer host */
double bw_client( const char *peeraddr );

/* _BW_FUNCS_H_ */
#endif 
