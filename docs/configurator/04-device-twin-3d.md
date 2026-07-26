# 04 — The 3D device twin

A rotatable, photoreal Midi Fighter Twister in the editor, whose LEDs and knobs
track the real hardware in real time and respond to edits made in the UI.

## The central decision: do not generate the mesh with AI

The 2026 image-to-3D tools are genuinely good now. Meshy, Tripo, Rodin Gen-2 and
Hunyuan3D will turn a photo into a textured mesh in under a minute, and Rodin
Gen-2 takes up to five reference images. For a hero prop in a game, that is the
right tool.

It is the wrong tool here, and the reason is specific rather than aesthetic.

This model has to be **rigged and addressable**. It needs:

- 16 knobs that rotate independently, on their own pivots
- 16 knobs that depress independently
- 176 ring-indicator segments that light individually
- 16 RGB under-ring emitters and 16 detent emitters
- 6 side buttons

That is **208 individually addressable emissive elements and 16 articulated
parts**. An image-to-3D generator returns a single watertight mesh with baked
texture and no part hierarchy. Nothing about that is separable — you cannot pick
out "ring segment 4 of encoder 11" from a mesh that never knew such a thing
existed. Retopologising and hand-splitting it into 208 named parts is strictly
more work than authoring the geometry correctly in the first place, and the
result would still be geometrically approximate.

There is also a much simpler reason: **the device is trivially parametric**. It is
a 150 × 150 × 25 mm extruded box with a 4 × 4 grid of revolved knobs and a
chamfered faceplate. That is a dozen primitives and two loops. AI generation
solves the hard problem of organic geometry; this object does not have that
problem.

**Where AI generation and photogrammetry do belong:** as *reference*, and for
surface detail. Specifically —

1. **Photogrammetry, if you own the device** (you do). 40–60 photos on a turntable
   gives exact proportions, real knurl geometry, and a colour-accurate albedo of
   the anodised faceplate. Use it as a dimensional reference to model against and
   as a source for baked normal/roughness maps. This is the highest-fidelity path
   available and it is cheap.
2. **Image-to-3D from web photos** as a stand-in reference while you are waiting
   to do (1), and to sanity-check silhouette and proportion.
3. **AI texture generation** for the anodised-aluminium roughness variation, the
   knurled rubber knob normal map, and the silkscreen. This is where the
   generative tools earn their place — 2D material maps, not geometry.

So the pipeline is *AI-assisted*, just not AI-authored.

## Building the geometry

Author it as **code**, not as a binary blob in the repo. Either a Blender Python
script or a parametric CAD kernel that exports glTF; either way, the source of
truth is a script with named dimensions and a build step that emits the asset.
That gives you diffable geometry, one place to fix a wrong dimension, and
regeneratable LODs.

Known dimensions: **150 × 150 × 25 mm**, ~0.55 kg. Encoder pitch, knob diameter,
faceplate chamfer and side-button geometry need to come from measurement or
photogrammetry — do not guess these; a wrong pitch is the thing that makes a
model look subtly fake.

### Part taxonomy

Names are the interface between the model and the state store, so fix them early.

```
device
├── enclosure            extruded box, chamfered edges, anodised material
├── faceplate            top surface, silkscreen decal
├── usb_port
├── side_button_{0..5}   3 per side, articulated on press
└── encoder_{0..15}
    ├── knob             rotates on Y, translates on Y when pressed
    ├── ring_diffuser    the translucent light guide
    ├── indicator_{0..10}  11 ring segments      ← emissive
    ├── rgb                 under-ring RGB        ← emissive
    └── detent              detent indicator      ← emissive
```

This maps exactly onto the firmware's LED layout. `gFRAME_BUFFER` is
`u16[32][16]` — 32 BCM frames × 16 encoders — and within each `u16`, per
`src/include/led/led.h`:

| Bit | Element |
| --- | --- |
| 0 | detent blue |
| 1 | detent red |
| 2 | RGB blue |
| 3 | RGB red |
| 4 | RGB green |
| 5–15 | the 11 ring indicators |

The twin renders exactly this structure, so telemetry maps onto it one-to-one
with no translation layer to get wrong.

### Rendering

- **react-three-fiber** + drei + `postprocessing`, targeting WebGL2 with a WebGPU
  path later.
- **Instancing.** One `InstancedMesh` for all 176 indicator segments, one for the
  16 ring diffusers, one for the 16 knobs. Three draw calls for the parts that
  change every frame.
- **Per-instance colour via `InstancedBufferAttribute`.** Telemetry writes into a
  `Float32Array` and flags `needsUpdate`. No React re-render, no scene graph
  traversal — the LED update path is a buffer write and a GPU upload. This is what
  makes 60 Hz free rather than expensive.
