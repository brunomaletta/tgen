"""Tests for docs/benchmark_render.py."""

import json
import os
import tempfile
import unittest
import xml.etree.ElementTree as ET

import benchmark_render


class BenchmarkRenderTest(unittest.TestCase):
    def test_member_href(self):
        href = benchmark_render._member_href(
            "structtgen_1_1wgraph",
            "structtgen_1_1wgraph_1a378f2357294d1be975f4b2c57600dbb0",
        )
        self.assertEqual(
            href,
            "structtgen_1_1wgraph.html#a378f2357294d1be975f4b2c57600dbb0",
        )

    def test_build_symbol_index(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = ET.Element("doxygen")
            compounddef = ET.SubElement(
                root, "compounddef", id="group__geometry"
            )
            sectiondef = ET.SubElement(compounddef, "sectiondef")
            memberdef = ET.SubElement(
                sectiondef,
                "memberdef",
                kind="function",
                id="group__geometry_1ga72def179092ee2d32f4bc760742dc293",
            )
            qn = ET.SubElement(memberdef, "qualifiedname")
            qn.text = "tgen::geometry::random_simple_polygon"
            path = os.path.join(tmp, "group__geometry.xml")
            ET.ElementTree(root).write(path, encoding="unicode")

            index = benchmark_render.build_symbol_index(tmp)
            self.assertIn("tgen::geometry::random_simple_polygon", index)
            self.assertTrue(
                index["tgen::geometry::random_simple_polygon"].startswith(
                    "group__geometry.html#"
                )
            )

    def test_render_html_links_symbol(self):
        data = {
            "generated_at": "2026-01-01T00:00:00Z",
            "compiler": "test",
            "flags": "-O2",
            "results": [
                {
                    "name": "tgen::geometry::random_simple_polygon",
                    "name_suffix": "",
                    "params": "n=1e6",
                    "doc_symbol": "tgen::geometry::random_simple_polygon",
                    "median_ms": 42,
                    "runs_ms": [40, 42, 44],
                }
            ],
        }
        index = {
            "tgen::geometry::random_simple_polygon": (
                "group__geometry.html#ga72def179092ee2d32f4bc760742dc293"
            )
        }
        html = benchmark_render.render_html(data, index)
        self.assertIn(
            'href="group__geometry.html#ga72def179092ee2d32f4bc760742dc293"',
            html,
        )
        self.assertIn("<strong>10<sup>6</sup></strong>", html)


if __name__ == "__main__":
    unittest.main()
