"""Optional read-only names and stock definitions from the user's game data.

Archive layout: Windows 0040665D (CSimpleFileLibrary).
House stock ordering: Windows 004149B2 and 00431F3C.
No original game data is shipped with the editor.
"""

from pathlib import Path
import re
import struct

from savegame import SaveError, decode_text, u32


def decode_cif(data: bytes) -> list[str]:
    """Decode the two serialized CStringArray memory blocks of a compiled INI."""
    position = 28

    def memory():
        nonlocal position
        if position + 12 > len(data):
            raise SaveError("Truncated compiled INI memory header.")
        size = u32(data, position + 8)
        position += 12
        if size > 16 * 1024 * 1024 or position + size > len(data):
            raise SaveError("Invalid compiled INI memory size.")
        block = bytearray(data[position:position + size])
        position += size
        mask, increment = 0x47, 0x7E
        for index, byte in enumerate(block):
            block[index] = ((byte - 1) & 255) ^ mask
            mask = (mask + increment) & 255
            increment = (increment + 0x21) & 255
        return block

    offsets = memory()
    position += 1
    strings = memory()
    if len(offsets) % 4:
        raise SaveError("Misaligned compiled INI string table.")
    result = []
    for (offset,) in struct.iter_unpack("<I", offsets):
        if offset == 0xFFFFFFFF:
            result.append("")
            continue
        if offset >= len(strings):
            raise SaveError("Compiled INI string offset is outside the string table.")
        end = strings.find(0, offset)
        if end < 0:
            raise SaveError("Unterminated compiled INI string.")
        result.append(decode_text(strings[offset:end]))
    return result


def archive_assets(path: Path, wanted: set[str]) -> dict[str, bytes]:
    result = {}
    size = path.stat().st_size
    with path.open("rb") as stream:
        def read(count):
            if count < 0 or count > size - stream.tell():
                raise SaveError(f"Truncated game archive: {path.name}")
            value = stream.read(count)
            if len(value) != count:
                raise SaveError("Unable to read game archive.")
            return value

        def integer():
            return struct.unpack("<I", read(4))[0]

        _, directories, files = struct.unpack("<3I", read(12))
        if directories > size // 8 or files > size // 12:
            raise SaveError("Invalid game archive directory counts.")
        for _ in range(directories):
            length = integer()
            if length > 4096:
                raise SaveError("Unsupported game archive path length.")
            read(length)
            read(4)
        entries = []
        for _ in range(files):
            length = integer()
            if length > 4096:
                raise SaveError("Unsupported game archive filename length.")
            name = read(length).decode("cp1252").replace("\\", "/").lower()
            offset, count = integer(), integer()
            if name in wanted:
                if count > 16 * 1024 * 1024 or offset + count > size:
                    raise SaveError("Invalid game definition entry.")
                entries.append((name, offset, count))
        for name, offset, count in entries:
            stream.seek(offset)
            result[name] = read(count)
    return result


def groups(lines: list[str], kind: str) -> list[list[tuple[str, str]]]:
    result, current = [], None
    for raw in lines:
        line = raw.strip(" \t\r\n")
        if line.startswith("\x01") or line.startswith("["):
            name = line[1:] if line.startswith("\x01") else line.strip("[] ")
            current = [] if name.lower() == kind else None
            if current is not None:
                result.append(current)
        elif current is not None:
            line = line.removeprefix("\x02")
            match = re.match(r"([A-Za-z0-9_]+)\s*(?:=\s*)?(.*)", line)
            if match:
                current.append((match[1].lower(), match[2].strip()))
    return result


class Catalog:
    def __init__(self):
        self.houses, self.goods, self.jobs, self.experiences = {}, {}, {}, {}
        self.root = None

    def load(self, root: Path):
        root = root.resolve()
        stems = ("housetypes", "goodtypes", "jobtypes", "humanjobexperiencetypes")
        wanted = {f"data/logic/{stem}.cif" for stem in stems}
        assets = {}
        # Normal files override archives. Reading the libraries does not extract them.
        for stem in stems:
            for suffix in (".ini", ".cif"):
                path = root / "data" / "logic" / (stem + suffix)
                if path.is_file():
                    data = path.read_bytes()
                    assets[stem] = decode_cif(data) if suffix == ".cif" else decode_text(data).splitlines()
                    break
        for library in sorted((root / "DataX" / "libs").glob("*.lib"), reverse=True):
            missing = {name for name in wanted if Path(name).stem not in assets}
            if not missing:
                break
            for name, data in archive_assets(library, missing).items():
                assets[Path(name).stem] = decode_cif(data)
        if "housetypes" not in assets:
            raise SaveError("No house definitions found. Choose the game root containing DataX/libs or data/logic.")
        houses, goods, jobs, experiences = {}, {}, {}, {}
        for rows in groups(assets["housetypes"], "logichousetype"):
            values = dict(rows)
            if "logictype" not in values:
                continue
            index = int(values["logictype"].split()[0])
            if not 0 < index < 55:
                continue
            if index in houses:
                raise SaveError("Repeated house-type groups require a separate review before their stock layout can be edited.")
            stocks = []
            for key, value in rows:
                if key == "logicstock":
                    tokens = value.split()
                    if len(tokens) < 3:
                        raise SaveError("Unsupported house stock definition.")
                    good, maximum, output = map(int, tokens[:3])
                    if not 0 <= good < 66 or not 0 <= maximum <= 1000 or len(stocks) >= 66:
                        raise SaveError("Unsupported house stock limits.")
                    stocks.append((good, maximum, bool(output)))
            houses[index] = {"name": values.get("debugname", f"Building type {index}").strip('"'), "stocks": stocks}
        for stem, kind, target in (("goodtypes", "goodtype", goods), ("jobtypes", "jobtype", jobs), ("humanjobexperiencetypes", "humanjobexperiencetype", experiences)):
            for rows in groups(assets.get(stem, []), kind):
                values = dict(rows)
                if "type" in values and "name" in values:
                    target[int(values["type"].split()[0])] = values["name"].strip('"')
        if not houses:
            raise SaveError("House definitions contain no recognized groups.")
        self.root, self.houses, self.goods, self.jobs, self.experiences = root, houses, goods, jobs, experiences

    def house_name(self, index: int) -> str:
        return self.houses.get(index, {}).get("name", f"Type {index}")

    def job_name(self, index: int) -> str:
        return self.jobs.get(index, f"Job {index}")

    def good_name(self, index: int) -> str:
        return self.goods.get(index, f"Good {index}")

    def experience_name(self, index: int) -> str:
        return self.experiences.get(index, f"Experience slot {index}")
