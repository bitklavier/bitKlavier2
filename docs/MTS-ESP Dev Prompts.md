
Below is a series of prompts you can use with Claude Code, AIR, or Junie. I’d recommend using them sequentially rather than asking for the whole implementation at once.
The overall strategy is:
Research / architecture only
Old implementation comparison
Integration design
Build-system integration
Coordinator/wrapper implementation
Tuning UI/state integration
Publishing/update behavior
Testing/review

```
Prompt 1 — Initial architecture review, no edits
Use this first. It asks the assistant to understand the current project and produce a plan without touching files.``` text
I want to integrate MTS-ESP master/publishing functionality into bitKlavier’s Tuning preparation.

Important design constraints:

- MTS-ESP master functionality is the priority, not receive/client mode.
- MTS-ESP should be configured from individual Tuning preparations.
- However, only one Tuning preparation in a bitKlavier gallery/plugin instance should be selected as the MTS-ESP master at a time.
- Treat this like an exclusive selection/radio-button model: each Tuning can offer “Set as MTS-ESP Master,” but the underlying saved state should probably be a single selected Tuning UUID, not independent booleans on every Tuning.
- The actual MTS-ESP master object should probably be owned/coordinated at plugin/SynthBase level, not by each Tuning processor independently.
- Separate saved intent from runtime status:
    - saved: which Tuning UUID is selected as bitKlavier’s MTS-ESP master source
    - runtime: whether bitKlavier successfully claimed/published as MTS-ESP master
- Avoid any blocking, allocation-heavy, locking, or external IPC work on the audio thread.
- Publishing to MTS-ESP should happen from a safe non-audio context or through a safe queued/deferred mechanism.
- If another application/plugin already owns the MTS-ESP master role, bitKlavier should fail gracefully and show/report status.
- Do not implement code yet.

Please inspect the current Tuning architecture and relevant state/save/load/UI patterns. Identify the files/classes involved and propose a staged integration plan.

Please answer with:

1. Current Tuning architecture summary.
2. Where the selected MTS master Tuning UUID should live.
3. Where the runtime MTS-ESP coordinator/master wrapper should live.
4. How the Tuning UI should interact with that coordinator.
5. How tuning changes should trigger MTS-ESP publishing safely.
6. Audio-thread-safety risks.
7. Save/load and deletion edge cases.
8. A staged file-by-file implementation plan.
9. Questions or ambiguities that should be resolved before coding.

Do not edit files yet.
```

```
Prompt 2 — Compare old bitKlavier implementation
Use this after making the old source code available to the assistant.``` text
I have an older bitKlavier implementation that included MTS-ESP support. Please inspect the old implementation and compare it to the current bitKlavier Tuning architecture.

Design goal for the new implementation:

- Implement MTS-ESP master/publishing mode first.
- Per-Tuning UI selection, but only one selected MTS-ESP master Tuning per gallery/plugin instance.
- Prefer a single saved selected Tuning UUID over per-Tuning boolean state.
- A plugin/SynthBase-level coordinator should own the actual MTS-ESP master lifecycle.
- Tuning processors should provide tuning data and notify the coordinator when their tuning changes.
- Do not perform MTS-ESP external calls directly on the audio thread.

Please produce a migration report, not code yet.

Include:

1. What the old implementation did:
   - master/publish?
   - receive/client?
   - both?
   - how it initialized MTS-ESP
   - how it updated tuning
   - where it stored state
   - how it handled UI/status
2. Which old code can be reused directly.
3. Which old code should be rewritten due to current architecture.
4. Any MTS-ESP API assumptions that need to be verified against the current MTS-ESP repository.
5. Risks around plugin lifecycle, standalone vs AU/VST3, and multiple plugin instances.
6. A proposed modernized design for bitKlavier 4.
7. Concrete file-by-file steps for a receive-free, master-only first implementation.

Do not edit files yet.
```

```
Prompt 3 — Write the implementation spec
This prompt is good for AIR or Claude after the architecture review. It produces a durable spec you can keep in docs/ or memory/.``` text
Based on the current bitKlavier architecture and the old MTS-ESP implementation, please write a concise implementation specification for “MTS-ESP Master support in Tuning preparations.”

The spec should assume:

- Master/publishing mode is phase 1.
- Receive/client mode is out of scope for now.
- Each Tuning preparation can be selected as the source for MTS-ESP master publishing.
- Only one Tuning per gallery/plugin instance can be selected at a time.
- Saved state should use one authoritative selected Tuning UUID, preferably at gallery/root level or another appropriate centralized location.
- Runtime MTS-ESP master ownership/status should be transient and not saved as if it were guaranteed.
- The actual MTS-ESP wrapper/coordinator should be plugin/SynthBase-level.
- Audio thread must never call blocking MTS-ESP operations.

