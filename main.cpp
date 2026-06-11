#include "SBUS_Reader.hpp"

#include <iostream>

int main() {
  SBUS sbus("/dev/ttyAMA0");
  while (true) {
    const Frame frame = sbus.read_frame();
    // for (std::size_t i = 0; i < 4; i++) {
    //   std::cout << "ch[" << i << "]: " << frame.channels[i] << "\n";
    // }
  }
  return 0;
}
