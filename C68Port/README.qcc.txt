You need to compile with:

	qcc -o whatever -Iqdos/include/ -Lqdos/lib/ whatever.c

however, sometimes qld won't link crt.o. In that case:

	qld *.o -Lqdos.lib/ -o whatever.o

usually works.

I think the trailing slash on the lib directory is the problem. Miss it out and it fails.

