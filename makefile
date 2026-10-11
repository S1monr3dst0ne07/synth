

run: build-yascl
	#./synth
	 adapt/virt-keyboard.sh | ./synth | ./adapt/vban

build-yascl:
	./yascl src/main.yap ./synth

build-c:
	gcc -g3 main.c -o synth -lm
