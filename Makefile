CC=g++
CFLAG=-g -lpthread -std=c++11 -W -I.



myserver:mylog myepoll http_conn main
	$(CC) $(CFLAG) -o myserver mylog.o myepoll.o http_conn.o main.o

mylog:logqueue.h mylog.h
	$(CC) $(CFLAG) -c mylog.cpp -o mylog.o
myepoll:myepoll.h
	$(CC) $(CFLAG) -c myepoll.cpp -o myepoll.o

http_conn:mylog.h mylocker.h timer_heap.h myepoll.h http_conn.h
	$(CC) $(CFLAG) -c http_conn.cpp -o http_conn.o

main:mylog.h mylocker.h threadpool.h http_conn.h timer_heap.h myepoll.h 
	$(CC) $(CFLAG) -c main.cpp -o main.o

clean:
	rm -f *.o
