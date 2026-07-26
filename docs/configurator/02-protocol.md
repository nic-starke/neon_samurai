# 02 — NSP, the device protocol

A framed, versioned, transport-agnostic protocol shared by the firmware and the
editor, generated from one schema so the two cannot drift.

Designed to be liftable: `nsp/` is a self-contained C directory depending on
nothing but `stdint.h` and `string.h`, with no allocation and no I/O of its own.
Drop it into any project, supply a write callback, done.

## What is wrong with the current scheme

Not a style critique — these are the specific things that block an editor.

**1. It puts 8-bit data in a 7-bit frame.** Every byte between `F0` and `F7` must
be ≤ 0x7F. The wire structs in `src/include/midi/sysex.h` declare `u16 red`,
`u16 green`, `u16 blue`, `u8` positions across the full 0–255 encoder range, and
`i8` signed ranges. None of those are representable. The firmware knows: the
`MF_SYSEX_GET` handler masks each byte with `& 0x7F` and carries the comment

> *"Parameters whose values can exceed 127 are not currently representable and
> will need a split-nibble encoding; none of the presently defined parameters
> do."*

That last clause is not accurate — `MF_SYSEX_PARAM_VMAP_RGB` does.

**2. Colour is broken twice over.** The wire union declares `{u16 red; u16 green;
u16 blue;}` (6 bytes), but `sysex_data_info` computes the expected length from
the *firmware* struct, `struct rgb_8` (3 bytes). So the length check enforces 3,
and `memcpy` then copies 3 bytes off the front of the `u16` triple — on
little-endian AVR that is `red_lo, red_hi, green_lo`. Setting a colour writes
garbage. Separately, HSV is what the firmware and EEPROM actually treat as
authoritative (hue 0–1535), and **there is no SysEx parameter for HSV at all**.
The editor's single most important write is, today, unimplementable.

**3. It serialises native structs.** `SYSEX_DATA_INFO` takes `offsetof` and
`sizeof` of live firmware types and `memcpy`s wire bytes straight over them. So
the wire format is the AVR-GCC ABI: enum width (2 bytes here, 4 on the host),
struct padding, bitfield packing, endianness. Change a field's type and every
client breaks silently. It also means the host test suite and the device can
disagree about the format while both compiling cleanly.

**4. One parameter per round trip.** ~960 parameters means ~960 request/response
pairs to read the device. At even 2 ms each that is two seconds of nothing;
realistically much worse. There is no bulk dump and no bulk restore, so "open the
editor" and "load a profile" are both unacceptably slow.

**5. No handshake.** No way to ask what firmware you are talking to, what it
supports, or what its config schema looks like. The editor cannot version-negotiate,
cannot warn about mismatches, and cannot degrade gracefully.

**6. No telemetry.** Nothing streams encoder positions, switch states or LED
values. The device view has nothing to render from.

**7. The manufacturer ID is not ours to use.** The header uses `0x53 0x41 0x4D` —
"SAM" in ASCII (`MIDI_MFR_ID_1..3` in `src/include/midi/midi_types.h`). But a
3-byte manufacturer ID must begin with `0x00`; a frame starting `F0 53` is read
by every other device as the **one-byte** ID `0x53`, which sits inside the
`0x40–0x5F` range allocated to Japanese manufacturers. So the frame is
non-conformant and can be misparsed on a shared MIDI bus. MIDI reserves **`0x7D`**
for educational and non-commercial use, which is exactly what this project is.
Move to `F0 7D ...`.

Points 1–3 are correctness bugs, 4–6 are capability gaps, 7 is conformance. A
patch fixes 1 and 2; only a redesign fixes 4–6. Hence NSP.

## Layering

```
┌────────────────────────────────────────────────────────┐
│ Application   typed ops, generated from schema         │
│               nsp_config_set(dev, path, value)         │
├────────────────────────────────────────────────────────┤
│ Session       request/response pairing on seq,         │
│               ACK/NAK + error codes, fragmentation,    │
│               subscriptions for async events           │
├────────────────────────────────────────────────────────┤
│ PDU           ver | flags | seq | opcode | len | body  │
│               | crc16                                  │
├────────────────────────────────────────────────────────┤
│ Transport shim                                          │
│   SysEx: F0 7D + 8-in-7 pack + F7                      │
│   HID:   report id + PDU, zero-padded to 64            │
│   CDC:   COBS framing                                  │
└────────────────────────────────────────────────────────┘
```

