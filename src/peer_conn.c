#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <time.h>
#include <string.h>

#include "../include/lc_samp.h"
#include "../include/sql_admin.h"
#include "../include/my_hosts.h"

#define CPORT "20121" /* progressive port number: sending periodically signals */

#define SPORT "20132" /* passive port number: listening for peers and send responses */
//#define MCAST_ADDR "239.225.0.1" /* site-local multicast address: 239.255.0.0 to 239.255.255.255 */

#define portlen 8 /* length of string to store port */

#define MAXLINE 80

/* Reference: Unix Network Programming www.unpbook.com/src.html */
/* server table is updated most frequently, using prepared statement */ 
#define PEERCONN_INSERT_STMT "insert into peer_connect( peer_name, peer_ip, avail_BW, curr_time ) values( ?,?,?,? )"

/* return readable string presentation of network address */
char*
socket_ntop( const struct sockaddr *sa )
{
    /* INET_ADDRSTRLEN = 16 */
    static char theip[INET_ADDRSTRLEN];

    char port[portlen];
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;

    if(inet_ntop(sin->sin_family, &sin->sin_addr, theip, sizeof(theip)) == NULL )
    {
        return NULL;
    }

    /* concatenate port number at the end*/
    /*
    if( ntohs(sin->sin_port) != 0 )
    {
        snprintf(port, sizeof(port), ":%d", ntohs(sin->sin_port));
        strcat( theip, port );
    }
    */

    return theip;
}


/* return an addrinfo structure from hostname, return NULL if error */
struct addrinfo*
get_hostaddr( const char* hostname, const char* service, int protocol_family, int socket_type )
{
    int result;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE; /* bind to ALL interfaces */
    hints.ai_family = protocol_family; /* AF_INET, AF_INET6, AF_UNSPEC */
    hints.ai_socktype = socket_type; /* SOCK_STREAM, SOCK_DGRAM, 0 */

    result = getaddrinfo( hostname, service, &hints, &res );
   
    if(result != 0)
    {
        perror( "getaddrinfo" );
        printf( "%s\n", gai_strerror(result) );
        return NULL;
    }else
    {
        return res; /* there is no guaranteed order in the returned list */
    }
}


/* return a sending socket to desthost bind to local CPORT
 * TODO: desthostname is received from peer signal
 */
int 
sending_socket( const char *desthostname, const char *serv, struct sockaddr **saptr, socklen_t *len ) 
/* copy sockaddr info from desthost to saptr and len */
{
    int sndsock;

    /* keep record of lists head for freeing later */
    struct addrinfo *local, *localcopy, *dest, *destcopy;
    char *myhostname;

    myhostname = NULL; /* bind to INADDR_ANY */
    local = get_hostaddr( myhostname, CPORT, AF_UNSPEC, SOCK_DGRAM );

    if(local == NULL)
    {
        exit(1);
    }
    localcopy = local;

    dest = get_hostaddr( desthostname, serv, AF_UNSPEC, SOCK_DGRAM );
    if(dest == NULL)
    {
        exit(1);
    }
    destcopy = dest; 

    do
    {
        /* create client socket using desthost information */
        sndsock = socket( dest->ai_family, dest->ai_socktype, dest->ai_protocol );
        if( sndsock < 0 )
        {
            continue;
        }

        /* bind sndsock to SPORT on localhost, otherwise it will be assigned random port */
        if( bind(sndsock, local->ai_addr, local->ai_addrlen) == 0 )
        {
            break;
        }

    }while( (dest = dest->ai_next) != NULL );

    /* make a deep copy of sockaddr info to papameters saptr and len */
    *saptr = malloc( dest->ai_addrlen );
    memcpy( *saptr, dest->ai_addr, dest->ai_addrlen );
    *len = dest->ai_addrlen;

    /* free using pointer to the head of lists */
    freeaddrinfo( localcopy );
    freeaddrinfo( destcopy );
    return sndsock;
}

