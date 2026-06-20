#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
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
#define POLY_IORING_OP_TIMEOUT 11
#define POLY_IORING_OP_TIMEOUT_REMOVE 12
#define POLY_IORING_OP_READ 22
#define POLY_IORING_OP_WRITE 23
#define POLY_IO_URING_NOP_USER_DATA 0x706f6c792d6e6f70ULL
#define POLY_IO_URING_WRITE_USER_DATA 0x706f6c792d777274ULL
#define POLY_IO_URING_READ_USER_DATA 0x706f6c792d726561ULL
#define POLY_IO_URING_TIMEOUT_USER_DATA 0x706f6c792d746d6fULL
#define POLY_IO_URING_CANCEL_USER_DATA 0x706f6c792d636e78ULL

#ifndef ECANCELED
#define ECANCELED 125
#endif

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

struct poly_timespec {
  int64_t sec;
  int64_t nsec;
};

struct poly_iouring {
  int fd;
  struct poly_io_uring_params params;
  uint8_t *sq_ring;
  size_t sq_ring_bytes;
  uint8_t *cq_ring;
  size_t cq_ring_bytes;
  struct poly_io_uring_sqe *sqes;
  uint32_t *sq_head;
  uint32_t *sq_tail;
  uint32_t *sq_mask;
  uint32_t *sq_array;
  uint32_t *cq_head;
  uint32_t *cq_tail;
  uint32_t *cq_mask;
  struct poly_io_uring_cqe *cqes;
};

static int poly_iouring_fail(const char *step, int code) {
  fprintf(stderr, "POLY_IO_URING_REAL_FAIL: step=%s code=%d errno=%d\n",
    step, code, errno);
  return code;
}

static void *poly_iouring_mmap(size_t bytes, int fd, uint64_t offset) {
  return mmap(NULL, bytes, PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_POPULATE, fd, (off_t) offset);
}

static int poly_iouring_open(struct poly_iouring *ring) {
  memset(ring, 0, sizeof(*ring));
  ring->fd = -1;

  int fd = (int) syscall(SYS_io_uring_setup, 8U, &ring->params);
  if (fd < 0)
    return poly_iouring_fail("setup", 10);
  ring->fd = fd;

  ring->sq_ring_bytes = ring->params.sq_off.array +
    ring->params.sq_entries * sizeof(uint32_t);
  ring->cq_ring_bytes = ring->params.cq_off.cqes +
    ring->params.cq_entries * sizeof(struct poly_io_uring_cqe);
  if ((ring->params.features & POLY_IORING_FEAT_SINGLE_MMAP) != 0 &&
      ring->cq_ring_bytes > ring->sq_ring_bytes)
    ring->sq_ring_bytes = ring->cq_ring_bytes;

  ring->sq_ring = poly_iouring_mmap(ring->sq_ring_bytes, fd,
    POLY_IORING_OFF_SQ_RING);
  if (ring->sq_ring == MAP_FAILED) {
    int saved = errno;
    close(fd);
    ring->fd = -1;
    errno = saved;
    return poly_iouring_fail("mmap-sq", 11);
  }

  ring->cq_ring = ring->sq_ring;
  if ((ring->params.features & POLY_IORING_FEAT_SINGLE_MMAP) == 0) {
    ring->cq_ring = poly_iouring_mmap(ring->cq_ring_bytes, fd,
      POLY_IORING_OFF_CQ_RING);
    if (ring->cq_ring == MAP_FAILED) {
      int saved = errno;
      munmap(ring->sq_ring, ring->sq_ring_bytes);
      close(fd);
      ring->fd = -1;
      errno = saved;
      return poly_iouring_fail("mmap-cq", 12);
    }
  }

  ring->sqes = poly_iouring_mmap(
    ring->params.sq_entries * sizeof(*ring->sqes), fd, POLY_IORING_OFF_SQES);
  if (ring->sqes == MAP_FAILED) {
    int saved = errno;
    if (ring->cq_ring != ring->sq_ring)
      munmap(ring->cq_ring, ring->cq_ring_bytes);
    munmap(ring->sq_ring, ring->sq_ring_bytes);
    close(fd);
    ring->fd = -1;
    errno = saved;
    return poly_iouring_fail("mmap-sqes", 13);
  }

  ring->sq_head = (uint32_t *) (void *)
    (ring->sq_ring + ring->params.sq_off.head);
  ring->sq_tail = (uint32_t *) (void *)
    (ring->sq_ring + ring->params.sq_off.tail);
  ring->sq_mask = (uint32_t *) (void *)
    (ring->sq_ring + ring->params.sq_off.ring_mask);
  ring->sq_array = (uint32_t *) (void *)
    (ring->sq_ring + ring->params.sq_off.array);
  ring->cq_head = (uint32_t *) (void *)
    (ring->cq_ring + ring->params.cq_off.head);
  ring->cq_tail = (uint32_t *) (void *)
    (ring->cq_ring + ring->params.cq_off.tail);
  ring->cq_mask = (uint32_t *) (void *)
    (ring->cq_ring + ring->params.cq_off.ring_mask);
  ring->cqes = (struct poly_io_uring_cqe *) (void *)
    (ring->cq_ring + ring->params.cq_off.cqes);
  return 0;
}

