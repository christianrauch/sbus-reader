#pragma once

#include <string>

class SBUS {
public:
    explicit SBUS(const std::string &device);
    std::string read() const;

// private:
//     std::string name_;
};
