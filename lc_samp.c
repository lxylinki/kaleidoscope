#include<time.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<net/if.h>
#include<netdb.h>
#include<ifaddrs.h>

#define MAXLINE 80

/* result structure */
typedef struct server_sample {
    char* host_name;
    char* ip_addr;
    char* mac_addr;
    float CPU_scale;
    long avail_RAM;
    long avail_disk;
    float NIC_load;
    float CPU_temp;
    struct tm* curr_time;
    float CPU_capacity;
    int core_num;
} server_sample;

/* return local hostname */
char* collect_hostname() {
    char* HOSTNAME_CMD = "hostname";
    FILE* info;
    char temp_name[MAXLINE];
    static char host_name[MAXLINE];
    
    if( (info = popen( HOSTNAME_CMD,"r")) == NULL )
    {
        fprintf(stderr, "popen failed.");
    }
    
    if( fgets( temp_name, MAXLINE, info) == NULL )
    {
        fprintf(stderr, "fgets failed.");
    }
    
    if( pclose(info) == -1 )
    {
        fprintf(stderr, "pclose failed.");
    }
    
    sscanf(temp_name, "%s %*s", &host_name); 
    return host_name;
}

/* find the ipv4/6 address of active nic (AF_INET, AF_INET6) */
void get_IP_addr(char *ip_addr, const int protocol_family) {
    struct ifaddrs *ifaddr, *ifa;
    static char hostip[NI_MAXHOST];

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for( ifa = ifaddr; ifa!=NULL; ifa=ifa->ifa_next )
    {
        if(ifa->ifa_addr==NULL)
            continue;

        int family;
        family = ifa->ifa_addr->sa_family;

        if(family != protocol_family)
            continue;

        if(!(ifa->ifa_flags & IFF_LOOPBACK))
        {
            if(ifa->ifa_flags & IFF_RUNNING)
            {
                int s;
                s = getnameinfo(ifa->ifa_addr,
                        (protocol_family==AF_INET)?sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6),
                        hostip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if(s!=0)
                {
                    printf("getnameinfo error: %s\n", gai_strerror(s));
                    exit(EXIT_FAILURE);
                }
                int addrlen = strlen(hostip);
                bcopy(hostip, ip_addr, addrlen);
                ip_addr[addrlen] = '\0';
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    if(ip_addr==NULL)
    {
        printf("%s\n", "error when getting ip address");
        exit(EXIT_FAILURE);
    }
    //return ip_addr;
}

/* find the first active NIC name: TODO there can be multiple active NICs */
char* get_NIC_name() {
    struct ifaddrs *ifaddr, *ifa;
    static char *nicname;

    if(getifaddrs(&ifaddr)== -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for( ifa = ifaddr; ifa!=NULL; ifa=ifa->ifa_next )
    {
        if(ifa->ifa_addr==NULL)
            continue;

        if(!(ifa->ifa_flags & IFF_LOOPBACK) )
        {
            if(ifa->ifa_flags & IFF_RUNNING)
            {
                nicname = ifa->ifa_name;
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
    return nicname;
}

/* RAM size */
long int collect_ram() {
    FILE* info;
    long int ramsize;
    char raminfo[MAXLINE];
    char* MEM_CMD = "grep 'MemFree' /proc/meminfo";
    
    if( (info = popen( MEM_CMD ,"r")) == NULL )
    {
        fprintf(stderr, "popen failed when get ram.");
    }
    while( fgets( raminfo, MAXLINE, info) != NULL )
    {
        sscanf(raminfo,"%*s %ld", &ramsize);
    }
    if( pclose(info) == -1 )
    {
        fprintf(stderr, "pclose failed when get ram.");
    }
    return ramsize;
}

/* disk vol */
long int collect_disk() {
    FILE* info;
    static char diskinfo[MAXLINE];
    long int diskspace = 0;
    long int tmp = 0;
    int count = 0;
    char* DISK_CMD = "df";
    
    if( (info = popen( DISK_CMD ,"r")) == NULL )
    {
        fprintf(stderr, "popen failed.");
    }
    
    while( fgets( diskinfo, MAXLINE, info) != NULL )
    {
        tmp = 0;

        if(count == 0)
        {
            count ++ ;
            continue;
        }else
        {
             sscanf(diskinfo,"%*s %*ld %*ld %ld", &tmp);
             diskspace += tmp;
             count ++ ;
        }
    }

    if( pclose(info) == -1 )
    {
        fprintf(stderr, "pclose failed.");
    }

    return diskspace;
}

int collect_procs() {
    return get_nprocs();
}

void find_NIC_dir(char* rxstats, char* txstats) {
    char* nic_name;
    nic_name = get_NIC_name();
    
    char* NIC_prefix = "/sys/class/net/";
    // received bytes
    char* NICrx_suffix = "/statistics/rx_bytes";
    // transmitted bytes
    char* NICtx_suffix = "/statistics/tx_bytes";

    strncpy(rxstats, NIC_prefix, strlen(NIC_prefix));
    strncat(rxstats, nic_name, strlen(nic_name));
    strncat(rxstats, NICrx_suffix, strlen(NICrx_suffix));
    
    strncpy(txstats, NIC_prefix, strlen(NIC_prefix));
    strncat(txstats, nic_name, strlen(nic_name));
    strncat(txstats, NICtx_suffix, strlen(NICtx_suffix));
    //printf("%s\n%s\n", rxstats, txstats);
}

/* return number of kilobytes per second */
float collect_nicload() {
    static char rxstats[MAXLINE];
    static char txstats[MAXLINE];
    find_NIC_dir(rxstats, txstats);

    int interval = 2; // the time interval in seconds
    int samplenum = 2; // take 2 samples with 1 second interval 
    float datarate = 0;
    int i; // loop index 
    
    FILE *rxbytes, *txbytes;
    long int rec[samplenum], send[samplenum];
    
    for(i=0; i<samplenum; i++)
    {
        rec[i] = 0;
        send[i] = 0;

        if( (rxbytes = fopen(rxstats, "r")) == NULL)
        {
            fprintf(stderr, "NIC rx stats file open failed");
        }

        fscanf(rxbytes, "%ld", &rec[i]);

        if( (txbytes = fopen(txstats, "r")) == NULL)
        {
            fprintf(stderr, "NIC tx stats file open failed");
        }

        fscanf(txbytes, "%ld", &send[i]);  
        fclose(rxbytes);
        fclose(txbytes);
       
        if( i == 0 )
        {
            sleep(interval);

        }else
        {
            datarate = rec[i] + send [i] - rec[i-1] - send[i-1];
            datarate = ( datarate / interval );
        }
    }
    datarate = (datarate / 1000) ; // in kilobytes per second
    return datarate;
}

char* find_MAC_addr()
{
    static char macaddr[MAXLINE];
    static char getmaccmd1[MAXLINE];
    static char getmaccmd2[MAXLINE];
    
    char* nicname; 
    nicname = get_NIC_name();
    
    char* getmac_prefix = "ifconfig ";
    // redhad cmd
    char* getmac_suffix1 = " | grep 'ether'";
    // debian cmd
    char* getmac_suffix2 = " | grep 'HWaddr'";

    strncpy( getmaccmd1, getmac_prefix, strlen(getmac_prefix) );
    strncpy( getmaccmd2, getmac_prefix, strlen(getmac_prefix) );

    strncat( getmaccmd1, nicname, strlen(nicname) );
    strncat( getmaccmd2, nicname, strlen(nicname) );
    
    /* concatenate suffix */
    strncat( getmaccmd1, getmac_suffix1, strlen(getmac_suffix1) );
    strncat( getmaccmd2, getmac_suffix2, strlen(getmac_suffix2) );
    //printf("MAC addr cmd1: %s\n", getmaccmd1);
    //printf("MAC addr cmd2: %s\n", getmaccmd2);
   
    FILE* info;
    char mac[MAXLINE];
    
    if( (info = popen( getmaccmd1,"r")) == NULL )
    {
        fprintf(stderr, "popen failed.");
    }
    
    if( fgets( mac, MAXLINE, info) == NULL )
    {
        //fprintf(stderr, "try cmd type2 ..\n");

        if( (info = popen( getmaccmd2,"r")) == NULL )
        {
            fprintf(stderr,"popen failed.");
        }

        if( fgets( mac, MAXLINE, info) == NULL )
        {
            fprintf(stderr, "fgets failed for cmd type2.");
        }
        
        if( pclose(info) == -1 )
        {
            fprintf(stderr, "pclose failed.");
        }
    
        sscanf(mac, "%*s %*s %*s %*s %s", &macaddr); 

    }else
    {
        if( pclose(info) == -1 )
        {
            fprintf(stderr, "pclose failed.");
        }
    
        sscanf(mac, "%*s %s", &macaddr); 
    }
    return macaddr;
}

float collect_cpu() {
    char* LOAD_CMD = "lscpu | grep 'CPU MHz'";
    FILE* info;
    float current_load;
    char temp_info[MAXLINE];

    if ((info = popen(LOAD_CMD, "r")) == NULL) {
        fprintf(stderr, "popen failed when get cpu load.");
    }
    if ((fgets(temp_info, MAXLINE, info)) == NULL) {
        fprintf(stderr, "fgets failed when get cpu load.");
    }
    sscanf(temp_info, "%*s %*s %f", &current_load);
    if (pclose(info) == -1) {
        fprintf(stderr, "pclose failed when get cpu load.");
    }
    return current_load;
}

float collect_max_cpu() {
    char* LOAD_CMD = "lscpu | grep 'CPU max MHz'";
    FILE* info;
    float current_load;
    char temp_info[MAXLINE];

    if ((info = popen(LOAD_CMD, "r")) == NULL) {
        fprintf(stderr, "popen failed when get cpu load.");
    }
    if ((fgets(temp_info, MAXLINE, info)) == NULL) {
        fprintf(stderr, "fgets failed when get cpu load.");
    }
    sscanf(temp_info, "%*s %*s %*s %f", &current_load);
    if (pclose(info) == -1) {
        fprintf(stderr, "pclose failed when get cpu load.");
    }
    return current_load;
}

float collect_min_cpu() {
    char* LOAD_CMD = "lscpu | grep 'CPU min MHz'";
    FILE* info;
    float current_load;
    char temp_info[MAXLINE];

    if ((info = popen(LOAD_CMD, "r")) == NULL) {
        fprintf(stderr, "popen failed when get cpu load.");
    }
    if ((fgets(temp_info, MAXLINE, info)) == NULL) {
        fprintf(stderr, "fgets failed when get cpu load.");
    }
    sscanf(temp_info, "%*s %*s %*s %f", &current_load);
    if (pclose(info) == -1) {
        fprintf(stderr, "pclose failed when get cpu load.");
    }
    return current_load;
}

/* return the highest tempreture among all cores */
float collect_cputemp() {
    FILE* info;
    float cputemp = 0;  
    char sensorinfo[MAXLINE];
    char* CORETEMP_CMD = "sensors | grep 'Core'";

    if( (info = popen( CORETEMP_CMD ,"r")) == NULL )
    {
        fprintf(stderr, "popen failed when getting cpu temprature.");
    }
    
    while( fgets( sensorinfo, MAXLINE, info) != NULL )
    {
        float prev;
        sscanf(sensorinfo,"%*s %*s%f", &prev);

        if( cputemp < prev )
        {
            cputemp = prev; // return the highest temp among cores
        }
    }

    if( pclose(info) == -1 )
    {
        fprintf(stderr, "pclose failed when get cpu temprature.");
    }
    return cputemp;
}

/* get local time as a tm structure */
struct tm* collect_currenttime() {
    static struct tm *nowtime;
    time_t now = time(NULL);
    nowtime = localtime(&now);
    printf("%s\n", asctime(nowtime));
    return nowtime;
}


/* convert into mysql time
MYSQL_TIME* get_mysqltime(struct tm* nowtime) {
    static MYSQL_TIME mytime; 
    mytime.year = nowtime->tm_year + 1900;
    mytime.month = nowtime->tm_mon + 1;
    mytime.day = nowtime->tm_mday;
    mytime.hour = nowtime->tm_hour;
    mytime.minute = nowtime->tm_min;
    mytime.second = nowtime->tm_sec;
    mytime.neg = nowtime->tm_isdst;
    return &mytime;
}*/

int main() {
    char* my_name;
    my_name = collect_hostname();

    char my_addr[NI_MAXHOST];
    get_IP_addr(my_addr, AF_INET);
    
    char* my_nic;
    my_nic = get_NIC_name();
    
    long my_ram;
    my_ram = collect_ram();

    int my_procs;
    my_procs = collect_procs();

    long my_disk;
    my_disk = collect_disk();

    float nic_load;
    nic_load = collect_nicload();

    char* my_MAC;
    my_MAC = find_MAC_addr();

    struct tm* my_time;
    my_time = collect_currenttime();

    printf("HOST\t%s\nIP(v4)\t%s\nNIC\t%s\nRAM\t%ldkB\nDISK\t%ldkB\nPROCS\t%d\nNIC_LOAD\t%.2fkB/s\nMAC\t%s\n", 
            my_name, my_addr, my_nic, my_ram, my_disk, my_procs, nic_load, my_MAC);
    
    float current_cpu;
    current_cpu = collect_cpu();
    printf("CPU\t%.2fMHz\n", current_cpu);

    float max_cpu;
    max_cpu = collect_max_cpu();
    printf("MAX_CPU\t%.2fMHz\n", max_cpu);

    float min_cpu;
    min_cpu = collect_min_cpu();
    printf("MIN_CPU\t%.2fMHz\n", min_cpu);

    float cpu_load;
    cpu_load = (current_cpu/max_cpu);
    printf("CPU_LOAD\t%.2f\n", cpu_load);
    
    return EXIT_SUCCESS;
}
