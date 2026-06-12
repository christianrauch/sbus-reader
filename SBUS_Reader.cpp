#include "SBUS_Reader.hpp"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/serial.h>

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

  // cfmakeraw
  // opt.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  // opt.c_oflag &= ~OPOST;
  // opt.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  // opt.c_cflag &= ~(CSIZE | PARENB);
  // opt.c_cflag |= CS8;

  // opt.c_cflag &= ~CRTSCTS;
  // opt.c_cflag |= BOTHER | CLOCAL | CREAD;
  // opt.c_iflag &= ~(IXON | IXOFF | IXANY);

  opt.c_cflag = CS8 | CSTOPB | PARENB | CLOCAL | CREAD | BOTHER;
  opt.c_ospeed = 100000;
  opt.c_cc[VMIN] = 1;
  // opt.c_cc[VMIN] = N;


  if (ioctl(fd, TCSETS2, &opt) < 0) {
    throw std::runtime_error(std::format("Failed to configure serial port: {}",
                                         std::string{std::strerror(errno)}));
  }

  print_opt(opt, "after");

  struct serial_struct ser_info;
  if (ioctl(fd, TIOCGSERIAL, &ser_info) < 0) {
      throw std::runtime_error(std::string{std::strerror(errno)});
      return;
  }

  ser_info.flags |= ASYNC_LOW_LATENCY;

  if (ioctl(fd, TIOCSSERIAL, &ser_info) < 0) {
      throw std::runtime_error(std::string{std::strerror(errno)});
  } else {
      std::cout << "Low latency mode enabled" << std::endl;
  }
}

SBUS::~SBUS() {
  if (fd >= 0) {
    close(fd);
  }
}

Frame SBUS::read_frame() {
  const auto start = std::chrono::steady_clock::now();
  const bool success = read();
  const auto end = std::chrono::steady_clock::now();
  const auto elapsed_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();
  if (success) {
    std::cout << "read frame in " << elapsed_us << " us\n";
  }

  const Frame frame = decode();

  if (success) {
    std::cout << "CH: ";
    for (int i = 0; i < 16; i++)
        std::cout << frame.channels[i] << " ";

    std::cout << "| Lost: " << frame.frame_lost << " Failsafe: " << frame.failsafe << std::endl;
  }

  return frame;
}

bool SBUS::read() {
  frame = {};

  std::cout << "READING new frame ..." << std::endl;

  bool found_header = false;

  ssize_t i = 0;

  while (true) {
    const ssize_t n = ::read(fd, frame.data()+i, frame.size()-i);

    std::cout << "read " << i << " + " << n << " bytes" << std::endl;

    if (n == -1) {
      std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
      return false;
    }

    if (n == 0)
    {
      continue;
    }

    if (!found_header)
    {
      const bool has_header = frame[0] == SBUS_HEADER;
      std::cout << "has_header: " << has_header << std::endl;
      found_header = !found_header && has_header;

      if (found_header) {
        std::cout << ">found header" << std::endl;
        // continue;
      }

      if (!found_header) {
        // continue searching for header
        continue;
      }
    }

    if ((i+n) == N)
    {
      // read full frame
      std::cout << "read max bytes" << std::endl;
      if (frame[N-1] == SBUS_END)
      {
        // correct end
        std::cout << "found end" << std::endl;
        break;
      }
      else
      {
        std::cerr << "Invalid S.BUS frame: incorrect end byte!" << std::endl;
        // return 0;
        // reset, repeat
        found_header = false;
        i = 0;
        continue;
      }
    }

    // if we are here, it means we have read some bytes but haven't found a valid frame yet
    i += n;
    std::cout << "search on " << i << "+" << std::endl;
  }

  // if (n == 0)
  // {
  //   return 0;
  // }

  if (frame[0] != SBUS_HEADER || frame[24] != SBUS_END)
  {
    std::cerr << "Invalid S.BUS frame!" << std::endl;
    throw std::runtime_error("Invalid S.BUS frame!");
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
