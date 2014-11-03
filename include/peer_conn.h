/* functions used in peer connection */
#ifndef _PEER_CONN_H_
#define _PEER_CONN_H_

/*
typedef struct peer_connect_row
{
    char *host1_name;
    char *host1_ip;
    char *host2_name;
    char *host2_ip;
    float avail_BW;
    MYSQL_TIME *curr_time;

} peertable_row;
*/

/* bind parameter values */
void bind_and_exec_peerconninsert( MYSQL *conn, MYSQL_STMT *peerconn_insert, const peertable_row *row );

/* prepare statement: peer_connect table insert statement */
void prepare_exec_peerconninsert( MYSQL *conn, const peertable_row *row );

/* return readable string presentation of network address */
char *socket_ntop( const struct sockaddr *sa );

/* return peer hostname/ip from peer message/info */
char *extract_peername( const char *msginfo, char *name, int maxlen );

/* return an addrinfo structure from hostname, return NULL if error */
struct addrinfo *get_hostaddr( const char *hostname, const char *service, int protocol_family, int socket_type );

/* return a sending socket to desthost bind to local CPORT */
/* copy sockaddr info from desthost to saptr and len */
int sending_socket( const char *desthostname, const char *serv, struct sockaddr **saptr, socklen_t *len ); 

/* compose signal to send (localhost name:port, pid) and send to peers sockaddr, which can be a multicast address */
void start_announcing( int sndsock, struct sockaddr *peers_addr, socklen_t peersa_len );

/* return a listening socket bind to specific port serv, by default on localhost */
int listening_socket( const char *serv );

/* receive datagrams unicast or multicast */
void start_receiving( int rcvsock );

/* the sending process: the peerhostname may be a multicast address */
void sending_process( const char *peerhostname, int interval );

/* start the receiving process */
void receiving_process();

/* a function ref from unp converting inet family to int */
int family_to_level( int family );

/* a func ref from unp to get the locally bound socket name */
int sockfd_to_family( int sockfd );

/* a function ref from unp to set the multicast loopback option */
int mcast_set_loop( int sockfd, int onoff );

/* a function ref from unp for joining multicast interface */
int mcast_join( int sockfd, const struct sockaddr *grp, socklen_t grplen, const char *ifname, u_int ifindex );

/* test using multicast MCAST_ADDR */
void multicasting_process( const char *mcastaddr, int interval );

/* _PEER_CONN_H_ */
#endif 