Please include:

1. User-facing behavior.
2. UI wording.
3. State model:
    - saved properties
    - transient runtime state
4. Coordinator responsibilities.
5. Tuning processor responsibilities.
6. MTS-ESP wrapper responsibilities.
7. Threading model.
8. Save/load behavior.
9. Deletion behavior.
10. Active piano switching behavior.
11. External conflict behavior if another MTS-ESP master exists.
12. Test plan.
13. Staged implementation checklist.

Do not edit code.
```

```
Prompt 4 — Build-system integration only
Use this when you’re ready to start coding. Keep this isolated.``` text
Please implement only the build-system/dependency preparation for optional MTS-ESP support.

Constraints:

- Do not yet integrate MTS-ESP into Tuning behavior.
- Do not add UI yet.
- Do not change audio processing yet.
- Add MTS-ESP behind a compile-time option such as BITKLAVIER_ENABLE_MTS_ESP or similar.
- The default can be ON or OFF; please recommend one and explain why before editing.
- Ensure the project can still build if MTS-ESP is disabled.
- Prefer a clean wrapper boundary so most of the app does not directly include MTS-ESP headers.
- If MTS-ESP source is vendored under third_party, wire it into CMake in a minimal way.
- If the exact MTS-ESP files/API are unclear, create a stub/no-op wrapper first and leave TODOs rather than guessing dangerously.

Please:

1. Inspect the current CMake structure.
2. Propose exact files to modify/add.
3. Then implement the minimal optional build integration.
4. Build or explain how to build.
5. Report any compile errors or API uncertainties.

Do not touch Tuning UI or runtime behavior in this step.
```

```
Prompt 5 — Add no-op coordinator/wrapper skeleton
This is a safe next step before real MTS calls.``` text
Please add a no-op MTS-ESP master coordinator/wrapper skeleton, without yet calling the real MTS-ESP API unless it is already clearly available.

Design:

- A plugin/SynthBase-level coordinator should manage:
    - selected master Tuning UUID
    - runtime publishing status
    - start/stop publishing
    - update/publish tuning table requests
- The coordinator should be safe to compile with MTS-ESP disabled.
- With MTS-ESP disabled, all methods should be no-ops and status should indicate “MTS-ESP disabled/unavailable.”
- The selected master Tuning UUID should eventually be saved in the gallery/root ValueTree, but if this step is too much, prepare the API and leave state hookup for the next step.
- Do not add UI yet.
- Do not publish from the audio thread.
- Do not change Tuning sound behavior.

Suggested classes, if appropriate:

- MTSESPMasterCoordinator
- MTSESPMasterWrapper or MTSESPMasterClient

Please implement:

1. New wrapper/coordinator files in an appropriate source location.
2. Compile-time guards for MTS-ESP enabled/disabled.
3. Public methods needed by Tuning UI later:
    - setSelectedMasterTuningUuid(...)
    - clearSelectedMasterTuning()
    - getSelectedMasterTuningUuid()
    - isTuningSelectedAsMaster(uuid)
    - getStatus()
    - notifyTuningChanged(uuid)
    - notifyTuningDeleted(uuid)
4. Hook coordinator ownership into SynthBase or another suitable plugin-level owner.
5. Ensure it builds.

Keep real MTS-ESP API calls stubbed unless the API is obvious and safe.
```

```
Prompt 6 — Add saved selected-Tuning UUID state
This step establishes the exclusive-selection model.``` text
Please implement saved state for selecting one Tuning preparation as the MTS-ESP master source.

Design constraints:

- Do not store independent “is master” booleans on each Tuning.
- Store one authoritative selected Tuning UUID, preferably at gallery/root ValueTree level, unless you find a better centralized existing place.
- Runtime status is transient and should not be saved as authoritative.
- If the selected UUID points to a deleted/nonexistent Tuning after load, handle gracefully.
- Do not add actual MTS-ESP publishing yet.
- Do not add full UI yet unless a tiny debug/helper is necessary.

Please implement:

1. Identifier/property additions as needed, e.g. mtsEspMasterTuningUuid.
2. SynthBase/coordinator methods to read/write this property.
3. Save/load behavior, using existing ValueTree mechanisms.
4. Cleanup when a selected Tuning is deleted:
   - clear selected UUID
   - stop runtime publishing if applicable
5. Methods for UI to query whether a given Tuning UUID is selected.
6. Tests or at least a manual test plan.

Be careful with ValueTree setProperty reentrancy: if multiple properties are updated, snapshot needed values before the first setProperty call.
```

