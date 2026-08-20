#!/usr/bin/env python3
"""Verify every relative import in webui/ resolves to a file that exists and
exports the names being imported.

There is no bundler or type checker here, so a renamed export or a moved file
fails only at runtime, in a browser, on the code path that happens to touch
it. This is the cheapest thing that catches it. Run from webui/, or pass the
directory as the first argument."""

import os
import re
import sys

IMPORT = re.compile(r'import\s+(?:([\w*]+|\{[^}]*\})\s+from\s+)?["\'](\.[^"\']+)["\']', re.S)
EXPORT_DECL = re.compile(r'export\s+(?:async\s+)?(?:function|class|const|let|var)\s+(\w+)')
EXPORT_LIST = re.compile(r'export\s*\{([^}]*)\}')


def source_files(root):
    for base, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d not in {"vendor", ".git"}]
        for name in sorted(files):
            if name.endswith((".js", ".html")):
                yield os.path.join(base, name)


def exported_names(path):
    text = open(path, encoding="utf-8").read()
    names = set(EXPORT_DECL.findall(text))
    for group in EXPORT_LIST.findall(text):
        for part in group.split(","):
            part = part.strip()
            if part:
                names.add(part.split(" as ")[-1].strip())
    return names


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    exports = {
        os.path.normpath(p): exported_names(p)
        for p in source_files(root)
        if p.endswith(".js")
    }

    problems = []
    checked = 0

    for path in source_files(root):
        text = open(path, encoding="utf-8").read()
        for clause, spec in IMPORT.findall(text):
            target = os.path.normpath(os.path.join(os.path.dirname(path), spec))
            checked += 1
            if not os.path.exists(target):
                problems.append(f"{path}: imports {spec}, which does not exist")
                continue
            # Vendored bundles are not scanned for exports, so they are checked
            # for existence only.
            if not clause.startswith("{") or target not in exports:
                continue
            for part in clause.strip("{}").split(","):
                name = part.strip().split(" as ")[0].strip()
                if name and name not in exports[target]:
                    problems.append(f"{path}: imports {{{name}}} from {spec}, which does not export it")

    for problem in problems:
        print(problem, file=sys.stderr)
    print(f"checked {checked} imports across {len(exports)} modules")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
