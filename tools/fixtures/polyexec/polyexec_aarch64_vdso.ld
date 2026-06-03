PHDRS
{
  text PT_LOAD FILEHDR PHDRS FLAGS(5);
  dynamic PT_DYNAMIC FLAGS(4);
}

SECTIONS
{
  . = SIZEOF_HEADERS;
  .hash : { *(.hash) } :text
  .gnu.hash : { *(.gnu.hash) } :text
  .dynsym : { *(.dynsym) } :text
  .dynstr : { *(.dynstr) } :text
  .gnu.version : { *(.gnu.version) } :text
  .gnu.version_d : { *(.gnu.version_d) } :text
  .gnu.version_r : { *(.gnu.version_r) } :text
  .rela.dyn : { *(.rela.dyn) } :text
  .rela.plt : { *(.rela.plt) } :text
  .plt : { *(.plt*) } :text
  .text : { *(.text*) } :text
  .rodata : { *(.rodata*) } :text
  .dynamic : { *(.dynamic) } :text :dynamic
  .got : { *(.got) *(.got.plt) } :text
  .data : { *(.data*) } :text
  .bss : { *(.bss*) *(COMMON) } :text
  /DISCARD/ : { *(.note.gnu.build-id) *(.comment) *(.eh_frame*) }
}
