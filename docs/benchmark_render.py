"""Render benchmark HTML from JSON + Doxygen XML doc links.

Reads ``docs/benchmark_results.json`` (timings + ``doc_symbol`` keys) and
``docs/build/xml`` (from the doc-prepare Doxygen pass) to write
``docs/build/benchmark_include.html`` with correct member links. Anchors are resolved
from Doxygen XML so C++ does not hardcode hash URLs.
"""

import argparse
import html
import json
import os
import sys
import xml.etree.ElementTree as ET


def _die(msg):
    sys.stderr.write("tgen benchmark_render: %s\n" % msg)
    sys.exit(1)


def _member_href(compound_id, member_id):
    prefix = compound_id + "_1"
    if member_id.startswith(prefix):
        anchor = member_id[len(prefix) :]
    else:
        anchor = member_id
    return "%s.html#%s" % (compound_id, anchor)


def build_symbol_index(xml_dir):
    """Map qualified name -> relative HTML href."""
    index = {}
    for name in os.listdir(xml_dir):
        if not name.endswith(".xml") or name == "index.xml":
            continue
        path = os.path.join(xml_dir, name)
        try:
            root = ET.parse(path).getroot()
        except (OSError, ET.ParseError) as exc:
            _die("%s: %s" % (path, exc))

        compounddef = root.find("compounddef")
        if compounddef is None:
            continue
        compound_id = compounddef.get("id")
        if not compound_id:
            continue

        for sectiondef in compounddef.findall("sectiondef"):
            for member in sectiondef.findall("memberdef"):
                if member.get("kind") != "function":
                    continue
                qn_el = member.find("qualifiedname")
                if qn_el is None or not (qn_el.text or "").strip():
                    continue
                qn = qn_el.text.strip()
                member_id = member.get("id")
                if not member_id:
                    continue
                index[qn] = _member_href(compound_id, member_id)
    return index


def _html_format_number(num):
    num = num.strip()
    e_pos = -1
    for sep in ("e", "E"):
        pos = num.find(sep)
        if pos != -1:
            e_pos = pos
            break
    if e_pos == -1:
        return html.escape(num)

    mantissa = num[:e_pos]
    exponent = num[e_pos + 1 :]
    if not mantissa or not exponent:
        return html.escape(num)
    exp_html = html.escape(exponent)
    if mantissa == "1":
        return "10<sup>%s</sup>" % exp_html
    return "%s&times;10<sup>%s</sup>" % (html.escape(mantissa), exp_html)


def _html_params_with_bold_numbers(params):
    out = []
    pos = 0
    while pos < len(params):
        eq = params.find("=", pos)
        if eq == -1:
            out.append(html.escape(params[pos:]))
            break
        out.append(html.escape(params[pos : eq + 1]))
        pos = eq + 1
        end = params.find(",", pos)
        if end == -1:
            end = len(params)
        out.append(
            "<strong>%s</strong>" % _html_format_number(params[pos:end])
        )
        pos = end
        if pos < len(params):
            out.append(html.escape(params[pos]))
            pos += 1
            while pos < len(params) and params[pos] == " ":
                out.append(html.escape(params[pos]))
                pos += 1
    return "".join(out)


def render_html(data, symbol_index):
    results = data.get("results", [])
    num_runs = len(results[0].get("runs_ms", [])) if results else 0

    lines = [
        '<div id="benchmark-results">',
        '  <p class="benchmark-meta">Timestamp: %s</p>'
        % html.escape(data.get("generated_at", "")),
        '  <p class="benchmark-meta">GCC version: %s</p>'
        % html.escape(data.get("compiler", "")),
        '  <p class="benchmark-meta">GCC flags: %s</p>'
        % html.escape(data.get("flags", "")),
        '  <div class="table-scroll">',
        '  <table class="markdownTable">',
        '    <tr class="markdownTableHead">',
        '      <th class="markdownTableHeadLeft">Operation</th>',
        '      <th class="markdownTableHeadLeft">Parameters</th>',
        '      <th class="markdownTableHeadRight num">Median out of %d runs (ms)</th>'
        % num_runs,
        "    </tr>",
    ]

    missing = []
    for row in results:
        name = row.get("name", "")
        suffix = row.get("name_suffix", "")
        if not suffix and " (" in name:
            # Backward compat: old JSON combined name + suffix.
            split = name.find(" (")
            if split != -1:
                suffix = name[split:]
                name = name[:split]

        params = row.get("params", "")
        median_ms = row.get("median_ms", 0)
        doc_symbol = row.get("doc_symbol", "")
        href = symbol_index.get(doc_symbol) if doc_symbol else None
        if doc_symbol and not href:
            missing.append(doc_symbol)

        lines.append('    <tr class="markdownTableRowOdd">')
        lines.append('      <td class="markdownTableBodyLeft">')
        if href:
            lines.append(
                '<a href="%s">%s</a>'
                % (html.escape(href, quote=True), html.escape(name))
            )
        else:
            lines.append(html.escape(name))
        lines.append(html.escape(suffix))
        lines.append("</td>")
        lines.append(
            '      <td class="markdownTableBodyLeft">%s</td>'
            % _html_params_with_bold_numbers(params)
        )
        lines.append(
            '      <td class="markdownTableBodyRight num">%s</td>'
            % html.escape(str(median_ms))
        )
        lines.append("    </tr>")

    lines.extend(["  </table>", "  </div>", "</div>", ""])

    if missing:
        sys.stderr.write(
            "tgen benchmark_render: warning: no doc link for: %s\n"
            % ", ".join(sorted(set(missing)))
        )

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", required=True, help="benchmark_results.json")
    parser.add_argument("--xml", required=True, help="Doxygen XML directory")
    parser.add_argument("--out", required=True, help="Output HTML path")
    args = parser.parse_args()

    if not os.path.isfile(args.json):
        _die("missing %s (run 'make benchmark' first)" % args.json)
    if not os.path.isdir(args.xml):
        _die("missing %s (run 'make doc' first)" % args.xml)

    with open(args.json, encoding="utf-8") as f:
        data = json.load(f)

    symbol_index = build_symbol_index(args.xml)
    html_out = render_html(data, symbol_index)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as f:
        f.write(html_out)

    print("tgen benchmark_render: wrote %s" % args.out)


if __name__ == "__main__":
    main()
