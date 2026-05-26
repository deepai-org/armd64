#include <errno.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

enum {
  POLY_ARCH_AARCH64 = 1,
  POLY_ARCH_RISCV = 2,
  MAX_PROGRAM_BYTES = 1024 * 1024,
  MAX_DYNAMIC_RELOCS = 4096,
  RELOC_BASE_ABSOLUTE = 0,
  RELOC_BASE_LOAD_BIAS = 1,
  RELOC_BASE_IMPORT_PAGE = 2,
  RELOC_BASE_IMPORT_CALL = 3
};

static const uint64_t POLY_IMPORT_CALL_COOKIE = 0xffffffffffffe000ULL;

struct poly_dynamic_reloc {
  size_t offset;
  uint64_t value;
  int base_kind;
};

struct poly_symbol_table {
  const Elf64_Sym *symbols;
  size_t symbol_count;
  const char *strings;
  size_t strings_size;
};

struct poly_program {
  const char *path;
  const char *arch_name;
  int arch;
  int elf_type;
  uint8_t *image;
  size_t image_size;
  size_t entry_offset;
  uint64_t base_vaddr;
  size_t loaded_bytes;
  struct poly_dynamic_reloc *relocs;
  size_t reloc_count;
};

struct poly_request {
  char path[160];
  char symbol[96];
  uint64_t expected;
  int check_expected;
};

static int parse_u64(const char *text, uint64_t *value) {
  char *end = NULL;
  errno = 0;
  unsigned long long parsed = strtoull(text, &end, 0);
  if (errno || end == text || *end != '\0')
    return -1;
  *value = (uint64_t) parsed;
  return 0;
}

static int parse_request(const char *arg, struct poly_request *request) {
  memset(request, 0, sizeof(*request));
  const char *expected = strchr(arg, '=');
  size_t path_len = expected ? (size_t) (expected - arg) : strlen(arg);
  const char *symbol = memchr(arg, '#', path_len);
  size_t symbol_len = 0;
  if (symbol) {
    symbol_len = path_len - (size_t) (symbol - arg) - 1;
    path_len = (size_t) (symbol - arg);
  }
  if (path_len == 0 || path_len >= sizeof(request->path)) {
    fprintf(stderr, "POLYCALL_FAIL: bad argument: %s\n", arg);
    return -1;
  }
  if (symbol && (symbol_len == 0 || symbol_len >= sizeof(request->symbol))) {
    fprintf(stderr, "POLYCALL_FAIL: bad symbol argument: %s\n", arg);
    return -1;
  }

  memcpy(request->path, arg, path_len);
  request->path[path_len] = '\0';
  if (symbol) {
    memcpy(request->symbol, symbol + 1, symbol_len);
    request->symbol[symbol_len] = '\0';
  }
  if (expected) {
    if (parse_u64(expected + 1, &request->expected) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: bad expected value: %s\n", arg);
      return -1;
    }
    request->check_expected = 1;
  }
  return 0;
}

static int read_file(const char *path, unsigned char **data, size_t *size) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "POLYCALL_FAIL: unable to open %s: %s\n", path, strerror(errno));
    return -1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "POLYCALL_FAIL: unable to seek %s\n", path);
    fclose(file);
    return -1;
  }
  long file_size = ftell(file);
  if (file_size <= 0) {
    fprintf(stderr, "POLYCALL_FAIL: unable to size %s\n", path);
    fclose(file);
    return -1;
  }
  rewind(file);

  unsigned char *buffer = malloc((size_t) file_size);
  if (!buffer) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory reading %s\n", path);
    fclose(file);
    return -1;
  }
  if (fread(buffer, 1, (size_t) file_size, file) != (size_t) file_size) {
    fprintf(stderr, "POLYCALL_FAIL: unable to read %s\n", path);
    free(buffer);
    fclose(file);
    return -1;
  }
  fclose(file);
  *data = buffer;
  *size = (size_t) file_size;
  return 0;
}

static void emit_u32(uint8_t *code, size_t *offset, uint32_t value) {
  code[(*offset)++] = (uint8_t) (value & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 8) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 16) & 0xff);
  code[(*offset)++] = (uint8_t) ((value >> 24) & 0xff);
}

static uint64_t align_down_u64(uint64_t value, uint64_t alignment) {
  return value & ~(alignment - 1);
}

