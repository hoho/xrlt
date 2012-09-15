<?xml version="1.0" encoding="utf-8"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <!-- Slices are functions of XRLT. Unnamed slices are applied in-place,
         named slices should be applied with <xrl:apply> instruction (or
         apply() JavaScript function (in case of JavaScript slices).

         For the moment, there are two types of slices: XML-slices (default)
         and JavaScript-slices. -->
    <xrl:slice>
        <!-- We're inside of unnamed XML-slice. It will apply in-place.
             So, from here XRLT processor will walk through slice's nodes.
             Unknown nodes will be just copied to the result tree,
             XRLT-instructions will be processed. -->
        <root><!-- Copy node to result tree. -->
            <xrl:apply name="xml-slice">
                <!-- Apply named slice with parameter. -->
                <xrl:with-param name="p1">yo</xrl:with-param>
            </xrl:apply>
            <xrl:apply name="js-slice">
                <!-- Apply another named slice with parameter. -->
                <xrl:with-param name="p2">
                    <boo>
                        <foo>moo</foo>
                    </boo>
                </xrl:with-param>
            </xrl:apply>
        </root>
    </xrl:slice>

    <xrl:slice name="xml-slice">
        <!-- Named XML-slice. -->
        <xrl:param name="p1" /><!-- Parameter p1 could be given. -->
        <hello-from-xml><!-- Copy node to the result tree. -->
            <hi><!-- Copy node to the result tree. -->
                <!-- Evaluate XPath-expression and add evaluation result to
                     the result tree. -->
                <xrl:value-of
                    select="concat('hello', ' from ', 'xml ', $p1)" />
            </hi>
        </hello-from-xml>
    </xrl:slice>

    <xrl:slice name="js-slice" type="javascript">
        <!-- Named JavaScript-slice. Parameter's p2 async="yes" attribute
             means, we don't want to wait for p2 parameter to be ready (say,
             p2 contains yet unfinished backend request result). -->
        <xrl:param name="p2" async="yes" />
        <![CDATA[
            // We can freely apply JavaScript-slices from XML-slices and
            // XML-slices from JavaScript-slices: parameters and the result
            // will be automatically mapped between XML and JSON using
            // reasonably-destructive data mapping.

            // We can return any JSON-serializable object as the result or
            // Deferred-object, if we need to get the result asynchronously.
            var ret = new Deferred();

            // Parameters are available as scope variables. When we have
            // async="yes" in parameter's declaration, we get Deferred-object
            // in parameter's variable.
            p2.then(function(value) {
                // Slice parameters' values are immutable, so, we have to copy
                // value, if we want to change it.
                var p = copy(value);

                // <boo><foo>moo</foo></boo> is passed to this slice, and
                // automatically mapped to {"boo": {"foo": "moo"}}.
                // Let's add something to foo property.
                p.boo.foo += ' yep';

                // Apply XML-slice with parameter p1.
                var xmlSliceResult = apply('xml-slice', {p1: new Date()});

                // The result will be:
                // {
                //     "hello-from-js": {
                //         "test": {"boo": {"foo": "moo yep"}},
                //         "test2": ... — result of xml-slice.
                //     }
                // }
                ret.resolve({
                    'hello-from-js': {
                        test: p,
                        test2: xmlSliceResult
                    }
                });
            });

            // Return Deferred-object.
            return ret;
        ]]>
    </xrl:slice>

</xrl:requestsheet>
