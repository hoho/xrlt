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
	@g++ -o .build/transform_test   -Wall -D__XRLT_NO_JAVASCRIPT__ -I../ -I/usr/include/libxml2 -L/usr/local/lib ../transform.c \
		../xrlt.c ../json2xml.c ../querystring.c ../include.c ../variable.c ../headers.c ../function.c ../import.c \
		../choose.c ../if.c ../log.c ../response.c ../valueof.c ../xrlterror.c transform/test.c -lxml2 -lyajl -lxslt
	@$(VALGRIND) .build/transform_test transform/test1.xrl transform/test1.in transform/test1.out \
                                       transform/test2.xrl transform/test2.in transform/test2.out \
                                       transform/test3.xrl transform/test3.in transform/test3.out \
                                       transform/test4.xrl transform/test4.in transform/test4.out \
                                       transform/test5.xrl transform/test5.in transform/test5.out \
                                       transform/test6.xrl transform/test6.in transform/test6.out \
                                       transform/test7.xrl transform/test7.in transform/test7.out \
                                       transform/test8.xrl transform/test8.in transform/test8.out \
                                       transform/test9.xrl transform/test9.in transform/test9.out \
                                       transform/test10.xrl transform/test10.in transform/test10.out \
                                       transform/test11.xrl transform/test11.in transform/test11.out \
                                       transform/test12.xrl transform/test12.in transform/test12.out \
                                       transform/test13.xrl transform/test13.in transform/test13.out \
                                       transform/test14.xrl transform/test14.in transform/test14.out \
                                       transform/test15_choose_if.xrl transform/test15_choose_if.in transform/test15_choose_if.out

.PHONY: transformjs
transformjs:
	@g++ -o .build/transform_testjs   -Wall -I../ -I/usr/include/libxml2 -L/usr/local/lib ../transform.c ../xrlt.c ../json2xml.c ../xml2json.cc ../querystring.c ../builtins.c ../include.c ../variable.c ../headers.c ../function.c ../js.cc ../xrlterror.c transform/test.c -lxml2 -lyajl -lv8 -lxslt

.PHONY: querystring
querystring:
	g++ -o .build/querystring_test   -Wall -I../ -I/usr/include/libxml2 ../xrlterror.c ../querystring.c querystring/test.c -lxml2 -lxslt

run:
	@echo "\nRunning tests:"
	@find .build -maxdepth 1 -type f -exec {} \;
	@echo "\ndone.\n"
