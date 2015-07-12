all: cg200.o bmpread.o
	gcc -o cg200 cg200.o bmpread.o -lglut -lGLU -lGL
bmpread.o: bmpread.c bmpread.h
	gcc -c bmpread.c
cg200.o: bmpread.h cg200.c cg200.h
	gcc -c cg200.c -lglut -lGL -lGLU
clean:
	rm *.o cg200