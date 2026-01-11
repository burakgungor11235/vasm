# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#

project = "varm"
copyright = "2025, varm Contributors"
author = "Burak Güngör"
release = "0.1.0"
version = "0.1.0"

# html_title = "varm"
html_short_title = "varm"
html_copy_source = False
html_use_index = True
html_split_index = False

master_doc = "index"
language = "en"

extensions = [
    "sphinx_immaterial",
    "sphinx.ext.autodoc",
    "sphinx.ext.napoleon",
    "sphinx.ext.viewcode",
    "sphinxcontrib.mermaid",
    "sphinx.ext.todo",
    "sphinx_copybutton",
    "varm_doctest",
]

html_theme = "sphinx_immaterial"
html_suffix = ".html"
html_static_path = ["source/_static"]
html_css_files = ["custom.css"]

# Immaterial theme options
html_theme_options = {
    "icon": {
        "repo": "fontawesome/brands/github",
    },
    "site_url": "https://vasm.readthedocs.io/",
    "repo_url": "https://github.com/burakgungor11235/vasm",
    "repo_name": "vasm",
    "edit_uri": "blob/main/docs/source",
    "globaltoc_collapse": True,
    "features": [
        "navigation.sections",
        "navigation.top",
        "search.suggest",
        "search.highlight",
        "search.share",
        "toc.follow",
        "toc.sticky",
        "content.tabs.link",
        "announce.dismiss",
    ],
    "palette": [
        {
            "media": "(prefers-color-scheme: light)",
            "scheme": "default",
            "primary": "grey",
            "accent": "blue",
            "toggle": {
                "icon": "material/brightness-7",
                "name": "Switch to dark mode",
            },
        },
        {
            "media": "(prefers-color-scheme: dark)",
            "scheme": "slate",
            "primary": "grey",
            "accent": "blue",
            "toggle": {
                "icon": "material/brightness-4",
                "name": "Switch to light mode",
            },
        },
    ],
    "font": {
        "text": "IBM Plex Sans",
        "code": "IBM Plex Mono",
    },
}


# Mermaid
mermaid_version = "10.6.1"
mermaid_js_url = "_static/mermaid/mermaid.min.js"

# Lexer
import sys
import os

_ext_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "source", "_ext"))
if _ext_path not in sys.path:
    sys.path.insert(0, _ext_path)

_static_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "source", "_static"))
_pygments_path = os.path.join(_static_path, "pygments")
if _pygments_path not in sys.path:
    sys.path.insert(0, _pygments_path)

try:
    lexer_module = __import__("vasm_lexer")
    VarmLexer = lexer_module.VarmLexer
    from sphinx.highlighting import lexers

    lexers["varm"] = VarmLexer(startinline=True)
    lexers["vasm"] = VarmLexer(startinline=True)
except Exception:
    pass

pygments_style = "monokai"

# Copy button
copybutton_prompt_text = r">>> |\$ |... |   \.\.\. "
copybutton_remove_prompt_markers = True
copybutton_copy_empty_lines = False
copybutton_minimum_lines = 1

# Exclude patterns
exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
]

# Linkcheck
linkcheck_timeout = 30
linkcheck_retries = 2
linkcheck_workers = 5

# varm doctest
varm_binary = "/home/personal/Projects/varm/build/src/varm"
vasm_binary = "/home/personal/Projects/varm/build/src/vasm"
varm_test_timeout = 10
varm_test_dir = None
