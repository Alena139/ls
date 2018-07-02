#ifndef CHECKER_H
#define CHECKER_H
#include "spdlog/spdlog.h"
#include "definitions.h"
#include "crc.h"
//класс для потока проверки Crc
class CheckThread {
  std::list<std::shared_ptr<Data>>::iterator last_checked_;
  uint64_t cur_block_;
  QueueData& queue_data_;
  std::map<uint32_t, CrcCtx>& checked_data_;
  uint32_t control_num_;
  bool& finished_;
  std::shared_ptr<spdlog::logger> log_; //потокобезопасный логгер

  uint32_t Crc(const Data& data) {
    return crc32(0, &data[0], data.size());
  }

  void Check() {
    auto &entry = checked_data_[cur_block_];
    if (!entry.failed) { //уже не сошлось, можно не считать
      auto crc = Crc(**last_checked_);
      uint32_t stored = 0;
      if (!entry.crc.compare_exchange_strong(stored, crc)) {
        if (stored != crc) {
          auto tmp = false;
          if (!entry.failed.compare_exchange_strong(tmp, true)) {//CAS
            log_->critical("CRC mismatch in block {}: expected {}, got {}", cur_block_, crc, stored);
          }
        }
      }
    }
  }

  void Account() {
    auto &entry = checked_data_[cur_block_];
    auto cur_count = entry.check_count.fetch_add(1);
    if (cur_count == control_num_) {
      std::lock_guard<std::mutex> lg(queue_data_.mtx);
      last_checked_ = queue_data_.q.erase(last_checked_); //все потоки посчитали, удалить из очереди
    } else {
      ++last_checked_;
    }
  }

  void Wait() {
    auto count = 0;
    while (last_checked_ == queue_data_.q.end() && count < 2) { //генерация не закончена, но очередь исчерпана
      //sleep(std::rand()%20);
      std::this_thread::yield();
      last_checked_ = queue_data_.q.begin();
      if (finished_) { //синхронизация без мьютекса
        ++count;
      }
    }
  }

  public:
  void Run() {
    last_checked_ = queue_data_.q.end();
    Wait();
    while (last_checked_ != queue_data_.q.end()) {
      Check();
      Account();
      Wait();
      cur_block_++;
    }
  }
  CheckThread(QueueData& data, std::map<uint32_t, CrcCtx>& checked_data, uint32_t control_num, bool& finished, std::shared_ptr<spdlog::logger> log)
    : cur_block_(0), queue_data_(data), checked_data_(checked_data), control_num_(control_num), finished_(finished), log_(log) {};
};

#endif //CHECKER_H
