// =============================================================================
// FILE: cpp.cpp
// TOPIC: Machine-Level Analysis of BlinkElement
//
// PLATFORM BASELINE (GCC / libstdc++ / Linux x86-64)
// ─────────────────────────────────────────────────
//  sizeof(std::string)                       = 32 bytes  (SSO variant)
//  sizeof(std::map<std::string,std::string>) = 48 bytes  (Red-Black tree manager)
//  sizeof(BlinkElement)                      = 112 bytes (32 + 32 + 48, no padding)
//
// OBJECT BLOCK LAYOUT (base = address of a BlinkElement instance)
// ───────────────────────────────────────────────────────────────
//  Offset   0 – 31  : tag_name  (std::string, 32-byte manager)
//  Offset  32 – 63  : id        (std::string, 32-byte manager)
//  Offset  64 – 111 : attrs     (std::map,    48-byte manager)
//
// std::string INTERNAL LAYOUT (GCC SSO, 32 bytes each)
// ─────────────────────────────────────────────────────
//  +0  [8 B]  _M_p           — raw pointer to char[].
//                              Points into _M_local_buf when string ≤ 15 chars (SSO).
//                              Points to a separate heap buffer otherwise.
//  +8  [8 B]  _M_string_length — number of chars in use (size_t).
//  +16 [16 B] _M_local_buf   — inline SSO buffer (15 chars + '\0').
//                              When string > 15 chars this slot holds
//                              _M_allocated_capacity instead.
//
// std::map INTERNAL LAYOUT (Red-Black tree manager, 48 bytes)
// ─────────────────────────────────────────────────────────────
//  +0  [8 B]  _M_header / color+parent pointer
//  +8  [8 B]  _M_header.left   (pointer to minimum/begin node)
//  +16 [8 B]  _M_header.right  (pointer to maximum/rbegin node)
//  +24 [8 B]  _M_node_count    (size_t — number of key-value pairs)
//  +32 [8 B]  _M_key_compare   (std::less<std::string>, stateless but padded)
//  +40 [8 B]  alignment tail
//
// Each key-value pair lives in a SEPARATE HEAP NODE (malloc'd on insert):
//  heap_node { rb_color(4B) | parent*(8B) | left*(8B) | right*(8B)
//              | std::string key(32B) | std::string value(32B) }
// =============================================================================

#include <iostream>
#include <string>
#include <map>
#include <cstdint>   // uintptr_t

// =============================================================================
// CLASS: BlinkElement
//
// The 112-byte RAM block layout when an instance is created:
//
//   base+0   ┌─────────────────────────────┐
//            │ tag_name  (std::string, 32B) │
//   base+32  ├─────────────────────────────┤
//            │ id        (std::string, 32B) │
//   base+64  ├─────────────────────────────┤
//            │ attrs     (std::map,    48B) │
//   base+112 └─────────────────────────────┘
//
// "Variable names" are human labels for these fixed byte offsets.
// The CPU only sees addresses and byte counts — never names.
// =============================================================================
class BlinkElement {
public:

    // ── MEMBER: tag_name  (Offset 0, 32 bytes) ───────────────────────────
    // This is NOT "the text div". It is a 32-byte control structure.
    //
    //   base+0  [8B]  _M_p          — pointer to character data
    //   base+8  [8B]  _M_length     — number of characters stored
    //   base+16 [16B] _M_local_buf  — inline SSO buffer (≤15 chars)
    //
    // Example: tag = "div" (3 chars, fits in SSO)
    //   _M_p      → base+16  (points INTO this same 32-byte block, no heap)
    //   _M_length = 3
    //   base+16   = { 'd','i','v','\0', 0,0,0,0,0,0,0,0,0,0,0,0 }
    std::string tag_name;   // Offset 0

    // ── MEMBER: id  (Offset 32, 32 bytes) ────────────────────────────────
    // Same 32-byte layout as tag_name.
    //
    // Example: element_id = "container" (9 chars, fits in SSO)
    //   _M_p      → base+48  (points into this block's own local_buf)
    //   _M_length = 9
    //   base+48   = { 'c','o','n','t','a','i','n','e','r','\0',... }
    std::string id;         // Offset 32

