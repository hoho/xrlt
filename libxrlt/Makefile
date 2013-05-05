xrlt:
	g++ -shared -o libxrlt.dylib -Wall -Werror -fPIC -I./ -I/usr/include/libxml2 -L/usr/local/lib xrlt.c xrlterror.c transform.c json2xml.c querystring.c builtins.c include.c variable.c headers.c function.c -lxml2 -lyajl
	sudo cp libxrlt.dylib /usr/local/lib