/* compose signal to send (localhost name:port, pid) and send to peers sockaddr, which is a multicast address */
void 
start_announcing( int sndsock, struct sockaddr *peers_addr, int interval, socklen_t peersa_len )
{
    int sentbytes;
    char myinfo[MAXLINE];
    struct utsname myname;

    if( uname(&myname) < 0 )
    {
        perror("uname");
    }

    snprintf( myinfo, MAXLINE, "%s:%s,%d", myname.nodename, CPORT, getpid() );
    printf("Local info to send: %s\n", myinfo);

    for( ; ; )
    {
        sentbytes = sendto( sndsock, myinfo, strlen(myinfo), 0, peers_addr, peersa_len );
        if ( sentbytes <0 )
        {
            perror("sendto");
            break;
        }else
        {
            printf("sent a signal of %d bytes ...\n", sentbytes);
            sleep(interval);
        }
    }
}


/* return a listening socket bind to specific port serv, by default on localhost */
int 
listening_socket( const char *serv )
{
    int rcvsock;
    char *hostname; /* localhost name */
    struct addrinfo *local, *localcopy;

    hostname = NULL; /* bind to ADDR_ANY locally */

    local = get_hostaddr( hostname, serv, AF_UNSPEC, SOCK_DGRAM );
    if( local == NULL )
    {
        exit(1);
    }
   
    /* make a local shallow copy pointing to the first structure in the list */
    localcopy = local;
    
    do
    {
        rcvsock = socket ( local->ai_family, local->ai_socktype, local->ai_protocol );
        if(rcvsock < 0)
        {
            continue;
        }

        if( bind(rcvsock, local->ai_addr, local->ai_addrlen) == 0 )
        {
            break; /* bind success */
        }

    }while( ( local = local->ai_next ) != NULL );

    /* free using the copy since the function requires the first structure in the list */
    freeaddrinfo(localcopy);
    printf("receiving socket created on port %s on localhost\n", serv);

    return rcvsock;
}

/* extract peer hostname/ip from peer message/info */
char*
extract_peername( const char *msginfo, char *name, int maxlen )
{
    /* make a local copy */
    char *copy = strdup(msginfo);
    char *sepmark = strchr(copy, ':'); 

    int namelen;
    namelen = (sepmark - copy);

    if(namelen >= maxlen)
    {
        namelen = maxlen-1;
    }
  
    bcopy(copy, name, namelen);
    name[namelen] = '\0';
    free(copy);    
    return name;
}

/* receive datagrams unicast or multicast, MYSQL is connected */
void 
start_receiving( int rcvsock )
{

    int rcvbytes;
    char peermsg[MAXLINE]; /* the message from peer hostname:portnum,pid */
    struct sockaddr_storage *peer_addr; /* store peer host addr info */
    
    socklen_t salen;
    peer_addr = malloc(salen);

    /* myname.nodename is my hostname */
    struct utsname myname;

    if( uname(&myname) < 0 )
    {
        perror("uname");
    }

    for( ; ; )
    {
        printf("listening to peers ...\n");
        salen = sizeof( struct sockaddr_storage ); 

        /* init salen with max size */
        rcvbytes = recvfrom( rcvsock, peermsg, MAXLINE, 0, (struct sockaddr *)peer_addr, &salen );
        if(rcvbytes < 0)
        {
            perror("recvfrom");
            exit(1);
        }
        peermsg[rcvbytes] = '\0'; /* null terminate */
        printf("Received message %s\n", peermsg);
    
        char myip[NI_MAXHOST]; 
        char *peerip; /* the peer ip */
        char peername[MAXLINE]; /* the peer hostname */

        if( get_IP_addr( myip, AF_INET ) == NULL )
        {
            printf("%s\n", "error when getting localhost ip address");
            exit(1);
        }
        /* TODO: verify this IS a peer by check CPORT in msg is 20121*/
        //char *sigcport;
        peerip = socket_ntop( (struct sockaddr *)peer_addr);
        
        if( extract_peername( peermsg, peername, sizeof(peername) ) == NULL )
        {
            printf("%s\n", "error when extracting peer hostname");
            exit(1);
        }

        printf( "MY IP %s\n", myip);
        printf( "MY NAME %s\n", myname.nodename);
        printf( "PEER IP %s\n", peerip);
        printf( "PEER NAME %s\n", peername );
        
        //printf( "carrying message %s\n", peermsg );

        /* db is already connected
         * add a row to peer_connect table
         * the first row for newly presented peer has bw 0
         * start bandwidth measuring towards each peer periodically
         */

        //prepare_exec_peerconninsert(conn); 
    }
}


