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

6. **Type as instruction manual.** When type is relevant, explain it as: the rule that tells the CPU how many bytes to read at this address and how to interpret the bit pattern. Not an abstract concept — a concrete byte-count + interpretation rule baked into the compiled instruction.

7. **Blueprint vs birth.** Class body = zero RAM. Constructor call in main = the only physical event. Always make this boundary explicit.

8. **When the student feels friction**, stop. Name the false model precisely. Replace it with physical reality only. Do not reassure — correct.         