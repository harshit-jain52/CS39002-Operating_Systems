TEXT_FILE = search.txt

run: LRU.cpp checkdb
	g++ -Wall -o runsearch LRU.cpp
	./runsearch

vrun: LRU.cpp checkdb
	g++ -Wall -DVERBOSE -o runsearch LRU.cpp
	./runsearch

db: gensearch.c
	gcc -Wall -o gensearch gensearch.c
	./gensearch

checkdb:
	@if [ ! -f $(TEXT_FILE) ]; then \
		echo "$(TEXT_FILE) not found, running 'make db'..."; \
		make db; \
	fi

clean:
	rm -f runsearch gensearch