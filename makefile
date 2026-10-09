

run: build
	./main

build:
	gcc -g3 src/main.c -o main -lm