```
Prompt 7 — Add Tuning UI control and status display
Use after the coordinator and state exist.``` text
Please add UI controls to the Tuning preparation for MTS-ESP master selection.

User-facing behavior:

- Each Tuning preparation shows an MTS-ESP section/control.
- The control should behave like an exclusive selection:
    - If this Tuning is not selected, clicking selects it as the MTS-ESP master source.
    - If this Tuning is already selected, clicking clears/disables MTS-ESP master selection.
    - Selecting this Tuning implicitly deselects any other Tuning, because the saved state is one selected UUID.
- Show runtime status text if practical:
    - Off
    - Selected
    - Publishing
    - Blocked/unavailable because another MTS-ESP master exists
    - MTS-ESP disabled at build time
- Do not use per-Tuning saved booleans for the exclusive master state.
- Do not perform real MTS-ESP work in UI callbacks directly; call the coordinator.

Please inspect the existing TuningParametersView/Tuning UI patterns and implement the smallest consistent UI addition.

Please include:

1. UI component additions.
2. Layout changes.
3. Button callback to coordinator.
4. Timer/listener/status refresh if needed.
5. Ensure opening multiple Tuning popups reflects the same selected UUID state.
6. Manual test plan.

Do not implement real MTS-ESP publishing unless the coordinator already supports it.
```

```
Prompt 8 — Connect Tuning changes to publish requests
This is where actual music data starts flowing, but still possibly with a no-op wrapper.``` text
Please connect Tuning changes to the MTS-ESP master coordinator’s publish/update mechanism.

Design constraints:

- Only the selected Tuning UUID should publish.
- If a non-selected Tuning changes, do nothing.
- If the selected Tuning changes, schedule/push an MTS-ESP tuning update safely.
- Avoid blocking/external MTS-ESP calls on the audio thread.
- If tuning changes are detected on the audio thread, use a lock-free flag/queue or defer to message thread/timer/background mechanism.
- Avoid allocation on the audio thread.
- Publishing should send a complete 128-note tuning table or the API-appropriate equivalent.
- If A4/global tuning changes affect the table, that should also cause an update.
- Do not change bitKlavier’s internal tuning behavior.

Please inspect how TuningProcessor/TuningState currently represents note frequencies/cents and how listeners/parameter callbacks are used.

Please implement:

1. A method for TuningProcessor or TuningState to export the current 128-note tuning table.
2. A safe notification path from selected Tuning changes to the coordinator.
3. Coordinator logic to ignore updates from non-selected Tunings.
4. Deferred/non-audio-thread publishing pathway.
5. Stub logging/status updates if real MTS-ESP calls are not implemented yet.
6. Manual tests:
   - select Tuning A, change Tuning A -> publish request
   - change Tuning B -> no publish
   - select Tuning B -> publish B immediately
   - delete selected Tuning -> publishing stops
```

```
Prompt 9 — Implement real MTS-ESP master publishing
Use this only after the wrapper and coordinator are in place.``` text
Please implement real MTS-ESP master/publishing calls inside the MTS-ESP wrapper only.

Important:

- Keep MTS-ESP SDK headers/includes isolated to the wrapper implementation as much as possible.
- Do not call MTS-ESP external/shared-memory/IPC functions from the audio thread.
- The coordinator should pass a prepared tuning table to the wrapper from a safe context.
- If MTS-ESP reports another master already exists or master initialization fails, status should become Blocked/Unavailable and the UI should report it.
- If MTS-ESP is disabled at compile time, wrapper remains no-op.
- If API details are ambiguous, stop and report uncertainties rather than guessing.

Please inspect the vendored/current MTS-ESP API and implement:

1. Master initialization.
2. Master shutdown.
3. Publish/update tuning table.
4. Status/error reporting.
5. Behavior when another master exists.
6. Behavior on plugin shutdown/destruction.
7. Standalone/AU/VST3 considerations.

