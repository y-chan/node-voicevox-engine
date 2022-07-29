#include <random>
#include <sstream>

#include "uuid_v4.h"

std::random_device rnd;

std::string uuid_v4() {
    std::string uuid_format = "xxxxxxxx-xxxx-4xxx-Zxxx-xxxxxxxxxxxx";
    std::mt19937 mt(rnd());
    std::stringstream ss;
    std::uniform_int_distribution<> rand_hex(0, 15);
    std::uniform_int_distribution<> digit_rand(0, 4);
    int value;
    for (int i = 0; i < uuid_format.size(); i++) {
        switch (uuid_format[i]) {
            case '4':
            case '-':
                ss << uuid_format[i];
                break;
            case 'Z':
                value = digit_rand(mt);
                ss << std::hex << (value + 8);
                break;
            default:
                value = rand_hex(mt);
                ss << std::hex << value;
                break;
        }
    }
    return ss.str();
}
