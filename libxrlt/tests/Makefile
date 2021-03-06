test: build run

clean:
	rm -rf .build

init:
	mkdir .build

build: clean init xrltstruct json2xml querystring js compile transform

.PHONY: xrltstruct
xrltstruct:
	@gcc -o .build/xrltstruct_test1   -Wall -I../ -I/usr/include/libxml2 xrltstruct/test1.c -lxml2
	@.build/xrltstruct_test1

.PHONY: json2xml
json2xml:
	gcc -o .build/json2xml_test          -Wall -I../ -I/usr/include/libxml2       \
                                 -L/usr/local/lib ../json2xml.c json2xml/test.c   \
                                 -lyajl -lxml2 -lxslt

.PHONY: js
js:
	gcc -o .build/js_test1_test1.o      -Wall -c -I../ -I/usr/include/libxml2 js/test1.c
	g++ -o .build/js_test1_js.o         -Wall -c -I../ -I/usr/include/libxml2 ../js.cc
	g++ -o .build/js_test1_xml2json.o   -Wall -c -I../ -I/usr/include/libxml2 ../xml2json.cc
	g++ -o .build/js_test1              -Wall .build/js_test1_test1.o .build/js_test1_js.o .build/js_test1_xml2json.o -lv8 -lxml2 -lxslt

	rm .build/js_test1_test1.o .build/js_test1_js.o .build/js_test1_xml2json.o

.PHONY: compile
compile:
	g++ -o .build/compile_test1   -Wall -I../ -I/usr/include/libxml2 -L/usr/local/lib ../transform.c ../xrlt.c ../json2xml.c ../querystring.c ../builtins.c ../include.c ../variable.c ../headers.c ../function.c ../xrlterror.c compile/test1.c -lxml2 -lyajl -lxslt

.PHONY: transform
transform:
	@DYLD_LIBRARY_PATH=../out/Release $(VALGRIND) ../out/Release/transform_test \
                                       transform/includes/test1.xrl transform/includes/test1.in transform/includes/test1.out \
                                       transform/includes/test2.xrl transform/includes/test2.in transform/includes/test2.out \
                                       transform/includes/test3.xrl transform/includes/test3.in transform/includes/test3.out \
                                       transform/includes/test4.xrl transform/includes/test4.in transform/includes/test4.out \
                                       transform/includes/test5.xrl transform/includes/test5.in transform/includes/test5.out \
                                       transform/includes/test6.xrl transform/includes/test6.in transform/includes/test6.out \
                                       transform/includes/test7.xrl transform/includes/test7.in transform/includes/test7.out \
                                       transform/includes/test8.xrl transform/includes/test8.in transform/includes/test8.out \
                                       \
                                       transform/headers/test1.xrl transform/headers/test1.in transform/headers/test1.out \
                                       \
                                       transform/functions/test1.xrl transform/functions/test1.in transform/functions/test1.out \
                                       \
                                       transform/querystrings_bodies/test1.xrl transform/querystrings_bodies/test1.in transform/querystrings_bodies/test1.out \
                                       transform/querystrings_bodies/test2.xrl transform/querystrings_bodies/test2.in transform/querystrings_bodies/test2.out \
                                       transform/querystrings_bodies/test3.xrl transform/querystrings_bodies/test3.in transform/querystrings_bodies/test3.out \
                                       transform/querystrings_bodies/test4.xrl transform/querystrings_bodies/test4.in transform/querystrings_bodies/test4.out \
                                       transform/querystrings_bodies/test5.xrl transform/querystrings_bodies/test5.in transform/querystrings_bodies/test5.out \
                                       \
                                       transform/transformations/test1.xrl transform/transformations/test1.in transform/transformations/test1.out \
                                       transform/transformations/test2.xrl transform/transformations/test2.in transform/transformations/test2.out \
                                       transform/transformations/test3.xrl transform/transformations/test3.in transform/transformations/test3.out \
                                       transform/transformations/test4.xrl transform/transformations/test4.in transform/transformations/test4.out \
                                       transform/transformations/test5.xrl transform/transformations/test5.in transform/transformations/test5.out \
                                       \
                                       transform/imports/test1.xrl transform/imports/test1.in transform/imports/test1.out \
                                       \
                                       transform/choose_if/test1.xrl transform/choose_if/test1.in transform/choose_if/test1.out \
                                       \
                                       transform/copyof/test1.xrl transform/copyof/test1.in transform/copyof/test1.out \
                                       \
                                       transform/foreach/test1.xrl transform/foreach/test1.in transform/foreach/test1.out

.PHONY: transformjs
transformjs:
	@DYLD_LIBRARY_PATH=../out/Release $(VALGRIND) ../out/Release/transform_test transform/rddm/test1.xrl transform/rddm/test1.in transform/rddm/test1.out

.PHONY: querystring
querystring:
	@../out/Release/querystring_test

run:
	@echo "\nRunning tests:"
	@find .build -maxdepth 1 -type f -exec {} \;
	@echo "\ndone.\n"
