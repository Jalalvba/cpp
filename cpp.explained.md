# cpp.cpp — Physical Reality, Line by Line

## Platform baseline (GCC / libstdc++ / Linux x86-64)

```
sizeof(std::string)                       = 32 bytes  (SSO variant)
sizeof(std::map<std::string,std::string>) = 48 bytes  (Red-Black tree manager)
sizeof(BlinkElement)                      = 112 bytes (32 + 32 + 48, no padding)
```

## Object block layout (base = address of a BlinkElement instance)

```
Offset   0 –  31 : tag_name  (std::string, 32 bytes)
Offset  32 –  63 : id        (std::string, 32 bytes)
Offset  64 – 111 : attrs     (std::map,    48 bytes)
```

---

## Class body — `class BlinkElement { … }`

**What the compiler reads:** Zero RAM allocated. This is a blueprint — a set of rules telling the compiler how many bytes to reserve and which constructors to call when an instance is created. The CPU never sees this. No bytes exist until a constructor call appears in `main`.

---

## `std::string tag_name` — Offset 0, 32 bytes

The 32-byte layout (GCC SSO):
- `+0  [8 B]` `_M_p` — raw pointer to character data. For SSO strings (≤15 chars), this points into `_M_local_buf` at `+16` (inside the same 32-byte block). No heap.
- `+8  [8 B]` `_M_string_length` — number of characters in use.
- `+16 [16 B]` `_M_local_buf` — inline SSO buffer (15 chars + `'\0'`).

"tag_name" is a human label. After compilation it becomes the address `base + 0`.

---

## `std::string id` — Offset 32, 32 bytes

Same 32-byte layout as `tag_name`. After compilation, "id" becomes `base + 32`.

---

## `std::map<std::string, std::string> attrs` — Offset 64, 48 bytes

The 48-byte **manager** lives inside the object block on the stack:
- `+0  [8 B]` `_M_header` — pointer to end-sentinel node (allocated once on the heap at construction)
- `+8  [8 B]` `_M_header.left` — pointer to minimum (leftmost) node
- `+16 [8 B]` `_M_header.right` — pointer to maximum (rightmost) node
- `+24 [8 B]` `_M_node_count` — number of key-value pairs (size_t)
- `+32 [8 B]` `_M_key_compare` — `std::less<std::string>` comparator (stateless)
- `+40 [8 B]` alignment tail

Each key-value pair lives in a **separate heap node** allocated by `malloc` on insert:
```
heap_node {
    rb_color (4 B) | parent* (8 B) | left* (8 B) | right* (8 B)
    | std::string key   (32 B)
    | std::string value (32 B)
}
```

---

## Constructor — `BlinkElement(const std::string& tag, const std::string& element_id)`

### Phase 1 — Colon list `: tag_name(tag), id(element_id)`

This is **initialization**. The CPU visits each member offset exactly once and calls its constructor directly on raw RAM. One write pass per member.

**Step A — `base+0` (`tag_name`):**
Calls `std::string::string(const std::string& tag)`.
- Reads `tag._M_length` = 3 ("div" ≤ 15 → SSO path).
- Copies `{'d','i','v','\0'}` into `base+16` (`_M_local_buf`).
- Writes `base+8` = 3 (`_M_length`).
- Writes `base+0` = `base+16` (`_M_p` points into own block).
- No heap allocation.

**Step B — `base+32` (`id`):**
Same process. "container" = 9 chars ≤ 15 → SSO.
- Copies into `base+48`.
- Writes `base+40` = 9.
- Writes `base+32` = `base+48`.

**Step C — `base+64` (`attrs`):**
`std::map` default constructor fires automatically (not listed in colon list).
- Allocates a small sentinel node on the heap.
- Writes `_M_node_count = 0`.
- Header pointers point to the sentinel.

### Phase 2 — Body `{ … }`

Members are fully alive in RAM. The `cout` dereferences `_M_p` at `base+0` → reads from `base+16` → 3 bytes sent to stdout. Same for `id`.

**Key distinction:**
- Colon list = one write. Member born with value.
- Body assignment would be: default-construct first (one write), then `operator=` overwrites (second write). Two ops instead of one. Never put member initialization in the body.

---

## Destructor — `~BlinkElement()`

Called when the object goes out of scope. Members destroyed in **reverse construction order**:

1. **`~attrs` (`base+64`):** Walks entire Red-Black tree. For each heap node: calls `~string` on value (may free heap buffer), calls `~string` on key (may free heap buffer), calls `free(node)`. Sets `_M_node_count = 0`.

2. **`~id` (`base+32`):** Checks: `id._M_p == base+48`? YES (SSO) → no heap free. The inline buffer dies with the 32-byte block.

3. **`~tag_name` (`base+0`):** Same check. "div" = SSO → no `free()`.

After destructor: the 112 stack bytes are reclaimed.

---

## `setAttribute(const std::string& name, const std::string& value)`

`attrs[name] = value` — the `operator[]` contract:
- O(log n) Red-Black tree traversal comparing key bytes.
- **Key missing:** `malloc` new heap node (~88 bytes). Copy-constructs `node.key` from `name`. Default-constructs `node.value = ""`. Wires node into tree (pointer updates + possible rotations). Increments `_M_node_count` at `base+88`.
- **Key exists:** returns reference to existing `node.value`. No `malloc`.
- Either path: assigns `value` to the returned reference via `operator=`. If new value fits in SSO → inline buffer overwritten, `_M_length` updated. No heap realloc.

---

## `getAttribute(const std::string& name) const`

`attrs.find(name)`:
- O(log n) traversal. Each step: `_M_p` dereference → compare character bytes → go left or right.
- Returns an iterator: an 8-byte pointer to a heap node, or the end-sentinel pointer.