static void emit_u64(uint8_t *code, size_t *offset, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    code[(*offset)++] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static void write_le64(uint8_t *bytes, uint64_t value) {
  for (unsigned n = 0; n < 8; n++)
    bytes[n] = (uint8_t) ((value >> (n * 8)) & 0xff);
}

static void emit_movabs_r10(uint8_t *code, size_t *offset, uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xba;
  emit_u64(code, offset, value);
}

static void emit_movabs_r11(uint8_t *code, size_t *offset, uint64_t value) {
  code[(*offset)++] = 0x49;
  code[(*offset)++] = 0xbb;
  emit_u64(code, offset, value);
}

static int detect_arch(uint16_t machine, struct poly_program *program) {
  if (machine == EM_AARCH64) {
    program->arch = POLY_ARCH_AARCH64;
    program->arch_name = "aarch64";
    return 0;
  }
  if (machine == EM_RISCV) {
    program->arch = POLY_ARCH_RISCV;
    program->arch_name = "riscv";
    return 0;
  }
  return -1;
}

static int check_elf_table(uint64_t offset, uint64_t count, uint64_t entsize,
    size_t file_size) {
  if (offset > file_size || entsize == 0 || count > (UINT64_MAX / entsize) ||
      count * entsize > file_size - offset)
    return -1;
  return 0;
}

static int resolve_elf_symbol_from_sections(const unsigned char *data,
    size_t size, const Elf64_Ehdr *ehdr, const char *symbol_name,
    uint64_t *symbol_vaddr) {
  if (ehdr->e_shentsize < sizeof(Elf64_Shdr) ||
      check_elf_table(ehdr->e_shoff, ehdr->e_shnum, ehdr->e_shentsize, size) < 0)
    return -1;

  const Elf64_Shdr *sections = (const Elf64_Shdr *) (data + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *symtab = &sections[n];
    if (symtab->sh_type != SHT_DYNSYM && symtab->sh_type != SHT_SYMTAB)
      continue;
    if (symtab->sh_entsize < sizeof(Elf64_Sym) ||
        symtab->sh_link >= ehdr->e_shnum ||
        symtab->sh_offset > size ||
        symtab->sh_size > size - symtab->sh_offset ||
        symtab->sh_size % symtab->sh_entsize != 0)
      continue;

    const Elf64_Shdr *strtab = &sections[symtab->sh_link];
    if (strtab->sh_type != SHT_STRTAB ||
        strtab->sh_offset > size ||
        strtab->sh_size > size - strtab->sh_offset)
      continue;

    const Elf64_Sym *symbols = (const Elf64_Sym *) (data + symtab->sh_offset);
    const char *strings = (const char *) (data + strtab->sh_offset);
    const size_t symbol_count = (size_t) (symtab->sh_size / symtab->sh_entsize);
    for (size_t s = 0; s < symbol_count; s++) {
      const Elf64_Sym *sym = (const Elf64_Sym *) ((const unsigned char *) symbols + s * symtab->sh_entsize);
      const unsigned type = ELF64_ST_TYPE(sym->st_info);
      if (sym->st_name >= strtab->sh_size ||
          sym->st_shndx == SHN_UNDEF ||
          (type != STT_FUNC && type != STT_NOTYPE))
        continue;
      if (strcmp(strings + sym->st_name, symbol_name) == 0) {
        *symbol_vaddr = sym->st_value;
        return 0;
      }
    }
  }

  return -1;
}

static int elf_vaddr_to_image_offset(const struct poly_program *program,
    uint64_t vaddr, uint64_t size, size_t *offset) {
  if (vaddr < program->base_vaddr || size > program->image_size)
    return -1;
  const uint64_t image_offset = vaddr - program->base_vaddr;
  if (image_offset > program->image_size - size)
    return -1;
  *offset = (size_t) image_offset;
  return 0;
}

static uint32_t relative_reloc_type_for_arch(int arch) {
  if (arch == POLY_ARCH_AARCH64)
    return R_AARCH64_RELATIVE;
  if (arch == POLY_ARCH_RISCV)
    return R_RISCV_RELATIVE;
  return UINT32_MAX;
}

static int symbolic_64_reloc_type_for_arch(int arch, uint32_t type) {
  if (arch == POLY_ARCH_AARCH64)
    return type == R_AARCH64_ABS64 || type == R_AARCH64_GLOB_DAT ||
      type == R_AARCH64_JUMP_SLOT;
  if (arch == POLY_ARCH_RISCV)
    return type == R_RISCV_64 || type == R_RISCV_JUMP_SLOT;
  return 0;
}

static const uint64_t poly_import_value = 123;

static int append_dynamic_reloc(struct poly_program *program, size_t offset,
    uint64_t value, int base_kind) {
  if (program->reloc_count >= MAX_DYNAMIC_RELOCS) {
    fprintf(stderr, "POLYCALL_FAIL: too many dynamic relocations: %s\n",
      program->path);
    return -1;
  }

  struct poly_dynamic_reloc *relocs = realloc(program->relocs,
    (program->reloc_count + 1) * sizeof(*program->relocs));
  if (!relocs) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory reading relocations: %s\n",
      program->path);
    return -1;
  }
  program->relocs = relocs;
  program->relocs[program->reloc_count].offset = offset;
  program->relocs[program->reloc_count].value = value;
  program->relocs[program->reloc_count].base_kind = base_kind;
  program->reloc_count++;
  return 0;
}

