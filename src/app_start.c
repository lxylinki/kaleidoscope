#include <my_global.h>
#include <my_sys.h>
#include <mysql.h>
#include <sys/socket.h>

#include "../include/lc_samp.h"
#include "../include/peer_conn.h"

int main() {
    daemon_process("127.0.0.1", 3);
    exit(EXIT_SUCCESS);
}
