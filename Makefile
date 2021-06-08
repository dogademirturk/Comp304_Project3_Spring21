all: install

install:
	gcc part1.c -o part1  && gcc part2.c -o part2 

clean:
	rm part1 && rm part2

p1:
	gcc part1.c -o part1  && ./part1 ./BACKING_STORE.bin addresses.txt

p2fifo:
	gcc part2.c -o part2  && ./part2 ./BACKING_STORE.bin addresses.txt -p 0

p2lru: 
	gcc part2.c -o part2  && ./part2 ./BACKING_STORE.bin addresses.txt -p 1


