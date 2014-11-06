CC=gcc
CFLAGS=-O2 -Wall

AR=ar
AROPTS=rs

LIB_DIR=../lib/
EXE_DIR=../bin/

SQL_INCS=${shell mysql_config --include}
SQL_LIBS=${shell mysql_config --libs}

obs: lc_samp.o sql_table.o bw_funcs.o
libs: lc_samp.a
execs: lc_samp sql_table bw_funcs

lc_samp.o: lc_samp.c
	${CC} -c lc_samp.c

lc_samp.a: lc_samp.o
	${AR} ${AROPTS} ${LIB_DIR}lc_samp.a lc_samp.o

lc_samp: lc_samp.c
	${CC} ${CFLAGS} -o ${EXE_DIR}lc_samp lc_samp.c

sql_table.o: sql_table.c
	${CC} ${CFLAGS} ${SQL_INCS} -c sql_table.c

sql_table: sql_table.o
	${CC} ${CFLAGS} ${SQL_INCS} -o ${EXE_DIR}sql_table sql_table.c ${SQL_LIBS}

bw_funcs: bw_funcs.c
	${CC} ${CFLAGS} -L${LIB_DIR} -o ${EXE_DIR}bw_funcs bw_funcs.c ${LIB_DIR}libiperf.a

peer_conn.o: peer_conn.c
	${CC} ${SQL_INCS} -c peer_conn.c

peer_conn.a: peer_conn.o
	${AR} ${AROPTS} ${LIB_DIR}peer_conn.a peer_conn.o


clean:
	rm -f lc_samp 
	rm -f sql_table
	rm -f *.~ .*.swp *.o *.a
