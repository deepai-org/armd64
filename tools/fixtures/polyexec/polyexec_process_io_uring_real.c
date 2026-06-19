#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_io_uring_setup
#define SYS_io_uring_setup __NR_io_uring_setup
#endif

#ifndef SYS_io_uring_enter
#define SYS_io_uring_enter __NR_io_uring_enter
#endif

#define POLY_IORING_OFF_SQ_RING 0ULL
#define POLY_IORING_OFF_CQ_RING 0x8000000ULL
#define POLY_IORING_OFF_SQES 0x10000000ULL
#define POLY_IORING_FEAT_SINGLE_MMAP (1U << 0)
#define POLY_IORING_ENTER_GETEVENTS (1U << 0)
#define POLY_IORING_OP_NOP 0
#define POLY_IO_URING_USER_DATA 0x706f6c792d757269ULL

struct poly_io_sqring_offsets {
  uint32_t head;
  uint32_t tail;
  uint32_t ring_mask;
  uint32_t ring_entries;
  uint32_t flags;
  uint32_t dropped;
  uint32_t array;
  uint32_t resv1;
  uint64_t user_addr;
};

struct poly_io_cqring_offsets {
  uint32_t head;
  uint32_t tail;
  uint32_t ring_mask;
  uint32_t ring_entries;
  uint32_t overflow;
  uint32_t cqes;
  uint32_t flags;
  uint32_t resv1;
  uint64_t user_addr;
};

struct poly_io_uring_params {
  uint32_t sq_entries;
  uint32_t cq_entries;
  uint32_t flags;
  uint32_t sq_thread_cpu;
  uint32_t sq_thread_idle;
  uint32_t features;
  uint32_t wq_fd;
  uint32_t resv[3];
  struct poly_io_sqring_offsets sq_off;
  struct poly_io_cqring_offsets cq_off;
};

struct poly_io_uring_sqe {
  uint8_t opcode;
  uint8_t flags;
  uint16_t ioprio;
  int32_t fd;
  uint64_t off;
  uint64_t addr;
  uint32_t len;
  uint32_t rw_flags;
  uint64_t user_data;
  uint16_t buf_index;
  uint16_t personality;
  int32_t splice_fd_in;
  uint64_t addr3;
  uint64_t pad2;
};

struct poly_io_uring_cqe {
  uint64_t user_data;
  int32_t res;
  uint32_t flags;
};

static int poly_iouring_fail(const char *step, int code) {
  fprintf(stderr, "POLY_IO_URING_NOP_FAIL: step=%s code=%d errno=%d\n",
    step, code, errno);
  return code;
}

static void *poly_iouring_mmap(size_t bytes, int fd, uint64_t offset) {
  return mmap(NULL, bytes, PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_POPULATE, fd, (off_t) offset);
}

