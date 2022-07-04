ENTRY(start)
phys = 0xFF800000;
SECTIONS
{
  . = 0xFF800000;
  .boot_entry_code phys : AT(phys) {
    _RECLAIMABLE_CODE_BEGIN_ = .;
    boot_entry_code = .;
    KEEP(*(.boot_entry_code*))
  }
  .reclaimable_text : {
    reclaimable_text = .;
    KEEP(*(.reclaimable_text*))
	_RECLAIMABLE_CODE_END_ = .;
  }
  .reclaimable_data : {
    _RECLAIMABLE_DATA_BEGIN_ = .;
    reclaimable_data = .;
    *(.reclaimable_data*)
	_RECLAIMABLE_DATA_END_ = .;
  }
  .text : {
    code = .;
    *(.text*)
	_TEXT_END_ = .;
  }
  .rodata : {
    rodata = .;
    *(.rodata*)
	_RODATA_END_ = .;
  }
  .data : {
    data = .;
    *(.data*)
  }
  .reclaimable_bss (NOLOAD) : {
    _DATA_END_ = .;
    _RECLAIMABLE_BSS_BEGIN_ = .;
    reclaimable_bss = .;
    KEEP(*(.reclaimable_bss*))
  }
  _RECLAIMABLE_BSS_END_ = .;
  .bss : {
    bss = .;
	*(COMMON)
    KEEP(*(.bss*))
  }
  _BSS_END_ = .;
  end = .;
  _IMAGE_END_ = .;
  /DISCARD/ : {
    *(.note*)
    *(.interp)
  }
}