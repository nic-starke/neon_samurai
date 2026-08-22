#!/usr/bin/env python3
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Check that the manual's internal links point at something real.
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
"""
A cross-reference in the manual that points at a page or anchor which does
not exist is invisible until a reader clicks it. Run after build_manual.py.
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
SITE = ROOT / "site"
MANIFEST = ROOT / "webui" / "manual.json"

LINK = re.compile(r'href="([^"]+)"')


def main():
    if not SITE.is_dir() or not MANIFEST.is_file():
        sys.exit("error: run tools/docs/build_manual.py first")

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))

    # Every id the manual can be linked to, per page.
    anchors = {}
    for page in SITE.glob("*.html"):
        ids = set(re.findall(r'id="([^"]+)"', page.read_text(encoding="utf-8")))
        anchors[page.name] = ids

    problems = []

    for page in sorted(SITE.glob("*.html")):
        for href in LINK.findall(page.read_text(encoding="utf-8")):
            if href.startswith(("http://", "https://", "mailto:")):
                continue

            target, _, fragment = href.partition("#")

            if target == "":
                target = page.name

            # A link that climbs out of site/ - the editor, for instance -
            # is checked as a path on disk, because the deployed tree keeps
            # site/ and webui/ as siblings just as a checkout does.
            if target.startswith(".."):
                if not (SITE / target).resolve().exists():
                    problems.append(f"{page.name}: link leaves the site and misses: {target}")
                continue

            if not target.endswith(".html"):
                if not (SITE / target).exists():
                    problems.append(f"{page.name}: missing file {target}")
                continue

            if target not in anchors:
                problems.append(f"{page.name}: link to missing page {target}")
                continue

            if fragment and fragment not in anchors[target]:
                problems.append(f"{page.name}: link to missing anchor {href}")

    # Every help topic must resolve to a real anchor on a real page, or the
    # editor's "read more" link goes nowhere.
    for topic_id, topic in manifest["topics"].items():
        page = f"{topic['page']}.html"
        if page not in anchors:
            problems.append(f"topic {topic_id}: page {page} does not exist")
        elif topic["anchor"] not in anchors[page]:
            problems.append(f"topic {topic_id}: anchor #{topic['anchor']} not on {page}")

    # The editor asks for help by topic id. Renaming a heading anchor in the
    # manual silently breaks those call sites otherwise - which is precisely
    # the drift that generating the help from the manual is meant to prevent.
    used = set()
    for source in sorted((ROOT / "webui").rglob("*.js")):
        text = source.read_text(encoding="utf-8")
        for match in re.finditer(
            r'(?:helpIcon|topic)\(\s*"([a-z0-9-]+)"|attachHelp\([^,]+,\s*"([a-z0-9-]+)"',
            text,
        ):
            topic_id = match.group(1) or match.group(2)
            used.add((topic_id, source.relative_to(ROOT)))

    for topic_id, source in sorted(used):
        if topic_id not in manifest["topics"]:
            problems.append(f"{source}: asks for help topic '{topic_id}', which the manual does not define")

    if problems:
        for problem in problems:
            print(f"error: {problem}", file=sys.stderr)
        sys.exit(f"{len(problems)} broken link(s)")

    total = sum(len(v) for v in anchors.values())
    print(
        f"{len(anchors)} page(s), {total} anchor(s), "
        f"{len(manifest['topics'])} topic(s), {len(used)} help reference(s) "
        "- all resolve"
    )


if __name__ == "__main__":
    main()
