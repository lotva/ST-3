// Copyright 2021 GHA Test Team

#include "TimedDoor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

void CloseDoorIfOpen(Door& d) {
  if (d.isDoorOpened()) {
    d.lock();
  }
}

}  // namespace

class TimedDoorTest : public ::testing::Test {
 protected:
  void SetUp() override { door_ = std::make_unique<TimedDoor>(0); }

  void TearDown() override { door_.reset(); }

  std::unique_ptr<TimedDoor> door_;
};

class TimerTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  Timer timer_;
};

class DoorInteractionTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TimedDoorTest, timeout_value_from_constructor) {
  EXPECT_EQ(door_->getTimeOut(), 0);
  TimedDoor anotherDoor(100);
  EXPECT_EQ(anotherDoor.getTimeOut(), 100);
}

TEST_F(TimedDoorTest, door_initially_closed) {
  EXPECT_FALSE(door_->isDoorOpened());
}

TEST_F(TimedDoorTest, lock_maintains_closed_state) {
  door_->lock();
  EXPECT_FALSE(door_->isDoorOpened());
}

TEST_F(TimedDoorTest, unlock_with_zero_timeout_raises_exception) {
  EXPECT_THROW(door_->unlock(), std::runtime_error);
}

TEST_F(TimedDoorTest, throw_state_no_exception_when_closed) {
  door_->lock();
  EXPECT_NO_THROW(door_->throwState());
}

TEST_F(TimedDoorTest, consecutive_unlock_calls_both_throw) {
  TimedDoor testDoor(0);
  EXPECT_THROW(testDoor.unlock(), std::runtime_error);
  EXPECT_THROW(testDoor.unlock(), std::runtime_error);
}

TEST_F(TimedDoorTest, door_reports_open_during_timer_wait) {
  TimedDoor testDoor(200);
  testDoor.lock();
  std::thread worker([&testDoor]() {
    try {
      testDoor.unlock();
    } catch (const std::runtime_error&) {
    }
  });
  bool foundOpen = false;
  for (int i = 0; i < 500 && !foundOpen; ++i) {
    if (testDoor.isDoorOpened()) {
      foundOpen = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  EXPECT_TRUE(foundOpen);
  worker.join();
}

TEST_F(TimerTest, register_with_zero_delay_calls_timeout) {
  testing::NiceMock<MockTimerClient> mockClient;
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  timer_.tregister(0, &mockClient);
}

TEST_F(TimerTest, register_with_short_delay_calls_timeout) {
  testing::NiceMock<MockTimerClient> mockClient;
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  timer_.tregister(5, &mockClient);
}

TEST_F(DoorInteractionTest, close_if_open_invokes_lock_when_open) {
  testing::StrictMock<MockDoor> mockDoor;
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(testing::Return(true));
  EXPECT_CALL(mockDoor, lock()).Times(1);
  CloseDoorIfOpen(mockDoor);
}

TEST_F(DoorInteractionTest, close_if_open_skips_lock_when_closed) {
  testing::StrictMock<MockDoor> mockDoor;
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(testing::Return(false));
  EXPECT_CALL(mockDoor, lock()).Times(0);
  CloseDoorIfOpen(mockDoor);
}

TEST(TimedDoorConcurrency, locking_before_timeout_prevents_exception) {
  TimedDoor testDoor(150);
  testDoor.lock();
  std::exception_ptr capturedException;
  std::thread worker([&testDoor, &capturedException]() {
    try {
      testDoor.unlock();
    } catch (...) {
      capturedException = std::current_exception();
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  testDoor.lock();
  worker.join();
  EXPECT_EQ(capturedException, nullptr);
}

