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
  struct termios2 opt = {};

  // if (ioctl(fd, TCGETS2, &opt) < 0) {
  //     throw std::runtime_error(std::format("Failed to configure serial port: {}",
  //                                        std::string{std::strerror(errno)}));
  // }

  auto print_opt = [](const termios2 &t, const char *label) {
    std::cout << "termios2 (" << label << ")\n"
              << "  c_iflag : 0x" << std::hex << t.c_iflag << std::dec << "\n"
              << "  c_oflag : 0x" << std::hex << t.c_oflag << std::dec << "\n"
              << "  c_cflag : 0x" << std::hex << t.c_cflag << std::dec << "\n"
              << "  c_lflag : 0x" << std::hex << t.c_lflag << std::dec << "\n"
              << "  c_line  : " << static_cast<int>(t.c_line) << "\n"
              << "  c_ispeed: " << t.c_ispeed << "\n"
              << "  c_ospeed: " << t.c_ospeed << "\n"
              << "  c_cc    : ";

    for (size_t i = 0; i < NCCS; ++i) {
      std::cout << static_cast<int>(t.c_cc[i]);
      if (i + 1 != NCCS) {
        std::cout << ", ";
      }
    }

    std::cout << '\n';
  };

  print_opt(opt, "before");


  opt.c_cflag = CS8 | CSTOPB | PARENB | CLOCAL | CREAD | BOTHER;
  opt.c_ospeed = 100000;
  opt.c_cc[VMIN] = 1;


  if (ioctl(fd, TCSETS2, &opt) < 0) {
    throw std::runtime_error(std::format("Failed to configure serial port: {}",
                                         std::string{std::strerror(errno)}));
  }

  print_opt(opt, "after");
}

SBUS::~SBUS() {
  if (fd >= 0) {
    close(fd);
  }
}

Frame SBUS::read_frame() {
  const auto start = std::chrono::steady_clock::now();
  const ssize_t n = read();
  const auto end = std::chrono::steady_clock::now();
  const auto elapsed_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  if (n > 0) {
    std::cout << "read (" << n << ") in " << elapsed_us << " us\n";
  }

  const Frame frame = decode();

  if (n > 0) {
    std::cout << "CH: ";
    for (int i = 0; i < 16; i++)
        std::cout << frame.channels[i] << " ";

    std::cout << "| Lost: " << frame.frame_lost << " Failsafe: " << frame.failsafe << std::endl;
  }

  return frame;
}

ssize_t SBUS::read() {
  frame = {};
  const ssize_t n = ::read(fd, frame.data(), frame.size());

  if (n == -1) {
    std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
    // return -1;
  }

  // if (n == 0)
  // {
  //   return 0;
  // }

  return n;
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
    // std::cout << "ch[" << i << "]: " << channels[i] << "\n";
    // f.channels[i] = channels[i] / 2048.;
    f.channels[i] = channels[i];
  }

  // boolean channels
  f.channels2[0] = frame[23] & 0x01;
  f.channels2[1] = frame[23] & 0x02;

  // lost frame and failsafe (multiple lost frames) indicators
  f.frame_lost = frame[23] & 0x04;
  f.failsafe = frame[23] & 0x08;

  return f;
}
