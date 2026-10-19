"""Export a saved world as clean-map terrain and native startup INI commands.

Windows evidence: 0040A6A6, 0041D24B, 0042FA61, 00434274,
0045099B, 0044E6FA and 00483E30. This is a new starting map, not a
serialization of running missions, animations, tasks or reservations.
"""

from pathlib import Path
import hashlib
import itertools
import struct
import uuid

from catalog import archive_assets, decode_cif, groups
from savegame import HEADER, SIGNATURE, HOUSES, HUMANS, ObjectArray, SaveError, checksum, decode_text, parse_chunks, read_string, text_bytes, u32


END = 0x78656E64
LOGIC_PLANES = ("lsiz", "lmhe", "lmpa", "lmpb", "lmlt", "lmlv", "lmlp", "lmco", "lmtw", "lmms", "lmpr", "lmwb", "lmbb", "lmro", "lmsb", "lmao", "lmhf", "laco")
RUNTIME_LIMITS = "This export omits mission definitions/progress and scripted AI. Animations, hitpoints, needs, equipment condition, exact ages, family/passenger links, demolition/upgrade progress and fractional inventory are not restored."


def marker(tag):
    return int.from_bytes(tag.encode("ascii"), "big")


def make_chunk(tag, version=0, payload=b""):
    return HEADER.pack(SIGNATURE, marker(tag), version, len(payload), 0, checksum(payload) if payload else 0, 0, 0) + payload


def ini_quote(value):
    text_bytes(value, 250)
    if any(char in value for char in ('"', '\r', '\n', '\t')) or "//" in value:
        raise SaveError("A saved name cannot be represented by the game's INI string syntax (quote, newline, tab or //).")
    return '"' + value + '"'


def signed(value):
    return value if value < 0x80000000 else value - 0x100000000


def unpack_plane(data, expected):
    """Validate a native raw/RLE envelope, then return its exact unpacked bytes."""
    if not 0 <= expected <= 64 * 1024 * 1024 or len(data) < 5 or data[0] not in (0, 1) or u32(data, 1) != len(data) - 5:
        raise SaveError("Invalid map plane envelope or unsupported plane size.")
    if data[0] == 0:
        if len(data) != expected + 5:
            raise SaveError("Raw map plane size disagrees with the map dimensions.")
        return data[5:]
    block = data[5:]
    if len(block) < 16:
        raise SaveError("Truncated packed map plane.")
    signature, method, output_size, input_size = struct.unpack_from("<4I", block)
    if signature != 0x5870636B or output_size != expected or input_size != len(block):
        raise SaveError("Packed map plane header disagrees with its enclosing chunk.")
    if method == marker("none"):
        if len(block) != 16 + expected:
            raise SaveError("Invalid unpacked packing-header size.")
        return block[16:]
    unit = {marker("rle8"): 1, marker("rle6"): 2}.get(method)
    if unit is None or expected % unit:
        raise SaveError("Unsupported map compression method.")
    output, position = bytearray(), 16
    while position < len(block):
        token = block[position]
        position += 1
        count = token & 127
        size = unit if token & 128 else count * unit
        if position + size > len(block) or len(output) + count * unit > expected:
            raise SaveError("Truncated or oversized map RLE packet.")
        value = block[position:position + size]
        output.extend(value * count if token & 128 else value)
        position += size
    if len(output) != expected:
        raise SaveError("Decoded map plane has the wrong size.")
    return bytes(output)


def pack_byte_plane(data):
    """Write the native RLE8 envelope used by map block planes (00430846)."""
    packets = bytearray()
    for value, values in itertools.groupby(data):
        count = sum(1 for _ in values)
        while count:
            size = min(count, 127)
            packets.extend((0x80 | size, value))
            count -= size
    block = struct.pack("<4I", 0x5870636B, marker("rle8"), len(data), 16 + len(packets)) + packets
    return b"\1" + struct.pack("<I", len(block)) + block