static int load_dynsym_from_sections(const unsigned char *data, size_t size,
    const Elf64_Ehdr *ehdr, struct poly_symbol_table *table) {
  memset(table, 0, sizeof(*table));
  if (ehdr->e_shentsize < sizeof(Elf64_Shdr) ||
      check_elf_table(ehdr->e_shoff, ehdr->e_shnum, ehdr->e_shentsize, size) < 0)
    return -1;

  const Elf64_Shdr *sections = (const Elf64_Shdr *) (data + ehdr->e_shoff);
  for (uint16_t n = 0; n < ehdr->e_shnum; n++) {
    const Elf64_Shdr *symtab = &sections[n];
    if (symtab->sh_type != SHT_DYNSYM)
      continue;
    if (symtab->sh_entsize < sizeof(Elf64_Sym) ||
        symtab->sh_link >= ehdr->e_shnum ||
        symtab->sh_offset > size ||
        symtab->sh_size > size - symtab->sh_offset ||
        symtab->sh_size % symtab->sh_entsize != 0)
      return -1;

    const Elf64_Shdr *strtab = &sections[symtab->sh_link];
    if (strtab->sh_type != SHT_STRTAB ||
        strtab->sh_offset > size ||
        strtab->sh_size > size - strtab->sh_offset)
      return -1;

    table->symbols = (const Elf64_Sym *) (data + symtab->sh_offset);
    table->symbol_count = (size_t) (symtab->sh_size / symtab->sh_entsize);
    table->strings = (const char *) (data + strtab->sh_offset);
    table->strings_size = (size_t) strtab->sh_size;
    return 0;
  }
  return -1;
}

static int load_dynsym_from_dynamic(const struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count, struct poly_symbol_table *table) {
  uint64_t symtab_vaddr = 0;
  uint64_t strtab_vaddr = 0;
  uint64_t hash_vaddr = 0;
  uint64_t strsz = 0;
  uint64_t syment = sizeof(Elf64_Sym);

  memset(table, 0, sizeof(*table));
  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_NULL:
        n = dyn_count;
        break;
      case DT_SYMTAB:
        symtab_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_STRTAB:
        strtab_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_HASH:
        hash_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_STRSZ:
        strsz = dyn[n].d_un.d_val;
        break;
      case DT_SYMENT:
        syment = dyn[n].d_un.d_val;
        break;
      default:
        break;
    }
  }

  if (!symtab_vaddr || !strtab_vaddr || strsz == 0 ||
      syment != sizeof(Elf64_Sym))
    return -1;

  size_t symtab_offset = 0;
  size_t strtab_offset = 0;
  size_t symbol_count = 0;
  if (elf_vaddr_to_image_offset(program, symtab_vaddr, sizeof(Elf64_Sym),
        &symtab_offset) < 0 ||
      elf_vaddr_to_image_offset(program, strtab_vaddr, strsz,
        &strtab_offset) < 0)
    return -1;
  if (hash_vaddr) {
    size_t hash_offset = 0;
    if (elf_vaddr_to_image_offset(program, hash_vaddr, 8, &hash_offset) < 0)
      return -1;
    const uint32_t *hash = (const uint32_t *) (program->image + hash_offset);
    const uint32_t nbucket = hash[0];
    const uint32_t nchain = hash[1];
    const uint64_t hash_size = (uint64_t) (2 + nbucket + nchain) *
      sizeof(uint32_t);
    if (nbucket == 0 || nchain == 0 ||
        elf_vaddr_to_image_offset(program, hash_vaddr, hash_size,
          &hash_offset) < 0)
      return -1;
    symbol_count = nchain;
  }
  else {
    if (strtab_offset <= symtab_offset ||
        (strtab_offset - symtab_offset) % sizeof(Elf64_Sym) != 0)
      return -1;
    symbol_count = (strtab_offset - symtab_offset) / sizeof(Elf64_Sym);
  }
  if (symbol_count > (SIZE_MAX / sizeof(Elf64_Sym)) ||
      elf_vaddr_to_image_offset(program, symtab_vaddr,
        (uint64_t) symbol_count * sizeof(Elf64_Sym), &symtab_offset) < 0)
    return -1;

  table->symbols = (const Elf64_Sym *) (program->image + symtab_offset);
  table->symbol_count = symbol_count;
  table->strings = (const char *) (program->image + strtab_offset);
  table->strings_size = (size_t) strsz;
  return 0;
}

