all: spawner server 

spawner: spawner.c
	gcc -I$(PMIX_ROOT)/include -Wall -o spawner spawner.c -L$(PMIX_ROOT)/lib -lpmix
server: server.c
	gcc -I$(PMIX_ROOT)/include -Wall -o server server.c -L$(PMIX_ROOT)/lib -lpmix

clean:
	rm ./*.txt ./spawner ./server
