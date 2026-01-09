Design Documentation
====================

.. warning::

   **varm is NOT stable.** This section documents design rationale and
   trade-offs for the current implementation only. Details may change
   without notice in future releases.

This section provides design documentation for developers who want to
understand **WHY** certain decisions were made, not just **WHAT** was
implemented. Understanding the rationale helps you:

* Comprehend the design philosophy behind varm
* Make informed decisions when extending the system
* Understand trade-offs inherent in any design choice
* Recognize what was deliberately omitted and why

.. toctree::
   :maxdepth: 2
   :caption: Contents

   design-rationale
   tradeoffs
   alternatives

1. Who Should Read This
-----------------------

This documentation is for developers who:

* Want to contribute to varm and need context on design decisions
* Are studying the implementation for educational purposes
* Need to understand the reasoning behind architectural choices
* Want to extend varm and need to understand constraints
* Are curious about the trade-offs involved in system design

2. What This Is Not
-------------------

This documentation does **NOT** cover:

* How to use varm (see :doc:`../../user/index`)
* Complete API documentation (see source code comments)
* Performance benchmarks (see :doc:`../../user/reference/index`)
* Future plans or roadmaps (see GitHub issues)

3. Design Philosophy
--------------------

varm's design philosophy prioritizes, in order:

1. **Educational value**: The system should be easy to understand
2. **Small implementation**: Code should be study-able
3. **ARM-like**: Familiar to those who know ARM architecture
4. **Fast compile times**: Quick development iteration

Performance is important, but not at the expense of simplicity.
This philosophy shapes every design decision.

4. Document Structure
---------------------

**design-rationale.rst**
   Explains the reasoning behind key architectural decisions. Each
   decision is presented with the problem it solves, the constraints
   considered, and why the chosen approach was selected.

**tradeoffs.rst**
   Documents specific trade-offs made during design. Every non-trivial
   design choice involves trade-offs; this section makes them explicit
   so you understand the costs and benefits.

**alternatives.rst**
   Describes approaches that were considered but rejected. Understanding
   why alternatives were not chosen is as important as understanding
   why the chosen approach was selected.

5. Caveats and Warnings
-----------------------

Throughout this documentation, you will find explicit warnings that
varm is not stable. This is intentional:

* The internal API may change without notice
* Implementation details may be refactored
* Design decisions may be revisited
* Performance characteristics may shift

If you build tools on top of varm, expect to update them when the
internal implementation changes.
