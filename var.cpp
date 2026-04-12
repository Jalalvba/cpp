#include <iostream>
#include <string>

void by_value(std::string tag) {
    int local = 0;
    std::cout << "by_value's stack frame area:     &local = " << &local << std::endl;
    std::cout << "  tag lives at:                  &tag   = " << &tag << std::endl;
}

void by_reference(const std::string& tag) {
    int local = 0;
    std::cout << "by_reference's stack frame area: &local = " << &local << std::endl;
    std::cout << "  tag points back to:            &tag   = " << &tag << std::endl;
}

void by_pointer(const std::string* tag) {
    int local = 0;
    std::cout << "by_pointer's stack frame area:   &local = " << &local << std::endl;
    std::cout << "  pointer itself lives at:       &tag   = " << &tag << std::endl;
    std::cout << "  pointer holds address:          tag   = " << tag << std::endl;
}

int main() {
    int local = 0;
    std::string original = "div";

    std::cout << "main's stack frame area:         &local    = " << &local << std::endl;
    std::cout << "  original lives at:             &original = " << &original << std::endl;
    std::cout << std::endl;

    by_value(original);
    std::cout << std::endl;

    by_reference(original);
    std::cout << std::endl;

    by_pointer(&original);

    return 0;
}
