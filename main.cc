#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>
#include <typeinfo>
#include <vector>

using namespace std;
using namespace std::chrono_literals;


using Iter = vector<int>::iterator;

struct accum_block {
  void operator()(Iter first, Iter last, int& result) {
    result = accumulate(first, last, result);
  }
};

int parallel_accum(Iter first, Iter last, int init) {
  size_t length = distance(first, last);
  if (!length)
    return init;
  const size_t min_per_thread = 25;
  const size_t max_threads = (length + min_per_thread - 1) / min_per_thread;
  const size_t hardware_threads = thread::hardware_concurrency();
  const size_t num_threads = min(
      hardware_threads != 0 ? hardware_threads : 2,
      max_threads);
  const size_t num_elems_block = length / num_threads;
  vector<int> results(num_threads);
  vector<thread> threads(num_threads - 1);

  Iter block_start = first;
  for (size_t i = 0; i < num_threads - 1; ++i) {
    Iter block_end = block_start;
    std::advance(block_end, num_elems_block);
    threads[i] = thread(
        accum_block{},
        block_start,
        block_end,
        std::ref(results[i]));
    block_start = block_end;
  }
  accum_block()(block_start, last, results[num_threads - 1]);

  std::for_each(
      begin(threads),
      end(threads),
      [](auto& el) { el.join(); });

  return accumulate(begin(results), end(results), init);
}

int main() {
  srand(time(NULL));
  std::vector<int> v(1000000);
  for_each(begin(v), end(v), [](auto& el) {
    el = rand() % 5;
  });
  cout << "Parallel way... " << endl;
  auto time_begin = chrono::high_resolution_clock::now();
  cout << "Parallel sum = " << parallel_accum(begin(v), end(v), 0) << endl;
  auto time_end = chrono::high_resolution_clock::now();
  std::chrono::duration<double> dur1 = time_end - time_begin;
  cout << "time elapsed: " << dur1.count() << endl;
  cout << "==================================================="
       << endl;
  cout << "Synchronous way... " << endl;
  time_begin = chrono::high_resolution_clock::now();
  cout << "sum sync = " << accumulate(begin(v), end(v), 0) << endl;
  time_end = chrono::high_resolution_clock::now();
  std::chrono::duration<double> dur2 = time_end - time_begin;
  cout << "time elapsed: " << dur2.count() << endl;
  return 0;
}
