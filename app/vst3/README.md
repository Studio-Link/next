# StudioLink VST3 Plug-in

Runs StudioLink inside a DAW. The DAW replaces the sound card: the plug-in
input is sent to the remote peers, the plug-in output carries the mix of all
remote peers.

```
  DAW track input  ──►  local StudioLink track  ──►  remote peers (SIP/Opus)
  DAW track output ◄──  aumix of all remote tracks
```

The plug-in has no editor. StudioLink is operated through its existing web
frontend, which the "Open Web UI" parameter launches.

## Build

```bash
make vst3sdk   # fetch external/vst3sdk (once)
make vst3
```

The result is `build/VST3/<config>/StudioLink.vst3`. Copy it to your VST3
folder (`~/.vst3` on Linux, `~/Library/Audio/Plug-Ins/VST3` on macOS,
`%COMMONPROGRAMFILES%\VST3` on Windows).

The plug-in is off by default in the normal build; `-DSL_BUILD_VST3=ON`
enables it.

## How it hooks into libsl

`libsl` picks the audio device driver for the local track from
`sl_conf()->play.mod` / `src.mod`, which defaults to `portaudio`. The plug-in
registers a baresip `ausrc`/`auplay` pair named `vst` and points the config at
it *before* `sl_init()` runs, because the local track chooses its driver
during `sl_tracks_init()`. From there on everything else in libsl is
unchanged.

| File | Role |
| --- | --- |
| `src/engine.c` | Shared, reference counted StudioLink instance running `re_main()` on its own thread |
| `src/driver.c` | The `vst` baresip driver and the two ring buffers to the audio thread |
| `src/io.c` | Per instance channel mapping, format and samplerate conversion |
| `src/resample.c` | libsamplerate wrapper for non 48 kHz hosts |
| `src/plugin.cpp` | VST3 `SingleComponentEffect` |
| `src/entry.cpp` | VST3 module factory |

### Threading

Three threads are involved:

- **DAW audio thread** — `process()` converts, pushes into `ab_in`, drives the
  20 ms frame exchange with baresip, pulls from `ab_out`. It never allocates
  and never blocks: the driver lock is taken with `mtx_trylock()` and simply
  skipped when the StudioLink thread is restarting the driver, in which case
  `aubuf` zero fills for a few blocks.
- **StudioLink thread** — `re_main()`, same as the CLI app.
- **DAW GUI thread** — parameter edits. These are marshalled onto the
  StudioLink thread through an `mqueue`, since libre objects must only be
  touched from there. Commands are only sent on actual value changes, so the
  audio thread stays free of syscalls in the steady state.

This is the same arrangement baresip's own `portaudio` module uses, where the
read/write handlers are likewise called from a foreign callback thread into
lock protected `aumix`/`aubuf` objects.

### Clock drift

The DAW clock and the aumix thread's OS clock drift apart just like a sound
card and the OS clock do. The correction lives in libsl (`libsl/src/drift.c`,
applied in `driver_write_handler`), because the buffer on that clock boundary
is `ab_mix` inside libsl either way, so the plug-in inherits it for free.

### Samplerate

StudioLink is fixed at 48 kHz stereo S16LE with 20 ms frames. libre's
`auresamp` only handles integer ratios, so 44.1 kHz hosts would be rejected —
`libsamplerate` is used instead (`libsl/src/resample.c`, shared with the
drift correction), with
`SRC_SINC_FASTEST` and preallocated state so `src_process()` does not allocate
on the audio thread. At 48 kHz the resampler degenerates to a copy.

## Known limitations

- **One active instance per host process.** libsl currently supports a single
  local track (`tracks.c`: *TODO: refactor allow multiple local tracks*) and
  keeps one global libre loop, HTTP server and track list. The first instance
  claims the audio bridge; any further instance loads but stays silent. Lifting
  this needs multi local track support in libsl first.
- **Stereo in / stereo out only.** Mono input is duplicated to both sides.
- **32 bit float processing only.** Double precision would only add
  conversions on the way to 16 bit.
- **Latency is not reported.** The delay is a network path, so the host cannot
  compensate it; `getLatencySamples()` returns 0 and the plug-in declares an
  infinite tail so hosts keep calling `process()` while remote peers are
  talking.
- `sl_open_webui()` launches Chromium with fixed flags on non-Windows hosts. A
  `sl_http_port()` getter in libsl would let the plug-in open the URL with the
  platform default browser instead.
- `sl_baresip_init()` calls `setbuf(stdout, NULL)` and `sys_coredump_set(true)`
  process wide, which is fine for a standalone app but intrusive inside a host
  process.

## Licensing

This repository is MIT. The Steinberg VST3 SDK is **not** — it is dual
licensed under GPLv3 or Steinberg's proprietary licence, and it is fetched
into `external/` rather than vendored. Distributing binaries of this plug-in
means accepting one of those two, and a GPLv3 build would relicense the
combined work. Steinberg also requires registering as a VST3 licensee before
shipping. This is a maintainer decision, not something the build system can
settle.
