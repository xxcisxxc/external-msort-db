#pragma once

#include <spdlog/spdlog.h>

#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include <thread>

class Timer {
private:
  std::chrono::time_point<std::chrono::high_resolution_clock>
      start_time; //!< start time
  std::chrono::time_point<std::chrono::high_resolution_clock>
      end_time; //!< end time

public:
  /**
   * @brief Construct a new TimerCPU object
   *
   */
  Timer() {}
  /**
   * @brief Start the timer
   *
   */
  void start(void * = nullptr) {
    start_time = std::chrono::high_resolution_clock::now();
  }
  /**
   * @brief Stop the timer
   *
   */
  void stop(void * = nullptr) {
    end_time = std::chrono::high_resolution_clock::now();
  }
  /**
   * @brief Get the duration in ms
   *
   * @return double duration in ms
   */
  double get_duration_ms() {
    return std::chrono::duration_cast<std::chrono::microseconds>(end_time -
                                                                 start_time)
               .count() /
           1000.0;
  }
  /**
   * @brief Destroy the TimerCPU object
   *
   */
  ~Timer() = default;
};

static inline std::filesystem::path kDir("data");

class Device {
private:
  double const _latency;       // in milliseconds
  double const _bandwidth;     // in bytes per millisecond
  std::size_t const _capacity; // capacity in bytes
  std::size_t _used;           // used capacity in bytes

  std::fstream _file;
  Timer _timer; // for timing

  std::size_t _base;

protected:
  /**
   * @brief Read / Write Latency
   *
   * @param bytes
   * @return double in milliseconds
   */
  inline double reach_time(std::size_t const bytes) const {
    return _latency + bytes / _bandwidth;
  }

public:
  const std::string name;

  /**
   * @brief Construct a new Device object
   *
   * @param latency milliseconds
   * @param bandwidth MB/s
   * @param capacity MB
   */
  Device(std::string name_, double const latency, double const bandwidth,
         std::size_t const capacity)
      : _latency(latency), _bandwidth(bandwidth * 1e-3 * 1024 * 1024),
        _capacity(capacity == ULONG_MAX ? ULONG_MAX : capacity * 1024 * 1024),
        _used(0), _base(0), name(name_) {
    if (std::filesystem::is_directory(kDir) == false) {
      std::filesystem::create_directory(kDir);
    }
    // Asynchronous I/O should relies on C++ async & future
    _file.open(kDir / name, std::ios::in | std::ios::out | std::ios::binary |
                                std::ios::trunc);
    if (!_file.is_open()) {
      throw std::runtime_error("Failed to open file");
    }
  }

  /**
   * @brief Read Data
   *
   * @param buffer pointer to read buffer
   * @param bytes number of bytes to read
   * @param offset start offset in bytes
   * @return ::ssize_t
   */
  ::ssize_t eread(char *buffer, std::size_t const bytes,
                  std::size_t const offset) {
    if (offset + bytes + _base > _used) {
      return -1;
    }
    spdlog::info("STATE -> READ_RUN_PAGES_{0}: Read sorted run pages from the "
                 "{0} device",
                 name);

    _timer.start();
    _file.seekg(offset + _base);
    _file.read(buffer, bytes);
    _timer.stop();
    if (_file.fail() || _file.bad()) {
      return -1;
    }

    double const reached = reach_time(bytes);
    if (_timer.get_duration_ms() < reached) {
      double const sleep_time = reached - _timer.get_duration_ms();
      std::size_t const sleep_time_us =
          static_cast<std::size_t>(sleep_time * 1000);
      std::this_thread::sleep_for(std::chrono::microseconds(sleep_time_us));
    }

    spdlog::info(
        "ACCESS -> A read to {} was made with size {} bytes and latency "
        "{} us",
        name, bytes, static_cast<std::size_t>(reached * 1000));
    return _file.gcount();
  }

  /**
   * @brief Write Data
   *
   * @param buffer pointer to write buffer
   * @param bytes number of bytes to write
   * @param offset start offset in bytes
   * @return ::ssize_t
   */
  ::ssize_t ewrite(char const *buffer, std::size_t const bytes,
                   std::size_t const offset) {
    if (bytes + offset + _base > _capacity) {
      return -1;
    }
    spdlog::info("STATE -> SPILL_RUNS_{0}: Spill sorted runs to the {0} device",
                 name);

    _timer.start();
    _file.clear();
    _file.seekp(offset + _base);
    _file.write(buffer, bytes);
    _timer.stop();
    if (_file.fail() || _file.bad()) {
      return -1;
    }

    std::size_t count = _file.tellp();
    if (_used < count) {
      _used = count;
    }

    count -= (offset + _base);

    _file.flush();

    double const reached = reach_time(bytes);
    if (_timer.get_duration_ms() < reached) {
      double const sleep_time = reached - _timer.get_duration_ms();
      std::size_t const sleep_time_us =
          static_cast<std::size_t>(sleep_time * 1000);
      std::this_thread::sleep_for(std::chrono::microseconds(sleep_time_us));
    }

    spdlog::info(
        "ACCESS -> A write to {} was made with size {} bytes and latency "
        "{} us",
        name, bytes, static_cast<std::size_t>(reached * 1000));
    return count;
  }

  ::ssize_t eappend(char const *buffer, std::size_t const bytes) {
    return ewrite(buffer, bytes, _used - _base);
  }

  inline void clear() { _used = 0; }

  std::size_t get_pos() const { return _used; }

  void eseek(std::size_t const offset) { _used = offset; }

  void set_base(std::size_t const base) { _base = base; }

  std::size_t get_base() const { return _base; }

  /**
   * @brief Destroy the Device object
   *
   */
  ~Device() { _file.close(); }

  std::future<::ssize_t> async_eread(char *buffer, std::size_t const bytes,
                                     std::size_t const offset) {
    return std::async(std::launch::async, &Device::eread, this, buffer, bytes,
                      offset);
  }

  std::future<::ssize_t> async_ewrite(char const *buffer,
                                      std::size_t const bytes,
                                      std::size_t const offset) {
    return std::async(std::launch::async, &Device::ewrite, this, buffer, bytes,
                      offset);
  }
};

class ReadDevice {
private:
  std::fstream _file;

public:
  ReadDevice(std::string name) {
    if (std::filesystem::is_directory(kDir) == false) {
      std::filesystem::create_directory(kDir);
    }
    _file.open(kDir / name, std::ios::in | std::ios::binary);
    if (!_file.is_open()) {
      throw std::runtime_error("Failed to open file");
    }
  }

  ::ssize_t read_only(char *buffer, std::size_t const bytes) {
    _file.read(buffer, bytes);
    if (_file.fail() || _file.bad() || _file.eof()) {
      return -1;
    }
    return _file.gcount();
  }

  ~ReadDevice() { _file.close(); }
};

class WriteDevice {
private:
  std::fstream _file;

public:
  WriteDevice(std::string name) {
    if (std::filesystem::is_directory(kDir) == false) {
      std::filesystem::create_directory(kDir);
    }
    _file.open(kDir / name, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!_file.is_open()) {
      throw std::runtime_error("Failed to open file");
    }
  }

  ::ssize_t append_only(char const *buffer, std::size_t const bytes) {
    _file.write(buffer, bytes);
    if (_file.fail() || _file.bad()) {
      return -1;
    }
    _file.flush();
    return bytes;
  }

  ~WriteDevice() { _file.close(); }
};