int main(void) {
  _Static_assert(sizeof(struct poly_io_uring_sqe) == 64,
    "io_uring SQE layout must stay ABI-compatible");
  _Static_assert(sizeof(struct poly_io_uring_cqe) == 16,
    "io_uring CQE layout must stay ABI-compatible");

  struct poly_io_uring_params params;
  memset(&params, 0, sizeof(params));

  int fd = (int) syscall(SYS_io_uring_setup, 8U, &params);
  if (fd < 0)
    return poly_iouring_fail("setup", 10);

  size_t sq_ring_bytes = params.sq_off.array +
    params.sq_entries * sizeof(uint32_t);
  size_t cq_ring_bytes = params.cq_off.cqes +
    params.cq_entries * sizeof(struct poly_io_uring_cqe);
  if ((params.features & POLY_IORING_FEAT_SINGLE_MMAP) != 0 &&
      cq_ring_bytes > sq_ring_bytes)
    sq_ring_bytes = cq_ring_bytes;

  uint8_t *sq_ring = poly_iouring_mmap(sq_ring_bytes, fd,
    POLY_IORING_OFF_SQ_RING);
  if (sq_ring == MAP_FAILED) {
    int saved = errno;
    close(fd);
    errno = saved;
    return poly_iouring_fail("mmap-sq", 11);
  }

  uint8_t *cq_ring = sq_ring;
  if ((params.features & POLY_IORING_FEAT_SINGLE_MMAP) == 0) {
    cq_ring = poly_iouring_mmap(cq_ring_bytes, fd, POLY_IORING_OFF_CQ_RING);
    if (cq_ring == MAP_FAILED) {
      int saved = errno;
      munmap(sq_ring, sq_ring_bytes);
      close(fd);
      errno = saved;
      return poly_iouring_fail("mmap-cq", 12);
    }
  }

  struct poly_io_uring_sqe *sqes = poly_iouring_mmap(
    params.sq_entries * sizeof(*sqes), fd, POLY_IORING_OFF_SQES);
  if (sqes == MAP_FAILED) {
    int saved = errno;
    if (cq_ring != sq_ring)
      munmap(cq_ring, cq_ring_bytes);
    munmap(sq_ring, sq_ring_bytes);
    close(fd);
    errno = saved;
    return poly_iouring_fail("mmap-sqes", 13);
  }

  uint32_t *sq_head = (uint32_t *) (void *) (sq_ring + params.sq_off.head);
  uint32_t *sq_tail = (uint32_t *) (void *) (sq_ring + params.sq_off.tail);
  uint32_t *sq_mask = (uint32_t *) (void *) (sq_ring + params.sq_off.ring_mask);
  uint32_t *sq_array = (uint32_t *) (void *) (sq_ring + params.sq_off.array);
  uint32_t *cq_head = (uint32_t *) (void *) (cq_ring + params.cq_off.head);
  uint32_t *cq_tail = (uint32_t *) (void *) (cq_ring + params.cq_off.tail);
  uint32_t *cq_mask = (uint32_t *) (void *) (cq_ring + params.cq_off.ring_mask);
  struct poly_io_uring_cqe *cqes =
    (struct poly_io_uring_cqe *) (void *) (cq_ring + params.cq_off.cqes);

  uint32_t tail = *sq_tail;
  uint32_t sq_index = tail & *sq_mask;
  memset(&sqes[sq_index], 0, sizeof(sqes[sq_index]));
  sqes[sq_index].opcode = POLY_IORING_OP_NOP;
  sqes[sq_index].user_data = POLY_IO_URING_USER_DATA;
  sq_array[sq_index] = sq_index;
  __sync_synchronize();
  *sq_tail = tail + 1;
  __sync_synchronize();

  long entered = syscall(SYS_io_uring_enter, fd, 1U, 1U,
    POLY_IORING_ENTER_GETEVENTS, NULL, 0U);
  if (entered < 0)
    return poly_iouring_fail("enter", 14);
  if (entered != 1)
    return poly_iouring_fail("enter-count", 15);
  if (*sq_head != tail + 1)
    return poly_iouring_fail("sq-head", 16);

  for (unsigned int attempt = 0; attempt < 8 && *cq_head == *cq_tail;
       attempt++) {
    (void) syscall(SYS_io_uring_enter, fd, 0U, 1U,
      POLY_IORING_ENTER_GETEVENTS, NULL, 0U);
    __sync_synchronize();
  }

  uint32_t head = *cq_head;
  if (head == *cq_tail)
    return poly_iouring_fail("cq-empty", 17);
  struct poly_io_uring_cqe *cqe = &cqes[head & *cq_mask];
  if (cqe->user_data != POLY_IO_URING_USER_DATA || cqe->res != 0)
    return poly_iouring_fail("cqe", 18);

  *cq_head = head + 1;
  __sync_synchronize();

  munmap(sqes, params.sq_entries * sizeof(*sqes));
  if (cq_ring != sq_ring)
    munmap(cq_ring, cq_ring_bytes);
  munmap(sq_ring, sq_ring_bytes);
  close(fd);

  puts("POLY_IO_URING_NOP_OK");
  return 42;
}
