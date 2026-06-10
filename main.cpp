#include "SBUS_Reader.hpp"

#include <iostream>

int main() {
  SBUS sbus("CMake");
  std::cout << sbus.read() << '\n';
  return 0;
}
