#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  DT_NULL = 0,
  DT_PLTRELSZ = 2,
  DT_HASH = 4,
  DT_STRTAB = 5,
  DT_SYMTAB = 6,
  DT_RELA = 7,
  DT_RELASZ = 8,
  DT_RELAENT = 9,
  DT_STRSZ = 10,
  DT_SYMENT = 11,
  DT_REL = 17,
  DT_RELSZ = 18,
  DT_RELENT = 19,
  DT_PLTREL = 20,
  DT_JMPREL = 23,
  DT_RELRSZ = 35,
  DT_RELR = 36,
  DT_RELRENT = 37,
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
  R_AARCH64_NONE = 0,
  R_AARCH64_ABS64 = 257,
  R_AARCH64_GLOB_DAT = 1025,
  R_AARCH64_JUMP_SLOT = 1026,
  R_AARCH64_RELATIVE = 1027,
  R_AARCH64_IRELATIVE = 1032,
  R_RISCV_NONE = 0,
  R_RISCV_64 = 2,
  R_RISCV_JUMP_SLOT = 5,
  R_RISCV_RELATIVE = 3,
  R_RISCV_IRELATIVE = 58,
  SHF_ALLOC = 2,
  SHF_EXECINSTR = 4,
  SHN_UNDEF = 0,
  SHT_DYNAMIC = 6,
  SHT_DYNSYM = 11,
  SHT_REL = 9,
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_RELA = 4,
  SHT_STRTAB = 3,
  STB_LOCAL = 0,
  STB_GLOBAL = 1,
  STT_NOTYPE = 0,
  STT_OBJECT = 1,
  STT_FUNC = 2
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

struct elf64_rel {
  uint64_t r_offset;
  uint64_t r_info;
};

struct elf64_shdr {
  uint32_t sh_name;
  uint32_t sh_type;
  uint64_t sh_flags;
  uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint64_t sh_addralign;
  uint64_t sh_entsize;
};

