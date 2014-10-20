CC=gcc
INCLUDES=${shell mysql_config --include}
LIBS=${shell mysql_config --libs}

CFLAGS= -g -Wall

all: lc_samp sql_table

lc_samp: lc_samp.c
	${CC} ${CFLAGS} -o lc_samp lc_samp.c

sql_table: sql_table.c
	${CC} ${CFLAGS} ${INCLUDES} -o sql_table sql_table.c ${LIBS}

clean:
	rm -f lc_samp sql_table *.~ .*.swp *.o
