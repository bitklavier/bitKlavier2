#!/usr/bin/env python3
"""
Analyze SF2 soundfont file: dump SHDR, preset/instrument zones, and sample assignments for specific notes.
"""

import struct
import sys
from pathlib import Path

SF2_PATH = "/Users/dtrueman/Documents/bitKlavier/soundfonts/dingfont.sf2"

SAMPLE_TYPES = {
    1: "monoSample",
    2: "rightSample",
    4: "leftSample",
    8: "linkedSample",
    0x8001: "ROMMonoSample",
    0x8002: "ROMRightSample",
    0x8004: "ROMLeftSample",
}

# SF2 generator enumerators
GEN_START_ADDR_OFFSET = 0
GEN_END_ADDR_OFFSET = 1
GEN_START_LOOP_ADDR_OFFSET = 2
GEN_END_LOOP_ADDR_OFFSET = 3
GEN_KEY_RANGE = 43
GEN_VEL_RANGE = 44
GEN_KEY_NUM = 46
GEN_OVERRIDE_ROOT_KEY = 58
GEN_SAMPLE_ID = 53
GEN_INSTRUMENT = 41

GEN_NAMES = {
    0: "startAddrsOffset",
    1: "endAddrsOffset",
    2: "startloopAddrsOffset",
    3: "endloopAddrsOffset",
    4: "startAddrsCoarseOffset",
    5: "modLfoToPitch",
    6: "vibLfoToPitch",
    7: "modEnvToPitch",
    8: "initialFilterFc",
    9: "initialFilterQ",
    10: "modLfoToFilterFc",
    11: "modEnvToFilterFc",
    12: "endAddrsCoarseOffset",
    13: "modLfoToVolume",
    14: "unused1",
    15: "chorusEffectsSend",
    16: "reverbEffectsSend",
    17: "pan",
    18: "unused2",
    19: "unused3",
    20: "unused4",
    21: "delayModLFO",
    22: "freqModLFO",
    23: "delayVibLFO",
    24: "freqVibLFO",
    25: "delayModEnv",
    26: "attackModEnv",
    27: "holdModEnv",
    28: "decayModEnv",
    29: "sustainModEnv",
    30: "releaseModEnv",
    31: "keynumToModEnvHold",
    32: "keynumToModEnvDecay",
    33: "delayVolEnv",
    34: "attackVolEnv",
    35: "holdVolEnv",
    36: "decayVolEnv",
    37: "sustainVolEnv",
    38: "releaseVolEnv",
    39: "keynumToVolEnvHold",
    40: "keynumToVolEnvDecay",
    41: "instrument",
    42: "reserved1",
    43: "keyRange",
    44: "velRange",
    45: "startloopAddrsCoarseOffset",
    46: "keynum",
    47: "velocity",
    48: "initialAttenuation",
    49: "reserved2",
    50: "endloopAddrsCoarseOffset",
    51: "coarseTune",
    52: "fineTune",
    53: "sampleID",
    54: "sampleModes",
    55: "reserved3",
    56: "scaleTuning",
    57: "exclusiveClass",
    58: "overridingRootKey",
    59: "unused5",
    60: "endOper",
}


def read_fourcc(f):
    return f.read(4).decode('ascii', errors='replace')


def read_u32(f):
    return struct.unpack('<I', f.read(4))[0]


def read_chunk_header(f):
    fourcc = f.read(4)
    if len(fourcc) < 4:
        return None, 0
    size = struct.unpack('<I', f.read(4))[0]
    return fourcc.decode('ascii', errors='replace'), size


