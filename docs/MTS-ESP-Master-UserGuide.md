# MTS-ESP Master (Tuning preparation)

## What it is

[MTS-ESP](https://github.com/ODDSound/MTS-ESP) is a system that lets one plugin
broadcast a tuning to every other MTS-ESP-aware plugin running in your DAW (or on
your computer, in the case of the standalone). bitKlavier can act as the
**MTS-ESP master**: any Tuning preparation can publish its tuning so that other
microtonal-capable instruments and effects automatically follow bitKlavier's
tuning — in real time, including while notes are sustaining.

This is **master/publishing only** for now: bitKlavier can *send* its tuning to
other plugins, but it does not yet *receive* tuning from another MTS-ESP master.

## How to use it

1. Open any Tuning preparation. At the top-left of the popup, next to the close
   (✕) button, you'll see a **Set MTS-ESP Master** button and a status label.
2. Click **Set MTS-ESP Master**. bitKlavier claims the MTS-ESP master role and
   begins publishing this Tuning. The button changes to **MTS-ESP Master: ON**.
3. Open any MTS-ESP-aware client plugin in your session — it will pick up
   bitKlavier's tuning automatically.
4. Click the button again to turn it off and release the master role.

**Only one Tuning can be the MTS-ESP master at a time.** Selecting a different
Tuning as master automatically deselects the previous one — there is a single
master per bitKlavier instance/gallery, not a per-Tuning switch.

Whatever the selected Tuning does, the clients follow. With **Spring** or
**Adaptive** tunings, the published tuning updates continuously as the tuning
evolves, so connected clients bend along with bitKlavier during sustained notes.

## Status messages

The label to the right of the button tells you what's happening:

| Message | Meaning |
|---|---|
| `off` | No Tuning is selected as master. |
| `MTS-ESP Master: ON` / `Publishing (N clients)` | This Tuning is the master and is broadcasting. `N` is how many MTS-ESP clients are currently connected (it can be 0 — that just means nothing is listening yet). |
| `another Tuning is master` | A *different* Tuning in this gallery is the master. Click this one's button to take over. |
| `Blocked - another MTS-ESP master is running` | Another application or plugin already owns the MTS-ESP master role. bitKlavier won't take it from them. (See "If it's blocked" below.) |
| `MTS-ESP library not installed` | The MTS-ESP system library isn't installed on this computer, so master mode can't run. Install it (it ships with bitKlavier) and try again. |
| `disabled in build` | This build of bitKlavier was compiled without MTS-ESP support. |

## If it's blocked

If you click **Set MTS-ESP Master** and bitKlavier reports it's blocked *but you
know nothing else is acting as master*, this is usually **stale state** left
behind by a plugin that crashed or was force-quit without releasing MTS-ESP.
bitKlavier will offer a dialog:

> **MTS-ESP master unavailable** — … you can reinitialize MTS-ESP and take over.
> **[Reinitialize & take over]** **[Cancel]**

Choosing **Reinitialize & take over** clears the orphaned state and makes
bitKlavier the master. Only do this if you're certain no other app is genuinely
acting as the MTS-ESP master, since it resets MTS-ESP for the whole system.

## Saving, loading, and switching

- The master selection is **saved with your gallery**. Reopening a gallery
  restores it and bitKlavier attempts to become master again.
- If the saved master Tuning no longer exists (e.g. it was deleted), the
  selection is simply cleared on load.
- Deleting the Tuning that is currently the master turns MTS-ESP master off.
- Switching the active piano does **not** interrupt publishing — the selected
  master keeps broadcasting regardless of which piano is active.
- bitKlavier releases the MTS-ESP master role automatically when you close the
  plugin or quit the app.

## Limitations

- **Master/publishing only** — there is no receive (client) mode yet.
- **One master per bitKlavier instance/gallery.**
- **One master per system** — this is an MTS-ESP rule, not a bitKlavier one. If
  you run two bitKlavier instances (or another master plugin), only one can hold
  the role; the others show `Blocked`.
- Other plugins must themselves support MTS-ESP to follow bitKlavier's tuning.
- The MTS-ESP system library must be installed (it ships with bitKlavier's
  installer).
