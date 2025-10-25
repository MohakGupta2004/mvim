build:
	mkdir bin; gcc mvim.c -o ./bin/mvim
run:
	gcc mvim.c -o /tmp/mvim && /tmp/mvim