Everything above the shim is byte-identical across transports. That is what lets
one editor codebase and one firmware handler serve all of them.

## The PDU

Little-endian, no implicit padding, no struct punning in either language —
explicit generated encode/decode functions.

```
offset  size  field
  0      1    version      protocol major; mismatch is a hard reject
  1      1    flags        bit0-1 kind: 0=REQ 1=RSP 2=EVT 3=reserved
                           bit2   more   more fragments follow
                           bit3   err    body is an error struct
  2      1    seq          rolling; RSP echoes the REQ's seq
  3      2    opcode
  5      2    length       body length
  7      N    body
 7+N     2    crc16        CCITT-FALSE over bytes 0..7+N-1
```

9 bytes of overhead. On HID's 64-byte report that leaves 55 bytes of body, and
fragmentation covers anything larger. CRC is redundant over USB's own CRC but
costs 2 bytes and catches framing bugs in the 8-in-7 packer, which is exactly
where bugs will be.

### Opcode space

| Range | Group | Examples |
| --- | --- | --- |
| `0x0000–0x00FF` | Session | `HELLO`, `PING`, `RESET`, `ERROR` |
| `0x0100–0x01FF` | Config | `PARAM_GET`, `PARAM_SET`, `SNAPSHOT_GET`, `SNAPSHOT_SET`, `STORE`, `FACTORY_RESET` |
| `0x0200–0x02FF` | Telemetry | `SUBSCRIBE`, `UNSUBSCRIBE`, `EVT_ENCODER`, `EVT_SWITCH`, `EVT_LED`, `EVT_MIDI` |
| `0x0300–0x03FF` | Debug | `LOG`, `STATS`, `LED_OVERRIDE`, `ENTER_BOOTLOADER` |
| `0x0400–` | Reserved / vendor | |

### `HELLO` — the handshake everything hangs off

Request is empty; response:

```c
struct nsp_hello {
  u8  proto_major, proto_minor;
  u16 fw_version;          // packed maj.min.patch
  u8  git_sha[4];          // short SHA of the build
  u8  device_id[8];        // XMega internal serial, from hal/signature.c
  u32 schema_hash;         // FNV-1a of the generated schema
  u32 caps;                // bitmap: LFO, OSC, HID, BULK, TELEMETRY, ...
  u8  num_banks, num_encoders, num_vmaps, num_side_switches;
  u16 max_pdu;             // largest body this build will accept
};
```

`schema_hash` is the important field. The editor ships codecs for known schema
hashes; on an exact match it uses the generated fast path, on a mismatch it warns
and falls back to `PARAM_GET`-by-name introspection. That is how you make the
editor forward- and backward-compatible without hand-maintaining a version matrix.

`device_id` also gives per-device profile association and — critically for
[05](05-firmware-update.md) — lets the flasher verify it is about to write to the
device it just backed up.

### Addressing

Replace the ad-hoc per-parameter index prefixes with one uniform path:

```c
struct nsp_path {
  u8 bank;      // 0..NUM_ENC_BANKS-1, or NSP_ALL
  u8 element;   // encoder 0..15, side switch 0x80|0..5, or NSP_ALL
  u8 vmap;      // 0..1, NSP_NONE for encoder-level fields, or NSP_ALL
  u8 field;     // generated field id
};
```

Four bytes, addresses everything, and `NSP_ALL` gives broadcast writes for free —
"set every encoder in bank 2 to channel 3" becomes one PDU instead of sixteen.
Field IDs come from the schema with declared type, range, unit and default, which
is also what lets the editor generate and validate controls rather than
hand-coding each one.

### Bulk config

`SNAPSHOT_GET` / `SNAPSHOT_SET` move the entire configuration as one fragmented,
CRC'd, schema-hash-tagged blob. Packed, the current config is roughly 2 KB — a
few dozen HID reports, tens of milliseconds. This is what "connect", "load
profile" and "backup before flashing" all use.

`STORE` is separate and explicit: it commits RAM state to EEPROM. Live edits go
to RAM only. The user hears the difference between "I'm auditioning" and "I'm
committing", the EEPROM's write-endurance is protected from a UI that fires on
every slider drag, and the editor gets a natural dirty-state indicator.

### Telemetry

```c
NSP_SUBSCRIBE { u32 streams; u16 interval_ms; }
```

