#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

#include <atomic>
#include <unordered_map>

static constexpr size_t kThreadCount = 6;
static constexpr size_t kThreadStackSize = 256 * 1024;

static thread_local uint64_t tls_cookie;
static std::atomic<uint64_t> worker_sum;
static unsigned char thread_stacks[kThreadCount][kThreadStackSize]
    __attribute__((aligned(16)));

static void mark_worker(char phase, uint64_t index) {
  char msg[] = "THREAD_HASH_WORKER_X_0\n";
  msg[19] = phase;
  msg[21] = (char) ('0' + (index % 10));
  register uint64_t x0 asm("x0") = 2;
  register uint64_t x1 asm("x1") = (uint64_t) (uintptr_t) msg;
  register uint64_t x2 asm("x2") = sizeof(msg) - 1;
  register uint64_t x8 asm("x8") = 64;
  asm volatile("svc #0"
               : "+r"(x0)
               : "r"(x1), "r"(x2), "r"(x8)
               : "memory");
}

static uint64_t mix_key(uint64_t value) {
  value ^= value >> 33;
  value *= 0xff51afd7ed558ccdULL;
  value ^= value >> 33;
  value *= 0xc4ceb9fe1a85ec53ULL;
  value ^= value >> 33;
  return value;
}

static void *worker_main(void *arg) {
  const uint64_t index = (uint64_t) (uintptr_t) arg;
  mark_worker('A', index);
  tls_cookie = 0x9e3779b97f4a7c15ULL ^ index;
  mark_worker('B', index);

  std::unordered_map<uint64_t, uint64_t> table;
  table.reserve(128);
  mark_worker('C', index);
  uint64_t local = 0;
  for (uint64_t n = 0; n < 96; n++) {
    const uint64_t key = mix_key((index << 32) | n);
    const uint64_t value = key ^ tls_cookie ^ n;
    table.emplace(key, value);
    local ^= table.find(key)->second;
    if (n == 0 || n == 31 || n == 63 || n == 95)
      mark_worker((char) ('D' + (n / 32)), index);
  }

  if (table.size() != 96)
    return (void *) (uintptr_t) 2;
  if (tls_cookie != (0x9e3779b97f4a7c15ULL ^ index))
    return (void *) (uintptr_t) 3;

  mark_worker('G', index);
  worker_sum.fetch_xor(local, std::memory_order_relaxed);
  mark_worker('H', index);
  return 0;
}

int main(void) {
  const char *default_stacks_env = getenv("POLY_THREAD_HASH_DEFAULT_STACKS");
  const bool use_default_stacks =
      default_stacks_env != nullptr && default_stacks_env[0] == '1';
  size_t thread_count = kThreadCount;
  const char *thread_count_env = getenv("POLY_THREAD_HASH_THREADS");
  if (thread_count_env != nullptr && thread_count_env[0] != '\0') {
    char *end = nullptr;
    unsigned long requested = strtoul(thread_count_env, &end, 10);
    if (end == thread_count_env || *end != '\0' || requested == 0 ||
        requested > kThreadCount)
      return 17;
    thread_count = requested;
  }

  tls_cookie = 0x123456789abcdef0ULL;

  std::unordered_map<uint64_t, uint64_t> table;
  table.reserve(512);
  uint64_t expected = 0;
  for (uint64_t n = 0; n < 384; n++) {
    const uint64_t key = mix_key(n + 0x1000);
    const uint64_t value = key + (n << 1);
    table.emplace(key, value);
    expected += value;
  }

  uint64_t actual = 0;
  for (const auto &entry : table)
    actual += entry.second;
  if (actual != expected || table.size() != 384)
    return 10;
  if (tls_cookie != 0x123456789abcdef0ULL)
    return 11;

  pthread_t threads[kThreadCount];
  pthread_attr_t attrs[kThreadCount];
  for (uint64_t n = 0; n < thread_count; n++) {
    if (pthread_attr_init(&attrs[n]) != 0)
      return 18;
    if (!use_default_stacks &&
        pthread_attr_setstack(&attrs[n], thread_stacks[n],
          kThreadStackSize) != 0)
      return 19;
    if (pthread_create(&threads[n], &attrs[n], worker_main,
          (void *) (uintptr_t) (n + 1)) != 0)
      return 20;
  }

  for (size_t n = 0; n < thread_count; n++) {
    void *result = 0;
    if (pthread_join(threads[n], &result) != 0)
      return 30;
    if (pthread_attr_destroy(&attrs[n]) != 0)
      return 31;
    if (result != 0)
      return 40 + (int) (uintptr_t) result;
  }

  if (worker_sum.load(std::memory_order_relaxed) == 0)
    return 50;
  if (tls_cookie != 0x123456789abcdef0ULL)
    return 51;
  return 42;
}
