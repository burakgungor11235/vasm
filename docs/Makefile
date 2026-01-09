# Makefile for varm Documentation
#
# This file provides convenient commands for building the
# varm documentation using Sphinx and Doxygen.
#
# Usage:
#   make html       # Build HTML documentation
#   make pdf        # Build PDF documentation
#   make latex      # Build LaTeX source
#   make doxygen    # Generate Doxygen XML
#   make clean      # Remove build artifacts
#   make install    # Install build dependencies
#

#---------------------------------------------------------------------------
# Configuration
#---------------------------------------------------------------------------

SPHINXOPTS     ?= -j auto
SPHINXBUILD    ?= sphinx-build
PAPER          ?= a4
BUILDDIR       ?= build
DOXYGENDIR     ?= doxy-output

#---------------------------------------------------------------------------
# Phony targets (always run even if files haven't changed)
#---------------------------------------------------------------------------

.PHONY: all clean html pdf latex latexpdf doxygen install livehtml check

#---------------------------------------------------------------------------
# Default target - build HTML documentation
#---------------------------------------------------------------------------

all: html
	@echo ""
	@echo "Documentation built successfully!"
	@echo "Open $(BUILDDIR)/html/index.html in a browser."

#---------------------------------------------------------------------------
# HTML documentation
#---------------------------------------------------------------------------

html: doxygen
	@echo "Building HTML documentation..."
	$(SPHINXBUILD) -b html $(SPHINXOPTS) source $(BUILDDIR)/html
	@echo ""
	@echo "HTML documentation built in $(BUILDDIR)/html/"

#---------------------------------------------------------------------------
# PDF documentation (requires LaTeX)
#---------------------------------------------------------------------------

pdf: latex
	@echo "Building PDF documentation..."
	cd $(BUILDDIR)/latex && latexmk -pdf -interaction=nonstopmode
	@echo ""
	@echo "PDF documentation built at $(BUILDDIR)/latex/varm.pdf"

latex: doxygen
	@echo "Building LaTeX documentation..."
	$(SPHINXBUILD) -b latex $(SPHINXOPTS) -D latex_paper_size=$(PAPER) source $(BUILDDIR)/latex
	@echo ""
	@echo "LaTeX source built in $(BUILDDIR)/latex/"

#---------------------------------------------------------------------------
# Doxygen XML generation
#---------------------------------------------------------------------------

doxygen:
	@echo "Generating Doxygen XML..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile; \
		echo "Doxygen XML generated in $(DOXYGENDIR)/xml/"; \
	else \
		echo "WARNING: doxygen not found. Install doxygen to generate API docs."; \
	fi

#---------------------------------------------------------------------------
# Install dependencies
#---------------------------------------------------------------------------

install:
	@echo "Installing documentation dependencies..."
	pip install -r requirements.txt
	@if ! command -v doxygen >/dev/null 2>&1; then \
		echo ""; \
		echo "WARNING: Doxygen not installed. Install via:"; \
		echo "  Ubuntu/Debian: apt install doxygen"; \
		echo "  macOS: brew install doxygen"; \
		echo "  Windows: https://www.doxygen.nl/download.html"; \
	else \
		echo "Doxygen is already installed."; \
	fi

#---------------------------------------------------------------------------
# Live preview (requires sphinx-autobuild)
#---------------------------------------------------------------------------

livehtml: doxygen
	@echo "Starting live HTML preview at http://localhost:8000..."
	@echo "Press Ctrl+C to stop."
	sphinx-autobuild -b html --ignore "*.dox" $(SPHINXOPTS) source $(BUILDDIR)/html

#---------------------------------------------------------------------------
# Check for broken links and issues
#---------------------------------------------------------------------------

check: doxygen
	@echo "Checking documentation for issues..."
	$(SPHINXBUILD) -b linkcheck $(SPHINXOPTS) source $(BUILDDIR)/linkcheck

#---------------------------------------------------------------------------
# Clean build artifacts
#---------------------------------------------------------------------------

clean:
	@echo "Cleaning documentation build artifacts..."
	rm -rf $(BUILDDIR)
	rm -rf $(DOXYGENDIR)
	find source -name "*.dox" -type f -delete 2>/dev/null || true
	find source -name "*.inc" -type f -delete 2>/dev/null || true
	@echo "Clean complete."

#---------------------------------------------------------------------------
# Help target
#---------------------------------------------------------------------------

help:
	@echo "varm Documentation Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Build HTML documentation (default)"
	@echo "  html      - Build HTML documentation"
	@echo "  pdf       - Build PDF documentation (requires LaTeX)"
	@echo "  latex     - Build LaTeX source files"
	@echo "  doxygen   - Generate Doxygen XML for API docs"
	@echo "  install   - Install documentation dependencies"
	@echo "  livehtml  - Start live preview server"
	@echo "  check     - Check for broken links"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - Python 3.9+ with pip"
	@echo "  - Doxygen (for API documentation)"
	@echo "  - LaTeX distribution (for PDF output)"
