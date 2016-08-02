all:
	gcc -I include -Wall src/main.c -lm -o bin/rinex2ubx
