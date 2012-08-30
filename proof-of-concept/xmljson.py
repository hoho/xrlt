from lxml import etree

def json2xml(json, root=None, name=None):
    if root is None:
        root = etree.Element("root")

    if json is None:
        pass
    elif isinstance(json, bool):
        root.attrib["type"] = "boolean"
        root.text = "true" if json else "false"
    elif isinstance(json, int) or isinstance(json, float):
        root.attrib["type"] = "number"
        root.text = str(json)
    elif isinstance(json, basestring):
        root.attrib["type"] = "string"
        root.text = json
    elif isinstance(json, list):
        for i in json:
            e = etree.Element("_" if name is None else name)
            json2xml(i, e)
            root.append(e)
    elif isinstance(json, dict):
        for key, value in json.iteritems():
            if isinstance(value, list):
                json2xml(value, root, key)
            else:
                e = etree.Element(key)
                json2xml(value, e, key)
                root.append(e)
    else:
        raise TypeError()
    return root


def xml2json(xml):
    if not len(xml):
        t = xml.attrib.get("type")
        if t == "boolean":
            return True if xml.text == "true" else False
        if t == "number":
            return int(xml.text)
        else:
            return xml.text

    ret = {}
    for e in xml:
        if e.tag in ret:
            l = ret[e.tag]
            if not isinstance(l, list):
                l = [l]
            l.append(xml2json(e))
            ret[e.tag] = l
        else:
            ret[e.tag] = xml2json(e)

    if len(ret) == 1 and "_" in ret:
        return ret["_"]
    else:
        return ret
