// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <iostream>
#include <stdexcept>

int main() {
  TimedDoor door(50);
  door.lock();
  std::cout << "Door locked, timeout=" << door.getTimeOut() << "ms\n";
  try {
    door.unlock();
    std::cout << "Door closed before timeout (no exception)\n";
  } catch (const std::runtime_error& e) {
    std::cout << "Exception caught: " << e.what() << '\n';
  }
  return 0;
}
