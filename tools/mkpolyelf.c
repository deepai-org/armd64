#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  EM_RISCV = 243,
  ET_EXEC = 2,
  EV_CURRENT = 1,
  PF_X = 1,
  PF_R = 4,
  PT_LOAD = 1
};

struct elf64_ehdr {
  unsigned char e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

struct elf64_phdr {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
};

static int parse_u32(const char *text, uint32_t *value) {
  char *end = NULL;
  errno = 0;
  unsigned long parsed = strtoul(text, &end, 0);
  if (errno || end == text || *end != '\0' || parsed > UINT32_MAX)
    return -1;
  *value = (uint32_t) parsed;
  return 0;
}

static int machine_for_arch(const char *arch) {
  if (strcmp(arch, "aarch64") == 0)
    return 183;
  if (strcmp(arch, "riscv") == 0)
    return EM_RISCV;
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s ARCH OUTPUT INSN...\n", argv[0]);
    return 2;
  }

  const int machine = machine_for_arch(argv[1]);
  if (!machine) {
    fprintf(stderr, "mkpolyelf: unsupported arch: %s\n", argv[1]);
    return 1;
  }

  const uint64_t text_offset = 0x1000;
  const uint64_t text_vaddr = 0x400000;
  const uint64_t text_size = (uint64_t) (argc - 3) * 4;

  FILE *out = fopen(argv[2], "wb");
  if (!out) {
    fprintf(stderr, "mkpolyelf: unable to open %s: %s\n", argv[2], strerror(errno));
    return 1;
  }

  struct elf64_ehdr ehdr;
  memset(&ehdr, 0, sizeof(ehdr));
  ehdr.e_ident[0] = 0x7f;
  ehdr.e_ident[1] = 'E';
  ehdr.e_ident[2] = 'L';
  ehdr.e_ident[3] = 'F';
  ehdr.e_ident[4] = 2;
  ehdr.e_ident[5] = 1;
  ehdr.e_ident[6] = EV_CURRENT;
  ehdr.e_type = ET_EXEC;
  ehdr.e_machine = (uint16_t) machine;
  ehdr.e_version = EV_CURRENT;
  ehdr.e_entry = text_vaddr;
  ehdr.e_phoff = sizeof(ehdr);
  ehdr.e_ehsize = sizeof(ehdr);
  ehdr.e_phentsize = sizeof(struct elf64_phdr);
  ehdr.e_phnum = 1;

  struct elf64_phdr phdr;
  memset(&phdr, 0, sizeof(phdr));
  phdr.p_type = PT_LOAD;
  phdr.p_flags = PF_R | PF_X;
  phdr.p_offset = text_offset;
  phdr.p_vaddr = text_vaddr;
  phdr.p_paddr = text_vaddr;
  phdr.p_filesz = text_size;
  phdr.p_memsz = text_size;
  phdr.p_align = 0x1000;

  if (fwrite(&ehdr, sizeof(ehdr), 1, out) != 1 || fwrite(&phdr, sizeof(phdr), 1, out) != 1) {
    fprintf(stderr, "mkpolyelf: header write failed\n");
    fclose(out);
    return 1;
  }

  if (fseek(out, (long) text_offset, SEEK_SET) != 0) {
    fprintf(stderr, "mkpolyelf: seek failed\n");
    fclose(out);
    return 1;
  }

  for (int n = 3; n < argc; n++) {
    uint32_t insn = 0;
    if (parse_u32(argv[n], &insn) < 0) {
      fprintf(stderr, "mkpolyelf: bad instruction: %s\n", argv[n]);
      fclose(out);
      return 1;
    }
    unsigned char bytes[4] = {
      (unsigned char) (insn & 0xff),
      (unsigned char) ((insn >> 8) & 0xff),
      (unsigned char) ((insn >> 16) & 0xff),
      (unsigned char) ((insn >> 24) & 0xff)
    };
    if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
      fprintf(stderr, "mkpolyelf: instruction write failed\n");
      fclose(out);
      return 1;
    }
  }

  if (fclose(out) != 0) {
    fprintf(stderr, "mkpolyelf: close failed\n");
    return 1;
  }
  return 0;
}
