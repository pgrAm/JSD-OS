ENTRY(start)
phys = 0xF000;
SECTIONS
{
  .text phys : AT(phys) {
    code = .;
    *(.text)
	_TEXT_END_ = .;
  }
  .rodata : {
    rodata = .;
    *(.rodata)
	_RODATA_END_ = .;
  }
  .data : {
    data = .;
    *(.data)
	_DATA_END_ = .;
  }
  .bss : {
    bss = .;
	*(COMMON)
    KEEP(*(.bss*))
  }
  _BSS_END_ = .;
  end = .;
  _IMAGE_END_ = .;
}