#pragma once

#include <string>
#include <array>
#include <asm/termbits.h>
#include <cstdint>

struct Frame {
    // std::array<double, 16> channels;
    std::array<uint16_t, 16> channels;
    std::array<bool, 2> channels2;
    bool frame_lost;
    bool failsafe;
};

class SBUS {
public:
    explicit SBUS(const std::string &device);

    ~SBUS();

    Frame read_frame();

private:
    int fd = 0;

    // static constexpr struct termios2 options = []
    // {
    //     // https://github.com/bolderflight/sbus/blob/main/README.md
    //     // - baud rate of 100000
    //     // - 8 data bits
    //     // - even parity
    //     // - 2 stop bits
    //     struct termios2 opt;
    //     opt.c_cflag = CS8 | CSTOPB | PARENB | CLOCAL | CREAD | BOTHER;
    //     opt.c_ospeed = 100000;
    //     return opt;
    // }();

    static constexpr size_t N = 25;
    static constexpr uint8_t SBUS_HEADER = 0x0F;
    static constexpr uint8_t SBUS_END = 0x0;
    std::array<uint8_t, N> frame = {};
    static constexpr std::size_t max_channels = 16;
    std::array<uint16_t, max_channels> channels;  // 1 - 16: 11 bit

    ssize_t read();
    Frame decode();
};
