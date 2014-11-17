CC=gcc
CFLAGS=-O2 -Wall

AR=ar
AROPTS=rs

LIB_DIR=../lib/
EXE_DIR=../bin/

SQL_INCS=${shell mysql_config --include}
SQL_LIBS=${shell mysql_config --libs}

lc_samp.o: lc_samp.c
	${CC} ${CFLAGS} ${SQL_INCS} -c lc_samp.c

sql_table.o: sql_table.c
	${CC} ${CFLAGS} ${SQL_INCS} -c sql_table.c

sql_table: sql_table.o
	${CC} ${CFLAGS} ${SQL_INCS} -o ${EXE_DIR}sql_table sql_table.c ${SQL_LIBS}

bw_funcs: bw_funcs.c
	${CC} ${CFLAGS} -L${LIB_DIR} -o ${EXE_DIR}bw_funcs bw_funcs.c ${LIB_DIR}libiperf.a

peer_conn.o: peer_conn.c
	${CC} ${SQL_INCS} -c peer_conn.c

util_funcs.a: lc_samp.o peer_conn.o
	${AR} ${AROPTS} ${LIB_DIR}util_funcs.a lc_samp.o peer_conn.o

app_start: app_start.c util_funcs.a
	${CC} ${CFLAGS} ${SQL_INCS} -o ${EXE_DIR}app_start app_start.c ${LIB_DIR}util_funcs.a ${SQL_LIBS}

clean:
	rm -f *.~ .*.swp *.o *.a
