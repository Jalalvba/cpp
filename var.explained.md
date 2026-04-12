# var.cpp — Physical Reality, Line by Line

## Platform baseline (GCC / libstdc++ / Linux x86-64)

- `sizeof(std::string)` = 32 bytes (SSO variant)
- Every function call gets its own stack frame at a different address range
- A reference is not an object. An 8-byte pointer is an object.

---

## `void by_value(std::string tag)`

**What the compiler reads:** "When called, copy-construct a brand-new 32-byte `std::string` at a new address inside this function's stack frame."

- The copy constructor fires: reads `_M_length` from the caller's object. If ≤ 15 chars (SSO), it writes the characters into the new object's own `_M_local_buf` (inline, no heap). Writes `_M_length`. Writes `_M_p` pointing into the new object's own buffer.
- Result: two independent 32-byte objects at two different addresses. The caller's object is untouched.
- `int local = 0` — 4 bytes written on this function's stack frame. Its address serves as a landmark showing where this frame lives.
- `&tag` prints the address of the NEW 32-byte copy — it is different from `&original` in `main`.
- When the function returns: `tag`'s destructor runs. The 32-byte copy is destroyed. The caller's object is untouched.

---

## `void by_reference(const std::string& tag)`

**What the compiler reads:** "Do NOT allocate a new object. Receive the address of the caller's object. `tag` is an alias — it occupies no RAM of its own."

- No copy constructor fires. No new bytes allocated.
- `&tag` prints the SAME address as `&original` in `main`. They are the same bytes in RAM.
- `const` is a compile-time promise: the compiler will reject any write through this alias. The CPU does not enforce it; it is gone after compilation.
- `int local = 0` — 4-byte landmark showing this function has its own stack frame at a different address range than `main`. The reference reaches across frames to `main`'s data.
- When the function returns: nothing to destroy. No object was ever created here.

---

## `void by_pointer(const std::string* tag)`

**What the compiler reads:** "Reserve 8 bytes on this function's stack frame to hold an address. That address is passed in through the door."

- `tag` is a real 8-byte object. It lives at some address on this function's stack. It holds the address of the caller's object as its value.
- Two distinct addresses are visible:
  - `&tag` — where the 8-byte pointer itself lives (this function's frame)
  - `tag` — the VALUE stored in those 8 bytes (the caller's object's address)
- `const` — the compiler will not allow writing through `tag`. Enforced at compile time only.
- `int local = 0` — 4-byte landmark. This function's frame is at yet another address range.
- When the function returns: the 8-byte pointer `tag` is destroyed. The object it pointed to is untouched. Caller's bytes are unaffected.

---

## `int main()`

`main` is a function like any other. The CPU builds a stack frame for it at program entry.

### `int local = 0`
4 bytes written at some address in `main`'s stack frame. Landmark only — shows the address range where `main` lives.

### `std::string original = "div"`
- The compiler sees a string literal `"div"`. The 4 bytes `{'d','i','v','\0'}` live in the `.rodata` section (read-only, baked into the binary).
- A 32-byte `std::string` is constructed at `original`'s stack address inside `main`'s frame.
- "div" = 3 chars ≤ 15 → SSO path:
  - `_M_p` (at `&original + 0`) written to point into `original`'s own inline buffer (`&original + 16`)
  - `_M_length` (at `&original + 8`) written = 3
  - `_M_local_buf` (at `&original + 16`) written = `{'d','i','v','\0', 0,0,...}`
  - No heap allocation. The characters live inside the 32-byte stack object.
- `original` is owned by `main`. Only `main` decides what to share.

### `by_value(original)`
**Through the door:** `main` hands a COPY. The copy constructor reads `original`'s bytes and writes them into a new 32-byte object inside `by_value`'s frame. Two independent objects now exist at two addresses.

### `by_reference(original)`
**Through the door:** `main` passes `original`'s address. No new object. `by_reference` sees `main`'s object directly through its address. It cannot reach anything else in `main`.

### `by_pointer(&original)`
**Through the door:** `main` passes `original`'s address explicitly as a pointer. `by_pointer` stores that address in its own 8-byte variable. Same data path as reference — different syntax.

### `return 0`
`main` returns. `original`'s destructor fires: "div" = SSO → `_M_p == &original + 16` → no heap buffer to free. The 32 stack bytes are reclaimed (rsp adjusts).
