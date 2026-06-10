#include "SBUS_Reader.hpp"

#include <iostream>

int main() {
  SBUS sbus("/dev/ttyAMA0");
  while (true) {
    const Frame frame = sbus.read_frame();
  }
  return 0;
}
