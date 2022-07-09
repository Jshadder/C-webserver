CC=g++
CFLAG=-g -lpthread -std=c++11 -W -I.



myserver:myepoll http_conn main
	$(CC) $(CFLAG) -o myserver myepoll.o http_conn.o main.o

myepoll:myepoll.h
	$(CC) $(CFLAG) -c myepoll.cpp -o myepoll.o

http_conn:mylocker.h timer_heap.h myepoll.h http_conn.h
	$(CC) $(CFLAG) -c http_conn.cpp -o http_conn.o

main:mylocker.h threadpool.h http_conn.h timer_heap.h myepoll.h 
	$(CC) $(CFLAG) -c main.cpp -o main.o

clean:
	rm -f *.o