    // ── MEMBER: attrs  (Offset 64, 48 bytes) ─────────────────────────────
    // The map MANAGER lives here (inside the object block).
    // The actual key-value data lives on the HEAP in separate tree nodes.
    //
    //   base+64  _M_header  — sentinel/end node pointer
    //   base+72  _M_begin   — pointer to leftmost (smallest key) node
    //   base+80  _M_end     — pointer to header (past-end sentinel)
    //   base+88  _M_count   — number of entries (size_t)
    //   base+96  comparator — std::less<std::string>
    //
    // Path from object → data:
    //   &div (stack) → base+64 (map manager, stack)
    //                → _M_header (pointer, 8B) → heap node (malloc'd)
    //                   → heap_node.key   (std::string, 32B)
    //                      → key._M_p → char[] (SSO inline or heap)
    //                   → heap_node.value (std::string, 32B)
    //                      → value._M_p → char[] (SSO inline or heap)
    std::map<std::string, std::string> attrs;  // Offset 64


    // =========================================================================
    // CONSTRUCTOR — The Birth Process
    //
    // Two distinct phases:
    //
    // PHASE 1 — COLON LIST  : tag_name(tag), id(element_id)
    // ───────────────────────────────────────────────────────
    // This is INITIALIZATION. The CPU visits each member offset ONCE and
    // calls its constructor directly on the raw RAM bytes. ONE write pass.
    //
    //   STEP A — base+0 (tag_name):
    //     Calls std::string::string(const std::string& tag).
    //     Reads tag._M_length. If ≤ 15 → SSO path:
    //       Copies chars into base+16 (local_buf).
    //       Writes base+8  = length.
    //       Writes base+0  = &base[16]  (pointer into own block).
    //     NO heap allocation for short strings.
    //
    //   STEP B — base+32 (id):
    //     Same process. "container" = 9 chars ≤ 15 → SSO.
    //       Copies chars into base+48 (local_buf of id).
    //       Writes base+40 = 9.
    //       Writes base+32 = &base[48].
    //
    //   STEP C — base+64 (attrs):
    //     std::map default-constructor fires automatically (not listed here).
    //     Sets _M_node_count = 0, header pointers point to sentinel. No heap.
    //
    // PHASE 2 — BODY  {}
    // ────────────────────
    // This is where ASSIGNMENT would live (if any).
    // ASSIGNMENT = constructor already ran + operator= overwrites = 2 ops.
    // Since no member assignment happens here, only cout runs.
    //
    // RULE: Initialization (colon list) is always cheaper than assignment.
    //       Prefer colon list for every member, every time.
    // =========================================================================
    BlinkElement(const std::string& tag, const std::string& element_id)
        : tag_name(tag),        // Direct-construct at base+0  — ONE write
          id(element_id)        // Direct-construct at base+32 — ONE write
          // attrs auto-default-constructs at base+64
    {
        // Members are fully alive in RAM here.
        // Printing tag_name: dereferences _M_p (→ base+16) → reads 3 bytes.
        // Printing id:       dereferences _M_p (→ base+48) → reads 9 bytes.
        std::cout << "[constructor] <" << tag_name << " id=\"" << id << "\">\n";
    }


    // =========================================================================
    // DESTRUCTOR — The Death Process
    //
    // Called when the object goes out of scope (stack) or delete fires (heap).
    // Members are destroyed in REVERSE construction order:
    //
    //   1. ~attrs (base+64):
    //      Walks entire Red-Black tree. For each node:
    //        - Calls ~string on node's key   (may free heap buffer)
    //        - Calls ~string on node's value (may free heap buffer)
    //        - Calls free() on the node itself
    //      Sets _M_node_count = 0.
    //
    //   2. ~id (base+32):
    //      Checks: id._M_p == &base[48]?
    //        YES (SSO) → nothing to free, inline buffer dies with the block.
    //        NO  (heap)→ calls free(id._M_p).
    //
    //   3. ~tag_name (base+0):
    //      Same check. "div" = SSO → no free().
    //
    //   After destructor: the 112 bytes are returned to allocator.
    // =========================================================================
    ~BlinkElement() {
        // attrs.size() reads _M_node_count at base+88 — a direct 8-byte read,
        // no pointer chasing needed.
        std::cout << "[destructor]  <" << tag_name << " id=\"" << id << "\"> — map had "
                  << attrs.size() << " attribute(s)\n";
    }


