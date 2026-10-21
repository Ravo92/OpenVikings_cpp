"""Package experimental civilization sprites in native Cultures BMD/LIB formats.

Requires Pillow. This asset conversion tool does not build or launch the game.
The namespaced archive is deliberately not an automatic override of installed data.
"""
from pathlib import Path
import argparse
import hashlib
import io
import json
import statistics
import struct

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
PREFIX = 'data\\remaster\\'
DIRECTIONS = ['south', 'southwest', 'west', 'northwest', 'north', 'northeast', 'east', 'southeast']
COMMON_POSES = ['idle', 'walk_left', 'walk_passing', 'walk_right', 'walk_passing_opposite']
# C2MapGeometry.cpp, Windows 0042A84A: E, SE, SW, W, NW, NE, N, S.
NATIVE_TO_SHEET = [6, 7, 1, 2, 3, 5, 4, 0]


def memory_block(data):
    # CMemory::Storable_SaveData: class ID, version, byte count, raw payload.
    return struct.pack('<III', 0x3E9, 0, len(data)) + data


def make_bmd(frames, palette_indices, empty=False, anchor=(32, 56)):
    records, packed, lines = bytearray(), bytearray(), bytearray()
    for number, frame in enumerate(frames):
        bounds = frame.getchannel('A').getbbox()
        if empty or bounds is None:
            records.extend(bytes(24))
            continue
        left, top, right, bottom = bounds
        first_line = len(lines) // 4
        records.extend(struct.pack('<IiiiiI', 4, left - anchor[0], top - anchor[1], right - left, bottom - top, first_line))
        for y in range(top, bottom):
            opaque = [x for x in range(left, right) if frame.getpixel((x, y))[3] != 0]
            if not opaque:
                lines.extend(struct.pack('<I', 0xFFFFFFFF))
                continue
            start, end = min(opaque), max(opaque) + 1
            if len(packed) >= 1 << 22:
                raise ValueError('BMD line stream exceeds the native 22-bit address range')
            lines.extend(struct.pack('<I', ((start - left) << 22) | len(packed)))
            x = start
            while x < end:
                visible = frame.getpixel((x, y))[3] != 0
                run = x + 1
                while run < end and run - x < 127 and (frame.getpixel((run, y))[3] != 0) == visible:
                    run += 1
                packed.append((run - x) | (0 if visible else 128))
                if visible:
                    for pixel in range(x, run):
                        index = palette_indices.getpixel(((number % 8) * frame.width + pixel, (number // 8) * frame.height + y))
                        packed.extend((index, frame.getpixel((pixel, y))[3]))
                x = run
            packed.append(0)
    # CBobManager's 28-byte serialized record, followed by three CMemory objects.
    header = struct.pack('<IIIII16s', 0x3F4, 0, 0, len(frames), len(packed), bytes(16))
    return header + memory_block(records) + memory_block(packed) + memory_block(lines)


def make_library(files):
    names = [(name.encode('ascii'), data) for name, data in sorted(files.items())]
    offset = 12 + sum(12 + len(name) for name, _ in names)
    table, payload = bytearray(struct.pack('<III', 0, 0, len(names))), bytearray()
    for name, data in names:
        if offset + len(data) > 0xFFFFFFFF:
            raise ValueError('Native LIB offsets and sizes must fit in 32 bits')
        table.extend(struct.pack('<I', len(name)) + name + struct.pack('<II', offset, len(data)))
        payload.extend(data)
        offset += len(data)
    return bytes(table + payload)


def animation_fragment(unit):
    # These are native-format fragments, not replacements for the installed tables.
    stem, tribe, job = unit['id'], unit['tribe'], unit['job']
    sequence = f'remaster_{stem}'
    lines = ['; EXPERIMENTAL: illustrative fragments only, not complete native tables.', '; Runtime uses draw-local frame selection and the original atomic impact clock.', '[bobseq]', f'imagelib "{stem}.bmd"', 'shadowlib "empty64.bmd"', f'seq "{sequence}" 0 64']
    attack = [5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 5] if unit['kind'] == 'bow' else [5, 5, 5, 5, 6, 6, 7, 7, 7, 7, 7, 0]
    for action, poses in [(2, [0]), (81, attack)]:
        lines += ['[gfxanimatomic]', f'logictribe {tribe}', f'logicjob {job}', f'logicatomicaction {action}', 'gfxanimmode 0', f'gfxbobseqbody "{sequence}"']
        for direction in range(8):
            values = ' '.join(str(pose * 8 + NATIVE_TO_SHEET[direction]) for pose in poses)
            lines.append(f'gfxanimframelistdir {direction} {values}')
    lines += ['[gfxwalkatomic]', f'logictribe {tribe}', f'logicjob {job}', 'logicgoodtype 0', f'gfxbobseqbody "{sequence}"']
    for direction in range(8):
        values = ' '.join(str(pose * 8 + NATIVE_TO_SHEET[direction]) for pose in [1, 2, 3, 4])
        lines.append(f'gfxwalkframelist {direction} {values}')
    lines += ['[jobbasegraphics]', f'logictribe {tribe}', f'logicjob {job}', f'gfxbobmanagerbody 0 "{PREFIX}{stem}.bmd" "{PREFIX}empty64.bmd"', f'gfxbobmanagerhead 0 "{PREFIX}empty64.bmd"', f'gfxpalettebasebody "{sequence}"', '[jobchangegraphics]', f'logictribe {tribe}', f'logicjob {job}', f'gfxbobmanagerbody 0 "{PREFIX}{stem}.bmd" "{PREFIX}empty64.bmd"', f'gfxbobmanagerhead 0 "{PREFIX}empty64.bmd"', '[GfxPalette256]', f'editname "{sequence}"', f'gfxfile "{PREFIX}{stem}.pcx"']
    return ('\r\n'.join(lines) + '\r\n').encode('ascii')


def occupied_bands(mask, vertical=True):
    bands = []
    for position in range(mask.height if vertical else mask.width):
        strip = mask.crop((0, position, mask.width, position + 1) if vertical else (position, 0, position + 1, mask.height))
        if strip.getbbox() is not None:
            if not bands or position > bands[-1][1]:
                bands.append([position, position + 1])
            else:
                bands[-1][1] = position + 1
    return bands


def source_frames(source, layout, cell_size=64, anchor=(32, 56)):
    if layout == 'uniform':
        if source.width != source.height:
            raise ValueError('Uniform source must be a square 8 by 8 sprite grid')
        bounds = [round(i * source.width / 8) for i in range(9)]
        return [source.crop((bounds[x], bounds[y], bounds[x + 1], bounds[y + 1])).resize((cell_size, cell_size), Image.Resampling.LANCZOS) for y in range(8) for x in range(8)], {'layout': layout}
    if layout != 'separated_rows':
        raise ValueError('Unsupported source layout: ' + layout)
    # Conversion only: detect isolated source cells, preserve their artwork, and apply one shared scale.
    mask = source.getchannel('A').point(lambda value: 255 if value > 127 else 0)
    rows = occupied_bands(mask)
    if len(rows) != 8:
        raise ValueError(f'Source must contain eight isolated pose rows; found {len(rows)}')
    boxes = []
    for top, bottom in rows:
        columns = occupied_bands(mask.crop((0, top, source.width, bottom)), vertical=False)
        if len(columns) != 8:
            raise ValueError(f'Every pose row must contain eight isolated directions; found {len(columns)}')
        boxes.append([(left, top, right, bottom) for left, right in columns])
    pivots = [statistics.median((boxes[row][col][0] + boxes[row][col][2]) / 2 for row in range(5)) for col in range(8)]
    padding = 4
    highest = max(bottom - top + padding for row in boxes for left, top, right, bottom in row)
    widest = max(max(pivots[col] - left, right - pivots[col]) + padding for row in boxes for col, (left, top, right, bottom) in enumerate(row))
    scale = min((anchor[1] - 4) / highest, (min(anchor[0], cell_size - anchor[0]) - 4) / widest)
    frames = []
    for row in boxes:
        for col, (left, top, right, bottom) in enumerate(row):
            crop = (max(0, left - padding), max(0, top - padding), min(source.width, right + padding), min(source.height, bottom + padding))
            tile = source.crop(crop)
            size = (max(1, round(tile.width * scale)), max(1, round(tile.height * scale)))
            tile = tile.resize(size, Image.Resampling.LANCZOS)
            x, y = round(anchor[0] + (crop[0] - pivots[col]) * scale), round(anchor[1] + (crop[1] - bottom) * scale)
            if x < 0 or y < 0 or x + tile.width > cell_size or y + tile.height > cell_size:
                raise ValueError('Detected sprite exceeds the native frame; refusing to clip its artwork')
            frame = Image.new('RGBA', (cell_size, cell_size))
            frame.alpha_composite(tile, (x, y))
            frames.append(frame)
    return frames, {'layout': layout, 'source_boxes': boxes, 'shared_scale': scale, 'column_pivots': pivots}


def build_unit(unit, source_path, output_directory):
    source_bytes = source_path.read_bytes()
    source = Image.open(io.BytesIO(source_bytes)).convert('RGBA')
    if min(source.size) < 512:
        raise ValueError('Source sprite sheet is too small')
    if source.getchannel('A').getextrema()[0] != 0:
        raise ValueError('Source must have a real transparent background')
    cell_size = unit.get('cell_size', 64)
    anchor = tuple(unit.get('anchor', [32, 56]))
    if not 32 <= cell_size <= 96 or not (0 <= anchor[0] < cell_size and 0 <= anchor[1] < cell_size):
        raise ValueError('Invalid native cell size or anchor')
    frames, slicing = source_frames(source, unit.get('source_layout', 'uniform'), cell_size, anchor)
    atlas = Image.new('RGBA', (cell_size * 8, cell_size * 8))
    for index, frame in enumerate(frames):
        atlas.paste(frame, ((index % 8) * cell_size, (index // 8) * cell_size))
    # Quantization only changes RGB. BMD type 4 stores each original alpha byte separately.
    indices = atlas.convert('RGB').quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pcx = io.BytesIO()
    palette_chart = Image.new('P', (16, 16))
    palette_chart.putpalette(indices.getpalette())
    palette_chart.putdata(range(256))
    palette_chart.save(pcx, format='PCX')
    metadata = {
        'format': 'OpenVikings experimental art pack 1', 'status': 'experimental-game-integration',
        'gameplay_enabled': True, 'source_sha256': hashlib.sha256(source_bytes).hexdigest(),
        'source_size': list(source.size), 'grid': [8, 8], 'frame_size': [cell_size, cell_size], 'frame_count': 64,
        'id': unit['id'], 'tribe': unit['tribe'], 'job': unit['job'], 'kind': unit['kind'],
        'fallback_job': unit['fallback_job'], 'palette_patch': unit['palette_patch'],
        'source': unit['source'], 'anchor': list(anchor), 'directions_requested': DIRECTIONS,
        'slicing': slicing,
        'poses_requested': COMMON_POSES + (['nock', 'draw', 'release'] if unit['kind'] == 'bow' else ['windup', 'contact', 'recovery']),
        'native_direction_to_sheet_column': NATIVE_TO_SHEET,
        'frame_order': 'pose * 8 + direction', 'native_bob_type': 4, 'alpha': '8-bit, preserved after downscaling',
        'generator': 'built-in image_gen', 'player_colors': 'runtime player tint of garment indices; heuristic draft mask',
        'team_mask': unit.get('team_mask', 'teal'),
        'known_limits': [
            'Generated facings and action poses need manual art review.',
            'Walking is a four-pose draft; attacking is a three-pose draft.',
            'Missing poses use native bow or sword graphics from the same civilization.',
            'No separate shadows for the custom frames; the garment mask is inferred from palette colors.',
            'Runtime reads BMD and palette directly; native table fragments remain illustrative.',
            'No game executable was built or launched to validate the integration.'
        ]
    }
    stem = unit['id']
    files = {PREFIX + stem + '.bmd': make_bmd(frames, indices, anchor=anchor), PREFIX + stem + '.pcx': pcx.getvalue(), PREFIX + stem + '.fragments.ini': animation_fragment(unit)}
    preview = output_directory / stem / 'atlas-native.png'
    preview.parent.mkdir(parents=True, exist_ok=True)
    quantized_preview = indices.convert('RGBA')
    quantized_preview.putalpha(atlas.getchannel('A'))
    quantized_preview.save(preview)
    return files, metadata, make_bmd(frames, indices, empty=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--units', type=Path, default=ROOT / 'assets/remaster/units.json')
    parser.add_argument('--source', type=Path, help='Optional replacement source for the Saracen longbow unit')
    parser.add_argument('--output', type=Path, default=ROOT / 'assets/remaster/remaster.lib')
    args = parser.parse_args()
    units = json.loads(args.units.read_text(encoding='utf-8'))['units']
    if len({unit['id'] for unit in units}) != len(units) or len({(unit['tribe'], unit['job']) for unit in units}) != len(units):
        raise ValueError('Unit identifiers and civilization/job pairs must be unique')
    files, metadata = {}, []
    for unit in units:
        source = args.source if args.source is not None and unit['id'] == 'saracen_longbow' else args.units.parent / unit['source']
        unit_files, description, empty = build_unit(unit, source, args.output.parent)
        files.update(unit_files)
        files.setdefault(PREFIX + 'empty64.bmd', empty)
        metadata.append(description)
    files[PREFIX + 'manifest.json'] = (json.dumps({'format': 'OpenVikings experimental art pack 1', 'units': metadata}, indent=2) + '\n').encode('utf-8')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(make_library(files))
    for name, data in files.items():
        (args.output.parent / Path(name.replace('\\', '/')).name).write_bytes(data)
    print(json.dumps({'archive': str(args.output), 'bytes': args.output.stat().st_size, 'units': len(units), 'frames': len(units) * 64, 'entries': len(files)}, indent=2))


if __name__ == '__main__':
    main()
