import PyV8
import json

class Global(PyV8.JSClass):
    def __init__(self, *args, **kwargs):
        self.state = {}
        super(Global, self).__init__(*args, **kwargs)
    def value(self, val):
        self.state["value"] = val
    def error(self, val):
        err = self.state.get("error")
        if err is None:
            err = []
        err.append(val)
        self.state["error"] = err


def exec_js(code, **kwargs):
    g = Global()
    ctxt = PyV8.JSContext(g)
    ctxt.enter()
    vargs = []
    cargs = []
    for key, value in kwargs.iteritems():
        vargs.append(key)
        cargs.append(json.dumps(value))
    ret = ctxt.eval("JSON.stringify((function(%s){%s})(%s));" % \
                    (",".join(vargs), code, ",".join(cargs)));
    return json.loads(ret) if ret else ret, g.state
