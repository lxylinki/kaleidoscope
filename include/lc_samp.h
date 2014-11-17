#ifndef _LC_SAMP_h_
#define _LC_SAMP_h_

/* result structure */
typedef struct server_sample {
    char* host_name;
    char* ip_addr;
//    char* mac_addr;
    char* ip6_addr;
    float CPU_scale;
    long avail_RAM;
    long avail_disk;
    float NIC_load;
    float CPU_temp;
//    struct tm* curr_time;
    MYSQL_TIME *curr_time;
//    float CPU_capacity;
    int core_num;
} server_row;


char* collect_hostname();

char* get_IP_addr(char* ip_addr, const int protocol_family);

char* get_NIC_name();

long collect_ram();

long collect_disk();

int collect_procs();

void find_NIC_dir(char* rxstats, char* txstats);

float collect_nicload();

float collect_cpu();

float collect_cputemp();

struct tm* collect_currenttime();

server_row* collect_server_row();

#endif
