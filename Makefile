PORT=50079
CFLAGS= -DPORT=\$(PORT) -g -std=gnu99 -Wall -Werror

friends_server: friends_server.o friends.o
	gcc ${CFLAGS} -o $@ $^

friends.o : friends.c friends.h
	gcc ${CFLAGS} -c $<

friends_server.o: friends_server.c friends.h
	gcc ${CFLAGS} -c $<

clean:
	rm -f *.o friends_server