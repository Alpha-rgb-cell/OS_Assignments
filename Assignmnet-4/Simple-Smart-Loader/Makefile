all:
	gcc  -m32 -no-pie -nostdlib -o fib fib.c
	gcc -m32 -o loader loader.c
	gcc -m32 -o large_segment large_segment.c
	gcc -m32 -o sum sum.c 


clean:
	-@rm -f fib loader
