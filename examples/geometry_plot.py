#!/usr/bin/env python3
"""Scatter-plot geometry example output (x y lines) as a standalone HTML file."""

from __future__ import annotations

import argparse
import html
import sys
from typing import Iterable


def read_points(lines: Iterable[str]) -> list[tuple[int, float, float]]:
    points: list[tuple[int, float, float]] = []
    for line in lines:
        line = line.strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) < 2:
            continue
        try:
            x = float(parts[0])
            y = float(parts[1])
        except ValueError:
            continue
        points.append((len(points), x, y))
    return points


def point_visual_style(n: int) -> tuple[float, float, bool]:
    """Return (radius, stroke_width, show_index) scaled for point count."""
    if n <= 32:
        return 5.0, 1.5, True
    if n <= 128:
        return 3.0, 1.0, False
    if n <= 512:
        return 2.0, 0.75, False
    return 1.25, 0.5, False


def render_html(points: list[tuple[int, float, float]]) -> str:
    if not points:
        return """<!DOCTYPE html>
<html lang="en">
<head><meta charset="utf-8"><title>geometry plot</title></head>
<body><p>No points to plot.</p></body>
</html>
"""

    xs = [p[1] for p in points]
    ys = [p[2] for p in points]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    span_x = max_x - min_x or 1.0
    span_y = max_y - min_y or 1.0
    pad = 0.08 * max(span_x, span_y)
    plot_min_x = min_x - pad
    plot_max_x = max_x + pad
    plot_min_y = min_y - pad
    plot_max_y = max_y + pad
    plot_span_x = plot_max_x - plot_min_x
    plot_span_y = plot_max_y - plot_min_y

    width = 720
    height = 720
    margin = 48

    def sx(x: float) -> float:
        return margin + (x - plot_min_x) / plot_span_x * (width - 2 * margin)

    def sy(y: float) -> float:
        return height - margin - (y - plot_min_y) / plot_span_y * (height - 2 * margin)

    grid_lines: list[str] = []
    for i in range(6):
        t = i / 5
        gx = plot_min_x + t * plot_span_x
        gy = plot_min_y + t * plot_span_y
        grid_lines.append(
            f'<line x1="{sx(gx):.2f}" y1="{margin}" x2="{sx(gx):.2f}" y2="{height - margin}" '
            f'class="grid"/>'
        )
        grid_lines.append(
            f'<line x1="{margin}" y1="{sy(gy):.2f}" x2="{width - margin}" y2="{sy(gy):.2f}" '
            f'class="grid"/>'
        )

    radius, stroke_width, show_index = point_visual_style(len(points))

    point_marks: list[str] = []
    for idx, x, y in points:
        cx, cy = sx(x), sy(y)
        label = (
            f'<text x="{cx + radius + 3:.2f}" y="{cy - radius - 3:.2f}">'
            f"{html.escape(str(idx))}</text>"
            if show_index
            else ""
        )
        point_marks.append(
            f'<g class="point">'
            f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="{radius:.2f}"/>'
            f"{label}"
            f'<title>#{idx} ({html.escape(f"{x:g}")}, {html.escape(f"{y:g}")})</title>'
            f"</g>"
        )

    summary = (
        f"{len(points)} points &middot; "
        f"x &isin; [{min_x:g}, {max_x:g}] &middot; "
        f"y &isin; [{min_y:g}, {max_y:g}]"
    )

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>geometry plot</title>
  <style>
    :root {{
      color-scheme: light dark;
      --bg: #f4f4f5;
      --panel: #ffffff;
      --text: #18181b;
      --muted: #71717a;
      --grid: #d4d4d8;
      --point: #2563eb;
      --point-stroke: #1d4ed8;
    }}
    @media (prefers-color-scheme: dark) {{
      :root {{
        --bg: #09090b;
        --panel: #18181b;
        --text: #fafafa;
        --muted: #a1a1aa;
        --grid: #3f3f46;
        --point: #60a5fa;
        --point-stroke: #93c5fd;
      }}
    }}
    body {{
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      background: var(--bg);
      color: var(--text);
      font-family: ui-sans-serif, system-ui, sans-serif;
    }}
    main {{
      width: min(760px, calc(100vw - 32px));
      background: var(--panel);
      border-radius: 16px;
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.08);
      padding: 24px;
    }}
    h1 {{
      margin: 0 0 8px;
      font-size: 1.25rem;
    }}
    p {{
      margin: 0 0 16px;
      color: var(--muted);
      font-size: 0.95rem;
    }}
    svg {{
      width: 100%;
      height: auto;
      display: block;
      border: 1px solid var(--grid);
      border-radius: 12px;
      background: color-mix(in srgb, var(--panel) 92%, var(--grid));
    }}
    .grid {{
      stroke: var(--grid);
      stroke-width: 1;
    }}
    .axis {{
      stroke: var(--muted);
      stroke-width: 1.5;
    }}
    .point circle {{
      fill: var(--point);
      stroke: var(--point-stroke);
      stroke-width: {stroke_width:.2f};
    }}
    .point text {{
      fill: var(--text);
      font-size: 12px;
      font-weight: 600;
      user-select: none;
    }}
  </style>
</head>
<body>
  <main>
    <h1>geometry::general_position</h1>
    <p>{summary}</p>
    <svg viewBox="0 0 {width} {height}" role="img" aria-label="Scatter plot of generated points">
      {''.join(grid_lines)}
      <line class="axis" x1="{margin}" y1="{height - margin}" x2="{width - margin}" y2="{height - margin}"/>
      <line class="axis" x1="{margin}" y1="{margin}" x2="{margin}" y2="{height - margin}"/>
      {''.join(point_marks)}
    </svg>
  </main>
</body>
</html>
"""


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Plot 'x y' lines from stdin into a standalone HTML scatter plot."
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Write HTML to this file (default: stdout).",
    )
    args = parser.parse_args()

    points = read_points(sys.stdin)
    doc = render_html(points)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as out:
            out.write(doc)
    else:
        sys.stdout.write(doc)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
