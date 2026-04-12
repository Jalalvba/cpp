#include <iostream>
#include <cstring>

struct BlinkElement {
    std::string tag_name;

    BlinkElement(const std::string& tag) : tag_name(tag) {
        std::cout << "=== INSIDE CONSTRUCTOR ===" << std::endl;
        std::cout << "Address of 'tag' reference itself: " << &tag << std::endl;
        std::cout << "Address of 'tag_name' member:      " << &tag_name << std::endl;
        std::cout << "Address of this (BlinkElement obj): " << this << std::endl;

        std::cout << "\n--- Are they the same object? ---" << std::endl;
        std::cout << "tag and tag_name same address? "
                  << (&tag == &tag_name ? "YES (same object)" : "NO (different objects, copy happened)")
                  << std::endl;

        // Show the internal buffer address (where the actual bytes sit)
        std::cout << "\n--- Where are the bytes 'd','i','v' physically? ---" << std::endl;
        std::cout << "tag.data()      = " << (void*)tag.data() << std::endl;
        std::cout << "tag_name.data() = " << (void*)tag_name.data() << std::endl;

        // SSO check: is the buffer inside the object itself?
        const char* tag_start = reinterpret_cast<const char*>(&tag);
        const char* tag_end = tag_start + sizeof(std::string);
        const char* tag_buf = tag.data();
        bool tag_uses_sso = (tag_buf >= tag_start && tag_buf < tag_end);

        const char* tn_start = reinterpret_cast<const char*>(&tag_name);
        const char* tn_end = tn_start + sizeof(std::string);
        const char* tn_buf = tag_name.data();
        bool tn_uses_sso = (tn_buf >= tn_start && tn_buf < tn_end);

        std::cout << "tag uses SSO (buffer inside object)?      " << (tag_uses_sso ? "YES" : "NO (heap)") << std::endl;
        std::cout << "tag_name uses SSO (buffer inside object)?  " << (tn_uses_sso ? "YES" : "NO (heap)") << std::endl;

        // Dump the actual bytes
        std::cout << "\n--- Raw bytes at tag_name.data() ---" << std::endl;
        for (size_t i = 0; i <= tag_name.size(); i++) {
            std::cout << "  byte[" << i << "] = 0x"
                      << std::hex << (int)(unsigned char)tag_name.data()[i]
                      << std::dec << " ('" << (tag_name.data()[i] ? tag_name.data()[i] : '0') << "')"
                      << std::endl;
        }
    }
};

int main() {
    std::cout << "=== STEP 0: Before anything ===" << std::endl;
    std::cout << "sizeof(std::string) = " << sizeof(std::string) << " bytes" << std::endl;

    std::cout << "\n=== STEP 1: String literal exists in .rodata ===" << std::endl;
    const char* literal = "div";
    std::cout << "Address of \"div\" literal: " << (void*)literal << std::endl;
    std::cout << "Bytes: ";
    for (int i = 0; i < 4; i++)
        std::cout << "0x" << std::hex << (int)(unsigned char)literal[i] << std::dec << " ";
    std::cout << std::endl;

    std::cout << "\n=== STEP 2: Temporary std::string constructed ===" << std::endl;
    // This is what the compiler does implicitly when you call BlinkElement("div")
    // It constructs a temporary std::string from the literal
    std::cout << "(The compiler builds a temporary std::string on the stack from the literal)" << std::endl;

    std::cout << "\n=== STEP 3: Constructor called ===" << std::endl;
    BlinkElement elem("div");

    std::cout << "\n=== STEP 4: After construction ===" << std::endl;
    std::cout << "elem lives at:          " << &elem << std::endl;
    std::cout << "elem.tag_name lives at: " << &elem.tag_name << std::endl;
    std::cout << "elem.tag_name value:    " << elem.tag_name << std::endl;
    std::cout << "(The temporary std::string is already destroyed — stack reclaimed)" << std::endl;

    return 0;
}