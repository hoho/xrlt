<?xml version="1.0"?>
<xrl:requestsheet xmlns:xrl="http://xrlt.net/Transform">
    <xrl:response>
        <xrl:include>
            <xrl:href>
                <xrl:include>
                    <xrl:href>/test/href/1</xrl:href>
                    <xrl:type>text</xrl:type>
                </xrl:include>
            </xrl:href>

            <xrl:type>
                <xrl:include>
                    <xrl:href>/test/href/2</xrl:href>
                    <xrl:type>text</xrl:type>
                </xrl:include>
            </xrl:type>

            <xrl:method>
                <xrl:include>
                    <xrl:href>/test/href/3</xrl:href>
                    <xrl:type>text</xrl:type>
                    <xrl:success select="concat('PO', /)" />
                </xrl:include>
            </xrl:method>

            <xrl:with-header>
                <xrl:name>
                    <xrl:include>
                        <xrl:href>/test/href/4</xrl:href>
                        <xrl:type>text</xrl:type>
                        <xrl:success select="/" />
                    </xrl:include>
                </xrl:name>
                <xrl:value>
                    <xrl:include>
                        <xrl:href>/test/href/5</xrl:href>
                        <xrl:type>text</xrl:type>
                        <xrl:success select="concat(text(), text())" />
                    </xrl:include>
                </xrl:value>
            </xrl:with-header>

            <xrl:with-header>
                <xrl:test>
                    <xrl:include>
                        <xrl:href>/test/href/6</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:test>
                <xrl:name>
                    <xrl:include>
                        <xrl:href>/test/href/7</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:name>
                <xrl:value>
                    <xrl:include>
                        <xrl:href>/test/href/8</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:value>
            </xrl:with-header>

            <xrl:with-param>
                <xrl:test>
                    <xrl:include>
                        <xrl:href>/test/href/9</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:test>
                <xrl:body>
                    <xrl:include>
                        <xrl:href>/test/href/10</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:body>
                <xrl:name>
                    <xrl:include>
                        <xrl:href>/test/href/11</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:name>
                <xrl:value>
                    <xrl:include>
                        <xrl:href>/test/href/12</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:value>
            </xrl:with-param>

            <xrl:with-param name="papapa">
                <xrl:include>
                    <xrl:href>/test/href/13</xrl:href>
                    <xrl:type>text</xrl:type>
                </xrl:include>
            </xrl:with-param>

            <xrl:with-body>
                <xrl:test>
                    <xrl:include>
                        <xrl:href>/test/href/14</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:test>
                <xrl:value>
                    <xrl:include>
                        <xrl:href>/test/href/15</xrl:href>
                        <xrl:type>text</xrl:type>
                    </xrl:include>
                </xrl:value>
            </xrl:with-body>

            <xrl:success select="/" />
        </xrl:include>
    </xrl:response>
</xrl:requestsheet>
