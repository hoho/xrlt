test: build run

clean:
	rm -rf .build

build: clean
	mkdir .build
	gcc -o .build/xrltstruct_test1   -Wall -I../ xrltstruct/test1.c
	gcc -o .build/rddm_test          -Wall -I../ -I/usr/include/libxml2       \
                                 -L/usr/local/lib ../rddm.c rddm/test.c   \
                                 -lyajl -lxml2

run:
	@echo "\nRunning tests:"
	@find .build -maxdepth 1 -type f -exec {} \;
	@echo "\ndone.\n"
