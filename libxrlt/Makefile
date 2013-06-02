xrlt:
	g++ -shared -o libxrlt.dylib -Wall -Werror -fPIC -I./ -I/usr/include/libxml2 -L/usr/local/lib xrlt.c xrlterror.c transform.c json2xml.c xml2json.cc querystring.c builtins.c include.c variable.c headers.c function.c js.cc -lxml2 -lyajl -lv8 -lxslt
	sudo cp libxrlt.dylib /usr/local/lib
