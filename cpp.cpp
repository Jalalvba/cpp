#include <iostream>
#include <string>
#include <map>
#include <cstdint>

class BlinkElement {
public:
    std::string tag_name;
    std::string id;
    std::map<std::string, std::string> attrs;

    BlinkElement(const std::string& tag, const std::string& element_id)
        : tag_name(tag),
          id(element_id)
    {
        std::cout << "[constructor] <" << tag_name << " id=\"" << id << "\">\n";
    }

    ~BlinkElement() {
        std::cout << "[destructor]  <" << tag_name << " id=\"" << id << "\"> — map had "
                  << attrs.size() << " attribute(s)\n";
    }

    void setAttribute(const std::string& name, const std::string& value) {
        attrs[name] = value;
    }

    std::string getAttribute(const std::string& name) const {
        auto it = attrs.find(name);
        if (it == attrs.end()) return "";
        return it->second;
    }

    bool hasAttribute(const std::string& name) const {
        return attrs.find(name) != attrs.end();
    }

    void printAllAttributes() const {
        if (attrs.empty()) {
            std::cout << "  (no attributes)\n";
            return;
        }
        for (const auto& pair : attrs) {
            std::cout << "  " << pair.first << " = \"" << pair.second << "\"\n";
        }
    }
};

int main() {
    std::cout << "=== sizeof breakdown ===\n";
    std::cout << "sizeof(BlinkElement)                     : " << sizeof(BlinkElement) << " bytes\n";
    std::cout << "sizeof(std::string)                      : " << sizeof(std::string)  << " bytes\n";
    std::cout << "sizeof(std::map<std::string,std::string>): " << sizeof(std::map<std::string,std::string>) << " bytes\n";
    std::cout << "\n";

    std::cout << "=== create element ===\n";
    BlinkElement div("div", "container");

    uintptr_t base = reinterpret_cast<uintptr_t>(&div);

    std::cout << "address of div                           : " << &div << "\n";
    std::cout << "offset of tag_name                       : "
              << (reinterpret_cast<uintptr_t>(&div.tag_name) - base) << "\n";
    std::cout << "offset of id                             : "
              << (reinterpret_cast<uintptr_t>(&div.id)       - base) << "\n";
    std::cout << "offset of attrs                          : "
              << (reinterpret_cast<uintptr_t>(&div.attrs)    - base) << "\n";
    std::cout << "address of div.attrs                     : " << (void*)&div.attrs << "\n";
    std::cout << "attrs.size() before inserts              : " << div.attrs.size() << "\n";
    std::cout << "\n";

    std::cout << "=== setAttribute ===\n";
    div.setAttribute("class",      "box highlight");
    div.setAttribute("data-index", "3");
    div.setAttribute("hidden",     "true");
    std::cout << "attrs.size() after 3 inserts: " << div.attrs.size() << "\n";
    std::cout << "\n";

    std::cout << "=== getAttribute ===\n";
    std::cout << "getAttribute(\"class\")      : \"" << div.getAttribute("class")      << "\"\n";
    std::cout << "getAttribute(\"data-index\") : \"" << div.getAttribute("data-index") << "\"\n";
    std::cout << "getAttribute(\"missing\")    : \"" << div.getAttribute("missing")    << "\"\n";
    std::cout << "\n";

    std::cout << "=== hasAttribute ===\n";
    std::cout << "hasAttribute(\"hidden\")  : " << div.hasAttribute("hidden")  << "\n";
    std::cout << "hasAttribute(\"missing\") : " << div.hasAttribute("missing") << "\n";
    std::cout << "\n";

    std::cout << "=== WARNING: operator[] inserts a blank entry if key is absent ===\n";
    std::cout << "attrs.size() before operator[] : " << div.attrs.size() << "\n";
    std::string val = div.attrs["typo"];
    std::cout << "attrs.size() after  operator[] : " << div.attrs.size() << "\n";
    std::cout << "value read : \"" << val << "\"\n";
    std::cout << "(a blank key \"typo\" now lives in the map)\n";
    std::cout << "\n";

    std::cout << "=== all attributes (including the accidental one) ===\n";
    div.printAllAttributes();
    std::cout << "\n";

    std::cout << "=== overwrite an attribute ===\n";
    div.setAttribute("class", "box");
    std::cout << "class after overwrite: \"" << div.getAttribute("class") << "\"\n";
    std::cout << "\n";

    std::cout << "=== end of main ===\n";
    return 0;
}
