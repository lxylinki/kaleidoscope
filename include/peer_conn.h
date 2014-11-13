/* functions used in peer connection */
#ifndef _PEER_CONN_H_
#define _PEER_CONN_H_

typedef struct peer_connect_row
{
    char* peer_name;
    char* peer_ip;
    float avail_BW;
    MYSQL_TIME* curr_time;
} peertable_row;

/* SQL routines */
//TODO
void bind_and_exec_peerconninsert( MYSQL* conn, MYSQL_STMT* peerconn_insert, const peertable_row* row );
void prepare_exec_peerconninsert( MYSQL* conn, const peertable_row* row );

/* return readable string presentation of network address */
char* socket_ntop( const struct sockaddr* sa );

/* return peer hostname/ip from peer message/info */
char* extract_peername( const char* msginfo, char* name, int maxlen );

/* return an addrinfo structure from hostname, return NULL if error */
struct addrinfo* get_hostaddr( const char* hostname, const char* service, int protocol_family, int socket_type );

/* return a sending socket to desthost bind to local CPORT */
/* copy sockaddr info from desthost to saptr and len */
int sending_socket( const char* desthostname, const char* serv, struct sockaddr** saptr, socklen_t* len ); 

/* compose signal to send (localhost name:port, pid) and send to peers sockaddr, which can be a multicast address */
void start_announcing( int sndsock, struct sockaddr* peers_addr, int interval, socklen_t peersa_len );

/* return a listening socket bind to specific port serv, by default on localhost */
int listening_socket( const char* serv );

/* receive datagrams unicast or multicast */
void start_receiving( int rcvsock );

/* the sending process: the peerhostname may be a multicast address */
void sending_process( const char* peerhostname, int interval );

/* start the receiving process */
void receiving_process();

/* start the receiving process with SQL connection*/
void receiving_process_SQL();

/* a function ref from unp converting inet family to int */
int family_to_level( int family );

/* a func ref from unp to get the locally bound socket name */
int sockfd_to_family( int sockfd );

/* start sending and unicasting*/
void daemon_process(const char* peeraddr, int interval);

/* _PEER_CONN_H_ */
#endif 
