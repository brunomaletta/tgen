"""Unit tests for ``llms_gen`` (the Doxygen-XML -> markdown converter).

Run from inside ``docs/``::

    python3 -m unittest test_llms_gen -v

Stdlib only (``unittest`` + ``xml.etree``); no pip dependencies.
"""

import os
import tempfile
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


class VisibilityScopedTagTest(unittest.TestCase):
    def test_htmlonly_content_dropped(self):
        xml = (
            "<para>before<htmlonly><script>junk()</script></htmlonly>after</para>"
        )
        out = llms_gen.render_node(el(xml))
        self.assertIn("before", out)
        self.assertIn("after", out)
        self.assertNotIn("junk", out)
        self.assertNotIn("<script", out)

    def test_latexonly_content_dropped(self):
        xml = "<para>a<latexonly>\\latex{cmd}</latexonly>b</para>"
        out = llms_gen.render_node(el(xml))
        self.assertIn("a", out)
        self.assertIn("b", out)
        self.assertNotIn("latex", out)


class DashEntityTest(unittest.TestCase):
    def test_ndash_and_mdash(self):
        xml = "<para>use <ndash/>flag and <mdash/>x</para>"
        out = llms_gen.render_node(el(xml))
        self.assertIn("--flag", out)
        self.assertIn("---x", out)


class SimpleSectBlockTest(unittest.TestCase):
    def test_simplesect_preserves_code_block(self):
        xml = (
            '<simplesect kind="note"><para>see'
            '<programlisting filename=".cpp"><codeline>'
            '<highlight class="normal">int x;</highlight>'
            "</codeline></programlisting></para></simplesect>"
        )
        out = llms_gen.render_node(el(xml))
        self.assertIn("**Note:**", out)
        self.assertIn("```cpp\nint x;", out)


class ErrorHandlingTest(unittest.TestCase):
    def test_discover_groups_missing_dir_exits(self):
        with self.assertRaises(SystemExit):
            llms_gen.discover_groups("/no/such/dir")

    def test_generate_missing_dir_exits(self):
        with tempfile.TemporaryDirectory() as out:
            with self.assertRaises(SystemExit):
                llms_gen.generate("/no/such/dir", out, "https://example.com/")

    def test_load_compounddef_malformed_exits(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "bad.xml")
            with open(path, "w") as f:
                f.write("<compounddef><not-closed>")
            with self.assertRaises(SystemExit):
                llms_gen._load_compounddef(d, "bad")

    def test_validate_missing_module_exits(self):
        with tempfile.TemporaryDirectory() as out:
            with open(os.path.join(out, "llms.txt"), "w") as f:
                f.write("# tgen\n")
            incomplete = [m for m in llms_gen.EXPECTED_MODULES if m != "list"]
            with self.assertRaises(SystemExit):
                llms_gen._validate(out, incomplete)


class MemberAndGroupTest(unittest.TestCase):
    def _member(self):
        xml = (
            '<memberdef kind="function" id="m1">'
            "<type>bool</type>"
            "<definition>bool tgen::math::is_prime</definition>"
            "<argsstring>(uint64_t n)</argsstring>"
            "<name>is_prime</name>"
            "<briefdescription><para>Checks if prime.</para></briefdescription>"
            "<detaileddescription>"
            '<para><parameterlist kind="param"><parameteritem>'
            "<parameternamelist><parametername>n</parametername></parameternamelist>"
            "<parameterdescription><para>Number.</para></parameterdescription>"
            "</parameteritem></parameterlist>"
            '<simplesect kind="return"><para>If prime.</para></simplesect></para>'
            "</detaileddescription>"
            "</memberdef>"
        )
        return el(xml)

    def test_member_signature(self):
        sig = llms_gen._member_signature(self._member())
        self.assertIn("is_prime", sig)
        self.assertIn("uint64_t n", sig)
        self.assertIn("bool", sig)

    def test_render_member(self):
        out = llms_gen.render_member(self._member())
        self.assertIn("#### `is_prime`", out)
        self.assertIn("```cpp", out)
        self.assertIn("Checks if prime.", out)
        self.assertIn("**Returns:** If prime.", out)

    def test_render_compound_members_skips_variables(self):
        compound = el(
            '<compounddef kind="struct">'
            '<sectiondef kind="public-func">'
            + ET.tostring(self._member(), encoding="unicode")
            + "</sectiondef>"
            '<sectiondef kind="public-attrib">'
            '<memberdef kind="variable" id="v1"><name>field</name>'
            "<type>int</type></memberdef>"
            "</sectiondef>"
            "</compounddef>"
        )
        members = llms_gen.render_compound_members(compound)
        self.assertIn("is_prime", members)
        self.assertNotIn("field", members)


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


class SmokeXmlTest(unittest.TestCase):
    XML_DIR = os.path.join(os.path.dirname(__file__), "build", "xml")

    def setUp(self):
        if not os.path.isdir(self.XML_DIR):
            self.skipTest("real XML not generated at build/xml")

    def test_discover_groups(self):
        groups = llms_gen.discover_groups(self.XML_DIR)
        names = [n for n, _ in groups]
        for expected in llms_gen.EXPECTED_MODULES:
            self.assertIn(expected, names)

    def test_generate_full(self):
        with tempfile.TemporaryDirectory() as out:
            llms_gen.generate(self.XML_DIR, out, "https://example.com/tgen/")
            index = os.path.join(out, "llms.txt")
            self.assertTrue(os.path.isfile(index))
            with open(index) as f:
                idx = f.read()
            self.assertIn("# tgen", idx)
            self.assertIn("## Modules", idx)
            for expected in llms_gen.EXPECTED_MODULES:
                self.assertTrue(
                    os.path.isfile(os.path.join(out, "llms", expected + ".md")),
                    expected,
                )
            with open(os.path.join(out, "llms", "list.md")) as f:
                listmd = f.read()
            self.assertIn("```cpp", listmd)
            self.assertNotIn("@tt{", listmd)
            self.assertNotIn("@pname{", listmd)
            self.assertIsNone(llms_gen.MARKUP_RE.search(listmd))

            with open(os.path.join(out, "llms", "base.md")) as f:
                basemd = f.read()
            for junk in ("<script", "wnext-viz", "addEventListener", "getElementById"):
                self.assertNotIn(junk, basemd)

            with open(os.path.join(out, "llms", "opts.md")) as f:
                optsmd = f.read()
            self.assertIn("--", optsmd)


if __name__ == "__main__":
    unittest.main()