struct elf64_sym {
  uint32_t st_name;
  unsigned char st_info;
  unsigned char st_other;
  uint16_t st_shndx;
  uint64_t st_value;
  uint64_t st_size;
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

static int parse_u16(const char *text, uint16_t *value) {
  char *end = NULL;
  errno = 0;
  unsigned long parsed = strtoul(text, &end, 0);
  if (errno || end == text || *end != '\0' || parsed > UINT16_MAX)
    return -1;
  *value = (uint16_t) parsed;
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

static uint32_t symbolic_reloc_type_for_machine(int machine) {
  if (machine == EM_AARCH64)
    return R_AARCH64_ABS64;
  if (machine == EM_RISCV)
    return R_RISCV_64;
  return 0;
}

static uint32_t jump_slot_reloc_type_for_machine(int machine) {
  if (machine == EM_AARCH64)
    return R_AARCH64_JUMP_SLOT;
  if (machine == EM_RISCV)
    return R_RISCV_JUMP_SLOT;
  return 0;
}

static uint32_t irelative_reloc_type_for_machine(int machine) {
  if (machine == EM_AARCH64)
    return R_AARCH64_IRELATIVE;
  if (machine == EM_RISCV)
    return R_RISCV_IRELATIVE;
  return 0;
}

static uint32_t none_reloc_type_for_machine(int machine) {
  if (machine == EM_AARCH64)
    return R_AARCH64_NONE;
  if (machine == EM_RISCV)
    return R_RISCV_NONE;
  return 0;
}

static uint32_t import_reloc_type_for_machine(int machine) {
  if (machine == EM_AARCH64)
    return R_AARCH64_GLOB_DAT;
  if (machine == EM_RISCV)
    return R_RISCV_64;
  return 0;
}

static uint64_t align_up_u64(uint64_t value, uint64_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

static unsigned char symbol_info(unsigned bind, unsigned type) {
  return (unsigned char) ((bind << 4) | (type & 0xf));
}

static void write_u32_le(unsigned char *bytes, uint32_t value) {
  bytes[0] = (unsigned char) (value & 0xff);
  bytes[1] = (unsigned char) ((value >> 8) & 0xff);
  bytes[2] = (unsigned char) ((value >> 16) & 0xff);
  bytes[3] = (unsigned char) ((value >> 24) & 0xff);
}

static int parse_text_token(const char *text, int machine, uint32_t *value, size_t *encoded_size) {
  if (strncmp(text, "h:", 2) == 0) {
    uint16_t parsed = 0;
    if (machine != EM_RISCV || parse_u16(text + 2, &parsed) < 0)
      return -1;
    *value = parsed;
    *encoded_size = 2;
    return 0;
  }

  if (parse_u32(text, value) < 0)
    return -1;
  *encoded_size = 4;
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s ARCH OUTPUT [--split-data64 VALUE|--dyn-none64 VALUE|--dyn-relative64 VALUE|--dyn-rel-relative64 VALUE|--dyn-relr64 VALUE|--dyn-relr-bitmap64 VALUE|--dyn-irelative64 VALUE|--dyn-symbol64 VALUE|--dyn-jump-slot64 VALUE|--dyn-rel-jump-slot64 VALUE|--dyn-import64 NAME|--dyn-import-func64 NAME] [--export NAME|--export-at NAME OFFSET|--export-dyntab NAME|--export-dyntab-at NAME OFFSET] INSN|h:HALFWORD...\n", argv[0]);
    return 2;
  }

  const int machine = machine_for_arch(argv[1]);
  if (!machine) {
    fprintf(stderr, "mkpolyelf: unsupported arch: %s\n", argv[1]);
    return 1;
  }

  int first_insn_arg = 3;
  int split_data = 0;
  int dyn_none = 0;
  int dyn_relative = 0;
  int dyn_rel_relative = 0;
  int dyn_relr = 0;
  int dyn_relr_bitmap = 0;
  int dyn_irelative = 0;
  int dyn_symbolic = 0;
  int dyn_jump_slot = 0;
  int dyn_rel_jump_slot = 0;
  int dyn_import = 0;
  int dyn_import_func = 0;
  uint64_t data_value = 0;
  if (strcmp(argv[3], "--split-data64") == 0 ||
      strcmp(argv[3], "--dyn-none64") == 0 ||
      strcmp(argv[3], "--dyn-relative64") == 0 ||
      strcmp(argv[3], "--dyn-rel-relative64") == 0 ||
      strcmp(argv[3], "--dyn-relr64") == 0 ||
      strcmp(argv[3], "--dyn-relr-bitmap64") == 0 ||
      strcmp(argv[3], "--dyn-irelative64") == 0 ||
      strcmp(argv[3], "--dyn-symbol64") == 0 ||
      strcmp(argv[3], "--dyn-jump-slot64") == 0 ||
      strcmp(argv[3], "--dyn-rel-jump-slot64") == 0 ||
      strcmp(argv[3], "--dyn-import64") == 0 ||
      strcmp(argv[3], "--dyn-import-func64") == 0) {
    dyn_import = strcmp(argv[3], "--dyn-import64") == 0;
    dyn_import_func = strcmp(argv[3], "--dyn-import-func64") == 0;
    if (argc < 7 || parse_u64(argv[4], &data_value) < 0) {
      if ((!dyn_import && !dyn_import_func) || argc < 6) {
        fprintf(stderr, "mkpolyelf: bad data option usage\n");
        return 2;
      }
    }
    split_data = 1;
    dyn_none = strcmp(argv[3], "--dyn-none64") == 0;
    dyn_relative = strcmp(argv[3], "--dyn-relative64") == 0;
    dyn_rel_relative = strcmp(argv[3], "--dyn-rel-relative64") == 0;
    dyn_relr = strcmp(argv[3], "--dyn-relr64") == 0;
    dyn_relr_bitmap = strcmp(argv[3], "--dyn-relr-bitmap64") == 0;
    dyn_irelative = strcmp(argv[3], "--dyn-irelative64") == 0;
    dyn_symbolic = strcmp(argv[3], "--dyn-symbol64") == 0 ||
      strcmp(argv[3], "--dyn-jump-slot64") == 0 ||
      strcmp(argv[3], "--dyn-rel-jump-slot64") == 0 ||
      dyn_import || dyn_import_func;
    dyn_jump_slot = strcmp(argv[3], "--dyn-jump-slot64") == 0 ||
      dyn_import_func;
    dyn_rel_jump_slot = strcmp(argv[3], "--dyn-rel-jump-slot64") == 0;
    first_insn_arg = 5;
  }
  const char *export_name = NULL;
  uint64_t export_offset = 0;
  int export_sections = 0;
  if (first_insn_arg + 2 <= argc && strcmp(argv[first_insn_arg], "--export") == 0) {
    export_name = argv[first_insn_arg + 1];
    if (export_name[0] == '\0') {
      fprintf(stderr, "mkpolyelf: bad export name\n");
      return 2;
    }
    export_sections = 1;
    first_insn_arg += 2;
  }
  else if (first_insn_arg + 3 <= argc &&
      strcmp(argv[first_insn_arg], "--export-at") == 0) {
    export_name = argv[first_insn_arg + 1];
    if (export_name[0] == '\0' ||
        parse_u64(argv[first_insn_arg + 2], &export_offset) < 0) {
      fprintf(stderr, "mkpolyelf: bad export-at usage\n");
      return 2;
    }
    export_sections = 1;
    first_insn_arg += 3;
  }
  else if (first_insn_arg + 2 <= argc &&
      strcmp(argv[first_insn_arg], "--export-dyntab") == 0) {
    export_name = argv[first_insn_arg + 1];
    if (export_name[0] == '\0') {
      fprintf(stderr, "mkpolyelf: bad export name\n");
      return 2;
    }
    first_insn_arg += 2;
  }
  else if (first_insn_arg + 3 <= argc &&
      strcmp(argv[first_insn_arg], "--export-dyntab-at") == 0) {
    export_name = argv[first_insn_arg + 1];
    if (export_name[0] == '\0' ||
        parse_u64(argv[first_insn_arg + 2], &export_offset) < 0) {
      fprintf(stderr, "mkpolyelf: bad export-dyntab-at usage\n");
      return 2;
    }
    first_insn_arg += 3;
  }
  if (first_insn_arg >= argc) {
    fprintf(stderr, "mkpolyelf: no instructions provided\n");
    return 2;
  }

  const int dyn_image = dyn_none || dyn_relative || dyn_rel_relative || dyn_relr ||
    dyn_relr_bitmap || dyn_irelative || dyn_symbolic;
  const uint64_t text_offset = 0x1000;
  const uint64_t text_vaddr = dyn_image ? 0 : 0x400000;
  const uint64_t data_offset = 0x3000;
  const uint64_t data_vaddr = dyn_image ? 0x2000 : 0x402000;
  const uint64_t dynamic_offset = data_offset + 0x100;
  const uint64_t dynamic_vaddr = data_vaddr + 0x100;
  const uint64_t rela_offset = data_offset + 0x200;
  const uint64_t rela_vaddr = data_vaddr + 0x200;
  uint64_t text_size = 0;
  for (int n = first_insn_arg; n < argc; n++) {
    uint32_t parsed = 0;
    size_t encoded_size = 0;
    if (parse_text_token(argv[n], machine, &parsed, &encoded_size) < 0) {
      fprintf(stderr, "mkpolyelf: bad instruction: %s\n", argv[n]);
      return 1;
    }
    text_size += encoded_size;
  }
  const uint64_t export_alignment_mask = machine == EM_RISCV ? 1 : 3;
  if (export_name && (export_offset >= text_size || (export_offset & export_alignment_mask) != 0)) {
    fprintf(stderr, "mkpolyelf: export offset is outside aligned text\n");
    return 2;
  }
  const int has_dynsym = export_name || dyn_symbolic;
  const int has_sections = export_name != NULL && export_sections;
  const char data_symbol_name[] = "poly_value";
  const char *import_symbol_name = (dyn_import || dyn_import_func) ?
    argv[4] : NULL;
  if ((dyn_import || dyn_import_func) && import_symbol_name[0] == '\0') {
    fprintf(stderr, "mkpolyelf: bad import symbol name\n");
    return 2;
  }
  const char *reloc_symbol_name = (dyn_import || dyn_import_func) ?
    import_symbol_name : data_symbol_name;
  const uint64_t export_name_offset = export_name ? 1 : 0;
  const uint64_t data_name_offset = dyn_symbolic ?
    1 + (export_name ? strlen(export_name) + 1 : 0) : 0;
  const uint64_t dynsym_count = 1 + (export_name ? 1 : 0) +
    (dyn_symbolic ? 1 : 0);
  const uint64_t data_symbol_index = 1 + (export_name ? 1 : 0);
  const uint64_t dynsym_size = dynsym_count * sizeof(struct elf64_sym);
  const uint64_t dynsym_offset = dyn_image && has_dynsym ?
    data_offset + 0x300 : 0x5000;
  const uint64_t dynsym_vaddr = dyn_image && has_dynsym ?
    data_vaddr + 0x300 : 0;
  const uint64_t dynstr_offset = dynsym_offset + dynsym_size;
  const uint64_t dynstr_vaddr = dynsym_vaddr ? dynsym_vaddr + dynsym_size : 0;
  const uint64_t dynstr_size = has_dynsym ?
    1 + (export_name ? strlen(export_name) + 1 : 0) +
    (dyn_symbolic ? strlen(reloc_symbol_name) + 1 : 0) : 0;
  const uint64_t dynhash_offset = has_dynsym ?
    align_up_u64(dynstr_offset + dynstr_size, 4) : 0;
  const uint64_t dynhash_vaddr = has_dynsym ?
    dynstr_vaddr + (dynhash_offset - dynstr_offset) : 0;
  const uint64_t dynhash_size = has_dynsym ?
    (2 + 1 + dynsym_count) * sizeof(uint32_t) : 0;
  const uint64_t data_size = dyn_image ?
    (has_dynsym ? 0x300 + dynsym_size + dynstr_size +
      (dynhash_offset - dynstr_offset - dynstr_size) + dynhash_size :
      0x200 + ((dyn_relr || dyn_relr_bitmap) ?
        (dyn_relr_bitmap ? 2 : 1) * sizeof(uint64_t) :
        (dyn_rel_relative ? sizeof(struct elf64_rel) :
          sizeof(struct elf64_rela)))) :
    (split_data ? 8 : 0);
  const uint64_t dynamic_count = dyn_image ? (has_dynsym ? 9 : 4) : 0;
  const uint64_t shstr_offset = align_up_u64(
    has_dynsym ? dynhash_offset + dynhash_size : dynstr_offset + dynstr_size,
    8);
  const char shstrtab[] = "\0.text\0.data\0.dynamic\0.rela.dyn\0.dynsym\0.dynstr\0.shstrtab\0";
  const uint64_t shstr_size = sizeof(shstrtab);
  const uint64_t shoff = align_up_u64(shstr_offset + shstr_size, 8);
  const uint16_t text_shndx = 1;
  const uint16_t data_shndx = 2;
  const uint16_t dynamic_shndx = 3;
  const uint16_t rela_shndx = 4;
  const uint16_t dynsym_shndx = 5;
  const uint16_t dynstr_shndx = 6;
  const uint16_t shstr_shndx = 7;
  const uint16_t shnum = has_sections ? 8 : 0;

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
  ehdr.e_type = dyn_image ? ET_DYN : ET_EXEC;
  ehdr.e_machine = (uint16_t) machine;
  ehdr.e_version = EV_CURRENT;
  ehdr.e_entry = text_vaddr;
  ehdr.e_phoff = sizeof(ehdr);
  ehdr.e_shoff = has_sections ? shoff : 0;
  ehdr.e_ehsize = sizeof(ehdr);
  ehdr.e_phentsize = sizeof(struct elf64_phdr);
  ehdr.e_phnum = dyn_image ? 3 : (split_data ? 2 : 1);
  ehdr.e_shentsize = has_sections ? sizeof(struct elf64_shdr) : 0;
  ehdr.e_shnum = shnum;
  ehdr.e_shstrndx = has_sections ? shstr_shndx : 0;

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
  if (dyn_image) {
    phdrs[2].p_type = PT_DYNAMIC;
    phdrs[2].p_flags = PF_R | PF_W;
    phdrs[2].p_offset = dynamic_offset;
    phdrs[2].p_vaddr = dynamic_vaddr;
    phdrs[2].p_paddr = dynamic_vaddr;
    phdrs[2].p_filesz = dynamic_count * sizeof(struct elf64_dyn);
    phdrs[2].p_memsz = dynamic_count * sizeof(struct elf64_dyn);
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
    size_t encoded_size = 0;
    if (parse_text_token(argv[n], machine, &insn, &encoded_size) < 0) {
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
    if (fwrite(bytes, encoded_size, 1, out) != 1) {
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
    uint64_t first_data_value = dyn_none ? data_value :
      ((dyn_rel_relative || dyn_relr || dyn_relr_bitmap) ? data_vaddr + 8 :
        (dyn_image ? 0 : data_value));
    for (unsigned n = 0; n < sizeof(bytes); n++)
      bytes[n] = (unsigned char) ((first_data_value >> (n * 8)) & 0xff);
    if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
      fprintf(stderr, "mkpolyelf: data write failed\n");
      fclose(out);
      return 1;
    }
    if (dyn_image) {
      uint64_t second_data_value = dyn_relr_bitmap ? data_vaddr + 16 :
        data_value;
      for (unsigned n = 0; n < sizeof(bytes); n++)
        bytes[n] = (unsigned char) ((second_data_value >> (n * 8)) & 0xff);
      if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: data write failed\n");
        fclose(out);
        return 1;
      }
      if (dyn_relr_bitmap) {
        for (unsigned n = 0; n < sizeof(bytes); n++)
          bytes[n] = (unsigned char) ((data_value >> (n * 8)) & 0xff);
        if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
          fprintf(stderr, "mkpolyelf: data write failed\n");
          fclose(out);
          return 1;
        }
      }

      if (fseek(out, (long) dynamic_offset, SEEK_SET) != 0) {
        fprintf(stderr, "mkpolyelf: dynamic seek failed\n");
        fclose(out);
        return 1;
      }
      struct elf64_dyn dyn[9];
      memset(dyn, 0, sizeof(dyn));
      dyn[0].d_tag = (dyn_relr || dyn_relr_bitmap) ? DT_RELR :
        dyn_rel_relative ? DT_REL :
        (dyn_jump_slot || dyn_rel_jump_slot) ? DT_JMPREL : DT_RELA;
      dyn[0].d_val = rela_vaddr;
      dyn[1].d_tag = (dyn_relr || dyn_relr_bitmap) ? DT_RELRSZ :
        dyn_rel_relative ? DT_RELSZ :
        (dyn_jump_slot || dyn_rel_jump_slot) ? DT_PLTRELSZ : DT_RELASZ;
      dyn[1].d_val = (dyn_relr || dyn_relr_bitmap) ?
        (dyn_relr_bitmap ? 2 : 1) * sizeof(uint64_t) :
        ((dyn_rel_relative || dyn_rel_jump_slot) ? sizeof(struct elf64_rel) :
          sizeof(struct elf64_rela));
      dyn[2].d_tag = (dyn_relr || dyn_relr_bitmap) ? DT_RELRENT :
        dyn_rel_relative ? DT_RELENT :
        (dyn_jump_slot || dyn_rel_jump_slot) ? DT_PLTREL : DT_RELAENT;
      dyn[2].d_val = (dyn_relr || dyn_relr_bitmap) ? sizeof(uint64_t) :
        dyn_rel_relative ? sizeof(struct elf64_rel) :
        dyn_rel_jump_slot ? DT_REL :
        dyn_jump_slot ? DT_RELA : sizeof(struct elf64_rela);
      if (has_dynsym) {
        dyn[3].d_tag = DT_SYMTAB;
        dyn[3].d_val = dynsym_vaddr;
        dyn[4].d_tag = DT_SYMENT;
        dyn[4].d_val = sizeof(struct elf64_sym);
        dyn[5].d_tag = DT_STRTAB;
        dyn[5].d_val = dynstr_vaddr;
        dyn[6].d_tag = DT_STRSZ;
        dyn[6].d_val = dynstr_size;
        dyn[7].d_tag = DT_HASH;
        dyn[7].d_val = dynhash_vaddr;
        dyn[8].d_tag = DT_NULL;
      }
      else {
        dyn[3].d_tag = DT_NULL;
      }
      if (fwrite(dyn, sizeof(struct elf64_dyn), dynamic_count, out) != dynamic_count) {
        fprintf(stderr, "mkpolyelf: dynamic write failed\n");
        fclose(out);
        return 1;
      }

      if (fseek(out, (long) rela_offset, SEEK_SET) != 0) {
        fprintf(stderr, "mkpolyelf: rela seek failed\n");
        fclose(out);
        return 1;
      }
      if (dyn_relr || dyn_relr_bitmap) {
        for (unsigned n = 0; n < sizeof(bytes); n++)
          bytes[n] = (unsigned char) ((data_vaddr >> (n * 8)) & 0xff);
        if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
          fprintf(stderr, "mkpolyelf: relr write failed\n");
          fclose(out);
          return 1;
        }
        if (dyn_relr_bitmap) {
          const uint64_t bitmap_entry = 3;
          for (unsigned n = 0; n < sizeof(bytes); n++)
            bytes[n] = (unsigned char) ((bitmap_entry >> (n * 8)) & 0xff);
          if (fwrite(bytes, sizeof(bytes), 1, out) != 1) {
            fprintf(stderr, "mkpolyelf: relr write failed\n");
            fclose(out);
            return 1;
          }
        }
        goto wrote_dynamic_relocs;
      }
      if (dyn_rel_relative || dyn_rel_jump_slot) {
        struct elf64_rel rel;
        memset(&rel, 0, sizeof(rel));
        rel.r_offset = data_vaddr;
        rel.r_info = dyn_rel_jump_slot ?
          ((data_symbol_index << 32) | jump_slot_reloc_type_for_machine(machine)) :
          (uint64_t) relative_reloc_type_for_machine(machine);
        if (fwrite(&rel, sizeof(rel), 1, out) != 1) {
          fprintf(stderr, "mkpolyelf: rel write failed\n");
          fclose(out);
          return 1;
        }
        goto wrote_dynamic_relocs;
      }
      struct elf64_rela rela;
      memset(&rela, 0, sizeof(rela));
      rela.r_offset = data_vaddr;
      if (dyn_symbolic) {
        rela.r_info = (data_symbol_index << 32) |
          (dyn_import ? import_reloc_type_for_machine(machine) :
            dyn_jump_slot ? jump_slot_reloc_type_for_machine(machine) :
            symbolic_reloc_type_for_machine(machine));
        rela.r_addend = 0;
      }
      else if (dyn_irelative) {
        rela.r_info = (uint64_t) irelative_reloc_type_for_machine(machine);
        rela.r_addend = (int64_t) text_vaddr;
      }
      else if (dyn_none) {
        rela.r_info = (uint64_t) none_reloc_type_for_machine(machine);
        rela.r_addend = 0;
      }
      else {
        rela.r_info = (uint64_t) relative_reloc_type_for_machine(machine);
        rela.r_addend = (int64_t) (data_vaddr + 8);
      }
      if (fwrite(&rela, sizeof(rela), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: rela write failed\n");
        fclose(out);
        return 1;
      }
wrote_dynamic_relocs:
      ;
    }
  }

  if (has_dynsym) {
    if (fseek(out, (long) dynsym_offset, SEEK_SET) != 0) {
      fprintf(stderr, "mkpolyelf: dynsym seek failed\n");
      fclose(out);
      return 1;
    }
    struct elf64_sym symbols[3];
    memset(symbols, 0, sizeof(symbols));
    symbols[0].st_info = symbol_info(STB_LOCAL, STT_NOTYPE);
    if (export_name) {
      symbols[1].st_name = (uint32_t) export_name_offset;
      symbols[1].st_info = symbol_info(STB_GLOBAL, STT_FUNC);
      symbols[1].st_other = 0;
      symbols[1].st_shndx = text_shndx;
      symbols[1].st_value = text_vaddr + export_offset;
      symbols[1].st_size = text_size - export_offset;
    }
    if (dyn_symbolic) {
      symbols[data_symbol_index].st_name = (uint32_t) data_name_offset;
      symbols[data_symbol_index].st_info = symbol_info(STB_GLOBAL,
        dyn_import_func ? STT_FUNC : STT_OBJECT);
      symbols[data_symbol_index].st_other = 0;
      symbols[data_symbol_index].st_shndx =
        (dyn_import || dyn_import_func) ? SHN_UNDEF : data_shndx;
      symbols[data_symbol_index].st_value =
        (dyn_import || dyn_import_func) ? 0 : data_vaddr + 8;
      symbols[data_symbol_index].st_size =
        (dyn_import || dyn_import_func) ? 0 : 8;
    }
    if (fwrite(symbols, sizeof(struct elf64_sym), dynsym_count, out) != dynsym_count) {
      fprintf(stderr, "mkpolyelf: dynsym write failed\n");
      fclose(out);
      return 1;
    }

    if (fseek(out, (long) dynstr_offset, SEEK_SET) != 0) {
      fprintf(stderr, "mkpolyelf: dynstr seek failed\n");
      fclose(out);
      return 1;
    }
    if (fputc('\0', out) == EOF) {
      fprintf(stderr, "mkpolyelf: dynstr write failed\n");
      fclose(out);
      return 1;
    }
    if (export_name &&
        fwrite(export_name, strlen(export_name) + 1, 1, out) != 1) {
      fprintf(stderr, "mkpolyelf: dynstr write failed\n");
      fclose(out);
      return 1;
    }
    if (dyn_symbolic &&
        fwrite(reloc_symbol_name, strlen(reloc_symbol_name) + 1, 1, out) != 1) {
      fprintf(stderr, "mkpolyelf: dynstr write failed\n");
      fclose(out);
      return 1;
    }

    if (dyn_image) {
      if (fseek(out, (long) dynhash_offset, SEEK_SET) != 0) {
        fprintf(stderr, "mkpolyelf: dynhash seek failed\n");
        fclose(out);
        return 1;
      }
      unsigned char hash_word[4];
      write_u32_le(hash_word, 1);
      if (fwrite(hash_word, sizeof(hash_word), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: dynhash write failed\n");
        fclose(out);
        return 1;
      }
      write_u32_le(hash_word, (uint32_t) dynsym_count);
      if (fwrite(hash_word, sizeof(hash_word), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: dynhash write failed\n");
        fclose(out);
        return 1;
      }
      write_u32_le(hash_word, dynsym_count > 1 ? 1 : 0);
      if (fwrite(hash_word, sizeof(hash_word), 1, out) != 1) {
        fprintf(stderr, "mkpolyelf: dynhash write failed\n");
        fclose(out);
        return 1;
      }
      for (uint64_t n = 0; n < dynsym_count; n++) {
        uint32_t chain = 0;
        if (n > 0 && n + 1 < dynsym_count)
          chain = (uint32_t) (n + 1);
        write_u32_le(hash_word, chain);
        if (fwrite(hash_word, sizeof(hash_word), 1, out) != 1) {
          fprintf(stderr, "mkpolyelf: dynhash write failed\n");
          fclose(out);
          return 1;
        }
      }
    }
  }

  if (has_sections) {
    if (fseek(out, (long) shstr_offset, SEEK_SET) != 0) {
      fprintf(stderr, "mkpolyelf: shstrtab seek failed\n");
      fclose(out);
      return 1;
    }
    if (fwrite(shstrtab, sizeof(shstrtab), 1, out) != 1) {
      fprintf(stderr, "mkpolyelf: shstrtab write failed\n");
      fclose(out);
      return 1;
    }

    if (fseek(out, (long) shoff, SEEK_SET) != 0) {
      fprintf(stderr, "mkpolyelf: section header seek failed\n");
      fclose(out);
      return 1;
    }
    struct elf64_shdr sections[8];
    memset(sections, 0, sizeof(sections));
    sections[text_shndx].sh_name = 1;
    sections[text_shndx].sh_type = SHT_PROGBITS;
    sections[text_shndx].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    sections[text_shndx].sh_addr = text_vaddr;
    sections[text_shndx].sh_offset = text_offset;
    sections[text_shndx].sh_size = text_size;
    sections[text_shndx].sh_addralign = 4;
    sections[data_shndx].sh_name = 7;
    sections[data_shndx].sh_type = SHT_PROGBITS;
    sections[data_shndx].sh_flags = SHF_ALLOC;
    sections[data_shndx].sh_addr = data_vaddr;
    sections[data_shndx].sh_offset = data_offset;
    sections[data_shndx].sh_size = data_size;
    sections[data_shndx].sh_addralign = 8;
    sections[dynamic_shndx].sh_name = 13;
    sections[dynamic_shndx].sh_type = SHT_DYNAMIC;
    sections[dynamic_shndx].sh_flags = SHF_ALLOC;
    sections[dynamic_shndx].sh_addr = dynamic_vaddr;
    sections[dynamic_shndx].sh_offset = dynamic_offset;
    sections[dynamic_shndx].sh_size = dynamic_count * sizeof(struct elf64_dyn);
    sections[dynamic_shndx].sh_link = dynstr_shndx;
    sections[dynamic_shndx].sh_addralign = 8;
    sections[dynamic_shndx].sh_entsize = sizeof(struct elf64_dyn);
    sections[rela_shndx].sh_name = 22;
    sections[rela_shndx].sh_type =
      (dyn_rel_relative || dyn_rel_jump_slot) ? SHT_REL : SHT_RELA;
    sections[rela_shndx].sh_flags = SHF_ALLOC;
    sections[rela_shndx].sh_addr = rela_vaddr;
    sections[rela_shndx].sh_offset = rela_offset;
    sections[rela_shndx].sh_size = (dyn_rel_relative || dyn_rel_jump_slot) ?
      sizeof(struct elf64_rel) : sizeof(struct elf64_rela);
    sections[rela_shndx].sh_link = dynsym_shndx;
    sections[rela_shndx].sh_addralign = 8;
    sections[rela_shndx].sh_entsize = (dyn_rel_relative || dyn_rel_jump_slot) ?
      sizeof(struct elf64_rel) : sizeof(struct elf64_rela);
    sections[dynsym_shndx].sh_name = 32;
    sections[dynsym_shndx].sh_type = SHT_DYNSYM;
    sections[dynsym_shndx].sh_flags = dyn_image ? SHF_ALLOC : 0;
    sections[dynsym_shndx].sh_addr = dynsym_vaddr;
    sections[dynsym_shndx].sh_offset = dynsym_offset;
    sections[dynsym_shndx].sh_size = dynsym_size;
    sections[dynsym_shndx].sh_link = dynstr_shndx;
    sections[dynsym_shndx].sh_info = 1;
    sections[dynsym_shndx].sh_addralign = 8;
    sections[dynsym_shndx].sh_entsize = sizeof(struct elf64_sym);
    sections[dynstr_shndx].sh_name = 40;
    sections[dynstr_shndx].sh_type = SHT_STRTAB;
    sections[dynstr_shndx].sh_flags = dyn_image ? SHF_ALLOC : 0;
    sections[dynstr_shndx].sh_addr = dynstr_vaddr;
    sections[dynstr_shndx].sh_offset = dynstr_offset;
    sections[dynstr_shndx].sh_size = dynstr_size;
    sections[dynstr_shndx].sh_addralign = 1;
    sections[shstr_shndx].sh_name = 48;
    sections[shstr_shndx].sh_type = SHT_STRTAB;
    sections[shstr_shndx].sh_offset = shstr_offset;
    sections[shstr_shndx].sh_size = shstr_size;
    sections[shstr_shndx].sh_addralign = 1;
    if (fwrite(sections, sizeof(sections), 1, out) != 1) {
      fprintf(stderr, "mkpolyelf: section header write failed\n");
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
