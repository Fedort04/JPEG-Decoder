assert.o: assert.c
	gcc -c -o assert.o assert.c

binReader.o: binReader.c
	gcc -c -o binReader.o binReader.c

background.o: background.c
	gcc -c -o background.o background.c

bin-read-test: binReader.o assert.o binReaderTest.c
	gcc -o binReaderTest binReader.o assert.o binReaderTest.c
	./binReaderTest

jpeg: binReader.o background.o main.c
	gcc -o main binReader.o background.o main.c
	./main
