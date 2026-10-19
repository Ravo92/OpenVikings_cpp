"""Lossless Windows Cultures savegame reader and field editor.

Format evidence: Windows 00410B85, 00426229, 0042A431, 00430D02,
00430DB7, 0048029C and 00490C90. Unknown payloads are never interpreted.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import hashlib
import json
import os
import struct
import tempfile


SIGNATURE = 0x78696F68
MAX_FILE_SIZE = 256 * 1024 * 1024
HEADER = struct.Struct("<8I")
INFO = 0x78736769
ARRAY_HEADER = 0x78616861
HUMANS = 0x78646875
HOUSES = 0x78646861
DYNAMIC_HUMANS = 0x4444686D
ARRAY_SPECS = {
    HUMANS: (1, 0xC58, "Villagers"),
    HOUSES: (2, 0x7E0, "Buildings"),
    0x78646161: (1, 0x98, "Animals"),
    0x78647661: (1, 0x54C, "Vehicles"),
    0x78646D6D: (1, 0x58, "Map moveables"),
}
METADATA = {"description": 0x49436973, "map_name": 0x49436D6E, "slot": 0x49437369}
CHUNK_NAMES = {INFO: "Save information", 0x78736776: "Save header", 0x7867616D: "Game state", 0x7867636F: "Players", DYNAMIC_HUMANS: "Villager appearance and names", ARRAY_HEADER: "Object array", 0x78656E64: "End marker"}
CHUNK_NAMES.update({key: spec[2] for key, spec in ARRAY_SPECS.items()})


class SaveError(ValueError):
    """An unsupported or malformed serialized value."""


def checksum(data: bytes) -> int:
    """XB_GetMemoryChecksum (00410B85), including its final word XOR."""
    result, state, word = 0x6515D4F1, 0x6FD318AB, 0
    complete = len(data) & ~3
    for (word,) in struct.iter_unpack(">I", memoryview(data)[:complete]):
        state ^= word
        result ^= state
        state = (state + 0x63FE53BA) & 0xFFFFFFFF
    for byte in data[complete:]:
        word = ((word << 8) | byte) & 0xFFFFFFFF
    return word ^ result


def u32(data: bytes, offset: int = 0) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise SaveError("Truncated integer.")
    return struct.unpack_from("<I", data, offset)[0]


def decode_text(value: bytes) -> str:
    """Match the remaster's UTF-8-first, legacy western text boundary."""
    try:
        return value.decode("utf-8-sig")
    except UnicodeDecodeError:
        return value.decode("cp1252", errors="replace")


def text_bytes(value: str, limit: int) -> bytes:
    if "\0" in value:
        raise SaveError("Text cannot contain a zero byte.")
    try:
        data = value.encode("utf-8")
    except UnicodeEncodeError as error:
        raise SaveError("Text contains an invalid Unicode surrogate.") from error
    if len(data) > limit:
        raise SaveError(f"Text is limited to {limit} UTF-8 bytes.")
    return data


def read_string(data: bytes, offset: int = 0) -> tuple[str, int]:
    if offset >= len(data):
        raise SaveError("Missing string length.")
    size = data[offset]
    end = offset + size + 2
    if end > len(data) or data[end - 1] != 0:
        raise SaveError("Truncated or unterminated game string.")
    return decode_text(data[offset + 1:end - 1]), end


@dataclass(eq=False)
class Chunk:
    header: tuple[int, ...]
    original: bytes
    offset: int
    payload: bytes
    prefix: bytes = b""
    children: list[Chunk] | None = None

    @property
    def marker(self) -> int:
        return self.header[1]

    @property
    def version(self) -> int:
        return self.header[2]

    @property
    def tag(self) -> str:
        return self.marker.to_bytes(4, "big").decode("ascii", errors="replace")

    def encode(self) -> bytes:
        payload = self.payload if self.children is None else self.prefix + b"".join(child.encode() for child in self.children)
        if payload == self.original:
            return HEADER.pack(*self.header) + self.original
        header = list(self.header)
        header[3], header[5] = len(payload), checksum(payload)
        return HEADER.pack(*header) + payload


def parse_chunks(data: bytes, base: int = 0) -> list[Chunk]:
    chunks, position = [], 0
    while position < len(data):
        if position + HEADER.size > len(data):
            raise SaveError(f"Truncated chunk header at 0x{base + position:X}.")
        header = HEADER.unpack_from(data, position)
        if header[0] != SIGNATURE:
            raise SaveError(f"Invalid chunk signature at 0x{base + position:X}.")
        end = position + HEADER.size + header[3]
        if end > len(data):
            raise SaveError(f"Chunk at 0x{base + position:X} extends beyond its containing data.")
        payload = data[position + HEADER.size:end]
        chunks.append(Chunk(header, payload, base + position, payload))
        if len(chunks) > 100000:
            raise SaveError("Too many chunks.")
        position = end
    return chunks