- **Emissive + selective bloom.** `EffectComposer` with `SelectiveBloom` over the
  emissive layer only, so the LEDs glow and the aluminium does not. Tune the
  threshold carefully — over-bloomed LEDs are the single most common way these
  renders look like a toy.
- **Materials.** `MeshPhysicalMaterial` with clearcoat for the anodised faceplate,
  a rough rubber for the knobs, and a translucent transmissive material for the
  ring diffusers so the indicators read as light *through* a surface rather than
  stickers on it.
- **Environment.** A single studio HDRI, `ACESFilmic` tone mapping, one soft key
  light. Contact shadows under the device (`drei/ContactShadows`) — cheap, and it
  is most of what sells the object as physically present.
- **Delivery.** glTF + Draco geometry compression + KTX2/Basis textures. Target
  under 2 MB. Two LODs via `drei/Detailed`; the low LOD drops knurl geometry to a
  normal map.
- **Interaction.** Raycast picking on the knob meshes for selection. Drag to
  rotate a knob → sends a value to the device. Orbit controls with sane limits and
  a "reset view" and a flat top-down preset.

Performance is genuinely not a concern at this scale — a few thousand triangles
and three instanced draw calls. Budget the effort on materials and lighting
instead, which is where the perceived quality actually lives.

### Colour fidelity

The detail that will make or break how real this feels: **the on-screen colour
must match the device**.

The firmware does not send RGB to the LEDs directly. It converts HSV (hue
0–1535) to RGB, applies **per-channel gamma LUTs** (`gamma_corrected_lut_red`,
`_green`, `_blue` in `src/led/color.c`), and drives the result as 32-frame binary
code modulation. Naively rendering the raw HSV in sRGB will look visibly wrong.

So: **export the firmware's gamma LUTs and HSV→RGB routine to TypeScript through
the same codegen as the protocol** ([02](02-protocol.md#codegen-one-schema-two-languages)).
The twin then reproduces the device's actual output, including its
non-linearities. It is a small amount of plumbing for a disproportionate gain in
realism, and it means the colour picker shows the truth.

Also worth modelling: the ring diffuser bleeds light between adjacent segments on
the real device. A small blur along the ring in the shader captures this and is
another of those details that reads as "correct" without anyone being able to say
why.

## Driving it

The twin is **not** a separate application. It is a renderer over the same store
everything else reads:

```
   hardware ──telemetry──►┐
                          ├──► state store ──┬──► 3D twin
   editor UI ──edits─────►┘                  ├──► 2D grid
                                             ├──► inspector
                                             └──► monitors
```

Consequences worth stating explicitly:

- Turning a physical encoder updates the store, so the knob rotates, the ring
  lights, the value readout changes, and the MIDI monitor logs — from one event.
- Dragging a knob in 3D writes to the store *optimistically* and sends to the
  device; the device's echo confirms or corrects. Standard optimistic-update
  reconciliation.
- Because the store is the interface, the twin works identically against the
  **simulator** ([03](03-editor-ux.md#stack)) — which is how you develop and demo
  it without hardware.

Two state layers, kept apart: **config** (what the device is set to — undoable,
persistable, diffable) and **runtime** (positions, LED values, switch states —
ephemeral, high-frequency, never undoable). Mixing them is the mistake that makes
undo stacks explode at 60 Hz.

## Scope control

This is the feature most likely to eat the project. Sequence it so there is
something working early and polish is incremental:

1. **Blockout.** Correct dimensions, plain materials, all 208 emitters wired and
   driven by telemetry. Ugly but *live* — and the live part is the actual
   feature.
2. **Materials pass.** Anodising, knurl, diffusers, HDRI, bloom.
3. **Detail pass.** Photogrammetry-derived normal maps, silkscreen, chamfers,
   USB port, screw heads.
4. **Interaction pass.** Drag-to-turn, press animation, hover-identify,
   camera presets.

Ship after 1. Everything after that is upside.

## Sources

- [Meshy image-to-3D](https://www.meshy.ai/features/image-to-3d) ·
  [Best AI 3D model generators 2026 comparison](https://medium.com/ideas-with-wings/best-ai-3d-model-generators-in-2026-tripo-ai-vs-meshy-rodin-kaedim-and-more-7eea7b05eb11) ·
  [Image-to-3D tools 2026](https://www.3daistudio.com/3d-generator-ai-comparison-alternatives-guide/best-image-to-3d-tools-2026)
- [react-three-fiber: scaling performance](https://r3f.docs.pmnd.rs/advanced/scaling-performance) ·
  [Three.js performance tips 2026](https://www.utsubo.com/blog/threejs-best-practices-100-tips)
- [Midi Fighter Twister product page](https://store.djtechtools.com/products/midi-fighter-twister) ·
  [hardware overview, Cycling '74](https://cycling74.com/articles/hardware-overview-midi-fighter-twister)
