# Comment Preparation — Text Formatting

The Comment preparation displays free-form text on the construction site. Double-click a comment box to edit; click away or press Escape to close.

Text supports lightweight Markdown formatting. The editor shows the raw syntax while you type; the rendered result appears on the closed box.

---

## Headings

Place `#`, `##`, or `###` at the very start of a line (followed by a space) to create a heading. Headings are always bold and scale relative to the base text size.

```
# Large heading
## Medium heading
### Small heading
```

---

## Inline styles

Wrap text with `*` or `**` anywhere on a line.

| Syntax | Result |
|---|---|
| `**bold text**` | **bold** |
| `*italic text*` | *italic* |
| `***bold and italic***` | ***bold and italic*** |

Styles cannot be nested (e.g. bold inside italic). Unmatched markers are rendered as literal characters.

---

## Color

Wrap text with `{colorname}` and `{/colorname}` tags. The color name must be one of the supported values listed below; unrecognized names are rendered as literal text.

```
{red}important note{/red}
{cyan}this section{/cyan} is worth reading
```

**Supported color names:**
`white` `yellow` `cyan` `green` `lightgreen` `coral` `orange` `red` `blue` `pink`

The default text color (set via the right-click menu) applies to all text that is not wrapped in a color tag.

---

## Lists

Begin a line with `- ` or `* ` for an unordered list item. Indent by two spaces per level to nest. Ordered lists use `1.`, `2.`, etc.

```
- First item
- Second item
  - Nested item
    - Deeply nested
- Back to top level

1. First step
2. Second step
```

Bullet glyphs by indent depth: `•` (level 0), `◦` (level 1), `▪` (level 2+).

> **Note:** Long list items that wrap onto multiple lines will not have a hanging indent — continuation lines start at the left margin. Keep list items short for the best appearance.

---

## Plain indentation

Lines with leading spaces (but no list marker) are indented literally. This is useful for simple code-style blocks or aligned text.

---

## Global formatting

The right-click menu provides options that apply to the entire comment regardless of inline markup:

- **Bold / Italic** — base style for all non-marked-up text
- **Color** — default text color for unmarked text
- **Horizontal / Vertical Alignment** — how text is positioned within the box
- **Background** — grey (default), black, or transparent

Inline markup is additive on top of these global settings. For example, if the global style is italic, `**bold**` will render as bold (not italic+bold) — the markup overrides the style for that span.

---

## Quick reference (also in the right-click menu)

```
# H1   ## H2   ### H3

**bold**   *italic*   ***bold+italic***

{red}colored text{/red}

- item          * item          1. ordered
  - nested        * nested
```
