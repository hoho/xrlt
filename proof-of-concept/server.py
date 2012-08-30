import SimpleHTTPServer
import SocketServer
import os
from xrlt import transform
from urllib import unquote_plus

PORT = 8000

class RequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
        filename = self.path.split("?")[0]
        if filename.endswith(".xrl"):
            params = {}
            params_ = self.path[len(filename) + 1:]
            if params_:
                params_ = params_.split("&")
                for p in params_:
                    (name, val) = p.split("=", 1)
                    val = unquote_plus(val).decode("utf-8")
                    params[name] = val
            path = self.translate_path(self.path)
            f = None
            try:
                f = open(path, "rb")
            except IOError:
                self.send_error(404, "File not found")
                return
            ret = transform(f, params)
            f.close()
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.send_header("Content-Length", str(len(ret)))
            self.end_headers()
            self.wfile.write(ret)
        else:
            f = self.send_head()
            if f:
                self.copyfile(f, self.wfile)
                f.close()


httpd = SocketServer.TCPServer(("", PORT), RequestHandler)
print "serving at port", PORT
httpd.serve_forever()
