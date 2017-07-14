compile: builtins.c lval.c mpc.c repl.c
	cc -g -I -std=c99 -Wall builtins.c lval.c mpc.c repl.c -ledit -lm -o repl

run: compile
	./repl
