"""Doxygen-XML -> LLM-friendly markdown converter for ``tgen``.

Reads a Doxygen XML output directory (``GENERATE_XML=YES``) and emits an
``llms.txt``-convention doc tree:

    <out>/llms.txt          index linking to one markdown file per module
    <out>/llms/<module>.md  one file per Doxygen @defgroup

The goal is a self-contained markdown reference an LLM agent can fetch and
navigate. See ``docs/plans/2026-05-20-llm-docs-design.md`` for the design.

Stdlib only (``xml.etree`` + ``argparse``); no pip/node dependencies, matching
the project's minimal-deps ethos.

Notes on this project's actual XML (Doxygen 1.17.0):
  * Markdown headings become nested ``sect1``..``sect4`` wrappers; only the
    innermost section carries a ``<title>``.
  * The ``@tt{}`` / ``@pname{}`` family of aliases expand to spans that are
    stripped in XML output, leaving only their (often auto-linked) inner text,
    so there is no leftover ``@...{`` markup to handle for them.
"""

import re

_SECT_TAGS = {"sect1": 2, "sect2": 3, "sect3": 4, "sect4": 5}

_PARAMLIST_LABELS = {
    "param": "Parameters",
    "templateparam": "Template parameters",
    "retval": "Return values",
    "exception": "Exceptions",
}

_SIMPLESECT_LABELS = {
    "return": "Returns",
    "note": "Note",
    "warning": "Warning",
    "see": "See also",
    "since": "Since",
    "pre": "Precondition",
    "post": "Postcondition",
    "attention": "Attention",
}


def _text(s):
    return s if s else ""


# --------------------------------------------------------------------------
# Inline / block rich-text rendering
# --------------------------------------------------------------------------


def render_children(node):
    """Render ``node.text`` + each child (via ``render_node``) + child tails."""
    parts = [_text(node.text)]
    for child in node:
        parts.append(render_node(child))
        parts.append(_text(child.tail))
    return "".join(parts)


def _render_programlisting(node):
    lines = []
    for codeline in node.findall("codeline"):
        lines.append(_render_codeline_text(codeline))
    code = "\n".join(lines).rstrip()
    return "\n```cpp\n" + code + "\n```\n"


def _render_codeline_text(node):
    parts = [_text(node.text)]
    for child in node:
        if child.tag == "sp":
            parts.append(" ")
        else:
            parts.append(_render_codeline_text(child))
        parts.append(_text(child.tail))
    return "".join(parts)


def _render_list(node, ordered):
    out = ["\n"]
    for i, item in enumerate(node.findall("listitem"), start=1):
        body = render_children(item).strip()
        # collapse internal newlines so a bullet stays on one logical line
        body = re.sub(r"\n+", " ", body).strip()
        marker = "%d. " % i if ordered else "- "
        out.append(marker + body + "\n")
    out.append("\n")
    return "".join(out)


def _render_section(node):
    title_el = node.find("title")
    if title_el is not None:
        level = _SECT_TAGS.get(node.tag, 2)
        if level > 6:
            level = 6
        title = "".join(title_el.itertext()).strip()
        body_parts = [_text(node.text)]
        for child in node:
            if child is title_el:
                body_parts.append(_text(child.tail))
                continue
            body_parts.append(render_node(child))
            body_parts.append(_text(child.tail))
        body = "".join(body_parts).strip()
        return "\n" + ("#" * level) + " " + title + "\n\n" + body + "\n"
    return render_children(node)


def _render_parameterlist(node):
    label = _PARAMLIST_LABELS.get(node.get("kind"), "Parameters")
    lines = ["\n**%s:**\n\n" % label]
    for item in node.findall("parameteritem"):
        names = []
        for nl in item.findall("parameternamelist"):
            for pn in nl.findall("parametername"):
                names.append("".join(pn.itertext()).strip())
        desc_el = item.find("parameterdescription")
        desc = render_children(desc_el).strip() if desc_el is not None else ""
        desc = re.sub(r"\n+", " ", desc).strip()
        name_str = ", ".join("`%s`" % n for n in names if n)
        if desc:
            lines.append("- %s — %s\n" % (name_str, desc))
        else:
            lines.append("- %s\n" % name_str)
    lines.append("\n")
    return "".join(lines)


def _render_simplesect(node):
    label = _SIMPLESECT_LABELS.get(node.get("kind"), "Note")
    body = render_children(node).strip()
    body = re.sub(r"\n+", " ", body).strip()
    return "\n**%s:** %s\n" % (label, body)


def render_node(node):
    """Render a single XML element to markdown. Never silently drops content."""
    tag = node.tag
    if tag == "para":
        return render_children(node).strip() + "\n\n"
    if tag == "computeroutput":
        return "`" + render_children(node).strip() + "`"
    if tag == "emphasis":
        return "*" + render_children(node).strip() + "*"
    if tag == "bold":
        return "**" + render_children(node).strip() + "**"
    if tag in _SECT_TAGS:
        return _render_section(node)
    if tag == "ref":
        return render_children(node)
    if tag == "ulink":
        return "[%s](%s)" % (render_children(node).strip(), node.get("url", ""))
    if tag == "programlisting":
        return _render_programlisting(node)
    if tag == "itemizedlist":
        return _render_list(node, ordered=False)
    if tag == "orderedlist":
        return _render_list(node, ordered=True)
    if tag == "parameterlist":
        return _render_parameterlist(node)
    if tag == "simplesect":
        return _render_simplesect(node)
    if tag == "linebreak":
        return "\n"
    if tag == "sp":
        return " "
    if tag == "title":
        # standalone title (not consumed by a section) -> plain text
        return render_children(node)
    # Unknown tag: render its children so nothing is silently lost.
    return render_children(node)


def _collapse(text):
    # Drop trailing whitespace and whitespace-only lines (the latter would
    # otherwise render as spurious markdown indented code blocks; they come
    # from indentation inside empty Doxygen description elements).
    text = "\n".join(
        "" if not line.strip() else line.rstrip() for line in text.split("\n")
    )
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def render_description(*nodes):
    """Render the given brief/detailed elements (skipping ``None``)."""
    parts = []
    for node in nodes:
        if node is None:
            continue
        parts.append(render_children(node))
    return _collapse("\n\n".join(parts))