static int resolve_symbol_from_table(const struct poly_symbol_table *table,
    const char *symbol_name, uint64_t *symbol_vaddr) {
  if (!table->symbols || !table->strings || !symbol_name)
    return -1;

  for (size_t s = 0; s < table->symbol_count; s++) {
    const Elf64_Sym *sym = &table->symbols[s];
    const unsigned type = ELF64_ST_TYPE(sym->st_info);
    if (sym->st_name >= table->strings_size ||
        sym->st_shndx == SHN_UNDEF ||
        (type != STT_FUNC && type != STT_NOTYPE))
      continue;
    if (strcmp(table->strings + sym->st_name, symbol_name) == 0) {
      *symbol_vaddr = sym->st_value;
      return 0;
    }
  }
  return -1;
}

static int resolve_dynamic_symbol(const struct poly_program *program,
    const Elf64_Dyn *dyn, size_t dyn_count, const char *symbol_name,
    uint64_t *symbol_vaddr) {
  struct poly_symbol_table table;
  if (load_dynsym_from_dynamic(program, dyn, dyn_count, &table) < 0)
    return -1;
  return resolve_symbol_from_table(&table, symbol_name, symbol_vaddr);
}

static int resolve_external_reloc_symbol(const struct poly_program *program,
    const char *symbol_name, uint64_t *symbol_value, int *base_kind) {
  if (strcmp(symbol_name, "poly_import_value") == 0) {
    *symbol_value = 0;
    *base_kind = RELOC_BASE_IMPORT_PAGE;
    return 0;
  }
  if (strcmp(symbol_name, "poly_import_add") == 0) {
    *symbol_value = 0;
    *base_kind = RELOC_BASE_IMPORT_CALL;
    return 0;
  }

  fprintf(stderr, "POLYCALL_FAIL: unresolved external relocation symbol=%s path=%s\n",
    symbol_name, program->path);
  return -1;
}

static int resolve_reloc_symbol(const struct poly_program *program,
    const struct poly_symbol_table *table, uint64_t symbol_index,
    uint64_t *symbol_value, int *base_kind) {
  if (!table->symbols || symbol_index >= table->symbol_count) {
    fprintf(stderr, "POLYCALL_FAIL: relocation symbol table missing: %s\n",
      program->path);
    return -1;
  }

  const Elf64_Sym *sym = &table->symbols[symbol_index];
  if (sym->st_name >= table->strings_size) {
    fprintf(stderr, "POLYCALL_FAIL: bad relocation symbol name index=%llu path=%s\n",
      (unsigned long long) symbol_index, program->path);
    return -1;
  }
  const char *symbol_name = table->strings + sym->st_name;
  if (sym->st_shndx == SHN_UNDEF) {
    *base_kind = RELOC_BASE_ABSOLUTE;
    return resolve_external_reloc_symbol(program, symbol_name, symbol_value,
      base_kind);
  }
  size_t symbol_offset = 0;
  if (elf_vaddr_to_image_offset(program, sym->st_value, 1, &symbol_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: relocation symbol escaped image: %s\n",
      program->path);
    return -1;
  }

  *symbol_value = sym->st_value;
  *base_kind = RELOC_BASE_LOAD_BIAS;
  return 0;
}

