OUTPUT_FORMAT("binary")
ENTRY(start)
phys = 0x001000;
SECTIONS
{
  .text phys : AT(phys) {
    code = .;
    *(.text)
    *(.rodata)
	_TEXT_END_ = .;
  }
  .data : AT(_TEXT_END_)
  {
    data = .;
    *(.data)
	_DATA_END_ = .;
  }
  .bss : AT(_DATA_END_)
  {
    bss = .;
    *(.bss)
    _BSS_END_ = .;
  }
  end = .;
}