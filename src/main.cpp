// Compiles cleanly with GCC 8.2:
//   g++ -std=c++17 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 -pthread main.cpp
//
// To run with valgrind, build as:
//   g++ -std=c++17 -g -O0 -pthread main.cpp
// To diagnose threading problems:
//   valgrind --tool=helgrind ./a.out
// or to check for leaks:
//   valgrind --leak-check=yes ./a.out

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
//------------------------------------------------------------------------------
//TODO: move to log.h
std::mutex sLogMtx;
std::unordered_map<std::thread::id, size_t> sThrId2NumberMap;
std::chrono::time_point<std::chrono::system_clock> sStartTime;
#define LOG(x) do {                                      \
  std::scoped_lock _(sLogMtx);                           \
  const auto id = std::this_thread::get_id();            \
  if (sThrId2NumberMap.count(id) == 0)                   \
    sThrId2NumberMap[id] = sThrId2NumberMap.size() + 1U; \
  const std::chrono::duration<float> elapsed = std::chrono::system_clock::now() - sStartTime; \
  std::cout <<std::fixed<<std::setprecision(1)<< elapsed.count() \
            <<" ["<< sThrId2NumberMap[id] <<"] "<< x << std::endl; } while(false)
//------------------------------------------------------------------------------
struct Socket;
std::mutex sSocketPtrsMutex;
std::vector<std::weak_ptr<Socket>> sSocketPtrs(1, std::weak_ptr<Socket>{}); // 1st Socket is "invalid"

struct Socket : public std::enable_shared_from_this<Socket> {
  // Returns non-zero "handle" equal to index of this Socket in sSocketPtrs vector 
  static size_t Open() {
    std::shared_ptr<Socket> s;
    {
      static std::mutex    sSktCreationMutex;
      std::lock_guard lock(sSktCreationMutex);
      RegisterSkt(s = std::make_shared<Socket>());
    }
    const size_t handle = s->getHandle();
    s->run();
    return handle;
  }

  static void Close(size_t handle) {
    std::lock_guard lock(sSocketPtrsMutex);
    if (handle > 0 && handle < sSocketPtrs.size()) {
      auto pS = sSocketPtrs.at(handle);
      if (auto s = pS.lock()) {
        s->m_quit = true;
      } else {
        LOG("Socket "<< handle <<" has already been deleted");
      }
    } // else invalid handle
  }

  // These are conceptually private, as the above static methods are the only
  // way to create/delete sockets, but `enable_shared_from_this` requires public
  // constructor and destructor.
  Socket() : m_handle(NextHandle()) {
    LOG("Socket " << m_handle << " constructed");
  }
  ~Socket() {
    LOG("Socket " << m_handle << " destructed");
  }

private:
  static size_t NextHandle() {
    std::lock_guard lock(sSocketPtrsMutex);
    return sSocketPtrs.size();
  }

  static void RegisterSkt(const std::shared_ptr<Socket>& s) {
    std::lock_guard lock(sSocketPtrsMutex);
    sSocketPtrs.emplace_back(s);
  }

  size_t getHandle() const noexcept { return m_handle; }

  void run() {
    std::thread([self = shared_from_this()]{ self->read(); }).detach();
  }

  // Normally, `read()` exits when server closes the socket.
  void read() {
    const size_t seconds = 2;
    LOG("Reading for "<< seconds <<" seconds");
    for (size_t i = 0; i < seconds * 10U; ++i) {
      if (m_quit.load()) {
        LOG("Quitting read early");
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      LOG("Still reading...");
    }
  }

//data:
  const size_t m_handle;
  std::atomic<bool> m_quit{false};
}; // struct Socket
//------------------------------------------------------------------------------
int main() {
  sStartTime = std::chrono::system_clock::now();

  const auto h = Socket::Open();
  LOG("Opened Socket "<< h << ", waiting for 1 second");
  std::this_thread::sleep_for(std::chrono::seconds(1));

  LOG("Closing Socket " << h << ", waiting for 1 second");
  Socket::Close(h);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  LOG("Exiting");
}