def parse_shdr(data):
    """Parse SHDR chunk into list of sample header dicts."""
    record_size = 46
    count = len(data) // record_size
    samples = []
    for i in range(count):
        rec = data[i * record_size:(i + 1) * record_size]
        name_raw = rec[0:20]
        name = name_raw.rstrip(b'\x00').decode('ascii', errors='replace')
        start, end, start_loop, end_loop, sample_rate = struct.unpack_from('<IIIII', rec, 20)
        original_pitch, pitch_correction = struct.unpack_from('<bB', rec, 40)
        # original_pitch is unsigned per spec
        original_pitch = rec[40]  # unsigned byte
        pitch_correction = struct.unpack_from('<b', rec, 41)[0]  # signed
        sample_link, sample_type = struct.unpack_from('<HH', rec, 42)
        samples.append({
            'index': i,
            'name': name,
            'start': start,
            'end': end,
            'startLoop': start_loop,
            'endLoop': end_loop,
            'sampleRate': sample_rate,
            'originalPitch': original_pitch,
            'pitchCorrection': pitch_correction,
            'sampleLink': sample_link,
            'sampleType': sample_type,
            'sampleTypeName': SAMPLE_TYPES.get(sample_type, f"unknown(0x{sample_type:04x})"),
        })
    return samples


def parse_phdr(data):
    """Parse PHDR (preset headers)."""
    record_size = 38
    count = len(data) // record_size
    presets = []
    for i in range(count):
        rec = data[i * record_size:(i + 1) * record_size]
        name = rec[0:20].rstrip(b'\x00').decode('ascii', errors='replace')
        preset, bank, bag_idx = struct.unpack_from('<HHH', rec, 20)
        presets.append({'name': name, 'preset': preset, 'bank': bank, 'bagIdx': bag_idx})
    return presets


def parse_pbag(data):
    """Parse PBAG (preset bags)."""
    record_size = 4
    count = len(data) // record_size
    bags = []
    for i in range(count):
        rec = data[i * record_size:(i + 1) * record_size]
        gen_idx, mod_idx = struct.unpack_from('<HH', rec)
        bags.append({'genIdx': gen_idx, 'modIdx': mod_idx})
    return bags


def parse_pgen(data):
    """Parse PGEN (preset generators)."""
    record_size = 4
    count = len(data) // record_size
    gens = []
    for i in range(count):
        rec = data[i * record_size:(i + 1) * record_size]
        oper = struct.unpack_from('<H', rec, 0)[0]
        # amount is a union: interpret as signed short, unsigned short, or two bytes
        lo = rec[2]
        hi = rec[3]
        amount_s16 = struct.unpack_from('<h', rec, 2)[0]
        amount_u16 = struct.unpack_from('<H', rec, 2)[0]
        gens.append({'oper': oper, 'lo': lo, 'hi': hi, 'amount_s16': amount_s16, 'amount_u16': amount_u16})
    return gens


def parse_inst(data):
    """Parse INST (instrument headers)."""
    record_size = 22
    count = len(data) // record_size
    insts = []
    for i in range(count):
        rec = data[i * record_size:(i + 1) * record_size]
        name = rec[0:20].rstrip(b'\x00').decode('ascii', errors='replace')
        bag_idx = struct.unpack_from('<H', rec, 20)[0]
        insts.append({'name': name, 'bagIdx': bag_idx})
    return insts


def parse_ibag(data):
    """Parse IBAG (instrument bags)."""
    record_size = 4
    count = len(data) // record_size
    bags = []
    for i in range(count):
        rec = data[i * record_size:(i + 1) * record_size]
        gen_idx, mod_idx = struct.unpack_from('<HH', rec)
        bags.append({'genIdx': gen_idx, 'modIdx': mod_idx})
    return bags


def parse_igen(data):
    """Parse IGEN (instrument generators)."""
    return parse_pgen(data)  # same structure


