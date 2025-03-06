IPC_FILES := a6_mutex a6_cook a6_waiter a6_customer a6_shm

all:
	gcc -Wall -o cook cook.c
	gcc -Wall -o waiter waiter.c
	gcc -Wall -o customer customer.c

db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt


run: all db
	touch $(IPC_FILES)
	gnome-terminal --title="Cook" -- bash -c "./cook; exec bash"
	sleep 1
	gnome-terminal --title="Waiter" -- bash -c "./waiter; exec bash"
	sleep 1
	gnome-terminal --title="Customer" -- bash -c "./customer; exec bash"


clean:
	-rm -f cook waiter customer gencustomers
	-rm -f $(IPC_FILES) customers.txt
