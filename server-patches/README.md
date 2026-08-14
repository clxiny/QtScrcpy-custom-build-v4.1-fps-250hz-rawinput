# Custom scrcpy 4.1 server

QtScrcpy bundles the scrcpy 4.1 Android server as a prebuilt APK. The stock
protocol has no command that pauses capture while keeping the control channel
alive, so this feature requires matching changes on both sides.

The first patch adds control message `23` (`SET_VIDEO_PAUSED`):

- pause signals EOS to the active `MediaCodec`;
- screen capture and the encoder are released on the Android device;
- the video thread blocks without producing frames;
- the control thread/socket remains active;
- resume creates a fresh capture and encoder session on the same video socket.

The second patch adds control message `24` (`RESET_INPUT_STATE`). A physical
touch may cancel an injected Android gesture without updating scrcpy's own
pointer table. The next desktop gesture would then be combined with a stale
pointer and controls could appear permanently dead. The reset command:

- asks Android to cancel the current global touch stream when the server has
  the privileged `MONITOR_INPUT` permission (for example an adbd-root session);
- otherwise injects `ACTION_CANCEL` for every pointer still tracked by scrcpy;
- clears the server pointer table, down-time, display and input-source state;
- keeps the video, control socket and application session alive.

The desktop sends this reset only at a new gesture boundary, before the
original touchscreen `DOWN`. It is not inserted into FPS `MOVE` packets or the
4 ms/250 Hz Raw Input path. `Ctrl+Shift+R` performs the same recovery manually
and also clears delayed desktop actions and the retained UHID device, so a
tablet reboot is no longer the first recovery step.

## Windows build

Install Git, JDK 17 and Android SDK Platform/Build Tools 36, then run from
PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\server-patches\build-custom-server.ps1
```

The script clones the official scrcpy `v4.1` source, applies both patches, builds
the server APK and replaces QtScrcpyCore's bundled server. The original APK is
kept as `scrcpy-server.official-v4.1`.

This source package already includes a rebuilt matching `scrcpy-server`. Do not
replace it with the stock v4.1 binary: a stock server does not understand
messages 23/24 and would close the custom control protocol.

The QtScrcpy desktop application must then be rebuilt so the custom server is
copied into its deployment directory.

## Absolute Android pointer and game touch bridge

The desktop patch creates an absolute UHID drawing tablet (`Digitizer` with
`INPUT_PROP_POINTER` and a stylus tool) instead of injecting
`SOURCE_MOUSE/ACTION_HOVER_MOVE`. Raw UHID hover reports pass through Android's
InputReader and PointerChoreographer, so Android 15/16 draws and keeps the real
system pointer visible. Synthetic hover injection only reaches applications and
cannot restore a system pointer hidden by a touchscreen event; that was the
cause of the disappearing cursor in the previous implementation.

The pointer uses absolute HID X/Y axes whose inclusive maxima are generated from
the current server-reported video dimensions. Android 16 deliberately applies a
single `max(xScale, yScale)` scale to both axes for drawing tablets. Therefore a
square 32767x32767 descriptor cannot align with a rectangular 2136x3200 display:
the cursor and injected touch separate progressively along the longer axis. The
dynamic descriptor preserves the display aspect ratio, so the UHID pointer and
the scrcpy `PositionMapper` finger click share the same normalized position.
Rotation or video-size changes automatically destroy and recreate the device
with the new range. Games which filter mouse/drawing-tablet button events still
receive primary DOWN/MOVE/UP as touchscreen events.

Android 16 can reject a click while a mouse or drawing-tablet hover stream is
still active (the equivalent upstream scrcpy workaround is `--no-mouse-hover`).
On primary press this build sends `InRange=0`, waits 24 ms for InputReader to
consume the hover exit, and only then starts the finger stream. Pointer reports
stay suspended until UP, after which the absolute system cursor is restored at
the release position. Fast clicks are retained as a valid timed DOWN/UP pair.

The same UHID report supports five buttons, vertical wheel and horizontal pan.
It uses the stock scrcpy 4.1 UHID protocol and therefore requires no additional
APK or mouse-specific server patch. Enable it from the mouse toolbar button and
press `Ctrl+Shift+M` to release the captured desktop cursor.

Pointer mode and the custom FPS keymap are mutually exclusive but switched in
both directions by `~`. From FPS mode, `~` exits the keymap and immediately
creates the visible absolute Android pointer for menu or weapon-purchase clicks.
Pressing `~` again sends `InRange=0` and suspends UHID reports while retaining
the idle descriptor, avoiding an Android InputReader device rescan in the FPS
hot path. FPS motion and mapped buttons then go exclusively through the original
direct `CMT_INJECT_TOUCH` keymap route; they never fall through to UHID or the
24 ms pointer-click bridge. The transition also clears the previous relative
position, pending coalesced delta and delayed small-eyes callbacks, and
recenters the Windows cursor to establish a clean movement baseline. An explicit pointer
disable or disconnect still destroys the UHID device.

## High-rate Windows FPS camera input

While the FPS keymap owns the mouse, the Windows client registers the mouse as
a Raw Input device and reads relative `WM_INPUT` deltas. Legacy accelerated
`WM_MOUSEMOVE` copies are suppressed only in this route, so FPS movement never
passes through UHID or the pointer-click delay. Each handler also drains any
additional accumulated raw records in aligned batches, preventing stale input
from sitting behind video/UI work. The first delta in a burst is sent
immediately; subsequent reports are coalesced into one direct
`CMT_INJECT_TOUCH` MOVE every 4 ms (up to 250 Hz), which preserves the complete
delta without allowing a 500/1000 Hz mouse to build a Qt event backlog. DOWN
and MOVE keep pressure at 1.0; only UP resets it to zero.

When the simulated camera finger reaches the 5%/95% safe boundary, the client
now sends the boundary point, performs an immediate UP/DOWN handoff at the
configured camera origin, and consumes the remaining delta in the same update.
It no longer discards the crossing report and the following five mouse reports.
The control TCP socket also enables `LowDelayOption` for latency-sensitive small
packets. If Raw Input registration fails, the client logs the Windows error and
keeps the original QMouseEvent path as a fallback.
