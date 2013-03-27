test: build run

clean:
	rm -rf .build

build: clean
	mkdir .build
	gcc -Wall -I../ xrltstruct/test1.c -o .build/xrltstruct_test1

run:
	@echo "\nRunning tests:"
	@find .build -type f -exec {} \;
	@echo "\ndone.\n"
