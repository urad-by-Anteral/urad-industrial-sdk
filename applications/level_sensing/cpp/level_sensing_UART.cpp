// uRAD High Accuracy Level Sensing — portable desktop reference client.
//
// Configures the radar over the control UART, streams frames from the data
// UART and prints the three fixed-point range measurements. Works on
// Windows and Linux with no external dependencies.
//
// Build:
//   g++ -std=c++17 -O2 -o level_sensing level_sensing_UART.cpp     (Linux)
//   cl /std:c++17 /O2 /EHsc level_sensing_UART.cpp                 (Windows)
//
// Usage:
//   level_sensing <control-port> <data-port> [--model AWR|IWR]
//                 [--baud N] [--frames N]
//
//   level_sensing COM5 COM4 --frames 10
//   level_sensing /dev/ttyUSB0 /dev/ttyUSB1 --model IWR
//
// --baud must match the data UART rate of the flashed firmware variant:
// 921600 for the standard *_921600_br.bin (default), 115200 or 9600 for
// the slower variants. The control UART always runs at 115200.

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

// Default product model for this repository (start frequency of the chirp):
// "IWR" = uRAD Industrial (60 GHz), "AWR" = uRAD Automotive (77 GHz).
constexpr const char *kDefaultModel = "IWR";

constexpr int kControlBaud = 115200;
constexpr int kDefaultDataBaud = 921600;
constexpr std::size_t kHeaderLen = 36;  // sync word (8) + 7 uint32 fields
constexpr std::array<std::uint8_t, 8> kMagicWords = {0x02, 0x01, 0x04, 0x03,
                                                     0x06, 0x05, 0x08, 0x07};

std::uint32_t u32(const std::uint8_t *p) {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) |
         (static_cast<std::uint32_t>(p[3]) << 24);
}