def analyze_sf2(path):
    with open(path, 'rb') as f:
        data = f.read()

    pos = 0
    # RIFF header
    riff_id = data[0:4].decode('ascii')
    riff_size = struct.unpack_from('<I', data, 4)[0]
    sfbk_id = data[8:12].decode('ascii')
    print(f"RIFF: {riff_id}, size={riff_size}, type={sfbk_id}")
    assert riff_id == 'RIFF' and sfbk_id == 'sfbk', "Not an SF2 file"

    chunks = {}
    pos = 12
    end = 8 + riff_size
    while pos < end and pos < len(data):
        chunk_id = data[pos:pos+4].decode('ascii', errors='replace')
        chunk_size = struct.unpack_from('<I', data, pos+4)[0]
        chunk_data = data[pos+8:pos+8+chunk_size]
        pos += 8 + chunk_size
        if chunk_size % 2 == 1:
            pos += 1  # pad byte

        if chunk_id == 'LIST':
            list_type = chunk_data[0:4].decode('ascii', errors='replace')
            print(f"  LIST '{list_type}', size={chunk_size}")
            chunks[list_type] = chunk_data[4:]
        else:
            print(f"  Chunk '{chunk_id}', size={chunk_size}")

    # Parse sub-chunks from pdta and sdta
    pdta_data = chunks.get('pdta', b'')
    sdta_data = chunks.get('sdta', b'')

    sub_chunks = {}
    for list_data in [pdta_data, sdta_data]:
        p = 0
        while p < len(list_data):
            if p + 8 > len(list_data):
                break
            cid = list_data[p:p+4].decode('ascii', errors='replace')
            csz = struct.unpack_from('<I', list_data, p+4)[0]
            cdata = list_data[p+8:p+8+csz]
            p += 8 + csz
            if csz % 2 == 1:
                p += 1
            sub_chunks[cid] = cdata
            print(f"    sub-chunk '{cid}', size={csz}")

    # Parse all sub-chunks
    shdr_data = sub_chunks.get('shdr', b'')
    phdr_data = sub_chunks.get('phdr', b'')
    pbag_data = sub_chunks.get('pbag', b'')
    pgen_data = sub_chunks.get('pgen', b'')
    inst_data = sub_chunks.get('inst', b'')
    ibag_data = sub_chunks.get('ibag', b'')
    imod_data = sub_chunks.get('imod', b'')
    igen_data = sub_chunks.get('igen', b'')

    samples = parse_shdr(shdr_data)
    presets = parse_phdr(phdr_data)
    pbags = parse_pbag(pbag_data)
    pgens = parse_pgen(pgen_data)
    insts = parse_inst(inst_data)
    ibags = parse_ibag(ibag_data)
    igens = parse_igen(igen_data)

    # ---- SHDR dump ----
    print("\n" + "="*80)
    print("SHDR - Sample Headers")
    print("="*80)
    print(f"Total samples: {len(samples)}")
    print()
    for s in samples:
        print(f"  [{s['index']:3d}] '{s['name']}'")
        print(f"         start={s['start']}, end={s['end']}, startLoop={s['startLoop']}, endLoop={s['endLoop']}")
        print(f"         sampleRate={s['sampleRate']}, originalPitch={s['originalPitch']}, pitchCorrection={s['pitchCorrection']}")
        print(f"         sampleLink={s['sampleLink']}, sampleType={s['sampleType']} ({s['sampleTypeName']})")
        print()

    # ---- Preset/Instrument zone structure ----
    print("="*80)
    print("Preset / Instrument Zone Structure")
    print("="*80)

    for pi, preset in enumerate(presets):
        if preset['name'] == 'EOP':
            continue
        print(f"\nPreset [{pi}]: '{preset['name']}' bank={preset['bank']} preset={preset['preset']}")
        # get preset bags for this preset
        bag_start = preset['bagIdx']
        bag_end = presets[pi+1]['bagIdx'] if pi+1 < len(presets) else len(pbags)

        for bi in range(bag_start, bag_end):
            if bi >= len(pbags):
                break
            gen_start = pbags[bi]['genIdx']
            gen_end = pbags[bi+1]['genIdx'] if bi+1 < len(pbags) else len(pgens)
            print(f"  Preset bag [{bi}] gens [{gen_start}..{gen_end}):")
            for gi in range(gen_start, gen_end):
                if gi >= len(pgens):
                    break
                g = pgens[gi]
                name = GEN_NAMES.get(g['oper'], f"gen{g['oper']}")
                if g['oper'] == GEN_KEY_RANGE:
                    print(f"    {name}: lo={g['lo']}, hi={g['hi']}")
                elif g['oper'] == GEN_VEL_RANGE:
                    print(f"    {name}: lo={g['lo']}, hi={g['hi']}")
                elif g['oper'] == GEN_INSTRUMENT:
                    inst_idx = g['amount_u16']
                    inst_name = insts[inst_idx]['name'] if inst_idx < len(insts) else '?'
                    print(f"    {name}: {inst_idx} ('{inst_name}')")
                else:
                    print(f"    {name}: {g['amount_s16']} (u16={g['amount_u16']})")

    print("\n" + "="*80)
    print("Instrument Zones (full detail)")
    print("="*80)

    for ii, inst in enumerate(insts):
        if inst['name'] == 'EOI':
            continue
        print(f"\nInstrument [{ii}]: '{inst['name']}'")
        bag_start = inst['bagIdx']
        bag_end = insts[ii+1]['bagIdx'] if ii+1 < len(insts) else len(ibags)

        for bi in range(bag_start, bag_end):
            if bi >= len(ibags):
                break
            gen_start = ibags[bi]['genIdx']
            gen_end = ibags[bi+1]['genIdx'] if bi+1 < len(ibags) else len(igens)

            zone_info = {}
            for gi in range(gen_start, gen_end):
                if gi >= len(igens):
                    break
                g = igens[gi]
                zone_info[g['oper']] = g

            # Summarize zone
            key_lo = zone_info.get(GEN_KEY_RANGE, {}).get('lo', 0)
            key_hi = zone_info.get(GEN_KEY_RANGE, {}).get('hi', 127)
            vel_lo = zone_info.get(GEN_VEL_RANGE, {}).get('lo', 0)
            vel_hi = zone_info.get(GEN_VEL_RANGE, {}).get('hi', 127)
            sample_id = zone_info.get(GEN_SAMPLE_ID, {}).get('amount_u16', None)
            root_key = zone_info.get(GEN_OVERRIDE_ROOT_KEY, {}).get('amount_s16', -1)

            sample_info = ""
            if sample_id is not None and sample_id < len(samples):
                s = samples[sample_id]
                sample_info = f"sample={sample_id} '{s['name']}' origPitch={s['originalPitch']} type={s['sampleTypeName']} link={s['sampleLink']}"
            elif sample_id is not None:
                sample_info = f"sample={sample_id} (out of range)"

            if zone_info:
                print(f"  Zone [{bi}]: key={key_lo}-{key_hi} vel={vel_lo}-{vel_hi} rootKey={root_key} | {sample_info}")
                # Print all generators for this zone
                for gi in range(gen_start, gen_end):
                    if gi >= len(igens):
                        break
                    g = igens[gi]
                    name = GEN_NAMES.get(g['oper'], f"gen{g['oper']}")
                    if g['oper'] == GEN_KEY_RANGE:
                        print(f"      {name}: lo={g['lo']}, hi={g['hi']}")
                    elif g['oper'] == GEN_VEL_RANGE:
                        print(f"      {name}: lo={g['lo']}, hi={g['hi']}")
                    elif g['oper'] == GEN_SAMPLE_ID:
                        sid = g['amount_u16']
                        sname = samples[sid]['name'] if sid < len(samples) else '?'
                        print(f"      {name}: {sid} ('{sname}')")
                    else:
                        print(f"      {name}: {g['amount_s16']} (u16={g['amount_u16']})")

    # ---- Query specific notes ----
    def find_zones_for_note(note, velocity=64):
        results = []
        for ii, inst in enumerate(insts):
            if inst['name'] == 'EOI':
                continue
            bag_start = inst['bagIdx']
            bag_end = insts[ii+1]['bagIdx'] if ii+1 < len(insts) else len(ibags)

            for bi in range(bag_start, bag_end):
                if bi >= len(ibags):
                    break
                gen_start = ibags[bi]['genIdx']
                gen_end = ibags[bi+1]['genIdx'] if bi+1 < len(ibags) else len(igens)

                zone_info = {}
                for gi in range(gen_start, gen_end):
                    if gi >= len(igens):
                        break
                    g = igens[gi]
                    zone_info[g['oper']] = g

                if not zone_info:
                    continue

                key_lo = zone_info.get(GEN_KEY_RANGE, {}).get('lo', 0)
                key_hi = zone_info.get(GEN_KEY_RANGE, {}).get('hi', 127)
                vel_lo = zone_info.get(GEN_VEL_RANGE, {}).get('lo', 0)
                vel_hi = zone_info.get(GEN_VEL_RANGE, {}).get('hi', 127)

                if key_lo <= note <= key_hi and vel_lo <= velocity <= vel_hi:
                    sample_id = zone_info.get(GEN_SAMPLE_ID, {}).get('amount_u16', None)
                    root_key = zone_info.get(GEN_OVERRIDE_ROOT_KEY, {}).get('amount_s16', -1)
                    s = samples[sample_id] if (sample_id is not None and sample_id < len(samples)) else None
                    results.append({
                        'instName': inst['name'],
                        'bagIdx': bi,
                        'keyLo': key_lo,
                        'keyHi': key_hi,
                        'velLo': vel_lo,
                        'velHi': vel_hi,
                        'sampleId': sample_id,
                        'rootKey': root_key,
                        'sample': s,
                        'zone_info': zone_info,
                    })
        return results

    NOTE_NAMES = {60: "C4 (middle C)", 67: "G4", 79: "G5"}
    QUERY_NOTES = [60, 67, 79]

    print("\n" + "="*80)
    print("Sample Assignments for Specific Notes")
    print("="*80)

    for note in QUERY_NOTES:
        note_name = NOTE_NAMES.get(note, f"note {note}")
        print(f"\nMIDI note {note} ({note_name}):")
        zones = find_zones_for_note(note)
        if not zones:
            print("  No zones found!")
        for z in zones:
            s = z['sample']
            print(f"  Instrument: '{z['instName']}', bag={z['bagIdx']}")
            print(f"    keyRange: {z['keyLo']}-{z['keyHi']}, velRange: {z['velLo']}-{z['velHi']}, rootKey: {z['rootKey']}")
            if s:
                print(f"    Sample [{z['sampleId']}]: '{s['name']}'")
                print(f"      start={s['start']}, end={s['end']}")
                print(f"      startLoop={s['startLoop']}, endLoop={s['endLoop']}")
                print(f"      sampleRate={s['sampleRate']}, originalPitch={s['originalPitch']}, pitchCorrection={s['pitchCorrection']}")
                print(f"      sampleLink={s['sampleLink']}, sampleType={s['sampleType']} ({s['sampleTypeName']})")
            else:
                print(f"    Sample ID: {z['sampleId']} (not found)")
            # Print all generators in the zone
            print(f"    All generators:")
            zone_info = z['zone_info']
            for oper, g in sorted(zone_info.items()):
                name = GEN_NAMES.get(g['oper'], f"gen{g['oper']}")
                if g['oper'] in (GEN_KEY_RANGE, GEN_VEL_RANGE):
                    print(f"      {name}: lo={g['lo']}, hi={g['hi']}")
                elif g['oper'] == GEN_SAMPLE_ID:
                    sid = g['amount_u16']
                    sname = samples[sid]['name'] if sid < len(samples) else '?'
                    print(f"      {name}: {sid} ('{sname}')")
                else:
                    print(f"      {name}: {g['amount_s16']} (u16={g['amount_u16']})")


if __name__ == '__main__':
    analyze_sf2(SF2_PATH)
