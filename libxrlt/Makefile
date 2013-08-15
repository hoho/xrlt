xrlt:
	g++ -shared -o libxrlt.so -Wall -Werror -fPIC -I./ -I/usr/include/libxml2 -L/usr/local/lib \
		xrlt.c xrlterror.c transform.c xpathfuncs.c json2xml.c xml2json.cc xml2js.cc querystring.c include.c \
		variable.c headers.c function.c import.c choose.c if.c log.c response.c valueof.c copyof.c js.cc \
		foreach.c ccan_json.c \
		-lxml2 -lyajl -lv8 -lxslt

install:
	sudo cp libxrlt.so /usr/local/lib
