# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file contains the Sphinx configuration for the varm project.
# It defines the project metadata, extensions, and styling.
#
# For the full list of built-in configuration values, see:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# ---------------------------------------------------------------------------
# Project information
# ---------------------------------------------------------------------------

project = "varm"
copyright = "2025, varm Contributors"
author = "varm Contributors"
release = "0.1.0"
version = "0.1.0"

# Tagline shown on the HTML title page
master_doc = "index"
language = "en"

# ---------------------------------------------------------------------------
# Extensions
# ---------------------------------------------------------------------------

extensions = [
    # Breathe - Doxygen integration
    "breathe",
    # Autodoc - Automatic documentation from docstrings
    "sphinx.ext.autodoc",
    # Napoleon - Google/NumPy style docstrings
    "sphinx.ext.napoleon",
    # Viewcode - Add links to source code
    "sphinx.ext.viewcode",
    # Mermaid - Diagram support
    "sphinxcontrib.mermaid",
    # Autodoc type hints
    "sphinx_autodoc_typehints",
    # Intersphinx - Links to external docs
    "sphinx.ext.intersphinx",
    # Todo - For TODO notes
    "sphinx.ext.todo",
]

# ---------------------------------------------------------------------------
# Breathe configuration
# ---------------------------------------------------------------------------

# Path to Doxygen XML output
breathe_projects = {"varm": "doxy-output/xml"}

# Breathe default project
breathe_default_project = "varm"

# ---------------------------------------------------------------------------
# Autodoc configuration
# ---------------------------------------------------------------------------

# Order of members in documentation
autodoc_member_order = "alphabetical"

# Document class members
autodoc_default_options = {
    "members": True,
    "undoc-members": True,
    "show-inheritance": True,
    "inherited-members": True,
}

# ---------------------------------------------------------------------------
# Napoleon configuration
# ---------------------------------------------------------------------------

napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = True
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True

# ---------------------------------------------------------------------------
# HTML output configuration
# ---------------------------------------------------------------------------

# Theme
html_theme = "pydata_sphinx_theme"

html_theme_options = {
    "default_mode": "dark",
    "show_nav_level": 1,
    "navigation_depth": 3,
    "collapse_navigation": True,
    "show_prev_next": True,
}

# Static files path (required for custom CSS/JS)
html_static_path = ["_static"]

html_css_files = [
    "css/carbon-theme.css",
    "css/ibm-plex-serif.css",
    "custom.css",
]

html_js_files = [
    "js/progress-bar.js",
    "js/theme-persistence.js",
]

# HTML output directory
html_output_dir = "html"

# HTML file suffix
html_suffix = ".html"

# HTML division by files
html_split_index = False

# HTML help files
html_use_index = True
html_genindex = "genindex.html"
html_split_nodes = True

# HTML tables
html_table_width = 0

# HTML additional files
html_extra_path = []

# ---------------------------------------------------------------------------
# LaTeX output configuration
# ---------------------------------------------------------------------------

# Paper size
latex_paper_size = "a4"

# Font size
latex_font_size = "10pt"

# LaTeX preamble
latex_elements = {
    "preamble": r"""
\usepackage{amsmath}
\usepackage{amssymb}
\usepackage{xcolor}
\usepackage{graphicx}
\usepackage{hyperref}
\usepackage{minted}
\usemintedstyle{colorful}
""",
    "fontpkg": r"""
\usepackage[T1]{fontenc}
\usepackage{charter}
""",
}

# LaTeX documents
latex_documents = [
    (master_doc, "varm.tex", "varm Documentation", "varm Contributors", "manual"),
]

# ---------------------------------------------------------------------------
# Mermaid configuration
# ---------------------------------------------------------------------------

mermaid_version = "10.6.1"
mermaid_js_url = "_static/mermaid/mermaid.min.js"

# ---------------------------------------------------------------------------
# Intersphinx configuration
# ---------------------------------------------------------------------------

intersphinx_mapping = {
    "python": ("https://docs.python.org/3", None),
}

# ---------------------------------------------------------------------------
# Source file patterns
# ---------------------------------------------------------------------------

exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "**.dox",
    "**.inc",
]

# ---------------------------------------------------------------------------
# Linkcheck configuration
# ---------------------------------------------------------------------------

linkcheck_timeout = 30
linkcheck_retries = 2
linkcheck_workers = 5

# ---------------------------------------------------------------------------
# Extensions configuration
# ---------------------------------------------------------------------------

# Todo notes
todo_include_todos = True

# ---------------------------------------------------------------------------
# Spelling
# ---------------------------------------------------------------------------

# Uncomment to enable spell checking (requires sphinxcontrib-spelling)
# spelling_lang = 'en_US'
# spelling_word_list = ['varm', 'vasm', 'ARM', 'RISC']

# ---------------------------------------------------------------------------
# Custom configuration
# ---------------------------------------------------------------------------

# Custom variables passed to templates
templates_path = ["_templates"]

# Path to custom template files
# templates_path = []

# Footer
html_last_updated_fmt = "%Y-%m-%d"

# Additional metadata
epub_title = project
epub_author = author
epub_publisher = author
epub_copyright = copyright
