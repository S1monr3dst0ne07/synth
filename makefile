

run: build
	 adapt/virt-keyboard.sh | ./main | ./adapt/vban

build:
	gcc -g3 src/main.c -o main -lm
