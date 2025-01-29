compile: schedule.c
	gcc -o schedule schedule.c

run: compile
	./schedule

vcompile: schedule.c
	gcc -o schedule -DVERBOSE schedule.c

vrun: vcompile
	./schedule

db: genproc.c
	gcc -Wall -o genproc genproc.c

clean:
	-rm -f genproc schedule proc.txt
	