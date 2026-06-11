#include "SBUS_Reader.hpp"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

SBUS::SBUS(const std::string &device) {
  if ((fd = open(device.c_str(), O_RDONLY | O_NOCTTY)) < 0) {
    throw std::runtime_error(
        std::format("{}: {}", std::string{std::strerror(errno)}, device));
  }

  // https://github.com/bolderflight/sbus/blob/main/README.md
  // - baud rate of 100000
  // - 8 data bits
  // - even parity
  // - 2 stop bits
  struct termios2 opt;

  if (ioctl(fd, TCGETS2, &opt) < 0) {
      perror("TCGETS2");
      exit(1);
  }

  opt.c_cflag = CS8 | CSTOPB | PARENB | CLOCAL | CREAD | BOTHER;
  opt.c_ospeed = 100000;


  if (ioctl(fd, TCSETS2, &opt) < 0) {
    throw std::runtime_error(std::format("Failed to configure serial port: {}",
                                         std::string{std::strerror(errno)}));
  }
}

SBUS::~SBUS() {
  if (fd >= 0) {
    close(fd);
  }
}

Frame SBUS::read_frame() {
  const auto start = std::chrono::steady_clock::now();
  read();
  const auto end = std::chrono::steady_clock::now();
  const auto elapsed_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  std::cout << "read() took " << elapsed_us << " us\n";

  return decode();
}

bool SBUS::read() {
  frame = {};
  const int n = ::read(fd, frame.data(), frame.size());

  if (n == -1) {
    std::cerr << "Error reading from serial port: " << strerror(errno)
              << std::endl;
    return false;
  }

  if (n == 0)
  {
    return false;
  }

  return true;
}

Frame SBUS::decode() {
  channels[0] = (uint16_t)((frame[1] | frame[2] << 8) & 0x07FF);
  channels[1] = (uint16_t)((frame[2] >> 3 | frame[3] << 5) & 0x07FF);
  channels[2] =
      (uint16_t)((frame[3] >> 6 | frame[4] << 2 | frame[5] << 10) & 0x07FF);
  channels[3] = (uint16_t)((frame[5] >> 1 | frame[6] << 7) & 0x07FF);
  channels[4] = (uint16_t)((frame[6] >> 4 | frame[7] << 4) & 0x07FF);
  channels[5] =
      (uint16_t)((frame[7] >> 7 | frame[8] << 1 | frame[9] << 9) & 0x07FF);
  channels[6] = (uint16_t)((frame[9] >> 2 | frame[10] << 6) & 0x07FF);
  channels[7] = (uint16_t)((frame[10] >> 5 | frame[11] << 3) & 0x07FF);
  channels[8] = (uint16_t)((frame[12] | frame[13] << 8) & 0x07FF);
  channels[9] = (uint16_t)((frame[13] >> 3 | frame[14] << 5) & 0x07FF);
  channels[10] =
      (uint16_t)((frame[14] >> 6 | frame[15] << 2 | frame[16] << 10) & 0x07FF);
  channels[11] = (uint16_t)((frame[16] >> 1 | frame[17] << 7) & 0x07FF);
  channels[12] = (uint16_t)((frame[17] >> 4 | frame[18] << 4) & 0x07FF);
  channels[13] =
      (uint16_t)((frame[18] >> 7 | frame[19] << 1 | frame[20] << 9) & 0x07FF);
  channels[14] = (uint16_t)((frame[20] >> 2 | frame[21] << 6) & 0x07FF);
  channels[15] = (uint16_t)((frame[21] >> 5 | frame[22] << 3) & 0x07FF);

  Frame f;

  for (size_t i = 0; i < max_channels; i++) {
    std::cout << "ch[" << i << "]: " << channels[i] << "\n";
    f.channels[i] = channels[i] / 2048.;
  }

  // boolean channels
  f.channels2[0] = frame[23] & 0x01;
  f.channels2[1] = frame[23] & 0x02;

  // lost frame and failsafe (multiple lost frames) indicators
  f.frame_lost = frame[23] & 0x04;
  f.failsafe = frame[23] & 0x08;

  return f;
}