static void poly_iouring_close(struct poly_iouring *ring) {
  if (ring->sqes != NULL && ring->sqes != MAP_FAILED)
    munmap(ring->sqes, ring->params.sq_entries * sizeof(*ring->sqes));
  if (ring->cq_ring != NULL && ring->cq_ring != MAP_FAILED &&
      ring->cq_ring != ring->sq_ring)
    munmap(ring->cq_ring, ring->cq_ring_bytes);
  if (ring->sq_ring != NULL && ring->sq_ring != MAP_FAILED)
    munmap(ring->sq_ring, ring->sq_ring_bytes);
  if (ring->fd >= 0)
    close(ring->fd);
}

static struct poly_io_uring_sqe *poly_iouring_prepare_at(
    struct poly_iouring *ring, unsigned int offset) {
  uint32_t tail = *ring->sq_tail + offset;
  uint32_t sq_index = tail & *ring->sq_mask;
  memset(&ring->sqes[sq_index], 0, sizeof(ring->sqes[sq_index]));
  ring->sq_array[sq_index] = sq_index;
  return &ring->sqes[sq_index];
}

static struct poly_io_uring_sqe *poly_iouring_prepare(struct poly_iouring *ring) {
  return poly_iouring_prepare_at(ring, 0);
}

static void poly_iouring_submit_prepared(struct poly_iouring *ring,
    unsigned int count) {
  __sync_synchronize();
  *ring->sq_tail += count;
  __sync_synchronize();
}

static int poly_iouring_enter(struct poly_iouring *ring, unsigned int submit,
    unsigned int wait_nr) {
  long entered = syscall(SYS_io_uring_enter, ring->fd, submit, wait_nr,
    POLY_IORING_ENTER_GETEVENTS, NULL, 0U);
  if (entered < 0)
    return poly_iouring_fail("enter", 14);
  if (entered != (long) submit)
    return poly_iouring_fail("enter-count", 15);
  return 0;
}

static int poly_iouring_wait_cqes(struct poly_iouring *ring,
    struct poly_io_uring_cqe *out, unsigned int want) {
  unsigned int seen = 0;
  for (unsigned int attempt = 0; seen < want && attempt < 32;
       attempt++) {
    while (seen < want && *ring->cq_head != *ring->cq_tail) {
      uint32_t head = *ring->cq_head;
      out[seen++] = ring->cqes[head & *ring->cq_mask];
      *ring->cq_head = head + 1;
      __sync_synchronize();
    }
    if (seen >= want)
      break;
    (void) syscall(SYS_io_uring_enter, ring->fd, 0U, want - seen,
      POLY_IORING_ENTER_GETEVENTS, NULL, 0U);
    __sync_synchronize();
  }
  if (seen != want)
    return poly_iouring_fail("cq-missing", 17);
  return 0;
}

static int poly_iouring_run_nop(struct poly_iouring *ring) {
  struct poly_io_uring_sqe *sqe = poly_iouring_prepare(ring);
  sqe->opcode = POLY_IORING_OP_NOP;
  sqe->user_data = POLY_IO_URING_NOP_USER_DATA;
  poly_iouring_submit_prepared(ring, 1);
  if (poly_iouring_enter(ring, 1, 1) != 0)
    return 14;

  struct poly_io_uring_cqe cqe;
  if (poly_iouring_wait_cqes(ring, &cqe, 1) != 0)
    return 17;
  if (cqe.user_data != POLY_IO_URING_NOP_USER_DATA || cqe.res != 0)
    return poly_iouring_fail("cqe", 18);
  puts("POLY_IO_URING_NOP_OK");
  return 0;
}