@dataclass(frozen=True)
class Field:
    key: str
    label: str
    offset: int
    maximum: int = 0x7FFFFFFF


# Serialized CHuman record offsets, established by Windows consumers.
HUMAN_FIELDS = (
    Field("hitpoints", "Hitpoints (raw)", 0xAA0),
    Field("food", "Food (0-15000; normal full 10000)", 0xAA8, 15000),
    Field("stamina", "Stamina (0-15000; normal full 10000)", 0xAAC, 15000),
    Field("social", "Social (0-15000; normal full 10000)", 0xAB0, 15000),
    Field("religion", "Religion (0-15000; normal full 10000)", 0xAB4, 15000),
    Field("shoe_condition", "Shoe condition (0-10000)", 0xADC, 10000),
    Field("tool_condition", "Tool condition (0-10000)", 0xAE0, 10000),
)
HOUSE_FIELDS = (Field("hitpoints", "Hitpoints (raw)", 0x7C8),)


class ObjectArray:
    def __init__(self, parent: Chunk, stride: int):
        self.parent = parent
        capacity = u32(parent.payload)
        children = parse_chunks(parent.payload[4:], parent.offset + 36)
        if len(children) != 1 or children[0].marker != ARRAY_HEADER or children[0].version != 1:
            raise SaveError("Unsupported nested object-array header.")
        self.chunk = children[0]
        data = self.chunk.payload
        if len(data) < 24:
            raise SaveError("Truncated array state.")
        saved_capacity, saved_stride, free, count, first, last = struct.unpack_from("<6i", data)
        if saved_capacity != capacity or capacity < 0 or capacity > 1000000 or saved_stride != stride or not 0 <= count <= capacity:
            raise SaveError("Unsupported array capacity, stride or count.")
        if len(data) != 24 + count * (4 + stride):
            raise SaveError("Array records do not match the serialized count.")
        self.capacity, self.stride, self.records = capacity, stride, {}
        for ordinal in range(count):
            position = 24 + ordinal * (4 + stride)
            index = u32(data, position)
            if index >= capacity or index in self.records or data[position + 4] == 0:
                raise SaveError("Invalid, duplicate or unallocated object index.")
            self.records[index] = position + 4
        parent.prefix, parent.children = parent.payload[:4], children

    def record(self, index: int) -> bytes:
        if index not in self.records:
            raise SaveError("The selected object does not exist.")
        start = self.records[index]
        return self.chunk.payload[start:start + self.stride]


