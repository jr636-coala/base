#define BASE_TEST_REGEX
#include "test.hpp"

int main(int argc, char** argv) {
  for (auto i = 0; i < 100; ++i) {
    Base::basetest();
  }
  return 0;
}
