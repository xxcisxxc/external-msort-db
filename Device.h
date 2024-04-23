#pragma once

#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
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

class Device {
private:
  double const _latency;       // in milliseconds
  double const _bandwidth;     // in bytes per millisecond
  std::size_t const _capacity; // capacity in bytes
  std::size_t _used;           // used capacity in bytes

  std::fstream _file;
  Timer _timer; // for timing

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
  /**
   * @brief Construct a new Device object
   *
   * @param latency milliseconds
   * @param bandwidth MB/s
   * @param capacity MB
   */
  Device(std::string name, double const latency, double const bandwidth,
         std::size_t const capacity)
      : _latency(latency), _bandwidth(bandwidth * 1e-3 * 1024 * 1024),
        _capacity(capacity == ULONG_MAX ? ULONG_MAX : capacity * 1024 * 1024),
        _used(0) {
    // Asynchronous I/O should relies on C++ async & future
    _file.open(name, std::ios::in | std::ios::out | std::ios::binary |
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
    if (offset + bytes > _used) {
      return -1;
    }

    _timer.start();
    _file.seekg(offset);
    _file.read(buffer, bytes);
    _timer.stop();
    if (_file.fail() || _file.bad()) {
      return -1;
    }
    if (_timer.get_duration_ms() < reach_time(bytes)) {
      double const sleep_time = reach_time(bytes) - _timer.get_duration_ms();
      std::size_t const sleep_time_us =
          static_cast<std::size_t>(sleep_time * 1000);
      std::this_thread::sleep_for(std::chrono::microseconds(sleep_time_us));
    }
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
    if (bytes + offset > _capacity) {
      return -1;
    }

    _timer.start();
    _file.clear();
    _file.seekp(offset);
    _file.write(buffer, bytes);
    _timer.stop();
    if (_file.fail() || _file.bad()) {
      return -1;
    }

    std::size_t count = _file.tellp();
    if (_used < count) {
      _used = count;
    }

    count -= offset;

    // _file.seekg(0, std::ios::end);
    // _used = _file.tellg();
    _file.flush();

    if (_timer.get_duration_ms() < reach_time(bytes)) {
      double const sleep_time = reach_time(bytes) - _timer.get_duration_ms();
      std::size_t const sleep_time_us =
          static_cast<std::size_t>(sleep_time * 1000);
      std::this_thread::sleep_for(std::chrono::microseconds(sleep_time_us));
    }

    return count;
  }

  ::ssize_t eappend(char const *buffer, std::size_t const bytes) {
    return ewrite(buffer, bytes, _used);
  }

  inline void clear() { _used = 0; }

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

  std::size_t get_pos() const { return _used; }
  void eseek(std::size_t const offset) { _used = offset; }
};

class ReadDevice {
private:
  std::fstream _file;

public:
  ReadDevice(std::string name) {
    _file.open(name, std::ios::in | std::ios::binary);
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
