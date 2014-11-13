#ifndef _LC_SAMP_h_
#define _LC_SAMP_h_

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

#endif