class Savegame:
    def __init__(self, data: bytes, path: Path | None = None):
        if len(data) > MAX_FILE_SIZE:
            raise SaveError("Save exceeds the editor's 256 MiB limit.")
        self.original, self.path = data, path
        self.chunks = parse_chunks(data)
        if not self.chunks or self.chunks[0].marker != 0x78736776 or self.chunks[0].version != 1:
            raise SaveError("Expected a Windows Cultures savegame with an xsgv version 1 header.")
        self.arrays, self.warnings, self.names = {}, [], {}
        self.history, self.redo_history = [], []
        self.top = {}
        for chunk in self.chunks:
            self.top.setdefault(chunk.marker, []).append(chunk)
        self.info = self.unique(INFO)
        if self.info.version == 2:
            self.info.children = parse_chunks(self.info.payload, self.info.offset + 32)
        else:
            self.warnings.append(f"Metadata version {self.info.version} is read-only.")
        for marker, (version, stride, label) in ARRAY_SPECS.items():
            if marker not in self.top:
                continue
            try:
                chunk = self.unique(marker)
                if chunk.version != version:
                    raise SaveError(f"Unsupported version {chunk.version}.")
                self.arrays[marker] = ObjectArray(chunk, stride)
            except SaveError as error:
                self.warnings.append(f"{label} are read-only: {error}")
        # Marker-only headers legitimately carry zero instead of a checksum.
        for chunk in self.chunks:
            for item in [chunk] + (chunk.children or []):
                if item.header[5] and checksum(item.original) != item.header[5]:
                    raise SaveError(f"Payload checksum mismatch in {item.tag} at 0x{item.offset:X}.")
        self._read_names()

    @classmethod
    def open(cls, path: Path) -> Savegame:
        path = path.resolve()
        if path.stat().st_size > MAX_FILE_SIZE:
            raise SaveError("Save exceeds the editor's 256 MiB limit.")
        return cls(path.read_bytes(), path)

    def unique(self, marker: int) -> Chunk:
        matches = self.top.get(marker, [])
        if len(matches) != 1:
            raise SaveError(f"Expected one {CHUNK_NAMES.get(marker, hex(marker))} chunk; found {len(matches)}.")
        return matches[0]

    def metadata_chunk(self, key: str) -> Chunk:
        matches = [chunk for chunk in (self.info.children or []) if chunk.marker == METADATA[key]]
        if len(matches) != 1 or matches[0].version != 1:
            raise SaveError(f"The {key} metadata field is missing or unsupported.")
        return matches[0]

    def metadata(self) -> dict:
        result = {"format_version": self.chunks[0].version}
        for key in METADATA:
            try:
                chunk = self.metadata_chunk(key)
                if key == "slot":
                    if len(chunk.payload) != 4:
                        raise SaveError("Invalid slot size.")
                    result[key] = struct.unpack("<i", chunk.payload)[0]
                else:
                    value, end = read_string(chunk.payload)
                    if end != len(chunk.payload):
                        raise SaveError("Unexpected string suffix.")
                    result[key] = value
            except SaveError:
                result[key] = None
        for chunk in self.info.children or []:
            if chunk.marker == 0x49437469 and len(chunk.payload) == 28:
                result["creation_time_fields"] = list(struct.unpack("<7I", chunk.payload))
            elif chunk.marker == 0x49436367 and len(chunk.payload) == 16:
                result["map_guid_bytes"] = chunk.payload.hex()
            elif chunk.marker == 0x49436D70:
                result["multiplayer"] = True
        return result

    def _read_names(self):
        if HUMANS not in self.arrays or DYNAMIC_HUMANS not in self.top:
            return
        try:
            chunk = self.unique(DYNAMIC_HUMANS)
            if chunk.version != 1 or u32(chunk.payload) != 1:
                raise SaveError("Unsupported dynamic villager version.")
            position, names = 4, {}
            for index in sorted(self.arrays[HUMANS].records):
                for _ in range(3):
                    _, position = read_string(chunk.payload, position)
                end = position + 0x888
                if end > len(chunk.payload):
                    raise SaveError("Truncated villager appearance record.")
                # Saved prefix starts at live +0x3C; live name is +0x85C.
                names[index] = (chunk, position + 0x820)
                position = end
            if position != len(chunk.payload):
                raise SaveError("Unexpected dynamic villager data suffix.")
            self.names = names
        except SaveError as error:
            self.warnings.append(f"Villager names are read-only: {error}")

    def name(self, index: int) -> str:
        if index not in self.names:
            return f"Villager {index}"
        chunk, offset = self.names[index]
        return decode_text(chunk.payload[offset:offset + 100].split(b"\0", 1)[0])

    def position(self, human: bytes) -> str:
        array = self.arrays.get(0x78646D6D)
        index = u32(human, 12)
        if array is None or index not in array.records:
            return "?"
        x, y = struct.unpack_from("<2h", array.record(index), 0x12)
        return f"{x}, {y}"

    def change(self, label: str, patches: list[tuple[Chunk, int, bytes, int]]):
        """Validate all ranges, then commit one undoable field-edit transaction."""
        working = {}
        ranges = {}
        for chunk, offset, replacement, old_size in patches:
            if chunk.children is not None or offset < 0 or old_size < 0 or offset + old_size > len(chunk.payload):
                raise SaveError("Invalid field patch.")
            for left, right in ranges.setdefault(chunk, []):
                if offset < right and left < offset + old_size:
                    raise SaveError("Overlapping field patches.")
            ranges[chunk].append((offset, offset + old_size))
            working.setdefault(chunk, []).append((offset, replacement, old_size))
        changes = []
        for chunk, edits in working.items():
            payload = chunk.payload
            for offset, replacement, old_size in sorted(edits, reverse=True):
                payload = payload[:offset] + replacement + payload[offset + old_size:]
            if payload != chunk.payload:
                changes.append((chunk, chunk.payload, payload))
        if changes:
            for chunk, before, after in changes:
                chunk.payload = after
            self.history.append((label, changes))
            self.redo_history.clear()

    def edit_metadata(self, values: dict):
        patches = []
        for key, value in values.items():
            chunk = self.metadata_chunk(key)
            if key == "slot":
                if not -0x80000000 <= int(value) <= 0x7FFFFFFF:
                    raise SaveError("Slot must fit a signed 32-bit integer.")
                replacement = struct.pack("<i", int(value))
            else:
                encoded = text_bytes(value, 198)
                replacement = bytes([len(encoded)]) + encoded + b"\0"
            patches.append((chunk, 0, replacement, len(chunk.payload)))
        self.change("Save information", patches)

    def edit_record(self, marker: int, index: int, values: dict, name: str | None = None):
        array = self.arrays[marker]
        fields = HUMAN_FIELDS if marker == HUMANS else HOUSE_FIELDS
        patches = []
        for field in fields:
            if field.key not in values:
                continue
            value = int(values[field.key])
            if not 0 <= value <= field.maximum:
                raise SaveError(f"{field.label}: expected a value from 0 to {field.maximum}.")
            patches.append((array.chunk, array.records[index] + field.offset, struct.pack("<I", value), 4))
        if name is not None:
            if index not in self.names:
                raise SaveError("No supported saved name is available for this villager.")
            encoded = text_bytes(name, 99) + b"\0"
            chunk, offset = self.names[index]
            # Preserve bytes beyond the new terminator, as a bounded C-string edit.
            patches.append((chunk, offset, encoded, len(encoded)))
        self.change(f"{'Villager' if marker == HUMANS else 'Building'} {index}", patches)

    def edit_experience(self, index: int, slot: int, amount: int):
        if not 0 <= slot < 78 or not 0 <= amount <= 10000:
            raise SaveError("Experience requires a slot from 0 to 77 and a value from 0 to 10000.")
        array = self.arrays[HUMANS]
        self.change(f"Villager {index}, experience {slot}", [(array.chunk, array.records[index] + 0x24 + slot * 4, struct.pack("<I", amount), 4)])

    def edit_stock(self, index: int, slot: int, whole: int, fraction: int, maximum: int):
        if not 0 <= slot < 66 or not 0 <= whole <= min(maximum, 1000) or not 0 <= fraction <= 9 or (whole == 1000 and fraction != 0):
            raise SaveError(f"Stock must be 0-{min(maximum, 1000)} whole units and 0-9 tenths; 1000 units requires zero tenths.")
        array = self.arrays[HOUSES]
        base = array.records[index]
        # Change only physical inventory. Incoming reservations, demand and minima
        # are independent fields and are intentionally retained.
        self.change(f"Building {index}, stock slot {slot}", [(array.chunk, base + 0x118 + slot * 4, struct.pack("<I", whole), 4), (array.chunk, base + 0x220 + slot * 4, struct.pack("<I", fraction), 4)])

    def undo(self):
        if self.history:
            label, changes = self.history.pop()
            for chunk, before, after in changes:
                chunk.payload = before
            self.redo_history.append((label, changes))

    def redo(self):
        if self.redo_history:
            label, changes = self.redo_history.pop()
            for chunk, before, after in changes:
                chunk.payload = after
            self.history.append((label, changes))

    def encode(self) -> bytes:
        return b"".join(chunk.encode() for chunk in self.chunks)

    def save_copy(self, destination: Path):
        destination = destination.resolve()
        if destination.suffix.lower() != ".sav":
            raise SaveError("Choose a filename ending in .sav.")
        if self.path is not None and (destination == self.path or (destination.exists() and os.path.samefile(destination, self.path))):
            raise SaveError("Use a different filename; the editor keeps the source save untouched.")
        data = self.encode()
        # Read the output through the same structural/version gates before writing.
        reread = Savegame(data)
        if reread.encode() != data:
            raise SaveError("Output failed lossless re-reading.")
        descriptor, temporary = tempfile.mkstemp(prefix=".openvikings-", suffix=".tmp", dir=destination.parent)
        try:
            with os.fdopen(descriptor, "wb") as output:
                output.write(data)
                output.flush()
                os.fsync(output.fileno())
            # Exclusive creation avoids overwriting an existing save or backup.
            with open(temporary, "rb") as source, destination.open("xb") as output:
                try:
                    while block := source.read(1024 * 1024):
                        output.write(block)
                    output.flush()
                    os.fsync(output.fileno())
                except BaseException:
                    output.close()
                    destination.unlink(missing_ok=True)
                    raise
        finally:
            Path(temporary).unlink(missing_ok=True)

    def summary(self) -> dict:
        return {"source_file": str(self.path) if self.path else None, "source_sha256": hashlib.sha256(self.original).hexdigest(), "source_bytes": len(self.original), "metadata": self.metadata(), "objects": {ARRAY_SPECS[marker][2]: len(array.records) for marker, array in self.arrays.items()}, "warnings": self.warnings, "source_chunks": [{"tag": chunk.tag, "version": chunk.version, "size": len(chunk.original), "offset": chunk.offset} for chunk in self.chunks]}


def inspect_file(path: Path) -> str:
    return json.dumps(Savegame.open(path).summary(), ensure_ascii=False, indent=2)
