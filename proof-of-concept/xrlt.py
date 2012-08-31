from lxml import etree
from xmljson import xml2json, json2xml
from js import exec_js
import re
import json
from copy import deepcopy
from urllib import urlencode, urlopen

def process_param(elem, state):
    name = elem.attrib.get("name")
    assert name is not None
    if name in state["var"]:
        return
    state["var"][name] = elem if len(elem) else elem.text


def process_slice(elem, state):
    name = elem.attrib.get("name")
    assert name is not None
    state["apply"][name] = elem


def process_variable(elem, state):
    name = elem.attrib.get("name")
    assert name is not None
    v = etree.Element(name)
    state["var"][name] = v
    process_elements(elem, state, v)


def process_field(elem, state, root):
    name = elem.attrib.get("name")
    assert name is not None
    f = etree.SubElement(root, name)
    val = state["params"].get(name)
    if elem.attrib.get("type") == "javascript":
        v = {name: {}}
        if val is not None:
            v[name]["value"] = val
        ret, st = exec_js(elem.text, **v)
        json2xml(st, f)
        v = st.get("value")
        if v is None and val is not None:
            etree.SubElement(f, "value").text = val
        elif v is not None:
            v = str(v)
            f.find("value").text = v
    else:
        if val is not None:
            etree.SubElement(f, "value").text = val
        state["var"][name] = f
        state["field"] = f
        process_elements(elem, state, etree.Element(name))
        state["field"] = None


def process_push(elem, state, root):
    name = elem.attrib.get("name")
    assert name is not None
    replace = elem.attrib.get("replace")
    select = elem.attrib.get("select")
    field = state["field"]
    v = field.find(name)
    if v is not None and replace  == "yes":
        toremove = []
        for e in field:
            if e.tag == name:
                toremove.append(e)
        for e in toremove:
            field.remove(e)
    v = etree.SubElement(field, name)
    if select is not None:
        context = get_context(state, root)
        v.text = context.xpath("string(%s)" % select, **state["var"])
    else:
        process_elements(elem, state, v)


def get_context(state, root):
    if len(state["context"]):
        return state["context"][-1]
    return root


def process_if(elem, state, root):
    test = elem.attrib.get("test")
    assert test is not None
    context = get_context(state, root)
    if context.xpath(test, **state["var"]):
        process_elements(elem, state, root)


def process_choose(elem, state, root):
    n = None
    context = get_context(state, root)
    for e in elem:
        if e.tag == "{http://xrlt.net/Transform}when":
            test = e.attrib.get("test")
            assert test is not None
            if context.xpath(test, **state["var"]):
                n = e
                break
            
        elif e.tag == "{http://xrlt.net/Transform}otherwise":
            n = e
            break
        else:
            raise TypeError()
    
    if n is not None:           
        process_elements(n, state, root)


def process_with_param(elem, state, root):
    name = elem.attrib.get("name")
    assert name is not None
    select = elem.attrib.get("select")
    if select is not None:
        context = get_context(state, root)
        val = context.xpath(select, **state["var"])
    else:
        val = etree.Element(name)
        process_elements(elem, state, val)
    state["var"][name] = val


def process_value_of(elem, state, root):
    select = elem.attrib.get("select")
    assert select is not None
    context = get_context(state, root)
    val = context.xpath("string(%s)" % select, **state["var"])
    root.text = val


def process_copy_of(elem, state, root):
    select = elem.attrib.get("select")
    assert select is not None
    context = get_context(state, root)
    val = context.xpath(select, **state["var"])
    for e in val:
        root.append(e)


def process_text(elem, state, root):
    if len(root):
        t = root[-1].tail or ""
        root[-1].tail = "%s%s" % (t, elem.text)
    else:
        t = root.text or ""
        root.text = "%s%s" % (t, elem.text)


