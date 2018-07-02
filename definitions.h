#ifndef DEFINITIONS_H
#define DEFINITIONS_H
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

using Data = std::vector<uint8_t>;
struct QueueData{
  std::list<std::shared_ptr<Data>> q;
  std::mutex mtx;
};


struct CrcCtx{
  std::atomic<uint32_t> crc;
  std::atomic<uint64_t> check_count;
  std::atomic_bool failed;
};

static const int kMaxBacklog = 10000; //макимальное число ожидающих пакетов в очереди
#endif //DEFINITIONS_H
