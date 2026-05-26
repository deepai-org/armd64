#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  DT_NULL = 0,
  DT_RELA = 7,
  DT_RELASZ = 8,
  DT_RELAENT = 9,
  EM_AARCH64 = 183,
  EM_RISCV = 243,
  ET_DYN = 3,
  ET_EXEC = 2,
  EV_CURRENT = 1,
  PF_X = 1,
  PF_W = 2,
  PF_R = 4,
  PT_DYNAMIC = 2,
  PT_LOAD = 1,
  R_AARCH64_RELATIVE = 1027,
  R_RISCV_RELATIVE = 3
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

struct elf64_dyn {
  int64_t d_tag;
  uint64_t d_val;
};

struct elf64_rela {
  uint64_t r_offset;
  uint64_t r_info;
  int64_t r_addend;
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

static int parse_u64(const char *text, uint64_t *value) {
  char *end = NULL;
  errno = 0;
  unsigned long long parsed = strtoull(text, &end, 0);
  if (errno || end == text || *end != '\0')
    return -1;
  *value = (uint64_t) parsed;
  return 0;
}

static int machine_for_arch(const char *arch) {
  if (strcmp(arch, "aarch64") == 0)
    return EM_AARCH64;
  if (strcmp(arch, "riscv") == 0)
    return EM_RISCV;
  return 0;
}

static uint32_t relative_reloc_type_for_machine(int machine) {
  if (machine == EM_AARCH64)
    return R_AARCH64_RELATIVE;
  if (machine == EM_RISCV)
    return R_RISCV_RELATIVE;
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s ARCH OUTPUT [--split-data64 VALUE|--dyn-relative64 VALUE] INSN...\n", argv[0]);
    return 2;
  }

  const int machine = machine_for_arch(argv[1]);
  if (!machine) {
    fprintf(stderr, "mkpolyelf: unsupported arch: %s\n", argv[1]);
    return 1;
  }

  int first_insn_arg = 3;
  int split_data = 0;
  int dyn_relative = 0;
  uint64_t data_value = 0;
  if (strcmp(argv[3], "--split-data64") == 0 ||
      strcmp(argv[3], "--dyn-relative64") == 0) {
    if (argc < 7 || parse_u64(argv[4], &data_value) < 0) {
      fprintf(stderr, "mkpolyelf: bad data option usage\n");
      return 2;
    }
    split_data = 1;
    dyn_relative = strcmp(argv[3], "--dyn-relative64") == 0;
    first_insn_arg = 5;
  }

  const uint64_t text_offset = 0x1000;
  const uint64_t text_vaddr = dyn_relative ? 0 : 0x400000;
  const uint64_t data_offset = 0x3000;
  const uint64_t data_vaddr = dyn_relative ? 0x2000 : 0x402000;
  const uint64_t dynamic_offset = data_offset + 0x100;
  const uint64_t dynamic_vaddr = data_vaddr + 0x100;
  const uint64_t rela_offset = data_offset + 0x200;
  const uint64_t rela_vaddr = data_vaddr + 0x200;
  const uint64_t text_size = (uint64_t) (argc - first_insn_arg) * 4;
  const uint64_t data_size = dyn_relative ? 0x200 + sizeof(struct elf64_rela) :
    (split_data ? 8 : 0);

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
  ehdr.e_type = dyn_relative ? ET_DYN : ET_EXEC;
  ehdr.e_machine = (uint16_t) machine;
  ehdr.e_version = EV_CURRENT;
  ehdr.e_entry = text_vaddr;
  ehdr.e_phoff = sizeof(ehdr);
  ehdr.e_ehsize = sizeof(ehdr);
  ehdr.e_phentsize = sizeof(struct elf64_phdr);
  ehdr.e_phnum = dyn_relative ? 3 : (split_data ? 2 : 1);

