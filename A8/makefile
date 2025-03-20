all:
	@if [ ! -d "input" ]; then $(MAKE) db; fi
	g++ -Wall -o resource -pthread resource.cpp
	g++ -Wall -D_DLAVOID -o resource_nodeadlock -pthread resource.cpp

allow: all
	./resource

avoid: all
	./resource_nodeadlock

db: geninput.c
	gcc -Wall -o geninput geninput.c
	mkdir -p input
	./geninput 10 20

clean:
	-rm -f resource resource_nodeadlock geninput