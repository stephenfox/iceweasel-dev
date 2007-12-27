<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:svg="http://www.w3.org/2000/svg">
<xsl:output method="xml" encoding="UTF-8"/>

<xsl:template match="node()|@*">
<xsl:copy>
<xsl:apply-templates select="node()|@*"/>
</xsl:copy>
</xsl:template>

<xsl:template match="svg:g[@id='g16948']"/>
<xsl:template match="svg:g[@id='g16941']"/>

<xsl:template match="svg:g[@id='layer2']">
<xsl:copy>
<xsl:attribute name="transform">scale(1.1) translate(-5, -14)</xsl:attribute>
<xsl:apply-templates select="node()|@*"/>
</xsl:copy>
</xsl:template>

</xsl:stylesheet>
