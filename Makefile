client:
	gcc -I . client.c -o client

server:
	gcc -I . server.c -o server 

clean:
	-rm -f .*o