After implementation, build and report results.
```

```
Prompt 10 — Lifecycle and edge-case pass
Use after the basic feature works.``` text
Please review and harden the MTS-ESP master integration.

Check these cases:

1. Gallery load with selected MTS master UUID.
2. Gallery load where selected UUID no longer exists.
3. Saving and reloading selected master state.
4. Deleting the selected Tuning.
5. Duplicating/copying a Tuning if supported.
6. Multiple Tuning preparations in one piano.
7. Multiple pianos in one gallery.
8. Active piano switching.
9. Multiple bitKlavier plugin instances.
10. Another application/plugin already acting as MTS-ESP master.
11. Closing plugin/editor while publishing.
12. Closing audio processor while publishing.
13. MTS-ESP disabled at compile time.
14. Standalone vs AU vs VST3.
15. pluginval validation.

Please inspect the code and identify any unsafe lifecycle, threading, or ValueTree issues. Fix small issues if straightforward; otherwise report recommended changes.

Pay special attention to:

- no audio-thread blocking/allocation
- no dangling TuningProcessor pointers
- no stale selected UUID
- clean shutdown
- no direct UI dependence in backend
- no inconsistent saved state
```

```
Prompt 11 — Final code review for audio-thread safety
This one is important.``` text
Please perform a focused audio-thread-safety review of the MTS-ESP master integration.

Assume that MTS-ESP calls may involve shared memory, locks, IPC, allocation, or other non-realtime-safe behavior unless proven otherwise.

Please identify every path that can lead to:

- MTS-ESP initialization
- MTS-ESP shutdown
- MTS-ESP publish/update
- selected master UUID changes
- Tuning table export
- status updates

For each path, classify whether it can run on:

- audio thread
- message thread
- background thread
- unknown thread

Then report:

1. Any audio-thread violations.
2. Any possible allocation or locking on the audio thread.
3. Any unsafe pointer lifetime.
4. Any ValueTree access from unsafe threads.
5. Any UI calls from non-message threads.
6. Recommended fixes.

Do not make large changes without asking; small safety fixes are okay.
```

```
Prompt 12 — Documentation/manual entry draft
Use once it’s working.``` text
Please draft a short user manual entry for the new MTS-ESP Master feature in the Tuning preparation.

Explain:

- What MTS-ESP is in this context.
- How to select a Tuning as the MTS-ESP Master.
- Only one Tuning can be selected as the master source at a time.
- Other MTS-ESP-aware plugins can follow bitKlavier’s tuning.
- What the status messages mean:
  - Off
  - Selected
  - Publishing
  - Unavailable/Blocked
  - Disabled
- What happens when switching galleries or reloading.
- What happens if another MTS-ESP master is already running.
- Any limitations:
  - master/publishing only, no receive mode yet
  - one selected master per bitKlavier instance/gallery
  - external plugins must support MTS-ESP

Keep the tone concise and user-facing.
```


A compact “all-in-one planning prompt”
If you want a shorter prompt for an initial AIR session, use this:
``` text
I want to add MTS-ESP master/publishing support to bitKlavier’s Tuning preparation.

Core design:
- Master/publishing mode is the priority; receive mode is not phase 1.
- The UI should be per Tuning preparation: each Tuning can offer “Set as MTS-ESP Master.”
- Only one Tuning can be selected as the MTS-ESP master source at a time.
- Prefer storing one selected Tuning UUID in centralized gallery/plugin state rather than storing booleans on every Tuning.
- The real MTS-ESP master object/lifecycle should be owned by a plugin/SynthBase-level coordinator, not by each Tuning processor.
- Separate saved selection from runtime status.
- No MTS-ESP calls or blocking work should occur on the audio thread.
- If another app/plugin already owns MTS-ESP master, bitKlavier should fail gracefully and show status.

Please inspect the current project and old MTS-ESP implementation if available, then produce a staged implementation plan. Do not edit code yet.

Include:
1. Current Tuning architecture summary.
2. Proposed state model.
3. Proposed coordinator/wrapper classes.
4. UI behavior.
5. Threading model.
6. Save/load/deletion behavior.
7. MTS-ESP API questions.
8. File-by-file staged implementation plan.
9. Testing plan.
```

 
My recommended order
I’d run them in this order:
- Prompt 1 — current architecture review
- Prompt 2 — old implementation comparison
- Prompt 3 — write implementation spec
- Prompt 4 — build-system integration
- Prompt 5 — no-op coordinator/wrapper
- Prompt 6 — selected UUID state
- Prompt 7 — Tuning UI
- Prompt 8 — publish requests
- Prompt 9 — real MTS-ESP calls
- Prompt 10/11 — hardening and safety review
- Prompt 12 — docs

- The most important rule: don’t let it jump straight to real MTS-ESP calls before the coordinator, state model, and audio-thread-safe update path are in place.
