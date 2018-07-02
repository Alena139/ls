#include <atomic>
#include <condition_variable>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <cstdint>
#include <thread>
#include <vector>
#include "spdlog/spdlog.h"

#include "checker.h"
#include "definitions.h"

//генератор имён для логгирования
struct NameGen {
  static spdlog::filename_t calc_filename(const spdlog::filename_t& filename) {
    std::tm tm = spdlog::details::os::localtime();
    spdlog::filename_t basename, ext;
    std::tie(basename, ext) = spdlog::details::file_helper::split_by_extenstion(filename);
    std::conditional<std::is_same<spdlog::filename_t::value_type, char>::value, fmt::MemoryWriter, fmt::WMemoryWriter>::type w;
    w.write(SPDLOG_FILENAME_T("{}_{:04d}_{:02d}_{:02d}{}"), basename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, ext);
    return w.str();
  }
};
//настройки
struct {
  uint32_t gen_thread_count; //количество генерирующих блоки потоков
  uint64_t block_count; //количество блоков
  uint32_t block_size; //количество блоков
  uint32_t check_thread_count; //количество проверяющих потоков
} Settings;



std::atomic<uint64_t> blocks_generated; //общее число уже сгенерированных блоков
std::shared_ptr<spdlog::logger> combined_logger; //потокобезопасный логгер


//заполнить мусором вектор байт
inline void FillRandom(std::mt19937_64& gen, std::shared_ptr<Data> vec) {
  static auto qw_size = Settings.block_size / 8; //целое число QWORD
  static auto rem_size = Settings.block_size % 8; //остаток байт
  auto data_start = reinterpret_cast<uint64_t*>(&(*vec)[0]);
  for (unsigned i = 0; i < qw_size; ++i) { //забить мусором
    *data_start++ = gen();
  }
  auto last = gen(); //последние 8 байт, не полностью
  memcpy(data_start, &last, rem_size); //добить до размера
}

//поток создания блоков
void GenerateBlock(QueueData& queue_data, bool& finished) {
  std::hash<std::thread::id> h;
  std::mt19937_64 gen(h(std::this_thread::get_id())); //инициализация случайным числом
  while (blocks_generated.fetch_add(1) < Settings.block_count) {
    auto vec = std::make_shared<Data>(Settings.block_size);
    FillRandom(gen, vec); //заполнить мусором
    std::lock_guard<std::mutex> lg(queue_data.mtx);
    while (queue_data.q.size() >= kMaxBacklog) {
      std::this_thread::yield();
    }
    queue_data.q.push_back(vec); //в очередь
  }
  finished = true; //все пакеты созданы
}

int main(int argc, char** argv) {
  namespace spd = spdlog;
  std::vector<spd::sink_ptr> sinks = { std::make_shared<spd::sinks::stdout_sink_mt>(), std::make_shared<spd::sinks::daily_file_sink<std::mutex, NameGen>>("app.log", 23, 59) };
  combined_logger = std::make_shared<spd::logger>("spd", begin(sinks), end(sinks));
  combined_logger->set_pattern("[%d.%m.%Y %H:%M:%e] %v");
  combined_logger->flush_on(spd::level::info);
  QueueData data;
  std::map<uint32_t, CrcCtx> checked_data;
  bool finished = false; //создание завершено
  if (argc != 5) {
    combined_logger->info("{} usage: generate_thread_count check_thread_count block_count block_size", argv[0]);
    exit(EXIT_SUCCESS);
  }
  Settings.gen_thread_count = std::atoi(argv[1]);
  Settings.check_thread_count = std::atoi(argv[2]);
  Settings.block_count = std::atoi(argv[3]);
  Settings.block_size = std::atoi(argv[4]);


  auto control_num = Settings.check_thread_count - 1; //значение, на котором надо удалять из очереди
  std::vector<std::thread> gen_threads;
  std::vector<std::thread> check_threads;
  for (unsigned i = 0; i < Settings.check_thread_count; ++i) {
    check_threads.emplace_back(std::thread(&CheckThread::Run, CheckThread(data, checked_data, control_num, finished, combined_logger)));
  }
  for (unsigned i = 0; i < Settings.gen_thread_count; ++i) {
    gen_threads.emplace_back(std::thread(GenerateBlock, std::ref(data), std::ref(finished)));
  }
  while (data.q.empty()) {
    ;
  }
  for (auto& t : gen_threads) {
    t.join();
  }
  for (auto& t : check_threads) {
    t.join();
  }
  exit(EXIT_SUCCESS);
}
