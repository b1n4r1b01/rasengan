all:	liblzfse.a
	clang -c rasengan.c -o rasengan.o
	clang rasengan.o liblzfse.a -o rasengan
	rm -rf lzfse
	rm *.o

liblzfse.a: lzfse/src
	cd lzfse; make; cp build/bin/liblzfse.a ../; cd ..
lzfse/src:
	git submodule init
	git submodule update

clean:
	rm *.o rasengan *.a Apple*
	