static int process_rela_table(struct poly_program *program,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    const Elf64_Dyn *dyn, size_t dyn_count, uint64_t rela_vaddr,
    uint64_t rela_size, const char *label) {
  size_t rela_offset = 0;
  if (elf_vaddr_to_image_offset(program, rela_vaddr, rela_size, &rela_offset) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: %s table out of loaded image: %s\n",
      label, program->path);
    return -1;
  }

  const Elf64_Rela *rela = (const Elf64_Rela *) (program->image + rela_offset);
  const size_t rela_count = (size_t) (rela_size / sizeof(Elf64_Rela));
  const uint32_t relative_type = relative_reloc_type_for_arch(program->arch);
  struct poly_symbol_table dynsym;
  memset(&dynsym, 0, sizeof(dynsym));
  for (size_t n = 0; n < rela_count; n++) {
    const uint64_t symbol_index = ELF64_R_SYM(rela[n].r_info);
    const uint32_t reloc_type = ELF64_R_TYPE(rela[n].r_info);
    uint64_t reloc_value = 0;
    int base_kind = RELOC_BASE_LOAD_BIAS;
    if (symbol_index == 0 && reloc_type == relative_type) {
      reloc_value = (uint64_t) rela[n].r_addend;
    }
    else if (symbol_index != 0 &&
        symbolic_64_reloc_type_for_arch(program->arch, reloc_type)) {
      if (!dynsym.symbols &&
          load_dynsym_from_dynamic(program, dyn, dyn_count, &dynsym) < 0 &&
          load_dynsym_from_sections(data, size, ehdr, &dynsym) < 0) {
        fprintf(stderr, "POLYCALL_FAIL: symbolic relocations require dynsym metadata: %s\n",
          program->path);
        return -1;
      }
      uint64_t symbol_value = 0;
      base_kind = RELOC_BASE_LOAD_BIAS;
      if (resolve_reloc_symbol(program, &dynsym, symbol_index,
            &symbol_value, &base_kind) < 0)
        return -1;
      reloc_value = symbol_value + (uint64_t) rela[n].r_addend;
    }
    else {
      fprintf(stderr, "POLYCALL_FAIL: unsupported dynamic relocation type=%llu sym=%llu path=%s\n",
        (unsigned long long) reloc_type,
        (unsigned long long) symbol_index,
        program->path);
      return -1;
    }
    size_t relocation_offset = 0;
    if (elf_vaddr_to_image_offset(program, rela[n].r_offset, 8,
          &relocation_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: relocation target out of image: %s\n",
        program->path);
      return -1;
    }
    if (append_dynamic_reloc(program, relocation_offset, reloc_value,
          base_kind) < 0)
      return -1;
  }
  return 0;
}

static int load_dynamic_relocs(struct poly_program *program,
    const unsigned char *data, size_t size, const Elf64_Ehdr *ehdr,
    const Elf64_Dyn *dyn, size_t dyn_count) {
  uint64_t rela_vaddr = 0;
  uint64_t rela_size = 0;
  uint64_t rela_ent = sizeof(Elf64_Rela);
  uint64_t jmprel_vaddr = 0;
  uint64_t pltrel_size = 0;
  uint64_t pltrel_type = 0;
  int saw_rel = 0;
  int saw_rela = 0;

  for (size_t n = 0; n < dyn_count; n++) {
    switch (dyn[n].d_tag) {
      case DT_NULL:
        n = dyn_count;
        break;
      case DT_RELA:
        rela_vaddr = dyn[n].d_un.d_ptr;
        saw_rela = 1;
        break;
      case DT_RELASZ:
        rela_size = dyn[n].d_un.d_val;
        break;
      case DT_RELAENT:
        rela_ent = dyn[n].d_un.d_val;
        break;
      case DT_JMPREL:
        jmprel_vaddr = dyn[n].d_un.d_ptr;
        break;
      case DT_PLTRELSZ:
        pltrel_size = dyn[n].d_un.d_val;
        break;
      case DT_PLTREL:
        pltrel_type = dyn[n].d_un.d_val;
        break;
      case DT_REL:
      case DT_RELSZ:
      case DT_RELENT:
        saw_rel = 1;
        break;
      default:
        break;
    }
  }

  if (saw_rel || pltrel_type == DT_REL) {
    fprintf(stderr, "POLYCALL_FAIL: REL relocations are not supported yet: %s\n",
      program->path);
    return -1;
  }
  if (rela_ent != sizeof(Elf64_Rela) ||
      (rela_size != 0 && (!saw_rela || rela_size % sizeof(Elf64_Rela) != 0))) {
    fprintf(stderr, "POLYCALL_FAIL: bad RELA dynamic table: %s\n",
      program->path);
    return -1;
  }
  if (pltrel_size != 0 &&
      (jmprel_vaddr == 0 || pltrel_type != DT_RELA ||
       pltrel_size % sizeof(Elf64_Rela) != 0)) {
    fprintf(stderr, "POLYCALL_FAIL: bad JMPREL dynamic table: %s\n",
      program->path);
    return -1;
  }

  if (rela_size != 0 &&
      process_rela_table(program, data, size, ehdr, dyn, dyn_count,
        rela_vaddr, rela_size, "RELA") < 0)
    return -1;
  if (pltrel_size != 0 &&
      process_rela_table(program, data, size, ehdr, dyn, dyn_count,
        jmprel_vaddr, pltrel_size, "JMPREL") < 0)
    return -1;
  return 0;
}

