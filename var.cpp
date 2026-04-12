#include <iostream>
#include <string>

void by_value(std::string tag) {
    std::cout << "VALUE:     &tag = " << &tag
              << "  (separate object, " << sizeof(tag) << " bytes)" << std::endl;
}

void by_reference(const std::string& tag) {
    std::cout << "REFERENCE: &tag = " << &tag
              << "  (same address as caller's object)" << std::endl;
}

void by_pointer(const std::string* tag) {
    std::cout << "POINTER:   tag  = " << tag << "  (address stored in pointer)"
              << "\n           &tag = " << &tag << "  (pointer itself lives here, "
              << sizeof(tag) << " bytes)" << std::endl;
}

int main() {
    std::string original = "div";
    std::cout << "ORIGINAL:  &original = " << &original << std::endl;
    std::cout << std::endl;

    by_value(original);
    by_reference(original);
    by_pointer(&original);

    return 0;
}