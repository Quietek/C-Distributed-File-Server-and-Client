all: DFC.c DFS.c
	gcc -lssl -lcrypto -Wextra -Wall -o DFC DFC.c
	gcc -o DFS DFS.c
clean:
	$(RM) DFC DFS
