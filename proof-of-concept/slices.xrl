<?xml version="1.0" encoding="utf-8"?>
<xrl:requestsheet
    xmlns:xrl="http://xrlt.net/Transform"
    xmlns:xrltype="http://xrlt.net/Type">


    <xrl:slice>
        <root>
            <xrl:apply name="xml-slice">
                <xrl:with-param name="p1">yo</xrl:with-param>
            </xrl:apply>
            <xrl:apply name="js-slice">
                <xrl:with-param name="p2">
                    <boo>
                        <foo>moo</foo>
                    </boo>
                </xrl:with-param>
            </xrl:apply>
        </root>
    </xrl:slice>


    <xrl:slice name="xml-slice">
        <xrl:param name="p1" />
        <hello-from-xml>
            <hi>
                <xrl:value-of select="concat('hello', ' from ', 'xml ', $p1)" />
            </hi>
        </hello-from-xml>
    </xrl:slice>

    <xrl:slice name="js-slice" type="javascript">
        <xrl:param name="p2" />
        <![CDATA[
            var ppp = copy(p2);
            ppp.boo.foo += ' yep';
            return {
                "hello-from-js": {
                    test: ppp,
                    test2: apply('xml-slice', {p1: new Date()})
                }
            }
        ]]>
    </xrl:slice>


</xrl:requestsheet>