def definition_lines(root, paths):
    assets = {}
    for key, path in paths.items():
        for suffix in (".ini", ".cif"):
            candidate = root / (path + suffix)
            if candidate.is_file():
                data = candidate.read_bytes()
                assets[key] = decode_cif(data) if suffix == ".cif" else decode_text(data).splitlines()
                break
    for library in sorted((root / "DataX" / "libs").glob("*.lib"), reverse=True):
        wanted = {path + suffix for key, path in paths.items() if key not in assets for suffix in (".ini", ".cif")}
        if not wanted:
            break
        found = archive_assets(library, wanted)
        for key, path in paths.items():
            if key in assets:
                continue
            for suffix in (".ini", ".cif"):
                if path + suffix in found:
                    data = found[path + suffix]
                    assets[key] = decode_cif(data) if suffix == ".cif" else decode_text(data).splitlines()
                    break
    missing = set(paths) - set(assets)
    if missing:
        raise SaveError("Map export requires game definitions: " + ", ".join(sorted(missing)))
    return assets


def export_definitions(catalog):
    if catalog.root is None:
        raise SaveError("Load the matching game installation with 'Game data...' before exporting a map.")
    assets = definition_lines(catalog.root, {"tribes": "data/logic/tribetypes/tribetypes", "vehicles": "data/logic/vehicletypes", "engine_houses": "data/engine2d/inis/houses/houses"})
    result = {"tribes": {}, "vehicles": {}, "engine_houses": {}}
    for key, kind in (("tribes", "tribetype"), ("vehicles", "vehicletype")):
        for rows in groups(assets[key], kind):
            values = dict(rows)
            if "type" in values and "name" in values:
                result[key][int(values["type"].split()[0])] = values["name"].strip('"')
    # Engine house IDs follow file group order, not the logic-house type IDs.
    for index, rows in enumerate(groups(assets["engine_houses"], "gfxhouse")):
        values = dict(rows)
        build, walk, logic = [], {}, {}
        for key, value in rows:
            if key in ("logicbuildblockarea", "logicwalkblockarea", "logictype"):
                numbers = [int(part) for part in value.split()]
                if key == "logicbuildblockarea" and len(numbers) == 3:
                    build.append(numbers)
                elif key == "logicwalkblockarea" and len(numbers) == 4:
                    walk.setdefault(numbers[0], []).append(numbers[1:])
                elif key == "logictype" and len(numbers) == 2:
                    logic[numbers[0]] = numbers[1]
                else:
                    raise SaveError("Unsupported engine house geometry definition.")
        if "editname" not in values:
            raise SaveError("An engine house definition has no stable name.")
        result["engine_houses"][index] = {"name": values["editname"].strip('"'), "build": build, "walk": walk, "logic": logic}
    return result


