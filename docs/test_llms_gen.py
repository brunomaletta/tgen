"""Unit tests for ``llms_gen`` (the Doxygen-XML -> markdown converter).

Run from inside ``docs/``::

    python3 -m unittest test_llms_gen -v

Stdlib only (``unittest`` + ``xml.etree``); no pip dependencies.
"""

import unittest
import xml.etree.ElementTree as ET

import llms_gen


def el(xml):
    """Parse an XML snippet into an Element."""
    return ET.fromstring(xml)


class RenderInlineTest(unittest.TestCase):
    def test_plain_para(self):
        out = llms_gen.render_node(el("<para>Hello world.</para>"))
        self.assertIn("Hello world.", out)

    def test_computeroutput_inline_code(self):
        out = llms_gen.render_node(el("<computeroutput>foo()</computeroutput>"))
        self.assertEqual("`foo()`", out.strip())

    def test_emphasis(self):
        out = llms_gen.render_node(el("<emphasis>italic</emphasis>"))
        self.assertEqual("*italic*", out.strip())

    def test_bold(self):
        out = llms_gen.render_node(el("<bold>strong</bold>"))
        self.assertEqual("**strong**", out.strip())

    def test_programlisting_fenced_cpp(self):
        xml = (
            '<programlisting filename=".cpp">'
            '<codeline><highlight class="keyword">auto</highlight>'
            '<highlight class="normal"><sp/>x<sp/>=<sp/>1;</highlight></codeline>'
            '<codeline><highlight class="normal">return<sp/>x;</highlight></codeline>'
            "</programlisting>"
        )
        out = llms_gen.render_node(el(xml))
        self.assertIn("```cpp", out)
        self.assertIn("auto x = 1;", out)
        self.assertIn("return x;", out)
        self.assertTrue(out.rstrip().endswith("```"))

    def test_itemizedlist(self):
        xml = (
            "<itemizedlist>"
            "<listitem><para>one</para></listitem>"
            "<listitem><para>two</para></listitem>"
            "</itemizedlist>"
        )
        out = llms_gen.render_node(el(xml))
        self.assertIn("- one", out)
        self.assertIn("- two", out)

    def test_orderedlist(self):
        xml = (
            "<orderedlist>"
            "<listitem><para>first</para></listitem>"
            "<listitem><para>second</para></listitem>"
            "</orderedlist>"
        )
        out = llms_gen.render_node(el(xml))
        self.assertIn("1. first", out)
        self.assertIn("2. second", out)

    def test_section_heading_with_title(self):
        # Doxygen wraps markdown headings as nested sect elements; the
        # innermost one carries the title.
        xml = "<sect1><title>Examples</title><para>body text</para></sect1>"
        out = llms_gen.render_node(el(xml))
        self.assertIn("## Examples", out)
        self.assertIn("body text", out)

    def test_nested_section_wrappers(self):
        xml = (
            "<sect1><sect2><sect3>"
            "<title>Complexity</title><para>O(1).</para>"
            "</sect3></sect2></sect1>"
        )
        out = llms_gen.render_node(el(xml))
        # innermost is sect3 -> level 4 heading
        self.assertIn("#### Complexity", out)
        self.assertIn("O(1).", out)

    def test_ref_keeps_text(self):
        out = llms_gen.render_node(
            el('<ref refid="x" kindref="member">tgen::list</ref>')
        )
        self.assertEqual("tgen::list", out.strip())

    def test_ulink(self):
        out = llms_gen.render_node(
            el('<ulink url="https://example.com">site</ulink>')
        )
        self.assertEqual("[site](https://example.com)", out.strip())

    def test_render_description_collapses_blank_lines(self):
        a = el("<briefdescription><para>Brief.</para></briefdescription>")
        b = el("<detaileddescription><para>Detail.</para></detaileddescription>")
        out = llms_gen.render_description(a, b)
        self.assertIn("Brief.", out)
        self.assertIn("Detail.", out)
        self.assertNotIn("\n\n\n", out)
        self.assertEqual(out, out.strip())

    def test_render_description_skips_none(self):
        b = el("<detaileddescription><para>Only detail.</para></detaileddescription>")
        out = llms_gen.render_description(None, b)
        self.assertIn("Only detail.", out)


class ParameterAndSimpleSectTest(unittest.TestCase):
    def test_parameterlist_and_return(self):
        xml = (
            "<detaileddescription><para>"
            '<parameterlist kind="param"><parameteritem>'
            "<parameternamelist><parametername>size</parametername></parameternamelist>"
            "<parameterdescription><para>length</para></parameterdescription>"
            "</parameteritem></parameterlist>"
            '<simplesect kind="return"><para>a generator</para></simplesect>'
            "</para></detaileddescription>"
        )
        out = llms_gen.render_description(None, el(xml))
        self.assertIn("**Parameters:**", out)
        self.assertIn("`size` — length", out)
        self.assertIn("**Returns:** a generator", out)

    def test_templateparam_label(self):
        xml = (
            "<detaileddescription><para>"
            '<parameterlist kind="templateparam"><parameteritem>'
            "<parameternamelist><parametername>T</parametername></parameternamelist>"
            "<parameterdescription><para>the type</para></parameterdescription>"
            "</parameteritem></parameterlist>"
            "</para></detaileddescription>"
        )
        out = llms_gen.render_description(None, el(xml))
        self.assertIn("**Template parameters:**", out)
        self.assertIn("`T` — the type", out)

    def test_simplesect_note(self):
        xml = (
            "<detaileddescription><para>"
            '<simplesect kind="note"><para>be careful</para></simplesect>'
            "</para></detaileddescription>"
        )
        out = llms_gen.render_description(None, el(xml))
        self.assertIn("**Note:** be careful", out)


if __name__ == "__main__":
    unittest.main()
