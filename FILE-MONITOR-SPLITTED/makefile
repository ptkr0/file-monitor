filemonitor: main.o skeleton.o readdir.o filefunc.o
	gcc main.o skeleton.o readdir.o filefunc.o -o filemonitor -lcrypto

main.o: main.c common.h skeleton.h readdir.h filefunc.h
	gcc -c main.c

skeleton.o: skeleton.c skeleton.h common.h
	gcc -c skeleton.c

filefunc.o: filefunc.c filefunc.h common.h readdir.h
	gcc -c filefunc.c -lcrypto

readdir.o: readdir.c readdir.h common.h filefunc.h
	gcc -c readdir.c 

clean: 
	rm -f filemonitor main.o skeleton.o readdir.o filefunc.o
