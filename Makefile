all: kmlpack kmlunpack

clean:
	rm -f kmlpack kmlunpack

kmlpack: kmlpack.c
	gcc -O3 -o $@ -Wall -pedantic -Werror -ldb $^

kmlunpack: kmlunpack.c
	gcc -O3 -o $@ -Wall -pedantic -Werror -ldb $^