std::uint16_t u16(const std::uint8_t *p) {
  return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

// Minimal cross-platform serial port (blocking reads with timeout).
class SerialPort {
 public:
  bool open(const std::string &name, int baudrate) {
#if defined(_WIN32)
    handle_ = CreateFileA(("\\\\.\\" + name).c_str(), GENERIC_READ | GENERIC_WRITE,
                          0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) return false;
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(handle_, &dcb);
    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    if (!SetCommState(handle_, &dcb)) return false;
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 300;  // ms
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    SetCommTimeouts(handle_, &timeouts);
    return true;
#else
    fd_ = ::open(name.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0) return false;
    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) return false;
    cfmakeraw(&tty);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 3;  // 0.3 s read timeout
    speed_t speed;
    switch (baudrate) {
      case 9600: speed = B9600; break;
      case 115200: speed = B115200; break;
      case 921600: speed = B921600; break;
      default:
        std::cerr << "Unsupported baud rate: " << baudrate << "\n";
        return false;
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    return tcsetattr(fd_, TCSANOW, &tty) == 0;
#endif
  }

  int read(std::uint8_t *buffer, std::size_t size) {
#if defined(_WIN32)
    DWORD count = 0;
    if (!ReadFile(handle_, buffer, static_cast<DWORD>(size), &count, nullptr))
      return -1;
    return static_cast<int>(count);
#else
    return static_cast<int>(::read(fd_, buffer, size));
#endif
  }

  bool write(const std::string &data) {
#if defined(_WIN32)
    DWORD written = 0;
    return WriteFile(handle_, data.data(), static_cast<DWORD>(data.size()),
                     &written, nullptr) &&
           written == data.size();
#else
    return ::write(fd_, data.data(), data.size()) ==
           static_cast<ssize_t>(data.size());
#endif
  }

  void close() {
#if defined(_WIN32)
    if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
#else
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
#endif
  }

  ~SerialPort() { close(); }

 private:
#if defined(_WIN32)
  HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
  int fd_ = -1;
#endif
};

std::vector<std::string> build_config(const std::string &model) {
  const char *start_freq = (model == "AWR") ? "77" : "60";
  const char *channel_cfg = (model == "AWR") ? "4 1 0" : "1 1 0";
  return {
      "flushCfg",
      "dfeDataOutputMode 1",
      std::string("channelCfg ") + channel_cfg,
      "adcCfg 2 1",
      "adcbufCfg 0 1 1 1",
      std::string("profileCfg 0 ") + start_freq + " 7 7 114.4 0 0 31.23 1 512 5000 0 0 48",
      "chirpCfg 0 0 0 0 0 0 0 1",
      "frameCfg 0 0 10 0 500 1 0",
      "lowPower 0 0",
      "guiMonitor 1 0 0 0 0 1",
      "RangeLimitCfg 2 1 0.1 12.0",
      "sensorStart",
  };
}

bool send_command(SerialPort &port, const std::string &command) {
  if (!port.write(command + "\n")) return false;
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  std::uint8_t response[256];
  int count = port.read(response, sizeof(response));
  if (count > 0) {
    std::cout.write(reinterpret_cast<const char *>(response), count);
  }
  return true;
}

bool read_exact(SerialPort &port, std::uint8_t *buffer, std::size_t size) {
  std::size_t total = 0;
  int empty_reads = 0;
  while (total < size) {
    int count = port.read(buffer + total, size - total);
    if (count < 0) return false;
    if (count == 0) {
      if (++empty_reads >= 100) return false;  // ~30 s of silence
      continue;
    }
    empty_reads = 0;
    total += static_cast<std::size_t>(count);
  }
  return true;
}

// Block until the 8-byte sync word is found in the stream.
bool sync_to_magic(SerialPort &port) {
  std::size_t matched = 0;
  std::uint8_t byte;
  int empty_reads = 0;
  while (matched < kMagicWords.size()) {
    int count = port.read(&byte, 1);
    if (count < 0) return false;
    if (count == 0) {
      if (++empty_reads >= 100) return false;
      continue;
    }
    empty_reads = 0;
    matched = (byte == kMagicWords[matched]) ? matched + 1
              : (byte == kMagicWords[0])     ? 1
                                             : 0;
  }
  return true;
}

// Decode the three Q20 fixed-point ranges from a type-1 TLV body.
// Layout after the 4-byte descriptor: r1_low, r3_low, r2_low (uint16),
// then r1, r2, r3 (int16 high halves). All low halves are unsigned.
void print_ranges(const std::uint8_t *body) {
  const std::uint16_t r1_low = u16(body + 4);
  const std::uint16_t r3_low = u16(body + 6);
  const std::uint16_t r2_low = u16(body + 8);
  const auto r1 = static_cast<std::int16_t>(u16(body + 10));
  const auto r2 = static_cast<std::int16_t>(u16(body + 12));
  const auto r3 = static_cast<std::int16_t>(u16(body + 14));

  const double scale = 1048576.0;  // 2^20
  std::printf("%.4f %.4f %.4f\n", (r1 * 65536.0 + r1_low) / scale,
              (r2 * 65536.0 + r2_low) / scale, (r3 * 65536.0 + r3_low) / scale);
}

}  // namespace

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <control-port> <data-port> [--model AWR|IWR] [--baud N]"
                 " [--frames N]\n";
    return 1;
  }
  const std::string control_name = argv[1];
  const std::string data_name = argv[2];
  std::string model = kDefaultModel;
  int data_baud = kDefaultDataBaud;
  long max_frames = 0;  // 0 = run until the stream times out

  for (int i = 3; i + 1 < argc; i += 2) {
    const std::string flag = argv[i];
    if (flag == "--model") {
      model = argv[i + 1];
    } else if (flag == "--baud") {
      data_baud = std::stoi(argv[i + 1]);
    } else if (flag == "--frames") {
      max_frames = std::stol(argv[i + 1]);
    } else {
      std::cerr << "Unknown option: " << flag << "\n";
      return 1;
    }
  }
  if (model != "AWR" && model != "IWR") {
    std::cerr << "--model must be AWR (Automotive) or IWR (Industrial)\n";
    return 1;
  }

  {
    SerialPort control;
    if (!control.open(control_name, kControlBaud)) {
      std::cerr << "Could not open control port " << control_name << "\n";
      return 1;
    }
    for (const auto &command : build_config(model)) {
      if (!send_command(control, command)) {
        std::cerr << "Could not send: " << command << "\n";
        return 1;
      }
    }
  }  // release the control UART before streaming

  SerialPort data;
  if (!data.open(data_name, data_baud)) {
    std::cerr << "Could not open data port " << data_name << "\n";
    return 1;
  }

  std::vector<std::uint8_t> payload;
  long frames = 0;
  while (max_frames == 0 || frames < max_frames) {
    if (!sync_to_magic(data)) {
      std::cerr << "Data stream timed out; is the sensor running?\n";
      return 1;
    }
    std::uint8_t header[kHeaderLen - kMagicWords.size()];
    if (!read_exact(data, header, sizeof(header))) {
      std::cerr << "Timed out reading a frame header\n";
      return 1;
    }
    const std::uint32_t total_len = u32(header + 4);
    const std::uint32_t num_tlvs = u32(header + 24);
    if (total_len < kHeaderLen || total_len > 4096) continue;  // corrupt

    payload.resize(total_len - kHeaderLen);
    if (!read_exact(data, payload.data(), payload.size())) {
      std::cerr << "Timed out reading a frame payload\n";
      return 1;
    }

    std::size_t offset = 0;
    for (std::uint32_t t = 0; t < num_tlvs; t++) {
      if (offset + 8 > payload.size()) break;
      const std::uint32_t tlv_type = u32(payload.data() + offset);
      const std::uint32_t tlv_length = u32(payload.data() + offset + 4);
      if (tlv_type > 20 || tlv_length > 10000) break;
      if (offset + 8 + tlv_length > payload.size()) break;
      if (tlv_type == 1 && tlv_length >= 16) {  // descriptor (4) + ranges (12)
        print_ranges(payload.data() + offset + 8);
        frames++;
      }
      offset += 8 + tlv_length;
    }
  }

  SerialPort control;
  if (control.open(control_name, kControlBaud)) {
    control.write("sensorStop\n");
  }
  return 0;
}