| Stream | Payload | Rate |
| --- | --- | --- |
| `ENCODER` | 16 × `{u8 pos, i8 vel}` = 32 B | 60 Hz |
| `SWITCH` | `u16` encoder switches + `u8` side switches | on change |
| `LED` | 16 × `{u16 indicator_mask, u8 r,g,b, u8 detent}` = 96 B | 30 Hz, or on change |
| `MIDI` | echo of MIDI the device sent/received | on change |

Encoder + switch at 60 Hz is ~2 KB/s. Adding LED at 30 Hz is ~3 KB/s. Both fit
comfortably inside HID's ~64 KB/s and are the reason the device view can be
genuinely real-time. Over the SysEx fallback, drop LED to on-change and encoder
to 30 Hz — which the SVG view tolerates better than a 3D scene would have, since
there is no camera motion to make the lower rate obvious.

The device must rate-limit itself and drop rather than queue — a stalled editor
must never back up the firmware's event loop.

## Codegen: one schema, two languages

```
protocol/nsp.schema.yaml
        │
        ├─► src/include/protocol/nsp_generated.h   firmware types + field table
        ├─► src/protocol/nsp_generated.c           encoders/decoders, no malloc
        ├─► packages/nsp/src/generated.ts          TS types + codecs + Zod schemas
        ├─► packages/nsp/src/vectors.json          golden test vectors
        └─► docs/configurator/protocol-reference.md
```

Schema entries look like:

```yaml
- id: VMAP_HSV
  scope: vmap
  fields:
    - { name: hue,        type: u16, min: 0, max: 1535, unit: deg1535 }
    - { name: saturation, type: u8,  min: 0, max: 255 }
    - { name: value,      type: u8,  min: 0, max: 255 }
  maps_to: { struct: virtmap, path: hsv }
```

The generator emits the C accessor against the real firmware struct — so the
`offsetof` coupling is *generated and checked* rather than hand-written and
hoped for — and the TS codec plus a Zod schema for the editor's forms.

**The anti-drift mechanism is `vectors.json`.** Both test suites — the existing
host suite under `tests/`, and Vitest on the editor side — decode every vector,
re-encode it, and assert byte equality. A change on one side that is not made on
the other fails CI on both. Add a round-trip fuzz test on each side too; the
existing `tests/test_sysex.c` is the natural home for the C half.

## The liftable C library

```
nsp/
  nsp.h            public API
  nsp_frame.c      PDU encode/decode, CRC16
  nsp_session.c    seq, request/response, fragmentation, timeouts
  nsp_codec_7bit.c 8-in-7 pack/unpack (SysEx transports only)
  nsp_cobs.c       COBS framing (byte-stream transports only)
```

No allocation, no globals beyond a caller-supplied context, no I/O:

```c
typedef struct {
  u8*    rx_buf;  u16 rx_cap;
  u8*    tx_buf;  u16 tx_cap;
  int  (*write)(void* user, const u8* data, u16 len);
  void (*on_pdu)(void* user, const nsp_pdu_t* pdu);
  void*  user;
} nsp_ctx_t;

int nsp_init(nsp_ctx_t* ctx);
int nsp_rx_bytes(nsp_ctx_t* ctx, const u8* data, u16 len);  // feed transport
int nsp_send(nsp_ctx_t* ctx, u16 opcode, u8 flags, const u8* body, u16 len);
```

Everything device-specific — the field table, the `HELLO` contents, the handlers
— lives outside `nsp/` in generated code and in `src/protocol/`. That separation
is what makes the directory reusable in your next project rather than
NEON_SAMURAI-shaped.

## Migration

Do not break the existing scheme on day one.

1. NSP over SysEx, under manufacturer ID `0x7D`, alongside the current `0x53 41 4D`
   handler. Different first byte, so they coexist with no ambiguity.
2. Editor speaks NSP-over-SysEx. Fully functional, if not fast.
3. Add the HID interface; editor prefers it, falls back automatically.
4. Once the editor is the only consumer, delete the old handler and
   `src/include/midi/sysex.h`.

Steps 1–2 are also the point at which the colour bug becomes fixable, since NSP
carries HSV properly.

## Sources

- MIDI 1.0 SysEx framing and the `0x7D` non-commercial ID: MIDI Association spec
- 8-in-7 packing is the conventional vendor approach to 8-bit payloads in SysEx
- Precedent for HID-based device configuration protocols: VIA/Vial, QMK