class MapExport:
    def __init__(self, save, catalog):
        self.save, self.catalog = save, catalog
        self.definitions = export_definitions(catalog)
        for required in (HUMANS, HOUSES, 0x78646161, 0x78647661, 0x78646D6D):
            if required not in save.arrays:
                raise SaveError("Map export requires supported villager, building, animal, vehicle and moveable arrays.")
        for array_marker, capacity in ((HUMANS, 1000), (HOUSES, 400), (0x78646161, 500), (0x78647661, 200)):
            if len(save.arrays[array_marker].records) > capacity:
                raise SaveError("The saved population exceeds the original game's new-map capacity; export would omit objects.")
        self.logic, self.engine = self.stream("xgma", 1), self.stream("XEma", 1)
        self.logical = self.by_tag(self.logic)
        self.graphical = self.by_tag(self.engine)
        size = self.logical.get("lsiz")
        if size is None or size.version != 1 or len(size.payload) != 8:
            raise SaveError("The save has no supported map dimensions.")
        self.width, self.height = struct.unpack("<2I", size.payload)
        if not 0 < self.width < 16384 or not 0 < self.height < 16384 or self.width * self.height > 16 * 1024 * 1024:
            raise SaveError("Map dimensions exceed the supported Win32 coordinate/plane range.")
        self.guid = uuid.uuid4().bytes_le
        self.counts = {}
        self.warnings = [RUNTIME_LIMITS]

    def stream(self, tag, version):
        start = self.save.unique(marker(tag))
        if start.version != version or start.payload:
            raise SaveError(f"Unsupported {tag} stream header.")
        output = []
        for chunk in self.save.chunks[self.save.chunks.index(start) + 1:]:
            if chunk.marker == END:
                if chunk.payload or chunk.version != 0:
                    raise SaveError("Unsupported map stream end marker.")
                return output
            output.append(chunk)
        raise SaveError(f"Unterminated {tag} map stream.")

    @staticmethod
    def by_tag(chunks):
        result = {}
        for chunk in chunks:
            if chunk.tag in result:
                raise SaveError("Duplicate map chunk: " + chunk.tag)
            result[chunk.tag] = chunk
        return result

    def houses(self):
        array = self.save.arrays[HOUSES]
        for index in sorted(array.records):
            house = array.record(index)
            definition = self.definitions["engine_houses"].get(u32(house, 0x10))
            if definition is None or definition["logic"].get(u32(house, 0x20)) != u32(house, 0x14):
                raise SaveError(f"Building {index} disagrees with the selected game's engine/logic house definitions.")
            if len(definition["build"]) > 100 or any(len(rows) > 100 for rows in definition["walk"].values()):
                raise SaveError("House block-area definitions exceed the native 100-row limit.")
            yield index, house, definition

    def area_indices(self, house, rows):
        x, y = struct.unpack_from("<2h", house, 0x1C)
        for dx, dy, width in rows:
            if not 0 <= width <= 1000:
                raise SaveError("Unsupported house-area width.")
            left, row_y = x + dx + ((dy & 1) & (y & 1)), y + dy
            for column in range(width):
                row_x = left + column
                if 0 <= row_x < self.width * 2 and 0 <= row_y < self.height * 2:
                    yield row_y * self.width * 2 + row_x

    def dat_bytes(self):
        macro, micro = self.width * self.height, self.width * self.height * 4
        output = [make_chunk("logi"), make_chunk("lgmm")]
        for tag in LOGIC_PLANES:
            chunk = self.logical.get(tag)
            if chunk is None and tag == "lmhf":
                continue
            if chunk is None or chunk.version != 1:
                raise SaveError("Missing or unsupported logic map layer: " + tag)
            if tag in ("lsiz", "laco"):
                if tag == "laco" and len(chunk.payload) != 4004:
                    raise SaveError("Unsupported continent table size.")
                output.append(chunk.encode())
                continue
            expected = macro if tag in ("lmhe", "lmpa", "lmpb") else micro * (2 if tag == "lmao" else 1)
            plane = unpack_plane(chunk.payload, expected)
            if tag in ("lmwb", "lmbb"):
                # Houses will be recreated by sethouse, which checks these masks.
                # Remove their existing masks using the native area definitions;
                # retain every point outside the saved houses' footprints.
                plane = bytearray(plane)
                for _, house, definition in self.houses():
                    rows = definition["build"] if tag == "lmbb" else definition["walk"].get(u32(house, 0x20), [])
                    for index in self.area_indices(house, rows):
                        plane[index] = 0
                output.append(make_chunk(tag, 1, pack_byte_plane(plane)))
            else:
                output.append(chunk.encode())
        # Clean sectors omit the two saved dimension DWORDs (0045099B).
        # Keep the standard version-4 layout. House placement executes the
        # native end-map-changes callback, which rebuilds invalidated sectors.
        walk = self.logical.get("sasw")
        expected_grid = ((self.width + 9) // 10, (self.height + 9) // 10)
        if walk is None or walk.version != 3 or len(walk.payload) != 8 + expected_grid[0] * expected_grid[1] * 104 or struct.unpack_from("<2I", walk.payload) != expected_grid:
            raise SaveError("Unsupported saved walk-sector layout.")
        sectors = bytearray(walk.payload[8:])
        if self.save.arrays[HOUSES].records:
            for position in range(0, len(sectors), 52):
                sectors[position] = 0
        output.append(make_chunk("lasw", 4, sectors))
        fish = self.logical.get("sfim")
        if fish is None or fish.version != 1 or len(fish.payload) != 6004:
            raise SaveError("Unsupported saved fish-manager layout.")
        output.extend((make_chunk("lafm", 2, fish.payload), make_chunk("xend")))
        if not self.engine or self.engine[0].tag != "emmm" or self.engine[0].version != 0 or self.engine[0].payload:
            raise SaveError("Missing engine clean-map header.")
        required = {"embr", "emm1", "emmi", "eapd", "empa", "empb", "eatd", "emt1", "emt2", "emt3", "emt4", "eald", "emla"}
        if not required.issubset(self.graphical):
            raise SaveError("The engine map stream is incomplete.")
        for chunk in self.engine:
            if chunk.tag == "emmm":
                output.append(chunk.encode())
                continue
            if chunk.tag not in required | {"emvc"} or chunk.version != 1:
                raise SaveError("Unsupported engine map layer: " + chunk.tag)
            if chunk.tag in ("eapd", "eatd", "eald"):
                count, position = u32(chunk.payload), 4
                if count > 65535:
                    raise SaveError("Engine name table exceeds its 16-bit map indices.")
                for _ in range(count):
                    _, position = read_string(chunk.payload, position)
                if position != len(chunk.payload):
                    raise SaveError("Invalid engine name table suffix.")
            else:
                expected = micro if chunk.tag == "emmi" else micro * 2 if chunk.tag == "emla" else macro * 2 if chunk.tag in ("empa", "empb") else macro
                unpack_plane(chunk.payload, expected)
            output.append(chunk.encode())
        output.extend((make_chunk("xend"), make_chunk("tend")))
        result = b"".join(output)
        parse_chunks(result)
        return result

    def point(self, record, moveable_offset):
        moveables = self.save.arrays[0x78646D6D]
        index = u32(record, moveable_offset)
        if index not in moveables.records:
            raise SaveError("An actor has no saved map position; export would omit it.")
        x, y = struct.unpack_from("<2h", moveables.record(index), 0x12)
        if not 0 <= x < self.width * 2 or not 0 <= y < self.height * 2:
            raise SaveError("An actor's saved position is outside the map.")
        return x, y

    def named(self, table, index):
        if index not in table:
            raise SaveError(f"Missing original game name for type {index}.")
        return ini_quote(table[index])

    def ini_files(self):
        control = self.save.unique(marker("xgco"))
        if control.version != 1 or len(control.payload) != 0x5D204:
            raise SaveError("Unsupported player-control save layout.")
        players = {index: control.payload[index * 0x4A80:(index + 1) * 0x4A80] for index in range(20) if control.payload[index * 0x4A80]}
        self.counts["players"] = len(players)
        metadata = self.save.metadata()
        strings = [metadata.get("map_name") or "Exported save map", metadata.get("description") or "Map exported from a savegame"]
        lines = ["// OpenVikings saved-world export. This starts a new map.", "// " + RUNTIME_LIMITS, "[logiccontrol]", "version 1", f"mapsize {self.width} {self.height}", "mapguid " + " ".join(str(byte) for byte in self.guid), "[logiccontrolend]", "[misc_maptype]", "maptype 6", "[misc_mapname]", "mapnamestringid 0", "mapdescriptionstringid 1", "[playerdata]"]
        for index, data in players.items():
            kind, tribe, color = (u32(data, offset) for offset in (4, 12, 16))
            if kind not in (1, 2) or not 1 <= tribe < 8 or color >= 10:
                raise SaveError(f"Player {index} cannot be represented by native playerdata.")
            lines.append(f"player {index} {kind} {tribe} {color}")
        for index, data in players.items():
            for other in players:
                relation = u32(data, 0x45A8 + other * 4)
                if 1 <= relation <= 3:
                    lines.append(f"diplomacy {index} {other} {relation}")
        lines.append("[playermisc]")
        for index, data in players.items():
            for command, offset in (("nametribe", 0x49AC), ("nametribeshort", 0x49E0)):
                name = decode_text(data[offset:offset + 52].split(b"\0", 1)[0])
                lines.append(f"{command} {index} {len(strings)}")
                strings.append(name)
            if data[0x49A8]:
                lines.append(f"playerneverdies {index}")
        lines.append("[allowedthings]")
        for index, data in players.items():
            for tribe in range(1, 8):
                for kind, count, allowed_at, available_at in (("house", 55, 0x2820, 0x3126), ("job", 56, 0x18, 0x948), ("good", 66, 0x1278, 0x1D4C)):
                    for item in range(1 if kind == "house" else 0, count):
                        item_index = tribe * count + item
                        # Windows forbid clears both permission and availability
                        # (00409784). A locked item must remain researchable.
                        if not data[allowed_at + item_index]:
                            lines.append(f"forbid{kind} {index} {tribe} {item}")
                        elif data[available_at + item_index]:
                            lines.append(f"allow{kind} {index} {tribe} {item}")
        lines.append("[StaticObjects]")
        homes = {}
        for index, house, definition in self.houses():
            owner, stage = u32(house, 12), u32(house, 0x20)
            if owner not in players:
                raise SaveError(f"Building {index} belongs to an inactive player.")
            x, y = struct.unpack_from("<2h", house, 0x1C)
            homes[index] = (x, y)
            lines.extend((f"// Source building {index}", f"sethouse {owner} {ini_quote(definition['name'])} {stage} {int(u32(house, 0x2C) == 3)} {x} {y} {signed(u32(house, 0x7CC))}"))
            stock = self.catalog.houses.get(u32(house, 0x14))
            if stock is None:
                raise SaveError("Missing logic-house stock definition.")
            for slot, (good, _, _) in enumerate(stock["stocks"]):
                amount = u32(house, 0x118 + slot * 4)
                if amount:
                    lines.append(f"addgoods {good} {amount}")
                if u32(house, 0x220 + slot * 4):
                    self.warnings.append(f"Building {index}, stock {slot}: fractional amount is not expressible by addgoods.")
        self.counts["buildings"] = len(homes)
        # Recreate vehicles before passengers; all name arguments are original
        # static-data names, not numeric IDs or guessed descriptions.
        for key, array_marker, player_at, tribe_at, type_at, position_at, mission_at in (("vehicles", 0x78647661, 0x1E0, 0x1E4, 0x1E8, 0x1DC, 0x51C), ("animals", 0x78646161, 0x10, 0x14, 0x18, 0xC, 0x8C)):
            array = self.save.arrays[array_marker]
            for index in sorted(array.records):
                data = array.record(index)
                player = u32(data, player_at)
                if player < 20 and player not in players:
                    raise SaveError(f"{key} {index} belongs to an inactive player.")
                x, y = self.point(data, position_at)
                tribe = self.named(self.definitions["tribes"], u32(data, tribe_at))
                type_name = self.named(self.definitions["vehicles"] if key == "vehicles" else self.catalog.jobs, u32(data, type_at))
                line = f"{'setvehicle' if key == 'vehicles' else 'setanimal'} {player} {tribe} {type_name} {x} {y} {signed(u32(data, mission_at))}"
                lines.append(line if key == "vehicles" else line + f" {signed(u32(data, 0x90))}")
            self.counts[key] = len(array.records)
        array = self.save.arrays[HUMANS]
        names = []
        for index in sorted(array.records):
            data = array.record(index)
            player = u32(data, 16)
            if player not in players:
                raise SaveError(f"Villager {index} belongs to an inactive player.")
            x, y = self.point(data, 12)
            # New unique tags permit stable setname lookup even when saved
            # mission-check IDs are duplicated. Running missions are not exported.
            check = 1000000 + index
            lines.extend((f"// Source villager {index}; saved mission-check {u32(data, 0xB80)}", f"sethuman {player} {self.named(self.definitions['tribes'], u32(data, 20))} {self.named(self.catalog.jobs, u32(data, 24))} {x} {y} {check} {signed(u32(data, 0xB84))}"))
            # The original spelling is intentional. The native startup handler
            # adds to zero-initialized experience and ignores heroes.
            for slot in range(78):
                amount = u32(data, 0x24 + slot * 4)
                if amount:
                    lines.append(f"setexpierence {slot} {min(amount, 10000)}")
            for offset, mask in ((0xB28, 1), (0xB34, 2)):
                if u32(data, offset) in homes:
                    hx, hy = homes[u32(data, offset)]
                    lines.append(f"attachtohouse {hx} {hy} {mask}")
            produced = u32(data, 0x1C)
            if produced:
                lines.append(f"setproducedgood {produced}")
            names.append(f"setname {check} {len(strings)}")
            strings.append(self.save.name(index))
        self.counts["villagers"] = len(array.records)
        # Guides have their own array nested inside the guide-manager chunk.
        guide = self.save.unique(marker("xdgx"))
        if guide.version != 1 or len(guide.payload) < 4:
            raise SaveError("Unsupported guide manager layout.")
        children = parse_chunks(guide.payload[:-4])
        if len(children) != 1 or children[0].tag != "xdga" or children[0].version != 1:
            raise SaveError("Unsupported saved guide array.")
        guides = ObjectArray(children[0], 0x184)
        if len(guides.records) > 400:
            raise SaveError("The saved guide count exceeds the original game's new-map capacity.")
        for index in guides.records:
            data = guides.record(index)
            if u32(data, 4) not in players:
                raise SaveError(f"Guide {index} belongs to an inactive player.")
            x, y = struct.unpack_from("<2h", data, 0x14)
            lines.append(f"setguide {u32(data, 4)} {x} {y}")
        self.counts["guides"] = len(guides.records)
        self.warnings.append("Hero experience is ignored by the original setexpierence startup handler.")
        lines.extend(("[misc_humannames]", *names, f"// Source save SHA-256: {hashlib.sha256(self.save.original).hexdigest()}"))
        text = ["[text]"] + [f"stringn {index} {ini_quote(value)}" for index, value in enumerate(strings)]
        encode = lambda rows: ("\r\n".join(rows) + "\r\n").encode("utf-8")
        string_file = encode(text)
        return {Path("map.ini"): encode(lines), Path("text/eng/strings.ini"): string_file, Path("text/ger/strings.ini"): string_file}

    def write(self, directory):
        directory = Path(directory).resolve()
        files = {Path("map.dat"): self.dat_bytes(), **self.ini_files()}
        # A new directory publishes the pair together without replacing any map.
        directory.mkdir()
        created_files, created_dirs = [], []
        try:
            for relative, data in files.items():
                target = directory / relative
                missing = []
                parent = target.parent
                while parent != directory and not parent.exists():
                    missing.append(parent)
                    parent = parent.parent
                for parent in reversed(missing):
                    parent.mkdir()
                    created_dirs.append(parent)
                with target.open("xb") as output:
                    created_files.append(target)
                    output.write(data)
            return {"directory": str(directory), "width": self.width, "height": self.height, "guid": self.guid.hex(), "counts": self.counts, "warnings": list(dict.fromkeys(self.warnings))}
        except BaseException:
            for target in reversed(created_files):
                target.unlink(missing_ok=True)
            for parent in reversed(created_dirs):
                parent.rmdir()
            directory.rmdir()
            raise
