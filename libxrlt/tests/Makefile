test: build run

clean:
	rm -rf .build

init:
	mkdir .build

build: clean init xrltstruct json2xml js compile include

.PHONY: xrltstruct
xrltstruct:
	gcc -o .build/xrltstruct_test1   -Wall -I../ xrltstruct/test1.c

.PHONY: json2xml
json2xml:
	gcc -o .build/json2xml_test          -Wall -I../ -I/usr/include/libxml2       \
                                 -L/usr/local/lib ../json2xml.c json2xml/test.c   \
                                 -lyajl -lxml2

.PHONY: js
js:
	gcc -o .build/js_test1_test1.o      -Wall -c -I../ -I/usr/include/libxml2 js/test1.c
	g++ -o .build/js_test1_js.o         -Wall -c -I../ -I/usr/include/libxml2 ../js.cc
	g++ -o .build/js_test1_xml2json.o   -Wall -c -I../ -I/usr/include/libxml2 ../xml2json.cc
	g++ -o .build/js_test1              -Wall .build/js_test1_test1.o .build/js_test1_js.o .build/js_test1_xml2json.o -lv8 -lxml2

	rm .build/js_test1_test1.o .build/js_test1_js.o .build/js_test1_xml2json.o

.PHONY: compile
compile:
	g++ -o .build/compile_test1   -Wall -I../ -I/usr/include/libxml2 -L/usr/local/lib ../transform.c ../xrlt.c ../json2xml.c ../builtins.c ../include.c ../variable.c ../xrlterror.c compile/test1.c -lxml2 -lyajl

.PHONY: include
include:
	g++ -o .build/include_test   -Wall -I../ -I/usr/include/libxml2 -L/usr/local/lib ../transform.c ../xrlt.c ../json2xml.c ../builtins.c ../include.c ../variable.c ../xrlterror.c include/test.c -lxml2 -lyajl

run:
	@echo "\nRunning tests:"
	@find .build -maxdepth 1 -type f -exec {} \;
	@echo "\ndone.\n"
