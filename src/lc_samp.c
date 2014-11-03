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
    printf("%s", asctime(nowtime));
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
    printf("my name is %s\n", my_name);

    char my_addr[NI_MAXHOST];
    get_IP_addr(my_addr, AF_INET);
    printf("my v4 addr is %s\n", my_addr);
    
    char my_addr6[NI_MAXHOST];
    get_IP_addr(my_addr6, AF_INET6);
    printf("my v6 addr is %s\n", my_addr6);
    
    char* my_nic;
    my_nic = get_NIC_name();
    printf("my network card is %s\n", my_nic);
    
    long my_ram;
    my_ram = collect_ram();
    printf("avail ram: %dkB\n", my_ram);

    int my_procs;
    my_procs = collect_procs();
    printf("processors: %d\n", my_procs);

    long my_disk;
    my_disk = collect_disk();
    printf("disk space: %d\n", my_disk);

    float nic_load;
    nic_load = collect_nicload();
    printf("data rate: %.2f\n", nic_load);

    struct tm* my_time;
    my_time = collect_currenttime();

    float current_cpu;
    current_cpu = collect_cpu();
    printf("CPU\t%.2fMHz\n", current_cpu);

    float current_temp;
    current_temp = collect_cputemp();
    printf("CPU\t%.2fC\n", current_temp);
    
    return EXIT_SUCCESS;
}
