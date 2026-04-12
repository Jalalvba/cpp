# classe.cpp — Physical Reality, Line by Line

## Platform baseline (GCC / libstdc++ / Linux x86-64)

- `sizeof(std::string)` = 32 bytes (SSO variant)
- `sizeof(BlinkElement)` = 32 bytes (one `std::string` member, no padding)
- SSO threshold: strings ≤ 15 chars store characters inline inside the 32-byte block. No heap.

---

## `struct BlinkElement` — Class body

Zero RAM. A blueprint. The CPU never executes this. No bytes exist until the constructor is called.

---

## `std::string tag_name`

When an instance exists, this member occupies bytes `base+0` through `base+31`:
- `+0  [8 B]` `_M_p` — pointer to character data
- `+8  [8 B]` `_M_string_length` — count of characters in use
- `+16 [16 B]` `_M_local_buf` — inline SSO buffer (15 chars + `'\0'`)

"tag_name" does not exist at runtime. It becomes the offset `+0` from the object's base address.

---

## Constructor — `BlinkElement(const std::string& tag) : tag_name(tag)`

`const std::string& tag` — no new object created. The compiler passes the address of the caller's `std::string`. `tag` is an alias. `&tag` prints the caller's object address.

### Colon list `: tag_name(tag)` — initialization

The CPU visits `base+0` exactly once and calls `std::string::string(const std::string& tag)`:
- Reads `tag._M_length`. "div" = 3 chars ≤ 15 → SSO path.
- Copies `{'d','i','v','\0'}` into `base+16` (the inline `_M_local_buf`).
- Writes `base+8` = 3 (`_M_string_length`).
- Writes `base+0` = `base+16` (`_M_p` points into the object's own block).
- No heap allocation. Characters live inside the 32-byte stack block.

This is one write pass. `tag_name` is born with the value. Compare to body assignment: default-construct first → then `operator=` overwrites → two passes.

---

## Constructor body — address inspection

### `&tag` vs `&tag_name`

- `&tag` = address of the caller's `std::string` (passed by reference — zero new bytes).
- `&tag_name` = `base+0` = the member's address inside the `BlinkElement` instance.
- They are at **different addresses**. The reference brought the caller's address through the door; the colon list copied the bytes into a new location.

### `tag.data()` vs `tag_name.data()`

Both return `_M_p` — the pointer to character bytes.
- `tag.data()` → points into the **caller's** SSO buffer (inside the caller's `std::string` block).
- `tag_name.data()` → points into `base+16` (inside this object's own 32-byte block).
- Different addresses. Two independent buffers. Two independent objects.

### SSO check

```cpp
const char* tag_start = reinterpret_cast<const char*>(&tag);
const char* tag_end   = tag_start + sizeof(std::string);  // +32
const char* tag_buf   = tag.data();
bool tag_uses_sso = (tag_buf >= tag_start && tag_buf < tag_end);
```

This checks: "is the character buffer's address inside the 32-byte object itself?"
- YES = SSO. Characters live inline at `+16`. No heap allocation.
- NO = heap. `_M_p` holds an address outside the object block.

For "div" (3 chars): both `tag` and `tag_name` are SSO → YES.

### Raw byte dump

Iterates from index 0 to `tag_name.size()` (inclusive, to show the null terminator).
- Dereferences `_M_p` (→ `base+16`) one byte at a time.
- Reads 4 bytes total: `0x64='d'`, `0x69='i'`, `0x76='v'`, `0x00='\0'`.
- These 4 bytes are physically stored at `base+16` through `base+19` inside the 32-byte stack block.

---

## `main()` — Line by Line

### STEP 0: `sizeof(std::string) = 32`

Resolved at compile time. Zero runtime cost. Tells you: every `std::string` object occupies exactly 32 bytes wherever it is placed (stack, heap, inside a struct).

### STEP 1: `const char* literal = "div"`

`"div"` is a string literal. The 4 bytes `{'d','i','v','\0'}` are baked into the `.rodata` section of the binary — read-only memory loaded once when the program starts. They live at a fixed address for the lifetime of the process.

`literal` is an 8-byte pointer on `main`'s stack holding that `.rodata` address.

The byte loop: reads 4 bytes directly from `.rodata`. No `std::string` involved. No SSO. No constructor.

### STEP 2: Temporary `std::string` (compiler note)

When `BlinkElement elem("div")` is processed, the compiler must convert `"div"` (a `const char*`) into a `const std::string&`. It constructs a temporary 32-byte `std::string` on `main`'s stack from the literal. This temporary is what gets passed by reference to the constructor.

### STEP 3: `BlinkElement elem("div")`

- The compiler reserved 32 bytes in `main`'s stack frame for `elem` at frame-entry.
- `BlinkElement::BlinkElement()` fires: colon list copy-constructs `tag_name` from the temporary. 32 bytes at `elem`'s address are initialized. One write pass.
- After the constructor returns, the temporary `std::string` is destroyed (SSO → no heap free). The characters now live in `elem.tag_name`'s own inline buffer.

### STEP 4: Post-construction inspection

- `&elem` = base address of the 32-byte `BlinkElement` block on `main`'s stack.
- `&elem.tag_name` = same address as `&elem` (first and only member, offset 0).
- `elem.tag_name` printed: dereferences `_M_p` (→ `base+16`) → reads 3 bytes → stdout.

### `return 0`

`elem` goes out of scope. `~BlinkElement()` fires (implicitly, compiler-inserted before `ret`):
- `~tag_name` (`base+0`): checks `_M_p == base+16`? YES (SSO). No `free()`. The 32 bytes are reclaimed with the stack frame.
