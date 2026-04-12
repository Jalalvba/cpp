# Claude Rules for Jalal's C++ Learning

## C++ Memory Explanation Rules

When explaining any C++ concept, variable, object, or operation:

1. **No ghosts.** Variable names do not exist at runtime. Always translate them to their physical reality: a base address + a byte offset. Never say "tag_name holds a string." Say "the 32 bytes at offset +0 contain a pointer, a size, and a capacity."

2. **No analogies.** No boxes, no houses, no mailboxes, no containers. Only: RAM addresses, byte counts, CPU instructions, pointer dereferences, stack vs heap allocation.

3. **Physical sequence first.** For every operation, state what the CPU actually does in order:
   - Which address is targeted
   - How many bytes are read or written
   - Whether a heap allocation occurs
   - Whether this is a first-write (initialization) or an overwrite (assignment)

4. **Initialization vs assignment.** Always distinguish:
   - Initializer list `: member(value)` → one write, member born with value
   - Body assignment `member = value` → member default-constructed first, then overwritten (two writes)

5. **String internals.** Never say a std::string "holds text." Always: 32 bytes (on 64-bit Linux) = 8-byte heap pointer + 8-byte size + 8-byte capacity + 8 bytes SSO buffer/padding. The characters live at a separate heap address pointed to by the pointer field.

6. **Type as instruction manual.** When type is relevant, explain it as: the instruction that tells the compiler how many bytes to allocate and which constructor to call. Not an abstract concept — a concrete byte-count + constructor baked into the compiled instruction.

7. **Blueprint vs birth.** Class body = zero RAM. Constructor call in main = the only physical event. Always make this boundary explicit.

8. **Function isolation.** Every function is an island. It has its own stack frame, its own addresses. It knows NOTHING about other functions. The parentheses are the only door between scopes:
   - By-value: copy bytes through the door (copy constructor duplicates from source address to new address)
   - By-reference: pass address through the door (no new object, alias to caller's object)
   - By-pointer: pass address through the door (8-byte variable holding caller's object address)
   - Owner decides what to share. Receiver only sees what was shared.

9. **Copy constructor.** Never say "it copies the object." Say: "it reads N bytes from the source address, allocates N bytes at a new address, writes the same bytes into the new location. Two independent objects now exist at two different addresses."

10. **Variable name = label.** After compilation, variable names are gone. They become stack offsets like `[rsp+16]`. Never treat a variable name as something that exists at runtime.

11. **When the student feels friction**, stop. Name the false model precisely. Replace it with physical reality only. Do not reassure — correct.

## C++ File Output Rules

When generating C++ learning code, always produce TWO files:

1. **`<name>.cpp`** — Pure code only. Zero comments. Clean, compilable, no noise.

2. **`<name>.explained.md`** — Line-by-line translation of the code into human-readable physical reality:
   - What the compiler does when it reads this line
   - What addresses are involved
   - How many bytes are allocated/copied/destroyed
   - Which scope owns what
   - What goes through the door (parentheses)

The .cpp is what you compile. The .md is what you study.