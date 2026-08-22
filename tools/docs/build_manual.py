#!/usr/bin/env python3
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Build the manual site and the in-app help index from one set of sources.
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
"""
Reads docs/manual/*.md and writes two things:

  site/            the manual as a static site, for GitHub Pages
  webui/manual.json  the same prose, keyed by topic, for the editor's help

The point of doing both from one source is that the help text a user sees
when they hover a control in the editor IS the manual - there is no second
copy to fall out of date.

A topic is any heading carrying an explicit id, written as:

    ### Detent mode {: #detent-mode }

Its summary is the first paragraph beneath it, and its detail is everything
up to the next heading of the same or higher level. The editor shows the
summary in a popover and links through to the full section on the site.
"""
import html
import json
import pathlib
import re
import sys

try:
    import markdown
    import yaml
except ImportError as exc:  # pragma: no cover - reported, not handled
    sys.exit(f"error: {exc.name} is required - pip install markdown pyyaml")

ROOT = pathlib.Path(__file__).resolve().parents[2]
MANUAL_DIR = ROOT / "docs" / "manual"
SITE_DIR = ROOT / "site"
WEBUI_DIR = ROOT / "webui"

FRONT_MATTER = re.compile(r"\A---\n(.*?)\n---\n", re.S)
HEADING = re.compile(r"^(#{1,6})\s+(.*?)(?:\s*\{:\s*#([a-z0-9-]+)\s*\})?\s*$", re.M)


def read_page(path):
    """Split a manual file into its front matter and body."""
    text = path.read_text(encoding="utf-8")
    match = FRONT_MATTER.match(text)

    if not match:
        sys.exit(f"error: {path.relative_to(ROOT)} has no front matter")

    meta = yaml.safe_load(match.group(1)) or {}
    body = text[match.end():]

    for required in ("id", "title", "summary"):
        if required not in meta:
            sys.exit(f"error: {path.relative_to(ROOT)} is missing '{required}'")

    meta["order"] = meta.get("order", 999)
    return meta, body


# Filled in by main(): source filename -> page id.
PAGE_IDS = {}

SOURCE_LINK = re.compile(r'href="([0-9]+-[a-z-]+\.md)(#[^"]*)?"')


def render(body):
    """Markdown to HTML, with heading ids and tables.

    Cross-references are written in the sources as ordinary links to the
    other markdown file, so the manual reads correctly on GitHub as well as
    on the generated site. They are rewritten to site pages here.
    """
    out = markdown.markdown(
        body,
        extensions=["extra", "attr_list", "toc", "sane_lists"],
        output_format="html5",
    )

    def to_page(match):
        filename, fragment = match.group(1), match.group(2) or ""
        if filename not in PAGE_IDS:
            sys.exit(f"error: link to unknown manual page '{filename}'")
        return f'href="{PAGE_IDS[filename]}.html{fragment}"'

    return SOURCE_LINK.sub(to_page, out)


def strip_tags(fragment):
    """Plain text of a rendered fragment, on one line."""
    text = re.sub(r"<[^>]+>", "", fragment)
    return re.sub(r"\s+", " ", text).strip()


def extract_topics(page_meta, body):
    """Pull every explicitly-identified heading out as a help topic."""
    topics = {}
    headings = list(HEADING.finditer(body))

    for index, match in enumerate(headings):
        level, title, topic_id = len(match.group(1)), match.group(2), match.group(3)

        if not topic_id:
            continue

        # The section runs until the next heading at the same level or higher.
        end = len(body)
        for later in headings[index + 1:]:
            if len(later.group(1)) <= level:
                end = later.start()
                break

        section = body[match.end():end].strip()

        # The summary is the first paragraph - the part worth showing in a
        # popover, rather than the whole section.
        first_para = section.split("\n\n", 1)[0].strip()

        if topic_id in topics:
            sys.exit(f"error: duplicate help topic id '{topic_id}'")

        topics[topic_id] = {
            "title": title.strip(),
            "summary": strip_tags(render(first_para)),
            "html": render(section),
            "page": page_meta["id"],
            "pageTitle": page_meta["title"],
            "anchor": topic_id,
        }

    return topics


def page_template(meta, content, nav, version):
    """One page of the manual site."""
    return f"""<!doctype html>
<html lang="en-GB">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(meta['title'])} - NEON_SAMURAI</title>
<meta name="description" content="{html.escape(meta['summary'])}">
<link rel="stylesheet" href="manual.css">
</head>
<body>
<a class="skip" href="#content">Skip to content</a>
<header class="site-header">
  <a class="site-header__brand" href="index.html">NEON_SAMURAI</a>
  <nav class="site-header__nav">
    <a href="index.html">Manual</a>
    <a href="../webui/index.html">Editor</a>
    <a href="https://github.com/nic-starke/neon_samurai">Source</a>
  </nav>
</header>
<div class="layout">
  <nav class="sidebar" aria-label="Manual contents">{nav}</nav>
  <main id="content" class="content">
    <h1>{html.escape(meta['title'])}</h1>
    <p class="lede">{html.escape(meta['summary'])}</p>
    {content}
  </main>
</div>
<footer class="site-footer">
  <p>NEON_SAMURAI {html.escape(version)} - MIT licensed. This firmware is not
  made by, or endorsed by, DJ Tech Tools.</p>
</footer>
</body>
</html>
"""


def project_version():
    text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r"^\s*VERSION\s+([0-9.]+)", text, re.M)
    return match.group(1) if match else "unknown"


def main():
    if not MANUAL_DIR.is_dir():
        sys.exit(f"error: {MANUAL_DIR.relative_to(ROOT)} does not exist")

    pages = []
    for path in sorted(MANUAL_DIR.glob("*.md")):
        meta, body = read_page(path)
        PAGE_IDS[path.name] = meta["id"]
        pages.append((meta, body))

    pages.sort(key=lambda p: (p[0]["order"], p[0]["id"]))

    if not pages:
        sys.exit("error: no manual pages found")

    version = project_version()

    nav_items = "".join(
        f'<a href="{meta["id"]}.html">{html.escape(meta["title"])}</a>'
        for meta, _ in pages
    )
    nav = f"<div class=\"sidebar__list\">{nav_items}</div>"

    SITE_DIR.mkdir(exist_ok=True)

    # Static assets travel with the generator, so site/ stays entirely generated.
    for asset in (pathlib.Path(__file__).parent / "assets").iterdir():
        (SITE_DIR / asset.name).write_bytes(asset.read_bytes())

    topics = {}
    for meta, body in pages:
        topics.update(extract_topics(meta, body))

        out = SITE_DIR / f"{meta['id']}.html"
        out.write_text(page_template(meta, render(body), nav, version), encoding="utf-8")

    # The first page doubles as the site index.
    first = pages[0][0]["id"]
    (SITE_DIR / "index.html").write_text(
        (SITE_DIR / f"{first}.html").read_text(encoding="utf-8"), encoding="utf-8"
    )

    manifest = {
        "version": version,
        "topics": topics,
        "pages": [
            {"id": m["id"], "title": m["title"], "summary": m["summary"]}
            for m, _ in pages
        ],
    }
    (WEBUI_DIR / "manual.json").write_text(
        json.dumps(manifest, indent=1, sort_keys=True) + "\n", encoding="utf-8"
    )

    print(f"{len(pages)} page(s), {len(topics)} help topic(s), version {version}")


if __name__ == "__main__":
    main()