  struct elf64_phdr phdrs[3];
  memset(phdrs, 0, sizeof(phdrs));
  phdrs[0].p_type = PT_LOAD;
  phdrs[0].p_flags = PF_R | PF_X;
  phdrs[0].p_offset = text_offset;
  phdrs[0].p_vaddr = text_vaddr;
  phdrs[0].p_paddr = text_vaddr;
  phdrs[0].p_filesz = text_size;
  phdrs[0].p_memsz = text_size;
  phdrs[0].p_align = 0x1000;
  if (split_data) {
    phdrs[1].p_type = PT_LOAD;
    phdrs[1].p_flags = PF_R | PF_W;
    phdrs[1].p_offset = data_offset;
    phdrs[1].p_vaddr = data_vaddr;
    phdrs[1].p_paddr = data_vaddr;
    phdrs[1].p_filesz = data_size;
    phdrs[1].p_memsz = data_size;
    phdrs[1].p_align = 0x1000;
  }
  if (dyn_relative) {
    phdrs[2].p_type = PT_DYNAMIC;
    phdrs[2].p_flags = PF_R | PF_W;
    phdrs[2].p_offset = dynamic_offset;
    phdrs[2].p_vaddr = dynamic_vaddr;
    phdrs[2].p_paddr = dynamic_vaddr;
    phdrs[2].p_filesz = 4 * sizeof(struct elf64_dyn);
    phdrs[2].p_memsz = 4 * sizeof(struct elf64_dyn);
    phdrs[2].p_align = 8;
  }

  if (fwrite(&ehdr, sizeof(ehdr), 1, out) != 1 ||
      fwrite(phdrs, sizeof(struct elf64_phdr), ehdr.e_phnum, out) != ehdr.e_phnum) {
    fprintf(stderr, "mkpolyelf: header write failed\n");
    fclose(out);
    return 1;
  }

  if (fseek(out, (long) text_offset, SEEK_SET) != 0) {
    fprintf(stderr, "mkpolyelf: seek failed\n");
    fclose(out);
    return 1;
  }

  for (int n = first_insn_arg; n < argc; n++) {
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

  if (split_data) {
    if (fseek(out, (long) data_offset, SEEK_SET) != 0) {
      fprintf(stderr, "mkpolyelf: data seek failed\n");
      fclose(out);
      return 1;
    }
    unsigned char bytes[8];
    for (unsigned n = 0; n < sizeof(bytes); n++)
      bytes[n] = dyn_relative ? 0 : (unsigned char) ((data_value >> (n * 8)) & 0xff);
    if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
      fprintf(stderr, "mkpolyelf: data write failed\n");
      fclose(out);
      return 1;
    }
    if (dyn_relative) {
      for (unsigned n = 0; n < sizeof(bytes); n++)
        bytes[n] = (unsigned char) ((data_value >> (n * 8)) & 0xff);
      if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: data write failed\n");
        fclose(out);
        return 1;
      }

      if (fseek(out, (long) dynamic_offset, SEEK_SET) != 0) {
        fprintf(stderr, "mkpolyelf: dynamic seek failed\n");
        fclose(out);
        return 1;
      }
      struct elf64_dyn dyn[4];
      memset(dyn, 0, sizeof(dyn));
      dyn[0].d_tag = DT_RELA;
      dyn[0].d_val = rela_vaddr;
      dyn[1].d_tag = DT_RELASZ;
      dyn[1].d_val = sizeof(struct elf64_rela);
      dyn[2].d_tag = DT_RELAENT;
      dyn[2].d_val = sizeof(struct elf64_rela);
      dyn[3].d_tag = DT_NULL;
      dyn[3].d_val = 0;
      if (fwrite(dyn, sizeof(dyn), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: dynamic write failed\n");
        fclose(out);
        return 1;
      }

      if (fseek(out, (long) rela_offset, SEEK_SET) != 0) {
        fprintf(stderr, "mkpolyelf: rela seek failed\n");
        fclose(out);
        return 1;
      }
      struct elf64_rela rela;
      memset(&rela, 0, sizeof(rela));
      rela.r_offset = data_vaddr;
      rela.r_info = (uint64_t) relative_reloc_type_for_machine(machine);
      rela.r_addend = (int64_t) (data_vaddr + 8);
      if (fwrite(&rela, sizeof(rela), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: rela write failed\n");
        fclose(out);
        return 1;
      }
    }
  }

  if (fclose(out) != 0) {
    fprintf(stderr, "mkpolyelf: close failed\n");
    return 1;
  }
  return 0;
}
