# AI Context Documentation - Usage Guide

This guide explains when and how to use the documentation in `docs/ai-context/` when working with AI assistants (Claude, Junie, Air) on the bitKlavier project.

## Quick Reference

| AI Tool | When to Use Context | Which Files |
|---------|-------------------|-------------|
| **AI Chat** | Starting new topic/session, planning, or exploration | `CLAUDE.md` + topic-specific files |
| **Claude Code / CLI** | Terminal-based implementation, repo investigation, multi-file tasks, running builds/tests, or tasks that benefit from subagents | `CLAUDE.md` for most tasks + relevant source/docs |
| **Junie Tasks** | IDE-integrated structured tasks, multi-file changes, or core systems | `CLAUDE.md` for most tasks + relevant system docs |
| **Air** | Broad architectural work, complex projects, or tasks needing maximum context | `CLAUDE.md` + all relevant files |

## Tool Positioning

- **AI Chat** is best for discussion, planning, debugging strategy, and exploring unfamiliar areas before implementation.
- **Claude Code / CLI** is best for terminal-based implementation and repository investigation. Use it when you want the assistant to inspect/edit files, run builds or tests, coordinate multi-file work, or use subagents for parallel investigation.
- **Junie Tasks** are best for IDE-integrated structured implementation work, especially when the task can be described clearly up front.
- **Air** is best for broad, complex, or architectural work where maximum context and project-wide awareness are important.

## Available Context Files

- **`CLAUDE.md`** — Main project guide covering architecture, build system, dependencies, and conventions
- **`RESONANCE_THREADING.md`** — Deep dive on multi-threaded Resonance implementation
- **`README.md`** — Overview of this directory (you're reading the expanded version now!)

## When to Reference Context

### ✅ Always Reference For:

**Larger/Complex Tasks** especially with Claude Code / CLI, Junie Tasks, or Air:
- Multi-file refactoring
- Architectural changes
- New feature implementation
- Performance optimization work
- Adding new preparations

**Critical System Work:**
- Audio thread modifications (real-time safety critical!)
- Build system changes
- Parameter system modifications
- Modulation system changes
- CI/CD pipeline updates

**Unfamiliar Territory:**
- First time working in a subsystem
- Understanding project conventions
- Learning the codebase structure

### ⚪ Optional For:

**Small, Isolated Tasks:**
- Fixing typos
- Simple UI tweaks
- Single-function bug fixes
- Tasks where relevant files are already attached as context

**Conversational Work:**
- If you've already discussed the approach in AI Chat first
- When the immediate context is sufficient

### ❌ Skip For:

- Trivial changes (renaming a variable)
- Quick experiments
- Read-only exploration

-------
## Best Practices

### ✅ DO:

- **Start with CLAUDE.md for unfamiliar subsystems** - It's your project onboarding doc
- **Reference specific docs for deep work** - e.g., RESONANCE_THREADING.md for Resonance optimization
- **Update context docs after significant changes** - Keep the knowledge base current
- **Include relevant code files as additional context** - AI works best with both overview + specifics
- **Mention critical constraints upfront** - Audio thread safety, real-time requirements, etc.

### ❌ DON'T:

- **Over-reference for trivial tasks** - "Fix this typo, but first read these 3 docs..." is overkill
- **Under-reference for complex work** - "Refactor the entire audio engine" without context sets AI up to fail
- **Assume AI remembers previous sessions** - Always provide context at the start of new work
- **Forget to update docs** - Stale documentation is worse than no documentation

## The Golden Rule

**Would a new human developer need to read this documentation before doing the task?**

- **Yes** → Attach the relevant context files
- **No** → You can probably skip it

## Context Size Guidelines

| Task Complexity | Context to Provide |
|----------------|-------------------|
| **Trivial** (1 line) | None needed |
| **Small** (1 file) | Maybe CLAUDE.md if touching core systems |
| **Medium** (2-5 files) | CLAUDE.md + relevant section-specific docs |
| **Large** (5+ files) | CLAUDE.md + all relevant docs + key source files |
| **Architectural** | Full context: all docs + architecture overview |

## Maintaining This System

### When to Add New Context Files:

Create a new file in `docs/ai-context/` when:
- A subsystem becomes complex enough to need dedicated documentation
- You've had to explain the same concepts multiple times
- There are critical design decisions that need to be preserved
- Historical context would help future work (like RESONANCE_THREADING.md)

### Naming Convention:

- `SYSTEM_NAME.md` - For major subsystems (e.g., `TUNING_SYSTEM.md`)
- `FEATURE_NAME_HISTORY.md` - For historical/implementation context
- Use UPPERCASE for main docs, Title_Case for supplementary

### Update Frequency:

- **CLAUDE.md** - Update after major version bumps, dependency changes, or architectural shifts
- **System-specific docs** - Update when implementation changes significantly
- **This guide** - Update when AI tool capabilities change or workflow evolves

## Example: Real Workflow

Here's how you might approach a real task:

### Scenario: Add new "Attack Shaping" parameter to Direct preparation

1. **Research phase** (AI Chat):
   ```
   Attach: docs/ai-context/CLAUDE.md
   
   "I want to add an attack shaping parameter to Direct. 
   Walk me through what's involved."
   ```

2. **Implementation phase** (Claude Code / CLI, Junie Tasks, or Air):
   ```
   Context: docs/ai-context/CLAUDE.md, docs/bitKlavierDevNotes.md
   
   Task: Add "Attack Shaping" parameter to DirectProcessor
   
   Requirements:
   - Add FloatParameter to DirectParams
   -- follow chowdsp pattern
   - Support audio-rate modulation
   - Add UI slider in DirectParametersView
   - Update modulation channel count if needed
   - Apply shaping in processBlock
   
   Constraints:
   - Real-time safe implementation
   - Range: -1.0 to 1.0 (exponential to linear envelope)
   ```

3. **Testing phase**:
    - Build and test manually
    - Verify modulation works
    - Check ThreadSanitizer

4. **Documentation phase**:
    - Update CLAUDE.md if this changes parameter counts or modulation setup
    - Update bitKlavierDevNotes.md examples if this is a new pattern

## Integration with Version Control

Consider adding this to your Git workflow:
### bash
1. Before major features, ensure docs are current:`git diff docs/ai-context/CLAUDE.md`
2. After significant changes, update context docs: `git add docs/ai-context/ && git commit -m "docs: update AI context for new Attack parameter system"`

## Getting Help

If you're unsure whether to include context:
- **Err on the side of including it** - Extra context rarely hurts, missing context often does
- **Start a conversation in AI Chat first** - Discuss the approach, then execute with Junie/Air
- **Iterate** - If AI struggles, add more context and try again

## Future Enhancements

Consider adding these as the project evolves:

- `MODULATION_SYSTEM.md` - Deep dive on state/audio-rate modulation
- `TUNING_SYSTEM.md` - Tuning architecture and scale formats
- `UI_ARCHITECTURE.md` - OpenGL rendering, component hierarchy
- `TESTING_GUIDE.md` - How to write tests for different subsystems
- `PERFORMANCE_PROFILING.md` - Guide to profiling and optimization

---

**Last Updated:** 2026-05-22  
**Version:** 4.9.10