**On miss** (`it == attrs.end()`): constructs a new empty `std::string` on the caller's stack. Returns it. 32 bytes, 0 chars, no heap.

**On hit:** `it->second` is a reference to the `std::string` value inside the heap node. Return statement copy-constructs a new `std::string` from that. If ≤ 15 chars → SSO, no heap alloc for the copy.

---

## `hasAttribute(const std::string& name) const`

`find()` returns either a pointer to a node or the end-sentinel pointer. Comparing two iterators = comparing two 64-bit integers. Result is 1 or 0 written into a register. Zero heap traffic, zero string copies.

---

## `printAllAttributes() const`

`attrs.empty()` reads `_M_node_count` at `base+88`. Single 8-byte read. No pointer chasing.

Range-for = in-order Red-Black tree traversal (left→node→right). Keys visited in lexicographic order (`std::less<std::string>`).

`const auto& pair` is a reference into the heap node. No copies. `pair.first` → `_M_p` → `char[]` → bytes sent to stdout.

---

## `main()` — Line by Line

### `sizeof(BlinkElement)` etc.

Resolved entirely at **compile time**. Zero CPU cost at runtime. These are the type lens: the compiler's instruction manual for byte counts and constructor selection.

### `BlinkElement div("div", "container")`

The compiler reserved 112 bytes in `main`'s stack frame at frame-entry (rsp adjusted). That reservation is instant and cost-free.

This line triggers:
1. Two temporary `std::string` objects from the string literals (or compiler may use `string_view` — impl detail).
2. `BlinkElement::BlinkElement()` called with refs to those temporaries.
3. Colon list fires: `base+0`, `base+32`, `base+64` initialized (see constructor above).
4. Constructor body: `cout` fires.

After this line: 112 stack bytes are fully initialized RAM.

### `reinterpret_cast<uintptr_t>(&div)`

Reinterprets the pointer bits as a plain 64-bit unsigned integer. Subtracting two such integers gives the exact byte distance between members — the runtime equivalent of `offsetof()`.

Expected offsets (GCC/libstdc++): `tag_name` = 0, `id` = 32, `attrs` = 64.

### `div.setAttribute("class", "box highlight")` (and two more)

Each call: O(log n) tree traversal → key not found → `malloc` heap node N1/N2/N3 → copy-construct key (SSO) → default-construct value → assign value (SSO) → increment `_M_node_count`. After 3 calls: `_M_node_count` at `base+88` = 3.

### `std::string val = div.attrs["typo"]`

**`operator[]` is a write operation disguised as a read.**

Machine-level effect:
1. O(log n) tree search: "typo" NOT found.
2. `malloc` fires → heap node N4 allocated.
3. `N4.key = "typo"` copy-constructed (5 chars, SSO).
4. `N4.value = ""` default-constructed (0 chars, SSO).
5. N4 wired into tree. `_M_node_count`: 3 → 4.
6. Reference to `N4.value` returned.
7. `val` (32-byte `std::string` on `main`'s stack) copy-constructed from ref → empty string.

N4 is now permanently in the map. Use `find()` or `getAttribute()` when reading only.

### `div.setAttribute("class", "box")`

Tree search: "class" FOUND (node N1). No `malloc`. `operator[]` returns reference to `N1.value`. `N1.value.operator=("box")`: old value "box highlight" (13 chars, SSO buffer) → overwritten with "box" (3 chars). `_M_length` updated 13 → 3. No heap realloc.

### `return 0` — end of scope

The compiler inserts a hidden call to `~BlinkElement()` before the `ret` instruction.

Destruction order (reverse of construction):
1. `~attrs`: traverses N1–N4. For each: `~string(value)` (SSO, no free), `~string(key)` (SSO, no free), `free(node)`. Four `free()` calls. `_M_node_count` → 0.
2. `~id`: "container" = SSO → `_M_p == base+48` → no free.
3. `~tag_name`: "div" = SSO → `_M_p == base+16` → no free.

112 stack bytes reclaimed. rsp adjusts. CPU executes `ret`.

---

## Full memory map — BlinkElement (112-byte object block)

```
Offset  Size  Field           Contents (div="div", id="container")
──────  ────  ──────────────  ──────────────────────────────────────
  0      8 B  tag_name._M_p   → base+16 (SSO: points into own block)
  8      8 B  tag_name.size   3
 16     15 B  tag_name.buf    "div\0" + 12 zero bytes
 31      1 B  null term       0x00
──────  ────  ──────────────  ──────────────────────────────────────
 32      8 B  id._M_p         → base+48 (SSO: points into own block)
 40      8 B  id.size         9
 48     15 B  id.buf          "container\0" + 6 zero bytes
 63      1 B  null term       0x00
──────  ────  ──────────────  ──────────────────────────────────────
 64      8 B  attrs.header    pointer to end-sentinel node (heap)
 72      8 B  attrs.begin     pointer to leftmost node (heap)
 80      8 B  attrs.end       pointer to header/sentinel
 88      8 B  attrs.count     4 (after operator[] trap)
 96      8 B  attrs.compare   std::less<std::string> (stateless)
104      8 B  padding         0x00…
```

## Heap nodes (allocated by std::map on each insert)

```
N1 "class"      → "box"           (SSO key, SSO value)
N2 "data-index" → "3"             (SSO key, SSO value)
N3 "hidden"     → "true"          (SSO key, SSO value)
N4 "typo"       → ""              (inserted silently by operator[])
```

## Full pointer chain (object → characters)

```
&div (stack)
  → base+64 (map manager, stack)
    → _M_header._M_left (pointer) → heap node N1
      → N1.key._M_p (pointer into N1's SSO buffer)
        → char[] "class\0"
```
