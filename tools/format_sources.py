"""Apply the project's C++ formatting rules without building the game.

clang-format handles whitespace. The token-aware passes add mandatory control
braces, separate statements and keep a blank line before terminal returns.
Only the project's own src/*.cpp and src/*.h files are selected by default.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path
import os
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
LEXER = re.compile(
    r'(?P<directive>^[ \t]*\#(?:[^\n]|(?<=\\)\n)*)'
    r'|(?P<comment>//[^\n]*|/\*.*?\*/)'
    r'|(?P<raw>(?:u8|u|U|L)?R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)")'
    r'|(?P<string>(?:u8|u|U|L)?"(?:\\.|[^"\\])*"|(?:u8|u|U|L)?\'(?:\\.|[^\'\\])*\')'
    r'|(?P<word>[A-Za-z_$][\w$]*)'
    r'|(?P<number>(?:[0-9]|\.[0-9])(?:[\w.]|\'[0-9A-Za-z])*)'
    r'|(?P<operator><=>|>>=|<<=|->\*|\.\.\.|::|->|\+\+|--|&&|\|\||==|!=|<=|>=|<<|>>|\+=|-=|\*=|/=|%=|&=|\|=|\^=|##)'
    r'|(?P<other>[^\s])',
    re.MULTILINE | re.DOTALL,
)


@dataclass(frozen=True)
class Token:
    text: str
    start: int
    end: int


class Source:
    def __init__(self, text: str):
        self.text = text
        self.tokens = [Token(m.group(), m.start(), m.end()) for m in LEXER.finditer(text) if m.lastgroup not in ("directive", "comment")]
        self.pairs: dict[int, int] = {}
        stack: list[int] = []
        for i, token in enumerate(self.tokens):
            if token.text in ("(", "[", "{"):
                stack.append(i)
            elif token.text in (")", "]", "}"):
                if not stack or self.tokens[stack[-1]].text != {")": "(", "]": "[", "}": "{"}[token.text]:
                    raise ValueError(f"Unbalanced delimiter near line {text.count(chr(10), 0, token.start) + 1}")
                opening = stack.pop()
                self.pairs[opening] = i
                self.pairs[i] = opening
        if stack:
            raise ValueError("Unclosed delimiter")

    def word(self, i: int) -> str:
        return self.tokens[i].text if 0 <= i < len(self.tokens) else ""

    def body(self, i: int) -> int:
        opening = i + 1
        if self.word(opening) == "constexpr":
            opening += 1
        if self.word(opening) != "(":
            raise ValueError(f"Expected control condition after {self.word(i)}")
        return self.pairs[opening] + 1

    def statement_end(self, i: int) -> int:
        word = self.word(i)
        if word == "{":
            return self.pairs[i] + 1
        if word == "if":
            end = self.statement_end(self.body(i))
            return self.statement_end(end + 1) if self.word(end) == "else" else end
        if word in ("for", "while", "switch", "catch"):
            return self.statement_end(self.body(i))
        if word == "do":
            end = self.statement_end(i + 1)
            if self.word(end) != "while":
                raise ValueError("Expected while after do body")
            return self.pairs[end + 1] + 2
        if word == "try":
            end = self.statement_end(i + 1)
            while self.word(end) == "catch":
                end = self.statement_end(end)
            return end
        cursor = i
        while cursor < len(self.tokens):
            word = self.word(cursor)
            if word == ";":
                return cursor + 1
            if word in ("(", "[", "{"):
                cursor = self.pairs[cursor] + 1
            elif word == "}":
                break
            else:
                cursor += 1
        raise ValueError(f"Statement without terminator near {self.tokens[i].start}")

    def last_returns(self) -> set[int]:
        def is_function(opening: int) -> bool:
            cursor = opening - 1
            if self.word(cursor) == "]":
                before_capture = self.word(self.pairs[cursor] - 1)
                return before_capture in ("=", "(", ",", "return", ":")
            while cursor >= 0 and self.word(cursor) not in ("{", "}", ";", "="):
                if self.word(cursor) == ")":
                    before = self.word(self.pairs[cursor] - 1)
                    if before in ("noexcept", "__declspec", "alignas"):
                        cursor = self.pairs[cursor] - 2
                        continue
                    return before not in ("if", "constexpr", "for", "while", "switch", "catch")
                cursor -= 1
            return False

        owners: list[int | None] = []
        last: dict[int, int] = {}
        for i, token in enumerate(self.tokens):
            if token.text == "{":
                owners.append(i if is_function(i) else (owners[-1] if owners else None))
            elif token.text == "}":
                owners.pop()
            elif token.text == "return" and owners and owners[-1] is not None:
                last[owners[-1]] = i
        return set(last.values())


def edits(text: str, insertions: dict[int, list[str]]) -> str:
    for offset in sorted(insertions, reverse=True):
        text = text[:offset] + "".join(insertions[offset]) + text[offset:]
    return text


def required_bodies(source: Source) -> set[int]:
    bodies: set[int] = set()
    last_returns = source.last_returns()
    for i, token in enumerate(source.tokens):
        if token.text == "for":
            bodies.add(source.body(i))
        elif token.text == "if":
            body = source.body(i)
            end = source.statement_end(body)
            if source.word(end) == "else":
                current = i
                while True:
                    body = source.body(current)
                    bodies.add(body)
                    end = source.statement_end(body)
                    if source.word(end) != "else":
                        break
                    following = end + 1
                    if source.word(following) != "if":
                        bodies.add(following)
                        break
                    current = following
            elif body in last_returns:
                # Keep terminal conditional returns braced so their blank line
                # never separates an unbraced if condition from its statement.
                bodies.add(body)
    return bodies


def add_braces(text: str) -> str:
    source = Source(text)
    changes: dict[int, list[str]] = defaultdict(list)
    for body in sorted(required_bodies(source)):
        if source.word(body) == "{":
            continue
        end = source.statement_end(body)
        close = source.tokens[end - 1].end
        newline = text.find("\n", close)
        tail = text[close:newline if newline >= 0 else len(text)]
        if re.match(r"[ \t]*//", tail):
            close += len(tail)
        changes[source.tokens[body].start].append("{\n")
        changes[close].append("\n}")
    result = edits(text, changes)
    checked = Source(result)
    if any(checked.word(i) != "{" for i in required_bodies(checked)):
        return add_braces(result)
    return result


def split_statements(text: str) -> str:
    source = Source(text)
    loop_headers = {i + 1 for i, token in enumerate(source.tokens) if token.text == "for"}
    stack: list[int] = []
    changes: dict[int, list[str]] = defaultdict(list)
    for i, token in enumerate(source.tokens):
        if token.text in ("(", "[", "{"):
            stack.append(i)
        elif token.text in (")", "]", "}"):
            stack.pop()
        elif token.text == ";":
            nearest_brace = max((n for n in stack if source.word(n) == "{"), default=-1)
            if any(n in loop_headers and n > nearest_brace for n in stack):
                continue
            if i + 1 < len(source.tokens) and "\n" not in text[token.end:source.tokens[i + 1].start]:
                changes[token.end].append("\n")
    return edits(text, changes)


def add_spacing(text: str) -> str:
    source = Source(text)
    last_returns = source.last_returns()
    blank_before: set[int] = set()
    for i, token in enumerate(source.tokens):
        if token.text in ("if", "for", "while", "switch", "try"):
            start = text.rfind("\n", 0, token.start) + 1
            if text[start:token.start].strip():
                continue
            previous_start = text.rfind("\n", 0, max(0, start - 1)) + 1
            previous = text[previous_start:start].strip()
            if previous and previous != "{" and not previous.endswith(":") and not re.match(r"(?:else|if|for|while)\b", previous):
                blank_before.add(start)
            end = source.statement_end(i)
            if end < len(source.tokens) and source.word(end) not in ("}", "else", "catch", ";", ",", ")", "]"):
                next_start = text.rfind("\n", 0, source.tokens[end].start) + 1
                end_line = text.find("\n", source.tokens[end - 1].end)
                if end_line >= 0 and next_start == end_line + 1:
                    blank_before.add(next_start)
        elif i in last_returns:
            start = text.rfind("\n", 0, token.start) + 1
            previous_start = text.rfind("\n", 0, max(0, start - 1)) + 1
            if not text[start:token.start].strip() and text[previous_start:start].strip():
                blank_before.add(start)
    result = text
    for offset in sorted(blank_before, reverse=True):
        result = result[:offset] + "\n" + result[offset:]
    return re.sub(r"\n[ \t]*\n(?:[ \t]*\n)+", "\n\n", result).rstrip()


def join_arguments(text: str) -> str:
    source = Source(text)
    replacements: set[tuple[int, int]] = set()
    controls = {"if", "for", "while", "switch", "catch", "sizeof", "alignof", "decltype", "static_assert", "noexcept", "alignas", "__declspec"}
    for i, token in enumerate(source.tokens):
        before = source.word(i - 1)
        if token.text != "(" or before in controls or not (before.isidentifier() or before in (">", ")", "]")):
            continue
        end = source.pairs[i]
        span = text[token.end:source.tokens[end].start]
        if "\n" not in span or any(marker in span for marker in ("//", "/*", "#")):
            continue
        # Lambda bodies, initializer blocks and multiline literals retain their
        # internal lines; ordinary declaration/call arguments stay on one line.
        if any(t.text in ("{", "}") or "\n" in t.text for t in source.tokens[i + 1:end]):
            continue
        for first, second in zip(source.tokens[i:end], source.tokens[i + 1:end + 1]):
            gap = text[first.end:second.start]
            if "\n" in gap and not gap.strip():
                replacements.add((first.end, second.start))
    for start, end in sorted(replacements, reverse=True):
        text = text[:start] + " " + text[end:]
    return text


def find_clang_format(explicit: str | None) -> str:
    found = explicit or os.environ.get("CLANG_FORMAT") or shutil.which("clang-format")
    if found:
        return found
    installed = sorted(Path("C:/Program Files/Microsoft Visual Studio").glob("*/Community/VC/Tools/Llvm/x64/bin/clang-format.exe"))
    if installed:
        return str(installed[-1])
    raise SystemExit("clang-format is required; set CLANG_FORMAT or pass --clang-format.")


def format_text(text: str, filename: Path, clang_format: str) -> str:
    braced = add_braces(text)
    separated = split_statements(braced)
    result = subprocess.run([clang_format, "--style=file", "--assume-filename=" + str(filename)], input=separated, text=True, encoding="utf-8", stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    formatted = add_spacing(join_arguments(result.stdout))
    if filename.name == "resource.h":
        # RC requires a terminated final directive; retain one newline without an empty trailing line.
        formatted = formatted.rstrip("\r\n") + "\n"
    if [t.text for t in Source(braced).tokens] != [t.text for t in Source(formatted).tokens]:
        raise ValueError("Formatter changed non-whitespace tokens")
    for kind in ("directive", "comment"):
        before = [re.sub(r"\s+", "", m.group()) for m in LEXER.finditer(text) if m.lastgroup == kind]
        after = [re.sub(r"\s+", "", m.group()) for m in LEXER.finditer(formatted) if m.lastgroup == kind]
        if before != after:
            raise ValueError(f"Formatter changed {kind} contents or order")
    return formatted


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="*", type=Path)
    parser.add_argument("--check", action="store_true", help="Report formatting differences without modifying files.")
    parser.add_argument("--clang-format")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    options = parser.parse_args()
    clang_format = find_clang_format(options.clang_format)
    files = [p.resolve() for p in options.files] if options.files else sorted(ROOT.joinpath("src").glob("*.cpp")) + sorted(ROOT.joinpath("src").glob("*.h"))

    def process(path: Path) -> tuple[Path, str | None]:
        original = path.read_text(encoding="utf-8-sig")
        try:
            result = format_text(original, path, clang_format)
        except Exception as error:
            raise RuntimeError(f"{path.relative_to(ROOT)}: {error}") from error
        return path, result if result != original else None

    # Finish all transformations and lexical checks before writing any source.
    with ThreadPoolExecutor(max_workers=options.jobs) as workers:
        results = list(workers.map(process, files))
    changed = [(p, s) for p, s in results if s is not None]
    if not options.check:
        for path, result in changed:
            path.write_text(result, encoding="utf-8", newline="")
    print(f"{'Needs formatting' if options.check else 'Formatted'}: {len(changed)} / {len(files)} source files")
    if options.check and changed:
        for path, _ in changed:
            print(path.relative_to(ROOT))
        raise SystemExit(1)


if __name__ == "__main__":
    main()
