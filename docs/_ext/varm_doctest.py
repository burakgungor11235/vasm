"""
varm_doctest: Sphinx extension for testing varm code examples

This extension provides a ``code-with-test`` directive that:
1. Displays varm assembly code with syntax highlighting
2. Assembles and runs the code using vasm and varm
3. Validates output against expected stdout and exit code
4. Fails the documentation build if tests fail

Usage:
    .. code-with-test:: varm
       :stdin: Hello
       :expected_stdout: HELLO
       :expected_exit: 0

       ; varm code here
       mov r0, #42
       halt

Configuration:
    varm_binary - Path to varm VM executable (default: "varm")
    vasm_binary - Path to vasm assembler (default: "varm")
    varm_test_timeout - Timeout in seconds (default: 10)
"""

import os
import sys
import tempfile
import shutil
import subprocess
import atexit
from pathlib import Path
from typing import Optional

from docutils import nodes
from docutils.statemachine import StringList
from sphinx.directives.code import CodeBlock
from sphinx.application import Sphinx
from sphinx.errors import SphinxError
from sphinx.util import logging

logger = logging.getLogger(__name__)


class VarmTestError(SphinxError):
    """Raised when a varm doctest fails."""

    pass


def normalize_output(text: str) -> str:
    """
    Normalize output for comparison by removing invisible characters.

    Strips:
    - Null bytes (\x00)
    - Leading/trailing whitespace
    - Carriage returns

    Args:
        text: The text to normalize

    Returns:
        Normalized text suitable for comparison
    """
    if text is None:
        return ""
    result = text.replace("\x00", "")
    result = result.replace("\r", "")
    return result.strip()


def run_varm_test(
    code: str,
    stdin: str,
    expected_stdout: str,
    expected_exit: int,
    varm_binary: str,
    vasm_binary: str,
    timeout: int,
    test_dir: str,
    test_name: str = "unnamed test",
) -> tuple[bool, str, str, int]:
    """
    Assemble and run varm code, validating output.

    Args:
        code: varm assembly code
        stdin: Input to pipe to program
        expected_stdout: Expected stdout (normalized for comparison)
        expected_exit: Expected exit code
        varm_binary: Path to varm executable
        vasm_binary: Path to vasm executable
        timeout: Seconds before timeout
        test_dir: Directory for temporary files
        test_name: Name for error messages

    Returns:
        Tuple of (success, stdout, stderr, exit_code)
    """
    temp_dir: str = tempfile.mkdtemp(prefix="varm_doctest_", dir=test_dir)

    try:
        vasm_file: str = os.path.join(temp_dir, "test.vasm")
        with open(vasm_file, "w", encoding="utf-8") as f:
            f.write(code)

        varm_file: str = os.path.join(temp_dir, "test.vm")
        assemble_result: subprocess.CompletedProcess = subprocess.run(
            [vasm_binary, "-o", varm_file, vasm_file],
            capture_output=True,
            text=True,
            timeout=timeout,
        )

        if assemble_result.returncode != 0:
            return False, "", assemble_result.stderr, assemble_result.returncode

        run_result: subprocess.CompletedProcess = subprocess.run(
            [varm_binary, varm_file],
            input=stdin,
            capture_output=True,
            text=True,
            timeout=timeout,
        )

        actual_stdout: str = run_result.stdout
        actual_exit: int = run_result.returncode

        normalized_expected: str = normalize_output(expected_stdout)
        normalized_actual: str = normalize_output(actual_stdout)

        stdout_ok: bool = normalized_expected == normalized_actual
        exit_ok: bool = actual_exit == expected_exit

        success: bool = stdout_ok and exit_ok
        return success, actual_stdout, run_result.stderr, actual_exit

    except subprocess.TimeoutExpired:
        return False, "", f"Test timed out after {timeout} seconds", -1
    except FileNotFoundError as e:
        return False, "", str(e), -1
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


def escape_rst(text: str) -> str:
    """Escape special RST characters in text for display."""
    if text is None:
        return ""
    special_chars: dict[str, str] = {
        "*": "\\*",
        "`": "\\`",
        "_": "\\_",
        "|": "\\|",
        "[": "\\[",
        "]": "\\]",
        "<": "\\<",
        ">": "\\>",
        "{": "\\{",
        "}": "\\}",
        "\\": "\\\\",
    }
    for char, escaped in special_chars.items():
        text = text.replace(char, escaped)
    return text


def process_escape_sequences(text: str) -> str:
    """Process escape sequences like <newline> in expected output.

    Supported escapes:
        <newline> - Newline character (\\n)
        <tab> - Tab character (\\t)
    """
    if text is None:
        return ""
    result: str = text.replace("<newline>", "\n")
    result = result.replace("<tab>", "\t")
    return result


