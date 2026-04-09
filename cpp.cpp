#include <iostream>
#include <string>
#include <cstddef>   // Required for offsetof

class BlinkElement {
public:
    // These are the "Houses" on our RAM street
    std::string tag_name;   // House 1 (32 bytes)
    std::string id;         // House 2 (32 bytes)
    std::string class_name; // House 3 (32 bytes) - NEW FIELD

    // Constructor: Takes 3 arguments now
    BlinkElement(const std::string& tag, const std::string& element_id, const std::string& cls)
        : tag_name(tag), id(element_id), class_name(cls)
    {
        std::cout << "[constructor] BlinkElement created: <" << tag_name 
                  << " id=\"" << id 
                  << "\" class=\"" << class_name << "\">\n";
    }

    // Destructor: Cleans up when the object dies
    ~BlinkElement() {
        std::cout << "[destructor]  BlinkElement destroyed: <" << tag_name << ">\n";
    }
};

int main() {
    std::cout << "=== sizeof and offsetof ===\n";
    // This will now show 96 bytes (32 * 3)
    std::cout << "sizeof(BlinkElement)      : " << sizeof(BlinkElement) << " bytes\n";
    std::cout << "offsetof tag_name         : " << offsetof(BlinkElement, tag_name) << "\n";
    std::cout << "offsetof id               : " << offsetof(BlinkElement, id) << "\n";
    std::cout << "offsetof class_name       : " << offsetof(BlinkElement, class_name) << "\n";
    std::cout << "\n";

    std::cout << "=== creating element on the stack ===\n";
    // Passing the 3rd argument "hero-btn"
    BlinkElement div("div", "container", "hero-btn");
    std::cout << "\n";

    std::cout << "=== RAM layout ===\n";
    std::cout << "address of div            : " << &div << "\n";
    std::cout << "address of div.tag_name   : " << (void*)&div.tag_name << "\n";
    std::cout << "address of div.id         : " << (void*)&div.id << "\n";
    std::cout << "address of div.class_name : " << (void*)&div.class_name << "\n";
    std::cout << "\n";

    // The "Two-Stage Argument" pattern: <Type Argument> + (Value Argument)
    uintptr_t base   = reinterpret_cast<uintptr_t>(&div);
    uintptr_t f_tag  = reinterpret_cast<uintptr_t>(&div.tag_name);
    uintptr_t f_id   = reinterpret_cast<uintptr_t>(&div.id);
    uintptr_t f_cls  = reinterpret_cast<uintptr_t>(&div.class_name);

    std::cout << "div.tag_name   is " << (f_tag - base) << " bytes from start\n";
    std::cout << "div.id         is " << (f_id  - base) << " bytes from start\n";
    std::cout << "div.class_name is " << (f_cls - base) << " bytes from start\n";
    std::cout << "\n";

    std::cout << "=== end of main — destructor fires next ===\n";
    return 0;
}