def process_include(elem, state, root):
    def xpath_repl(matchobj):
        expr = matchobj.group(1)
        context = get_context(state, root)
        return context.xpath("string(%s)" % expr, **state["var"])
    href = elem.attrib.get("href")
    assert href is not None
    href = re.sub(r"{(.*)}", xpath_repl, href)
    type = elem.attrib.get("type") or "xml"
    params = {}
    success = None
    failure = None
    for e in elem:
        if e.tag == "{http://xrlt.net/Transform}with-param":
            name = e.attrib["name"]
            assert name is not None
            select = e.attrib.get("select")
            if select is None:
                val = etree.Element(name)
                process_elements(e, state, val)
                params[name] = val.text.encode("utf-8")
            else:
                context = get_context(state, root)
                params[name] = context.xpath("string(%s)" % select,
                                             **state["var"]).encode("utf-8")
        elif e.tag == "{http://xrlt.net/Transform}success":
            success = e
        elif e.tag == "{http://xrlt.net/Transform}failure":
            failure = e
    try:
        data = urlopen("%s%s%s" % \
                       (href, "?" if len(params) else "", urlencode(params)))
        ret = data.read()
        data.close()
        ret = json2xml(json.loads(ret))
    except:
        if failure is not None:
            process_elements(failure, state, root)
    else:
        if success is None:
            #print etree.tostring(ret)
            for e in ret:
                root.append(e)
        else:
            e = etree.Element(ret[0].tag)
            for c in ret[0]:
                e.append(deepcopy(c))
            ret = e
            state["context"].append(ret)
            process_elements(success, state, root)
            state["context"].pop()


def process_for_each(elem, state, root):
    select = elem.attrib.get("select")
    assert select is not None
    context = get_context(state, root)
    val = context.xpath(select, **state["var"])
    for e in val:
        state["context"].append(e)
        process_elements(elem, state, root)
        state["context"].pop()


def process_transform(elem, state, root):
    href = elem.attrib.get("href")
    assert href is not None
    xml = etree.parse(href)
    xsl = etree.XSLT(xml)
    tmp = etree.Element("transform")
    process_elements(elem, state, tmp)
    root.text = etree.CDATA(unicode(xsl(tmp[0])))


def apply_slice(elem, apply, state, root):
    if elem.attrib.get("type") == "javascript":
        pass
    else:
        if apply is not None:
            a = etree.Element("apply")
            process_elements(apply, state, a)
            state["context"].append(a)
        process_elements(elem, state, root)
        if apply is not None:
            state["context"].pop()


def process_elements(elem, state, root):
    if len(elem) == 0 and elem.text is not None:
        if len(root):
            t = root[-1].tail or ""
            root[-1].tail = "%s%s" % (t, elem.text)
        else:
            t = root.text or ""
            root.text = "%s%s" % (t, elem.text)
        return
    for e in elem.findall("{http://xrlt.net/Transform}form"):
        process_slice(e, state)
    for e in elem.findall("{http://xrlt.net/Transform}slice"):
        if e.attrib.get("name") is not None:
            process_slice(e, state)
    for e in elem:
        if e.tag == "{http://xrlt.net/Transform}param":
            process_param(e, state)
        elif e.tag == "{http://xrlt.net/Transform}form":
            pass 
        elif e.tag == "{http://xrlt.net/Transform}slice":
            if e.attrib.get("name") is None:
                apply_slice(e, None, state, root)
        elif e.tag == "{http://xrlt.net/Transform}variable":
            process_variable(e, state)
        elif e.tag == "{http://xrlt.net/Transform}apply":
            name = e.attrib.get("name")
            assert name is not None
            a = state["apply"].get(name)
            assert a is not None
            apply_slice(a, e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}field":
            process_field(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}if":
            process_if(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}choose":
            process_choose(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}with-param":
            process_with_param(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}push":
            process_push(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}value-of":
            process_value_of(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}copy-of":
            process_copy_of(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}text":
            process_text(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}include":
            process_include(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}for-each":
            process_for_each(e, state, root)
        elif e.tag == "{http://xrlt.net/Transform}transform":
            process_transform(e, state, root)
        else:
            n = etree.SubElement(root, e.tag, **e.attrib)
            n.text = e.text
            process_elements(e, state, n)


def transform(f, params):
    parser = etree.XMLParser(remove_blank_text=True, remove_comments=True)
    xml = etree.XML(f.read(), parser=parser)
    state = {"apply": {}, "var": {}, "context": [], "params": params}
    ret = etree.Element("ret")
    process_elements(xml, state, ret)
    if len(ret):
        return etree.tostring(ret[0], pretty_print=True)
    else:
        return ret.text.encode("utf-8")


if __name__ == "__main__":
    f = open("search.xrl")
    print transform(f, {"text": "fuck"})
    f.close()

