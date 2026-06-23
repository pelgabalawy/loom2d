"""
Test the text API surface that does not require a GL context.

Font.load() and TextNode rendering need a real Renderer (atlas upload), so they
are exercised by the examples/text_demo render path, not here. The pure word-wrap
helper and the enum/class bindings are testable headlessly.
"""
import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import Font, TextNode, TextAlign


def _uniform(s, adv=10.0):
    return [adv] * len(s)


class TestWrapLines:
    def test_no_wrap_when_width_zero(self):
        s = "hello world"
        lines = Font.wrap_lines(s, _uniform(s), 0.0)
        assert lines == [(0, len(s))]

    def test_explicit_newline(self):
        s = "ab\ncd"
        lines = Font.wrap_lines(s, _uniform(s), 0.0)
        assert [s[a:b] for a, b in lines] == ["ab", "cd"]

    def test_breaks_at_space(self):
        s = "hello world"
        lines = Font.wrap_lines(s, _uniform(s), 55.0)
        assert [s[a:b] for a, b in lines] == ["hello", "world"]

    def test_long_word_not_broken(self):
        s = "supercalifragilistic"
        lines = Font.wrap_lines(s, _uniform(s), 30.0)
        assert lines == [(0, len(s))]


class TestEnumAndClasses:
    def test_text_align_values(self):
        assert TextAlign.Left != TextAlign.Center
        assert {TextAlign.Left, TextAlign.Center, TextAlign.Right}

    def test_textnode_is_a_node(self):
        # Constructible with a None font (no GL needed); behaves like a Node.
        node = TextNode(None, "hi")
        assert node.text == "hi"
        node.text = "bye"
        assert node.text == "bye"
        assert node.size.x == 0.0  # no font -> empty layout