static int poly_iouring_run_rw(struct poly_iouring *ring) {
  const char path[] = "/tmp/poly-iouring-real.dat";
  int file_fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0600);
  if (file_fd < 0)
    return poly_iouring_fail("rw-open", 30);
  (void) unlink(path);

  const char write_buf[] = "poly-io-uring-rw-check";
  char read_buf[sizeof(write_buf)];
  memset(read_buf, 0, sizeof(read_buf));

  struct poly_io_uring_sqe *sqe = poly_iouring_prepare(ring);
  sqe->opcode = POLY_IORING_OP_WRITE;
  sqe->fd = file_fd;
  sqe->off = 0;
  sqe->addr = (uint64_t) (uintptr_t) write_buf;
  sqe->len = sizeof(write_buf);
  sqe->user_data = POLY_IO_URING_WRITE_USER_DATA;
  poly_iouring_submit_prepared(ring, 1);
  if (poly_iouring_enter(ring, 1, 1) != 0) {
    close(file_fd);
    return 31;
  }

  struct poly_io_uring_cqe cqe;
  if (poly_iouring_wait_cqes(ring, &cqe, 1) != 0) {
    close(file_fd);
    return 32;
  }
  if (cqe.user_data != POLY_IO_URING_WRITE_USER_DATA ||
      cqe.res != (int32_t) sizeof(write_buf)) {
    close(file_fd);
    return poly_iouring_fail("write-cqe", 33);
  }

  sqe = poly_iouring_prepare(ring);
  sqe->opcode = POLY_IORING_OP_READ;
  sqe->fd = file_fd;
  sqe->off = 0;
  sqe->addr = (uint64_t) (uintptr_t) read_buf;
  sqe->len = sizeof(read_buf);
  sqe->user_data = POLY_IO_URING_READ_USER_DATA;
  poly_iouring_submit_prepared(ring, 1);
  if (poly_iouring_enter(ring, 1, 1) != 0) {
    close(file_fd);
    return 34;
  }

  if (poly_iouring_wait_cqes(ring, &cqe, 1) != 0) {
    close(file_fd);
    return 35;
  }
  close(file_fd);
  if (cqe.user_data != POLY_IO_URING_READ_USER_DATA ||
      cqe.res != (int32_t) sizeof(read_buf))
    return poly_iouring_fail("read-cqe", 36);
  if (memcmp(read_buf, write_buf, sizeof(write_buf)) != 0)
    return poly_iouring_fail("read-data", 37);

  puts("POLY_IO_URING_RW_OK");
  return 0;
}

static int poly_iouring_run_timeout_cancel(struct poly_iouring *ring) {
  struct poly_timespec timeout = { 1, 0 };
  struct poly_io_uring_sqe *timeout_sqe = poly_iouring_prepare_at(ring, 0);
  timeout_sqe->opcode = POLY_IORING_OP_TIMEOUT;
  timeout_sqe->addr = (uint64_t) (uintptr_t) &timeout;
  timeout_sqe->len = 1;
  timeout_sqe->user_data = POLY_IO_URING_TIMEOUT_USER_DATA;

  struct poly_io_uring_sqe *cancel_sqe = poly_iouring_prepare_at(ring, 1);
  cancel_sqe->opcode = POLY_IORING_OP_TIMEOUT_REMOVE;
  cancel_sqe->addr = POLY_IO_URING_TIMEOUT_USER_DATA;
  cancel_sqe->user_data = POLY_IO_URING_CANCEL_USER_DATA;

  poly_iouring_submit_prepared(ring, 2);
  if (poly_iouring_enter(ring, 2, 2) != 0)
    return 40;

  struct poly_io_uring_cqe cqes[2];
  if (poly_iouring_wait_cqes(ring, cqes, 2) != 0)
    return 41;

  int saw_timeout_cancel = 0;
  int saw_cancel_ok = 0;
  for (unsigned int i = 0; i < 2; i++) {
    if (cqes[i].user_data == POLY_IO_URING_TIMEOUT_USER_DATA &&
        cqes[i].res == -ECANCELED)
      saw_timeout_cancel = 1;
    if (cqes[i].user_data == POLY_IO_URING_CANCEL_USER_DATA &&
        cqes[i].res == 0)
      saw_cancel_ok = 1;
  }
  if (!saw_timeout_cancel || !saw_cancel_ok)
    return poly_iouring_fail("timeout-cancel-cqe", 42);

  puts("POLY_IO_URING_TIMEOUT_CANCEL_OK");
  return 0;
}

int main(void) {
  _Static_assert(sizeof(struct poly_io_uring_sqe) == 64,
    "io_uring SQE layout must stay ABI-compatible");
  _Static_assert(sizeof(struct poly_io_uring_cqe) == 16,
    "io_uring CQE layout must stay ABI-compatible");

  struct poly_iouring ring;
  int status = poly_iouring_open(&ring);
  if (status != 0)
    return status;

  status = poly_iouring_run_nop(&ring);
  if (status == 0)
    status = poly_iouring_run_rw(&ring);
  if (status == 0)
    status = poly_iouring_run_timeout_cancel(&ring);

  poly_iouring_close(&ring);
  if (status != 0)
    return status;
  return 42;
}
