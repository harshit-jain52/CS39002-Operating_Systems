IPC_FILES := a6_mutex a6_cook a6_waiter a6_customer a6_shm
TEXT_FILE := customers.txt
CC := gcc
CFLAGS := -g -I. -Wall -L.
NAME = barfoobar
LIBNAME = lib$(NAME).a 
OBJFILES = $(NAME).o

library: $(OBJFILES)
	ar rs $(LIBNAME) $(OBJFILES)

$(OBJFILES): barfoobar.h barfoobar.c

all: library cook.c waiter.c customer.c
	$(CC) $(CFLAGS) -o cook cook.c -l$(NAME)
	$(CC) $(CFLAGS) -o waiter waiter.c -l$(NAME)
	$(CC) $(CFLAGS) -o customer customer.c -l$(NAME)

db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt

run: all
	@if [ ! -f $(TEXT_FILE) ]; then \
		echo "$(TEXT_FILE) not found, running 'make db'..."; \
		make db; \
	fi
	@touch $(IPC_FILES)
	gnome-terminal --title="Cook" --geometry=80x24+0+0 -- bash -c "./cook; exec bash"
	sleep 1
	gnome-terminal --title="Waiter" --geometry=80x24+850+0 -- bash -c "./waiter; exec bash"
	gnome-terminal --title="Customer" --geometry=80x24+400+500 -- bash -c "./customer; exec bash"

clean:
	-rm -f cook waiter customer gencustomers
	-rm -f $(IPC_FILES) customers.txt
	-rm -f $(LIBNAME) $(OBJFILES)
