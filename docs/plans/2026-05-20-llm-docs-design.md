# Design: LLM-friendly documentation for tgen

Date: 2026-05-20

## Problem

`tgen` ships rich human-facing docs built by Doxygen (HTML on GitHub Pages). We
want to *additionally* publish an LLM-friendly markdown version so an agent can
be handed a single link, navigate the docs by fetching, and learn to use the
library correctly. The existing HTML docs must stay exactly as they are.

## Requirements (decided during brainstorming)

- **Scope: full reference.** Capture both doc sources — the hand-written `.dox`
  group pages (prose + `cpp` examples) *and* the inline per-function API
  comments in `single_include/tgen.h`. A `.dox`-only converter would miss the
  inline API docs, so this is generated from Doxygen's unified representation.
- **Structure: index + per-module leaves** (the `llms.txt` convention). A small
  index links to one markdown file per module; an agent fetches the index, sees
  the map, and drills into only the leaves it needs.
- **Hosting: GitHub Pages.** Output lands in `docs/build`, which the existing
  `docs.yml` workflow already deploys, giving stable
  `rsalesc.github.io/tgen/...` URLs. Pages serves `.md`/`.txt` as raw text
  (`.nojekyll` is already set), which is what an agent wants when fetching.

## Approach

Reuse the canonical Doxygen representation rather than re-parsing sources by
hand.

1. **Second Doxygen run for XML.** A new `make llms` target runs Doxygen again
   with `GENERATE_XML=YES`, `GENERATE_HTML=NO`, `GENERATE_LATEX=NO` into a
   throwaway `docs/build/xml` dir (same `@INCLUDE = Doxyfile` + override pipe
   pattern the existing `doc` target uses). The HTML build is untouched.
2. **Self-contained converter.** `docs/llms_gen.py` (Python 3 stdlib only —
   `xml.etree`, no pip/node deps, matching the project's minimal-deps ethos)
   walks the XML and emits markdown.
3. **Cleanup.** The `xml` temp dir is deleted after conversion so raw XML never
   publishes to Pages.

`make llms` is wired into `make doc` so the two always build together locally,
and CI (`docs.yml`) runs `make doc llms` before uploading the Pages artifact.

## Output layout

```
docs/build/
  llms.txt              # index
  llms/
    base.md  opts.md  list.md  pair.md  permutation.md
    str.md   tree.md   graph.md  math.md  hack.md  misc.md
```

- **`llms.txt`** — `llms.txt`-convention index: an `H1` title, a short summary
  blockquote (mental model: generator -> value -> operation pipeline), then a
  list of modules. Each entry shows the module's one-line `@brief` and an
  **absolute** `https://<base>/llms/<mod>.md` URL. The base URL is a make
  variable (default `rsalesc.github.io/tgen`) so the upstream fork can override
  it.
- **`llms/<mod>.md`** — per group: the group overview + examples, then every
  member (free functions from the group's `sectiondef`s and members of
  `innerclass` structs like `list`, `str`, `wtree`, `wgraph`). Each member
  renders signature, template params, params, returns, and description.

## Markdown rendering rules (Doxygen XML -> markdown)

Walk `detaileddescription`/`briefdescription` nodes:

| XML node | Markdown |
|---|---|
| `<para>` | paragraph |
| `<programlisting>` (codeline/highlight) | ` ```cpp ` fenced block |
| `<computeroutput>` (`@tt{}`, `@pname{}`) | inline `` `code` `` |
| `<itemizedlist>/<listitem>` | `- ` bullets |
| `<emphasis>` / `<bold>` | `*italic*` / `**bold**` |
| `<heading level=n>` (e.g. `### Examples`) | `#`*n* heading |
| `<ref>` | link text (kept inline) |
| `<ulink url=...>` | `[text](url)` |
| `<parameterlist kind="param"/"templateparam">` | params section |
| `<simplesect kind="return">` | `**Returns:**` line |

Member signatures are built from `<type>`, `<name>`, and `<argsstring>` (or
reconstructed from `<param>` children).

## Validation

The script self-checks its output and exits non-zero if:

- any unconverted `@command{...}` Doxygen markup leaks into the markdown, or
- the index or any of the 11 expected module files is missing.

No new test framework is introduced (consistent with the gtest-only C++ suite).
CI's existing docs build will surface failures.

## Local development note

Doxygen is not installed on the dev machine; install via `brew install doxygen`
to develop/test the converter. XML format is stable across versions, so the
brew version is fine even though CI pins 1.14.0.

## Out of scope

- Changing the existing HTML docs in any way.
- A combined single-file (`llms-full.txt`) dump — can be added later if needed.