    // =========================================================================
    // METHOD: setAttribute
    //
    // attrs[name] = value
    //
    // std::map::operator[] contract:
    //   - Traverse Red-Black tree comparing key bytes (O(log n)).
    //   - If key EXISTS:  return reference to existing value node. No alloc.
    //   - If key MISSING:
    //       malloc() a new tree node on the heap.
    //       Copy-construct node.key   from `name`.
    //       Default-construct node.value = "".
    //       Wire node into tree (pointer updates + possible rotations).
    //       Increment _M_node_count at base+88.
    //   - Assign `value` to the returned reference via operator=.
    //       If old value's heap buffer is too small → realloc.
    //       If new value fits in SSO → free old buffer, write inline.
    // =========================================================================
    void setAttribute(const std::string& name, const std::string& value) {
        attrs[name] = value;
        // Each call: 1 tree traversal + possibly 1 malloc + string writes.
    }


    // =========================================================================
    // METHOD: getAttribute
    //
    // attrs.find(name):
    //   - O(log n) tree traversal. Each step compares key bytes via _M_p.
    //   - Returns an iterator (an 8-byte pointer to a tree node, or end sentinel).
    //
    // On MISS (it == end()):
    //   - Constructs a new empty std::string on the caller's stack.
    //   - Returns it (no heap alloc — SSO, 0 chars).
    //
    // On HIT:
    //   - it->second is a reference to the value std::string inside the heap node.
    //   - Return statement copy-constructs a new std::string from that reference.
    //   - If value ≤ 15 chars → SSO, no heap alloc for the copy.
    // =========================================================================
    std::string getAttribute(const std::string& name) const {
        auto it = attrs.find(name);       // it = 8-byte pointer to node (or end)
        if (it == attrs.end()) return ""; // new empty 32-byte string on stack
        return it->second;                // copy-construct value from heap node
    }


    // =========================================================================
    // METHOD: hasAttribute
    //
    // find() returns the end-sentinel pointer when key is absent.
    // Comparing two iterators = comparing two 64-bit integers (pointer values).
    // Result is 1 or 0 written into a register. No heap traffic whatsoever.
    // =========================================================================
    bool hasAttribute(const std::string& name) const {
        return attrs.find(name) != attrs.end();  // pointer comparison → bool
    }


    // =========================================================================
    // METHOD: printAllAttributes
    //
    // std::map range-for = in-order Red-Black tree traversal (left→node→right).
    // Keys are visited in alphabetical order (std::less<std::string> ordering).
    //
    // `const auto& pair` is a reference into the heap node — no copies made.
    //   pair.first  = reference to node's key   std::string (32B in heap node)
    //   pair.second = reference to node's value std::string (32B in heap node)
    //
    // Printing follows the pointer chain:
    //   pair.first → _M_p → char[] → bytes sent to stdout
    // =========================================================================
    void printAllAttributes() const {
        if (attrs.empty()) {               // reads _M_node_count (base+88) == 0
            std::cout << "  (no attributes)\n";
            return;
        }
        for (const auto& pair : attrs) {   // iterator walks heap nodes in key order
            std::cout << "  " << pair.first << " = \"" << pair.second << "\"\n";
        }
    }
};