static int load_elf_program(const char *path, const char *symbol_name,
    struct poly_program *program) {
  memset(program, 0, sizeof(*program));
  program->path = path;

  unsigned char *data = NULL;
  size_t size = 0;
  if (read_file(path, &data, &size) < 0)
    return -1;

  if (size < sizeof(Elf64_Ehdr)) {
    fprintf(stderr, "POLYCALL_FAIL: ELF too small: %s\n", path);
    free(data);
    return -1;
  }

  const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *) data;
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
      (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
      detect_arch(ehdr->e_machine, program) < 0) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported ELF header: %s\n", path);
    free(data);
    return -1;
  }

  if (ehdr->e_phentsize < sizeof(Elf64_Phdr) ||
      ehdr->e_phoff > size ||
      (uint64_t) ehdr->e_phnum * ehdr->e_phentsize > size - ehdr->e_phoff) {
    fprintf(stderr, "POLYCALL_FAIL: bad ELF program header table: %s\n", path);
    free(data);
    return -1;
  }
  program->elf_type = ehdr->e_type;

  uint64_t base_vaddr = UINT64_MAX;
  uint64_t limit_vaddr = 0;
  int found_load = 0;
  uint64_t dynamic_vaddr = 0;
  uint64_t dynamic_size = 0;

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_DYNAMIC) {
      dynamic_vaddr = phdr->p_vaddr;
      dynamic_size = phdr->p_filesz;
      continue;
    }
    if (phdr->p_type != PT_LOAD)
      continue;
    if (phdr->p_filesz > phdr->p_memsz ||
        phdr->p_offset > size || phdr->p_filesz > size - phdr->p_offset ||
        phdr->p_vaddr > UINT64_MAX - phdr->p_memsz) {
      fprintf(stderr, "POLYCALL_FAIL: bad ELF load segment: %s\n", path);
      free(data);
      return -1;
    }
    uint64_t segment_base = align_down_u64(phdr->p_vaddr, 0x1000);
    uint64_t segment_limit = phdr->p_vaddr + phdr->p_memsz;
    if (segment_base < base_vaddr)
      base_vaddr = segment_base;
    if (segment_limit > limit_vaddr)
      limit_vaddr = segment_limit;
    found_load = 1;
  }

  if (!found_load || limit_vaddr <= base_vaddr ||
      limit_vaddr - base_vaddr > MAX_PROGRAM_BYTES - 4) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported ELF load image: %s\n", path);
    free(data);
    return -1;
  }

  program->base_vaddr = base_vaddr;
  program->image_size = (size_t) (limit_vaddr - base_vaddr + 4);
  program->image = calloc(1, program->image_size);
  if (!program->image) {
    fprintf(stderr, "POLYCALL_FAIL: out of memory loading %s\n", path);
    free(data);
    return -1;
  }

  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type != PT_LOAD)
      continue;
    uint64_t load_offset = phdr->p_vaddr - base_vaddr;
    if (load_offset > program->image_size ||
        phdr->p_filesz > program->image_size - load_offset) {
      fprintf(stderr, "POLYCALL_FAIL: ELF load segment out of range: %s\n", path);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
    memcpy(program->image + load_offset, data + phdr->p_offset, (size_t) phdr->p_filesz);
    program->loaded_bytes += (size_t) phdr->p_filesz;
  }

  uint64_t entry_vaddr = ehdr->e_entry;
  if (symbol_name && symbol_name[0] != '\0') {
    int resolved = -1;
    if (dynamic_size != 0 && dynamic_size % sizeof(Elf64_Dyn) == 0) {
      size_t dynamic_offset = 0;
      if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
            &dynamic_offset) == 0) {
        resolved = resolve_dynamic_symbol(program,
          (const Elf64_Dyn *) (program->image + dynamic_offset),
          (size_t) (dynamic_size / sizeof(Elf64_Dyn)), symbol_name,
          &entry_vaddr);
      }
    }
    if (resolved < 0)
      resolved = resolve_elf_symbol_from_sections(data, size, ehdr,
        symbol_name, &entry_vaddr);
    if (resolved < 0) {
      fprintf(stderr, "POLYCALL_FAIL: symbol not found: %s\n", symbol_name);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
  }

  int entry_in_exec = 0;
  for (uint16_t n = 0; n < ehdr->e_phnum; n++) {
    const Elf64_Phdr *phdr = (const Elf64_Phdr *) (data + ehdr->e_phoff + (uint64_t) n * ehdr->e_phentsize);
    if (phdr->p_type == PT_LOAD && (phdr->p_flags & PF_X) &&
        entry_vaddr >= phdr->p_vaddr &&
        entry_vaddr < phdr->p_vaddr + phdr->p_filesz) {
      entry_in_exec = 1;
      break;
    }
  }
  if (!entry_in_exec || entry_vaddr < base_vaddr || entry_vaddr >= limit_vaddr) {
    fprintf(stderr, "POLYCALL_FAIL: unsupported ELF entry image: %s\n", path);
    free(program->image);
    program->image = NULL;
    free(data);
    return -1;
  }
  program->entry_offset = (size_t) (entry_vaddr - base_vaddr);

  if (dynamic_size != 0) {
    if (dynamic_size % sizeof(Elf64_Dyn) != 0) {
      fprintf(stderr, "POLYCALL_FAIL: bad dynamic segment size: %s\n", path);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
    size_t dynamic_offset = 0;
    if (elf_vaddr_to_image_offset(program, dynamic_vaddr, dynamic_size,
          &dynamic_offset) < 0) {
      fprintf(stderr, "POLYCALL_FAIL: dynamic segment out of loaded image: %s\n",
        path);
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
    if (load_dynamic_relocs(program, data, size, ehdr,
          (const Elf64_Dyn *) (program->image + dynamic_offset),
          (size_t) (dynamic_size / sizeof(Elf64_Dyn))) < 0) {
      free(program->relocs);
      program->relocs = NULL;
      program->reloc_count = 0;
      free(program->image);
      program->image = NULL;
      free(data);
      return -1;
    }
  }

  free(data);
  return 0;
}

static int emit_and_call(const struct poly_program *program, uint64_t *result) {
  const uint32_t fallback_ret = program->arch == POLY_ARCH_AARCH64 ? 0xd65f03c0U : 0x00008067U;
  const size_t stub_size = 10 + 10 + 8 + 1;
  const size_t code_size = stub_size;
  const size_t foreign_size = program->image_size;
  uint8_t *code = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    fprintf(stderr, "POLYCALL_FAIL: x86 stub mmap failed: %s\n", strerror(errno));
    return -1;
  }
  uint8_t *foreign = mmap(NULL, foreign_size, PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (foreign == MAP_FAILED) {
    fprintf(stderr, "POLYCALL_FAIL: foreign mmap failed: %s\n", strerror(errno));
    munmap(code, code_size);
    return -1;
  }
  uint8_t *import_page = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (import_page == MAP_FAILED) {
    fprintf(stderr, "POLYCALL_FAIL: import mmap failed: %s\n", strerror(errno));
    munmap(foreign, foreign_size);
    munmap(code, code_size);
    return -1;
  }
  write_le64(import_page, poly_import_value);

  size_t offset = 0;
  const uint64_t return_rip = (uint64_t) (uintptr_t) (code + 28);
  const uint64_t foreign_target = (uint64_t) (uintptr_t) (foreign + program->entry_offset);
  emit_movabs_r10(code, &offset, foreign_target);
  emit_movabs_r11(code, &offset, return_rip);
  if (program->arch == POLY_ARCH_AARCH64) {
    const uint8_t pcall[] = { 0x40, 0x0f, 0x0b, 0x50, 0x43, 0x41, 0x36, 0x34 };
    memcpy(code + offset, pcall, sizeof(pcall));
    offset += sizeof(pcall);
  }
  else {
    const uint8_t pcall[] = { 0x40, 0x0f, 0x0b, 0x50, 0x43, 0x52, 0x56, 0x36 };
    memcpy(code + offset, pcall, sizeof(pcall));
    offset += sizeof(pcall);
  }
  code[offset++] = 0xc3;
  memcpy(foreign, program->image, program->image_size);
  const uint64_t load_bias = (uint64_t) (uintptr_t) foreign - program->base_vaddr;
  for (size_t n = 0; n < program->reloc_count; n++) {
    if (program->relocs[n].offset > foreign_size ||
        foreign_size - program->relocs[n].offset < 8) {
      fprintf(stderr, "POLYCALL_FAIL: relocation target escaped image: %s\n",
        program->path);
      munmap(import_page, 4096);
      munmap(foreign, foreign_size);
      munmap(code, code_size);
      return -1;
    }
    uint64_t reloc_base = 0;
    if (program->relocs[n].base_kind == RELOC_BASE_LOAD_BIAS)
      reloc_base = load_bias;
    else if (program->relocs[n].base_kind == RELOC_BASE_IMPORT_PAGE)
      reloc_base = (uint64_t) (uintptr_t) import_page;
    else if (program->relocs[n].base_kind == RELOC_BASE_IMPORT_CALL)
      reloc_base = POLY_IMPORT_CALL_COOKIE;
    const uint64_t reloc_value = program->relocs[n].value + reloc_base;
    write_le64(foreign + program->relocs[n].offset, reloc_value);
  }
  offset = program->image_size - 4;
  emit_u32(foreign, &offset, fallback_ret);

  uint64_t (*entry)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) =
    (uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)) code;
  *result = entry(1, 2, 3, 4, 5, 6, 7, 8, 9);
  munmap(import_page, 4096);
  munmap(foreign, foreign_size);
  munmap(code, code_size);
  return 0;
}

