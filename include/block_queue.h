#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
//примитивный класс многопоточной очереди

template<class Data> class MtQueue {
  std::mutex mtx_;
  std::queue<Data> queue_;
  public:
  void push(const Data& in) {

  }

  bool try_pop(Data& out) {

  }

};




#endif BLOCK_QUEUE_H