/* a function ref from unp converting inet family to int */
int
family_to_level(int family)
{
	switch (family) 
    {
	    case AF_INET:
	    	return IPPROTO_IP;
        #ifdef	IPV6
	    case AF_INET6:
		    return IPPROTO_IPV6;
        #endif /* IPV6 */
	    default:
		    return -1;
	}
}


/* a func ref from unp to get the locally bound socket name */
int
sockfd_to_family(int sockfd)
{
	struct sockaddr_storage ss;
	socklen_t	len;

	len = sizeof(ss);
	if (getsockname(sockfd, (struct sockaddr *) &ss, &len) < 0)
		return(-1);
	return(ss.ss_family);
}
/* end sockfd_to_family */


/* the sending process: the peerhostname could be a multicast address */
void 
sending_process( const char *peerhostname, int interval )
{
    int sndsock;
    socklen_t salen;
    struct sockaddr *peeraddr;

    /* fill in the info */
    sndsock = sending_socket( peerhostname, SPORT, &peeraddr, &salen );
    start_announcing( sndsock, peeraddr, interval, salen );
    close(sndsock);
}

void 
receiving_process()
{
    int rcvsock;
    rcvsock = listening_socket( SPORT );
    start_receiving( rcvsock );
    close(rcvsock);
}

/* start the receiving process */
void 
receiving_process_SQL()
{
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
    if( mysql_real_connect(conn, my_host_name, my_user_name, my_password, \
        my_db_name, my_port_num, my_socket_name, my_flags ) == NULL )
    {
        fprintf(stderr, "mysql_real_connect() failed: \n%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }
     /* use created database */
    if(mysql_query(conn, "use prototype"))
    {
        fprintf(stderr, "use database failed: \n%s\n", mysql_error(conn));
        exit(1);
    }
    /* connection established */

    int rcvsock;
    rcvsock = listening_socket( SPORT );

    /* start listening and fill in peer_conn table */
    start_receiving( rcvsock );
    close(rcvsock);

    /* close db connection */
    mysql_close(conn);
    mysql_library_end();   

}


void
conn_process( const char *peeraddr, int interval )
{
    int sndsock, rcvsock;
    const int on = 1;

    socklen_t salen;
    struct sockaddr *paddr, *sarecv;

    /* fill in the info */
    sndsock = sending_socket( peeraddr, SPORT, &paddr, &salen );

    rcvsock = socket( paddr->sa_family, SOCK_DGRAM, 0 );
    if( rcvsock < 0 )
    {
        perror("socket");
        exit(1);
    }

    setsockopt( rcvsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
    sarecv = malloc( salen );
    memcpy( sarecv, paddr, salen ); /* copy the peer addrinfo to sarecv */
    if( bind( rcvsock, sarecv, salen ) < 0 )
    {
        perror("receiving socket binding");
        exit(1);
    }

    if( fork() == 0 )
    {
        start_receiving( rcvsock );
    }

    /* TODO need to include multiple peer addrs */
    start_announcing( sndsock, paddr, interval, salen );
    close(sndsock);

    close(rcvsock);
}
