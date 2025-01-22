compile: schedule.c
	gcc -Wall -o schedule schedule.c -lm

run: compile
	./schedule > output.txt

vcompile: schedule.c
	gcc -Wall -o schedule -DVERBOSE schedule.c -lm

vrun: vcompile
	./schedule > verbose_output.txt

db: genproc.c
	gcc -Wall -o genproc genproc.c

clean:
	-rm -f genproc schedule proc.txt output.txt verbose_output.txt