// =============================================================================
// main() — The High-Level Trigger
//
// main() is the entry point. The CPU builds a stack frame for it.
// Local variables live in that frame.
//
// Execution flow:
//   main() ──triggers──▶ BlinkElement constructor (RAM writes)
//          ──triggers──▶ setAttribute calls       (heap allocs)
//          ──triggers──▶ getAttribute / has calls  (tree reads)
//          ──triggers──▶ operator[] trap            (silent heap alloc)
//          ──triggers──▶ BlinkElement destructor   (heap frees)
// =============================================================================
int main() {

    // ── SIZEOF REPORT ─────────────────────────────────────────────────────
    // sizeof() is resolved entirely at COMPILE TIME. Zero CPU cost at runtime.
    // These numbers are the Type Lens: the compiler's instruction manual
    // telling the CPU how many bytes to reserve and how to interpret them.
    std::cout << "=== sizeof breakdown ===\n";
    std::cout << "sizeof(BlinkElement)                     : " << sizeof(BlinkElement) << " bytes\n";
    std::cout << "sizeof(std::string)                      : " << sizeof(std::string)  << " bytes\n";
    std::cout << "sizeof(std::map<std::string,std::string>): " << sizeof(std::map<std::string,std::string>) << " bytes\n";
    std::cout << "\n";


    // ── OBJECT BIRTH ──────────────────────────────────────────────────────
    // The compiler reserved 112 bytes in main's stack frame for `div`
    // at frame-entry (rsp adjusted). That reservation is instant.
    //
    // This line triggers:
    //   1. Two temporary std::string objects for the literals "div" and
    //      "container" (or the compiler may pass them as string_view — impl detail).
    //   2. BlinkElement::BlinkElement() called with refs to those strings.
    //   3. Colon list fires: base+0, base+32, base+64 initialized.
    //   4. Constructor body: cout fires (reads _M_p pointers).
    //
    // After this line: 112 stack bytes are fully initialized RAM.
    std::cout << "=== create element ===\n";
    BlinkElement div("div", "container");   // 112 bytes on stack, 3 members initialized


    // ── ADDRESS ARITHMETIC ────────────────────────────────────────────────
    // reinterpret_cast<uintptr_t> reinterprets the pointer bits as a plain
    // 64-bit unsigned integer. Subtracting two such integers gives the exact
    // byte distance — the runtime equivalent of offsetof().
    //
    // Expected results (GCC/libstdc++):
    //   tag_name offset = 0
    //   id       offset = 32
    //   attrs    offset = 64
    uintptr_t base = reinterpret_cast<uintptr_t>(&div);
    // `base` is now the numeric address of the 112-byte block's first byte.

    std::cout << "address of div                           : " << &div << "\n";
    std::cout << "offset of tag_name                       : "
              << (reinterpret_cast<uintptr_t>(&div.tag_name) - base) << "\n";
    std::cout << "offset of id                             : "
              << (reinterpret_cast<uintptr_t>(&div.id)       - base) << "\n";
    std::cout << "offset of attrs                          : "
              << (reinterpret_cast<uintptr_t>(&div.attrs)    - base) << "\n";

    // This prints an address WITHIN the 112-byte stack block,
    // confirming the map manager is NOT on the heap — only its nodes are.
    std::cout << "address of div.attrs                     : " << (void*)&div.attrs << "\n";
    std::cout << "attrs.size() before inserts              : " << div.attrs.size() << "\n";
    // _M_node_count at base+88 = 0
    std::cout << "\n";


    // ── ATTRIBUTE INSERTION ───────────────────────────────────────────────
    // Each setAttribute triggers:
    //   a) O(log n) tree traversal (key not found on first insert)
    //   b) malloc() → new heap node (~88 bytes: rb fields + two std::strings)
    //   c) key   copy-constructed into node (SSO if ≤15 chars)
    //   d) value copy-constructed into node (SSO if ≤15 chars)
    //   e) tree pointer rewiring + Red-Black rebalancing rotations
    //   f) _M_node_count++ (base+88)
    //
    // "class"      =  5 chars → SSO key,  "box highlight" = 13 chars → SSO value
    // "data-index" = 10 chars → SSO key,  "3"             =  1 char  → SSO value
    // "hidden"     =  6 chars → SSO key,  "true"          =  4 chars → SSO value
    std::cout << "=== setAttribute ===\n";
    div.setAttribute("class",      "box highlight"); // malloc → heap node N1
    div.setAttribute("data-index", "3");             // malloc → heap node N2
    div.setAttribute("hidden",     "true");          // malloc → heap node N3
    std::cout << "attrs.size() after 3 inserts: " << div.attrs.size() << "\n";
    // base+88 (_M_node_count) = 3
    std::cout << "\n";


    // ── ATTRIBUTE READ ─────────────────────────────────────────────────────
    // find() = O(log n) tree traversal, read-only, no heap traffic.
    // On hit:  iterator points to heap node → read ->second → copy to return value.
    // On miss: iterator == end sentinel → getAttribute returns "" (stack-allocated).
    std::cout << "=== getAttribute ===\n";
    std::cout << "getAttribute(\"class\")      : \"" << div.getAttribute("class")      << "\"\n";
    std::cout << "getAttribute(\"data-index\") : \"" << div.getAttribute("data-index") << "\"\n";
    std::cout << "getAttribute(\"missing\")    : \"" << div.getAttribute("missing")    << "\"\n";
    // "missing": find returns end() → no heap alloc → returns empty string
    std::cout << "\n";


    // ── BOOLEAN CHECK ──────────────────────────────────────────────────────
    // hasAttribute = find() pointer comparison → 1 register operation → bool.
    // Zero heap traffic, zero string copies.
    std::cout << "=== hasAttribute ===\n";
    std::cout << "hasAttribute(\"hidden\")  : " << div.hasAttribute("hidden")  << "\n";
    std::cout << "hasAttribute(\"missing\") : " << div.hasAttribute("missing") << "\n";
    std::cout << "\n";


    // ── THE OPERATOR[] TRAP ────────────────────────────────────────────────
    //
    // WARNING: std::map::operator[] is a WRITE operation disguised as a read.
    //
    // Contract: "If key not found, INSERT it with default value, return ref."
    //
    // Machine-level effect of  div.attrs["typo"]:
    //   1. O(log n) tree search: "typo" NOT found.
    //   2. malloc() fires → heap node N4 allocated.
    //   3. N4.key   = "typo" copy-constructed (5 chars, SSO).
    //   4. N4.value = ""     default-constructed (0 chars, SSO).
    //   5. N4 wired into Red-Black tree. Rebalancing may rotate nodes.
    //   6. _M_node_count at base+88 incremented: 3 → 4.
    //   7. Reference to N4.value returned.
    //   8. `val` (std::string on main's stack) copy-constructed from ref → "".
    //
    // N4 is NOW PERMANENTLY IN THE MAP even though you never intended to add it.
    // Use find() or getAttribute() when you only want to read.
    std::cout << "=== WARNING: operator[] inserts a blank entry if key is absent ===\n";
    std::cout << "attrs.size() before operator[] : " << div.attrs.size() << "\n";
    std::string val = div.attrs["typo"];  // SILENT INSERTION + malloc (heap node N4)
    std::cout << "attrs.size() after  operator[] : " << div.attrs.size() << "\n";
    std::cout << "value read : \"" << val << "\"\n";
    std::cout << "(a blank key \"typo\" now lives in the map)\n";
    std::cout << "\n";


    // ── PRINT ALL ──────────────────────────────────────────────────────────
    // In-order traversal of Red-Black tree → keys in alphabetical order.
    // Expected: "class", "data-index", "hidden", "typo"
    //           (std::less<std::string> orders by lexicographic byte comparison)
    std::cout << "=== all attributes (including the accidental one) ===\n";
    div.printAllAttributes();
    std::cout << "\n";


    // ── OVERWRITE EXISTING ATTRIBUTE ───────────────────────────────────────
    // setAttribute("class", "box"):
    //   1. Tree search: "class" FOUND (node N1). No malloc.
    //   2. operator[] returns reference to N1.value.
    //   3. N1.value.operator=("box"):
    //      Old value "box highlight" (13 chars, SSO) → SSO buffer overwritten.
    //      New value "box"           ( 3 chars, SSO) → fits inline, no heap alloc.
    //      _M_length updated from 13 → 3.
    //   Node N1's value std::string now holds "box".
    std::cout << "=== overwrite an attribute ===\n";
    div.setAttribute("class", "box");   // replace "box highlight" → "box"
    std::cout << "class after overwrite: \"" << div.getAttribute("class") << "\"\n";
    std::cout << "\n";


    // ── SCOPE END — AUTOMATIC DESTRUCTION ─────────────────────────────────
    // `div` reaches end of scope here. The compiler inserts a hidden call to
    // ~BlinkElement() BEFORE the `ret` instruction.
    //
    // Destruction order (reverse of construction):
    //
    //   1. ~attrs (base+64):
    //      Traverses all 4 heap nodes (N1, N2, N3, N4).
    //      For each node:
    //        ~string(node.value) → SSO: no free()
    //        ~string(node.key)   → SSO: no free()
    //        free(node_ptr)      → returns tree node memory to allocator
    //      _M_node_count → 0.
    //
    //   2. ~id (base+32):
    //      "container" = 9 chars → SSO: _M_p == &base[48] → no heap free().
    //
    //   3. ~tag_name (base+0):
    //      "div" = 3 chars → SSO: _M_p == &base[16] → no heap free().
    //
    //   After destructor body: 112 stack bytes reclaimed (rsp adjusts on return).
    std::cout << "=== end of main ===\n";
    return 0;
    // ↑ ~BlinkElement() fires HERE, before CPU executes `ret`
}

