"""Tests for docs/benchmark_check.py."""

import json
import tempfile
import unittest

import benchmark_check


class BenchmarkCheckTest(unittest.TestCase):
    def write_json(self, path, results):
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"results": results}, f)

    def test_compare_passes_within_threshold(self):
        row = {
            "name": "tgen::tree::gen",
            "name_suffix": "",
            "params": "n=1e6",
            "median_ms": 100,
        }
        with tempfile.TemporaryDirectory() as tmp:
            base = tmp + "/base.json"
            cur = tmp + "/cur.json"
            self.write_json(base, [row])
            self.write_json(cur, [{**row, "median_ms": 150}])
            self.assertEqual(benchmark_check.compare(base, cur, 2.0), [])

    def test_compare_fails_on_regression(self):
        row = {
            "name": "tgen::tree::gen",
            "name_suffix": "",
            "params": "n=1e6",
            "median_ms": 100,
        }
        with tempfile.TemporaryDirectory() as tmp:
            base = tmp + "/base.json"
            cur = tmp + "/cur.json"
            self.write_json(base, [row])
            self.write_json(cur, [{**row, "median_ms": 250}])
            errors = benchmark_check.compare(base, cur, 2.0)
            self.assertEqual(len(errors), 1)
            self.assertIn("regression", errors[0])

    def test_compare_fails_on_missing_case(self):
        base_row = {
            "name": "a",
            "name_suffix": "",
            "params": "p",
            "median_ms": 1,
        }
        cur_row = {
            "name": "b",
            "name_suffix": "",
            "params": "p",
            "median_ms": 1,
        }
        with tempfile.TemporaryDirectory() as tmp:
            base = tmp + "/base.json"
            cur = tmp + "/cur.json"
            self.write_json(base, [base_row])
            self.write_json(cur, [cur_row])
            errors = benchmark_check.compare(base, cur, 2.0)
            self.assertTrue(any("missing case" in e for e in errors))
            self.assertTrue(any("new case" in e for e in errors))


if __name__ == "__main__":
    unittest.main()
