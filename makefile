

run: build
	 adapt/virt-keyboard.sh | ./main

build:
	gcc -g3 src/main.c -o main -lm