static void free_program(struct poly_program *program) {
  free(program->image);
  free(program->relocs);
  program->image = NULL;
  program->relocs = NULL;
  program->image_size = 0;
  program->entry_offset = 0;
  program->loaded_bytes = 0;
  program->reloc_count = 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s foreign-function.elf[=expected]...\n", argv[0]);
    return 2;
  }

  puts("POLYCALL: start");
  for (int n = 1; n < argc; n++) {
    struct poly_request request;
    if (parse_request(argv[n], &request) < 0)
      return 1;

    struct poly_program program;
    const char *symbol_name = request.symbol[0] ? request.symbol : NULL;
    if (load_elf_program(request.path, symbol_name, &program) < 0)
      return 1;

    printf("POLYCALL_ELF: arch=%s type=%u image_bytes=%zu loaded_bytes=%zu entry_offset=%zu relocs=%zu symbol=%s path=%s\n",
      program.arch_name, (unsigned) program.elf_type, program.image_size,
      program.loaded_bytes, program.entry_offset, program.reloc_count,
      symbol_name ? symbol_name : "-", program.path);

    uint64_t result = 0;
    if (emit_and_call(&program, &result) < 0) {
      free_program(&program);
      return 1;
    }

    printf("POLYCALL_RESULT: arch=%s value=%llu path=%s\n",
      program.arch_name, (unsigned long long) result, program.path);
    if (request.check_expected) {
      printf("POLYCALL_EXPECT: arch=%s expected=%llu path=%s\n",
        program.arch_name, (unsigned long long) request.expected, program.path);
      if (result != request.expected) {
        fprintf(stderr, "POLYCALL_FAIL: %s expected %llu got %llu\n",
          program.path, (unsigned long long) request.expected, (unsigned long long) result);
        free_program(&program);
        return 1;
      }
    }
    free_program(&program);
  }

  puts("POLYCALL_OK");
  return 0;
}