class CodeWithTest(CodeBlock):
    """
    Directive for displaying and testing varm code.

    Displays the code with syntax highlighting and validates it by
    assembling and running with varm/vasm.

    Example:
        .. code-with-test:: varm
           :stdin: Hello
           :expected_stdout: HELLO
           :expected_exit: 0

           mov r0, #42
           halt
    """

    option_spec = CodeBlock.option_spec.copy()
    option_spec["stdin"] = lambda x: x
    option_spec["expected_stdout"] = lambda x: x
    option_spec["expected_exit"] = lambda x: x
    option_spec["test-name"] = lambda x: x

    def run(self) -> list[nodes.Node]:
        """
        Process the directive: test code, then display it.

        If the test fails, raises BuildError with detailed logs.
        """

        # temporary fix, will be adding new env vars in qol.sh in the future for this kind of workload.
        app: Sphinx = self.state.document.settings.env.app
        varm_binary: str = app.config.varm_binary
        vasm_binary: str = app.config.vasm_binary
        timeout: int = app.config.varm_test_timeout
        test_dir: Optional[str] = app.config.varm_test_dir

        if test_dir is None:
            test_dir = tempfile.gettempdir()

        test_name: str = self.options.get("test-name", "doctest")
        stdin: str = self.options.get("stdin", "") or ""
        expected_stdout: str = self.options.get("expected_stdout", "") or ""
        expected_exit_str: str = self.options.get("expected_exit", "0") or "0"

        # Portability: Check if we should skip tests
        is_prod = os.environ.get("is_prod", "").lower() in ("true", "1", "yes")
        
        # If in production or binaries are missing, just show the code
        if is_prod:
            logger.info(f"[{test_name}] skipping test (is_prod=True)")
            return super().run()

        # Check if binaries exist before trying to run
        if not os.path.exists(varm_binary) or not os.path.exists(vasm_binary):
            logger.warning(f"[{test_name}] skipping test: binaries not found at {varm_binary} or {vasm_binary}")
            return super().run()

        expected_stdout = process_escape_sequences(expected_stdout)

        try:
            expected_exit: int = int(expected_exit_str)
        except ValueError:
            raise VarmTestError(
                f"Invalid exit code '{expected_exit_str}', must be an integer"
            )

        code: str = "\n".join(self.content)

        success: bool
        actual_stdout: str
        stderr: str
        exit_code: int
        success, actual_stdout, stderr, exit_code = run_varm_test(
            code=code,
            stdin=stdin,
            expected_stdout=expected_stdout,
            expected_exit=expected_exit,
            varm_binary=varm_binary,
            vasm_binary=vasm_binary,
            timeout=timeout,
            test_dir=test_dir,
            test_name=test_name,
        )

        if not success:
            error_msg: str = self._format_error(
                test_name=test_name,
                code=code,
                stdin=stdin,
                expected_stdout=expected_stdout,
                actual_stdout=actual_stdout,
                expected_exit=expected_exit,
                actual_exit=exit_code,
                stderr=stderr,
            )
            raise VarmTestError(error_msg)

        nodes_list: list[nodes.Node] = super().run()

        for node in nodes_list:
            if isinstance(node, nodes.container):
                node.setdefault("classes", []).append("varm-doctest-success")

        return nodes_list

    def _format_error(
        self,
        test_name: str,
        code: str,
        stdin: str,
        expected_stdout: str,
        actual_stdout: str,
        expected_exit: int,
        actual_exit: int,
        stderr: str,
    ) -> str:
        """Format detailed error message for test failure."""
        lines: list[str] = [
            "=" * 80,
            f"VARMDOCTEST FAILURE: {test_name}",
            "=" * 80,
            "",
            "CODE:",
            "-" * 40,
            escape_rst(code),
            "",
        ]

        if stdin:
            lines.extend(
                [
                    "STDIN:",
                    "-" * 40,
                    escape_rst(stdin),
                    "",
                ]
            )

        normalized_expected: str = normalize_output(expected_stdout)
        normalized_actual: str = normalize_output(actual_stdout)

        lines.extend(
            [
                "EXPECTED STDOUT (normalized):",
                "-" * 40,
                escape_rst(normalized_expected),
                "",
                "ACTUAL STDOUT (normalized):",
                "-" * 40,
                escape_rst(normalized_actual),
                "",
                "RAW EXPECTED:",
                "-" * 40,
                escape_rst(repr(expected_stdout)),
                "",
                "RAW ACTUAL:",
                "-" * 40,
                escape_rst(repr(actual_stdout)),
                "",
                "EXPECTED EXIT CODE:",
                "-" * 40,
                str(expected_exit),
                "",
                "ACTUAL EXIT CODE:",
                "-" * 40,
                str(actual_exit),
                "",
            ]
        )

        if stderr:
            lines.extend(
                [
                    "STDERR:",
                    "-" * 40,
                    escape_rst(stderr),
                    "",
                ]
            )

        lines.extend(
            [
                "=" * 80,
                "BUILD ABORTED: varm doctest failed",
                "=" * 80,
            ]
        )

        return "\n".join(lines)


def setup(app: Sphinx) -> dict:
    """
    Initialize the varm_doctest extension.

    Adds configuration options and registers the code-with-test directive.
    """
    app.add_config_value("varm_binary", "varm", "env")
    app.add_config_value("vasm_binary", "vasm", "env")
    app.add_config_value("varm_test_timeout", 10, "env")
    app.add_config_value("varm_test_dir", None, "env")

    app.add_directive("code-with-test", CodeWithTest)

    return {
        "version": "0.1.0",
        "env_version": 1,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
