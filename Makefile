compile: lval.c mpc.c repl.c
	cc -I -std=c99 -Wall lval.c mpc.c repl.c -ledit -lm -o repl

run: compile
	./repl