// =============================================================================
// MEMORY MAP — BlinkElement (112-byte object block)
//
// ┌────────┬──────┬────────────────┬───────────────────────────────────────────┐
// │ Offset │ Size │ Field / Sub    │ Contents ("div", "container" example)     │
// ├────────┼──────┼────────────────┼───────────────────────────────────────────┤
// │   0    │  8 B │ tag_name._M_p  │ → base+16 (SSO: points into own block)   │
// │   8    │  8 B │ tag_name.size  │ 3                                         │
// │  16    │ 15 B │ tag_name.buf   │ "div\0" + 12 zero bytes                   │
// │  31    │  1 B │ null term      │ 0x00                                      │
// ├────────┼──────┼────────────────┼───────────────────────────────────────────┤
// │  32    │  8 B │ id._M_p        │ → base+48 (SSO: points into own block)   │
// │  40    │  8 B │ id.size        │ 9                                         │
// │  48    │ 15 B │ id.buf         │ "container\0" + 6 zero bytes              │
// │  63    │  1 B │ null term      │ 0x00                                      │
// ├────────┼──────┼────────────────┼───────────────────────────────────────────┤
// │  64    │  8 B │ attrs.header   │ pointer to end-sentinel node (heap)       │
// │  72    │  8 B │ attrs.begin    │ pointer to leftmost node (heap)           │
// │  80    │  8 B │ attrs.end      │ pointer to header/sentinel (heap)         │
// │  88    │  8 B │ attrs.count    │ 3 (after 3 inserts), 4 after operator[]   │
// │  96    │  8 B │ attrs.compare  │ std::less<std::string> (stateless)        │
// │ 104    │  8 B │ padding        │ 0x00...                                   │
// └────────┴──────┴────────────────┴───────────────────────────────────────────┘
//
// HEAP (separate from object block, allocated by std::map on each insert):
//
//   Node N1 "class" → "box":
//     { rb_color | parent* | left* | right*
//       | std::string "class" (32B, SSO)
//       | std::string "box"   (32B, SSO) }
//
//   Node N2 "data-index" → "3":      (similar structure)
//   Node N3 "hidden"     → "true":   (similar structure)
//   Node N4 "typo"       → "":       (inserted silently by operator[])
//
// POINTER CHAIN (full path from object to character data):
//   &div (stack addr)
//     → base+64 (map manager in object block)
//       → _M_header._M_left (pointer to N1 on heap)
//         → N1.key._M_p (pointer into N1's SSO buffer)
//           → char[] "class\0"
// =============================================================================
