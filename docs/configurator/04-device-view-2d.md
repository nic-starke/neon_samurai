# 04 — The 2D device view

A stylised, schematic representation of the Midi Fighter Twister at the centre of
the editor, whose LEDs and knobs track the real hardware in real time and respond
to edits made in the UI.

Supersedes an earlier photoreal-3D plan. The reasoning for the change is in
[Why 2D is the better call](#why-2d-is-the-better-call) — it is not only a cost
decision.

## The idea

Not a picture of the device. A **precision instrument diagram** of it — flat,
top-down, drawn rather than rendered. Thin strokes, generous negative space,
silkscreen-style labels, and light that reads as light because it is the only
saturated thing on the panel.

The reference points are technical drawings and Teenage Engineering's product
diagrams, VIA's keyboard view, and circuit-board silkscreen — not a product
photo. The device is recognisably a Twister from its proportions (square body,
4×4 grid, three side buttons per edge), not from its texture.

```
     ┌──────────────────────────────────────────┐
   ▢ │   ( 00 )    ( 01 )    ( 02 )    ( 03 )   │ ▢
     │                                          │
   ▢ │   ( 04 )    ( 05 )    ( 06 )    ( 07 )   │ ▢
     │                                          │
   ▢ │   ( 08 )    ( 09 )    ( 10 )    ( 11 )   │ ▢
     │                                          │
     │   ( 12 )    ( 13 )    ( 14 )    ( 15 )   │
     └──────────────────────────────────────────┘
       ▢ = side switch      ( ) = encoder + ring
```

Each `( )` is drawn as an arc of eleven tick marks around a knob with a rotating
pointer, sitting on a soft radial halo — see [Anatomy](#anatomy).

## Why 2D is the better call

The cost saving is real but it is the least interesting reason.

**A schematic can annotate; a render cannot.** This is the actual argument. A
photoreal knob can only show you what the hardware shows you — 11 lit segments.
A drawn one can simultaneously show the virtmap's *range*, the portion of
rotation it occupies, the current value, the dead zone outside it, the MIDI
assignment, and whether it collides with another control. The 3D twin would have
been a beautiful display of a small amount of information. The 2D view is an
information display, and this is a configuration editor.

**One view instead of two.** The 3D plan needed a parallel 2D grid anyway, for
accessibility and for bulk editing speed. That was two implementations of the
same thing kept in sync forever. Now there is one.

**Accessibility stops being a compromise.** SVG elements are real DOM. Every
encoder is focusable, labellable, and keyboard-navigable for free, rather than
needing a shadow DOM tree mirroring a WebGL canvas.

**It works everywhere.** No WebGL dependency for the core UI means Safari and
Firefox render the editor perfectly. They still cannot *connect* (see
[01](01-transport.md#browser-support-summary)), but the whole interface now works
against the simulator on every browser — which turns the public demo from a
Chromium-only curiosity into something anyone can open.

**The quality floor is much higher.** Photoreal that is slightly off — wrong knob
pitch, over-bloomed LEDs, a plasticky faceplate — reads as cheap. A clean
schematic essentially cannot look bad. Lower ceiling on spectacle, far higher
floor, and much less of the outcome depends on getting materials and lighting
right.

**What it removes:** the glTF/Draco/KTX2 asset pipeline, HDRI lighting, LOD, a
photogrammetry session, mobile GPU performance work, and ~600 KB of
three.js/r3f/postprocessing dependencies.

**What it costs:** the wow factor. Mitigations in [Keeping it
striking](#keeping-it-striking).

## Anatomy

The emitter structure is unchanged — the device still has 208 individually
addressable emitters and the view still models all of them, one-to-one with
`gFRAME_BUFFER`.

Per encoder:

| Element | Drawn as |
| --- | --- |
| 11 ring indicators | tick marks on a ~270° arc around the knob |
| RGB underglow | a soft radial halo behind the knob |
| Detent | a marker at the arc's centre point |
| Knob | a circle with a rotating pointer notch |
| Press state | inner fill brightens, knob scales ~2% |

Per `src/include/led/led.h`, within each `u16` of `gFRAME_BUFFER`: bit 0 detent
blue, bit 1 detent red, bit 2 RGB blue, bit 3 RGB red, bit 4 RGB green, bits
5–15 the eleven indicators. The SVG maps onto this directly, so telemetry needs
no translation layer.

Plus 6 side buttons, three per edge, drawn as rounded bars that light on press.

## Rendering

**SVG, not canvas.** ~250 elements is nothing for the DOM, and DOM is what buys
focus, labels, hit-testing, CSS theming and export. Canvas would mean
reimplementing all of that.

**Keep React out of the 60 Hz path.** React renders the structure once. Telemetry
updates go through a single `requestAnimationFrame` loop that writes attributes
directly via refs:

```ts
// one loop, no reconciliation, no re-render
for (let e = 0; e < 16; e++) {
  const s = ledState[e];
  knobRefs[e].style.transform = `rotate(${s.angle}deg)`;
  haloRefs[e].setAttribute('fill', s.rgbCss);
  for (let i = 0; i < 11; i++)
    tickRefs[e][i].setAttribute('opacity', s.indicator[i] ? '1' : '0.12');
}
```

208 attribute writes per frame is well within budget. If profiling ever says
otherwise, the escape hatch is a canvas overlay for the emitter layer only,
keeping SVG for structure and interaction — but start simple.

**Glow without filters.** `feGaussianBlur` recomputed every frame is the one
thing that will actually be slow. Do not use it. Each halo is a static
radial-gradient circle whose `fill` and `opacity` you mutate, composited with
`mix-blend-mode: screen` so overlapping light adds the way real light does. Zero
filter cost, and additive blending is most of what sells an LED as emitting
rather than being coloured.

**Motion.** Knob rotation is a CSS transform, so it is GPU-composited. Indicator
transitions get a very short (~60 ms) opacity ease so fast movement reads as a
sweep rather than a strobe — but value-following elements never animate their
*position*, or the view lags the hardware and feels disconnected.

**Theming.** Chrome colours are CSS custom properties, so light/dark and any
accent are free. LED colours are data and come from telemetry, never from theme.

## Colour fidelity

Unchanged from the 3D plan, and more important here — there is no lighting model
to hide behind, so the colours are compared directly against the hardware sitting
next to the screen.

The firmware converts HSV (hue 0–1535) to RGB, applies per-channel gamma LUTs
(`gamma_corrected_lut_red/_green/_blue` in `src/led/color.c`), and drives the
result as 32-frame binary code modulation. Rendering raw HSV as sRGB will look
visibly wrong.

**Export the firmware's gamma LUTs and HSV→RGB routine to TypeScript through the
same codegen as the protocol** ([02](02-protocol.md#codegen-one-schema-two-languages)).
The view then reproduces the device's actual output, including its
non-linearities, and the colour picker shows the truth.

Also worth reproducing: light bleeds between adjacent segments on the real ring.
A slight overlap in the tick-mark gradients captures this and is one of those
details that reads as correct without anyone being able to say why.

## Overlays — the payoff

The same SVG, different fill rules. This is what the render could not do.

| Mode | Shows |
| --- | --- |
| **Live** | what the LEDs are actually doing right now |
| **Assignment** | each encoder tinted by MIDI channel, labelled with its CC or note |
| **Conflicts** | duplicate channel + CC pairs flagged across the whole bank |
| **Range** | each virtmap's range and rotational span drawn as arc segments |
| **Diff** | what differs from the saved profile, or from what is in EEPROM |
| **Activity** | a session heatmap of which controls you actually touch |

Each is a few lines of styling over a structure that already exists. Together
they are more useful than the 3D view would ever have been, and **Conflicts**
and **Diff** in particular are things no editor in this category ships.

## Interaction

- **Click** an encoder to select it. Selection is shared with the tree, the
  table and the inspector ([03](03-editor-ux.md#principles)).
- **Drag** a knob to send a value — test a mapping in your DAW without touching
  the hardware.
- **Hover** to pulse that encoder on the physical device (`LED_OVERRIDE`).
- **Arrow keys** move between encoders in grid order; `Tab` moves between
  regions. Every encoder carries an `aria-label` like *"Encoder 7, bank 2, CC 21
  channel 1, value 94"*.
- **Shift/ctrl-click** for multi-select, then edit sixteen at once.
- **Right-click** for copy/paste/reset on a single control.

## Export

Because the view is SVG, "export this bank as an image" is almost free — and it
is a genuinely good feature. A clean diagram of your mapping, with the
**Assignment** overlay on, is exactly what people paste into forum posts, README
files and gig notes. Offer SVG and PNG, light and dark.

## Keeping it striking

The honest cost of this change is spectacle. Where to spend the effort instead:

1. **Light quality.** Additive blending, correct gamma, a subtle halo falloff.
   Get this right and the LEDs look genuinely luminous.
2. **Restraint in the chrome.** The panel should be near-monochrome so the LED
   colours are the only saturated thing on screen. This is the same constraint as
   [03 § Visual language](03-editor-ux.md#visual-language) and it is what makes
   the view feel like an instrument.
3. **Micro-interactions.** Knob press, selection ring, hover halo, the sweep on a
   fast turn. Small, fast, and everywhere.
4. **An isometric variant** for the landing page, README and screenshots — same
   layout constants, different projection. Gives you the hero image without
   putting a 3D engine in the product.

## Scope

1. **Structure.** Correct proportions, all 208 emitters and 16 knobs drawn and
   wired to state. Plain styling. *Ship here* — it is live from this point.
2. **Light pass.** Halos, blending, gamma, motion.
3. **Overlays.** Assignment, conflicts, range, diff.
4. **Polish.** Silkscreen labels, side buttons, export, isometric variant.

Step 1 is days rather than weeks, and it has no asset pipeline in front of it —
which is why the device view now lands in phase 2 rather than phase 4
([06](06-roadmap.md)).
