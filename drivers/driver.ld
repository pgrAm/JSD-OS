PHDRS
{
  headers PT_PHDR PHDRS ;
  interp PT_INTERP ;
  /* text PT_LOAD FILEHDR PHDRS ; */
  data PT_LOAD FILEHDR PHDRS;
  dynamic PT_DYNAMIC ;
}

SECTIONS
{
  . = SIZEOF_HEADERS;
  .interp : { *(.interp) } :data :interp
  .text : { *(.text) } :data
  .rodata : { *(.rodata) } 
  .data : { *(.data) } :data
  .dynamic : { *(.dynamic) } :data :dynamic
}