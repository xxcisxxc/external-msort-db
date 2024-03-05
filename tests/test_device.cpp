#include "Device.h"
#include "catch2/catch_amalgamated.hpp"

TEST_CASE("Read & Write Device", "[device]") {
  Device d("./tests/test_device.bin", 1, 1, 1);
  char buffer[10] = "abcdefghi";
  ::ssize_t ret = d.ewrite(buffer, sizeof(buffer), 0);
  REQUIRE(ret == 10);
  char read_buffer[10];
  ret = d.eread(read_buffer, sizeof(read_buffer), 2);
  REQUIRE(ret == -1);
  ret = d.eread(read_buffer, sizeof(read_buffer) - 2, 2);
  REQUIRE(ret == 8);
  REQUIRE(read_buffer[0] == 'c');
  REQUIRE(read_buffer[1] == 'd');
  REQUIRE(read_buffer[2] == 'e');
  REQUIRE(read_buffer[3] == 'f');
  REQUIRE(read_buffer[4] == 'g');
  REQUIRE(read_buffer[5] == 'h');
  REQUIRE(read_buffer[6] == 'i');
  REQUIRE(read_buffer[7] == '\0');
}

TEST_CASE("Write out of scope", "[device]") {
  Device d("./tests/test_device.bin", 1, 1, 1);
  char *buffer = new char[1024 * 2048];
  for (int i = 0; i < 1024 * 2048; i++) {
    buffer[i] = 'a' + i % 26;
  }
  ::ssize_t ret = d.ewrite(buffer, 1024 * 2048, 0);
  REQUIRE(ret == -1);
  ret = d.ewrite(buffer, 1024 * 1024, 3);
  REQUIRE(ret == -1);
  ret = d.ewrite(buffer, 1024 * 1024, 0);
  REQUIRE(ret == 1024 * 1024);
  char read_buffer[10];
  ret = d.eread(read_buffer, sizeof(read_buffer), 2);
  REQUIRE(ret == 10);
  REQUIRE(read_buffer[0] == 'c');
  REQUIRE(read_buffer[1] == 'd');
  REQUIRE(read_buffer[2] == 'e');
  REQUIRE(read_buffer[3] == 'f');
  REQUIRE(read_buffer[4] == 'g');
  REQUIRE(read_buffer[5] == 'h');
  REQUIRE(read_buffer[6] == 'i');
  REQUIRE(read_buffer[7] == 'j');
  REQUIRE(read_buffer[8] == 'k');
  REQUIRE(read_buffer[9] == 'l');
  delete[] buffer;
}

TEST_CASE("Write Bandwidth & Latency", "[device]") {
  class TDevice : public Device {
  public:
    TDevice(std::string name, std::size_t const latency,
            std::size_t const bandwidth, std::size_t const capacity)
        : Device(name, latency, bandwidth, capacity) {}
    double reach_time(std::size_t const bytes) const {
      return Device::reach_time(bytes);
    }
  };
  TDevice d("./tests/test_device.bin", 1, 1, 1);
  char *buffer = new char[1024 * 1024];
  for (int i = 0; i < 1024 * 1024; i++) {
    buffer[i] = 'a' + i % 26;
  }
  Timer t;
  t.start();
  ::ssize_t ret = d.ewrite(buffer, 1024 * 1024, 0);
  t.stop();
  REQUIRE(ret == 1024 * 1024);
  REQUIRE(std::abs(d.reach_time(1024 * 1024) - t.get_duration_ms()) < 0.5);

  t.start();
  ret = d.eread(buffer, 1024 * 1024, 0);
  t.stop();
  REQUIRE(ret == 1024 * 1024);
  REQUIRE(std::abs(d.reach_time(1024 * 1024) - t.get_duration_ms()) < 0.5);
  delete[] buffer;
}
