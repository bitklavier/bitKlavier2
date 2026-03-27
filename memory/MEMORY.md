# bitKlavier2 Memory

## Piano Switching Performance (ConstructionSite)

Quick wins landed on branch `danwork8` (March 2026):
- Added `is_rebuilding_` flag to skip N redundant `updateScrollBars()` calls during `rebuildAllGui()` — now called once at the end of `setActivePiano()` instead of per-preparation
- Removed dead `setCentrePosition(s->curr_point)` in `moduleAdded` (was immediately overwritten by the scroll-adjusted version)

**Deferred: per-piano component cache**
See `memory/piano_cache.md` for full design and trade-offs. Key risks that need solving before implementing:
- Cache invalidation: `prep_list->removeListener(this)` is called when a piano is hidden, so ValueTree changes (add/delete prep, undo) while it's cached won't update the cache → stale restore
- `all_sliders_` map accumulation: `removeSubSection` doesn't clean those maps, so cached pianos leave stale entries that could affect modulation display
- Memory: all pianos' GL textures and component trees stay alive simultaneously
