use strict;
use warnings;
use File::Basename;
use File::Path;
use File::Copy;
use Cwd;

my $cc = "clang";
my $cpp = "clang++";
my $asm = "nasm";
my $ld = "ld.lld";
my $ar = "llvm-ar";

my $builddir = getcwd . "/builddir";

mkpath($builddir);

my @common_flags = qw(-target i386-elf -Wuninitialized -Wall -fno-asynchronous-unwind-tables -march=i386  -O2 -mno-sse -mno-mmx  -fomit-frame-pointer -fno-builtin -I ./ -Werror=implicit-function-declaration -flto -I clib/include -D__I386_ONLY);
my @cpp_flags = qw(-std=c++17 -fno-rtti -fno-exceptions -I cpplib/include);
my @c_flags = qw(-std=c99 -Wc++-compat);
my @asm_flags = qw(-f elf);

my @kernel_flags = qw(-D __KERNEL -mno-implicit-float -ffreestanding);
my @driver_flags = (@kernel_flags, qw(-fPIC));
my @shlib_flags = qw(-fPIC);
my @user_flags = qw(-I api/ -nodefaultlibs);

my @user_ld_flags = qw(-L./ -l:libclang_rt.builtins-i386.a);
my @shlib_ld_flags = qw(-O2 -shared --lto-O3 --gc-sections);
my @driver_ld_flags = qw(-L./ -l:libclang_rt.builtins-i386.a -O2 -shared --lto-O2 --gc-sections);
my @kernel_ld_flags = qw(-L./ -l:libclang_rt.builtins-i386.a --lto-O2 -N -O2 -Ttext=0x8000 -T linker.ld);

mkpath("$builddir/tools");
system("clang++ tools/rdfs.cpp -o $builddir/tools/rdfs.exe -std=c++17 -O2");

my @kernel_src = qw(	
	kernel/kernel.asm
	kernel/interrupt.asm
	kernel/paging.asm
	kernel/syscall.asm

	kernel/kernel.c
	kernel/memorymanager.c
	kernel/physical_manager.c
	kernel/filesystem.cpp
	kernel/elf.cpp
	kernel/interrupt.cpp
	kernel/syscall.c
	kernel/task.cpp
	kernel/locks.c
	kernel/driver_loader.cpp
	kernel/display.cpp
	kernel/sysclock.cpp

	drivers/display/basic_text/basic_text.cpp
	drivers/formats/rdfs.cpp
	drivers/ramdisk.c
	drivers/kbrd.c
	drivers/cmos.cpp
	drivers/pit.cpp		
);

my @clib_src = qw(	
	clib/string.c
	clib/stdio.c
	clib/ctype.c
	clib/time.c
	clib/stdlib.c
	clib/liballoc.c
	clib/string.asm
);

build(	name => "kernal.elf", 
		src => [@kernel_src, @clib_src], 
		flags => [@common_flags, @kernel_flags], 
		ldflags => \@kernel_ld_flags);

system("objcopy -O binary $builddir/kernal.elf $builddir/kernal.sys --set-section-flags .bss=alloc,load,contents --gap-fill 0");

build_driver("AT_kbrd.drv", ["drivers/at_kbrd.c"]);
my $isa_dma = 
build_driver("isa_dma.drv", ["drivers/isa_dma.c"]);
my $floppy_drv = build_driver("floppy.drv", ["drivers/floppy.c"], [link_lib($isa_dma)]);
my $drv_lib = 
build_driver("drvlib.lib", ["drivers/drvlib.cpp"]);

my $vga_drv = build_driver("vga.drv", 		["drivers/display/vga/vga.cpp"], [link_lib($drv_lib)]);
my $mbr_drv = build_driver("mbr.drv", 		["drivers/formats/mbr.cpp"], 	[link_lib($drv_lib)]);
my $ata_drv = build_driver("ata.drv", 		["drivers/ata.cpp"], 			[link_lib($drv_lib)]);
my $fat_drv = build_driver("fat.drv", 		["drivers/formats/fat.cpp"], 	[link_lib($drv_lib)]);
my $iso_drv = build_driver("iso9660.drv",	["drivers/formats/iso9660.cpp"],[link_lib($drv_lib)]);

my @libemu_src = qw(	
	drivers/display/vesa/libx86emu/api.c
	drivers/display/vesa/libx86emu/decode.c 
	drivers/display/vesa/libx86emu/mem.c	
	drivers/display/vesa/libx86emu/ops.c	
	drivers/display/vesa/libx86emu/ops2.c	
	drivers/display/vesa/libx86emu/prim_ops.c
);

my $libemu = build(	static => 'true',
					name => "libx86emu.drv.a", 
					src => \@libemu_src,
					flags => [@common_flags, @driver_flags, "-fvisibility=hidden", "-std=gnu99", "-I", "drivers/display/vesa/"], 
					ldflags => []);

my $vesa_drv = build_driver("vesa.drv", 
							["drivers/display/vesa/vesa.cpp"], 
							[$libemu, link_lib($drv_lib)]);

my $clib = build_shared("clib.lib", \@clib_src);
my $graphics = 
build_shared("graphics.lib", ["api/graphics/graphics.c"], [link_lib($clib)]);
my $kb = 
build_static("keyboard.a", ["api/keyboard.c"]);
my $cppr = 
build_static("cppruntime.a", ["api/cppruntime.cpp"]);

my $shell = build(name => "shell.elf", src => ["api/crt0.c", "api/crti.asm", "shell/shell.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($graphics), $kb, $cppr]);

my $primes = build(name => "primes.elf", src => ["api/crt0.c", "api/crti.asm", "apps/primes.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($graphics), $kb, $cppr]);

my $graphicstest = build(name => "graphic.elf", src => ["api/crt0.c", "api/crti.asm", "apps/graphic.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($graphics), $kb, $cppr]);

mkpath("$builddir/fdboot");
system("$builddir/tools/rdfs configs/cdboot/init.sys $builddir/drvlib.lib $builddir/iso9660.drv $builddir/ata.drv $builddir/AT_kbrd.drv -o $builddir/cdboot/init.rfs");
mkpath("$builddir/cdboot");
system("$builddir/tools/rdfs configs/fdboot/init.sys $builddir/drvlib.lib $builddir/fat.drv $builddir/floppy.drv $builddir/AT_kbrd.drv $builddir/isa_dma.drv -o $builddir/fdboot/init.rfs");

system("nasm boot/boot_sect.asm -i boot -f bin -o $builddir/boot_sect.bin");

build_fdimage(
	name => "$builddir/os.vfd",
	boot_file => "$builddir/boot_sect.bin", 
	files => [
		"$builddir/fdboot/init.rfs",
		"$builddir/kernal.sys",
		$shell,
		"fonts/font08.psf",
		"fonts/font16.psf",
		"test.bat",
		"readme.txt",
		$primes,
		$ata_drv,
		$vga_drv,
		$iso_drv,
		$mbr_drv,
		$graphics,
		$clib
	]
);

mkpath("$builddir/iso");

build_fdimage(
	name => "$builddir/iso/isoboot.img",
	boot_file => "$builddir/boot_sect.bin", 
	files => [
		"$builddir/cdboot/init.rfs",
		"$builddir/kernal.sys",
	]
);

copy($graphicstest, "$builddir/iso/graphic.elf");
copy($graphics, 	"$builddir/iso/graphics.lib");
copy($clib, 		"$builddir/iso/clib.lib");
copy($shell, 		"$builddir/iso/shell.elf");
copy($primes, 		"$builddir/iso/primes.elf");

mkpath("$builddir/iso/drivers");
copy($fat_drv, 		"$builddir/iso/drivers/fat.drv");
copy($floppy_drv, 	"$builddir/iso/drivers/floppy.drv");
copy($isa_dma, 		"$builddir/iso/drivers/isa_dma.drv");
copy($vga_drv, 		"$builddir/iso/drivers/vga.drv");
copy($vesa_drv, 	"$builddir/iso/drivers/vesa.drv");
copy($mbr_drv, 		"$builddir/iso/drivers/mbr.drv");

mkpath("$builddir/iso/fonts");
copy("fonts/font08.psf", "$builddir/iso/font08.psf");
copy("fonts/font16.psf", "$builddir/iso/font16.psf");

system("tools/mkisofs -U -b isoboot.img -hide isoboot.img -V \"JSD-OS\" -iso-level 3 -o $builddir/boot.iso \"$builddir/iso\"");

sub build_fdimage
{
	my %args = @_;
	my $files = $args{files};
    my $name = $args{name};
	my $boot_file = $args{boot_file};
	
	unlink($name);
	system("tools/fat_imgen -c -f \"$name\"");
	if(defined($boot_file)) {
		system("tools/fat_imgen -m -f \"$name\" -s \"$boot_file\"");
	}
	foreach my $file (@$files) {
		system("tools/fat_imgen -m -f \"$name\" -i \"$file\"");
	}
}

sub build_static
{
	my $nm = $_[0];
	my $src = $_[1];
	my $deps = $_[2];
	
	return build(	static => 'true',
					name => $nm, 
					src => $src,
					flags => [@common_flags, @user_flags], 
					ldflags => []);
}

sub build_shared
{
	my $nm = $_[0];
	my $src = $_[1];
	my $deps = $_[2];
	
	return build(	name => $nm, 
					src => $src,
					flags => [@common_flags, @user_flags, @shlib_flags], 
					ldflags => [@shlib_ld_flags, @user_ld_flags, defined($deps) ? @$deps : ()]);
}

sub build_driver
{
	my $nm = $_[0];
	my $src = $_[1];
	my $deps = $_[2];
	
	my @dflags = (@common_flags, @driver_flags);
	
	return build(name => $nm, src => $src, flags => \@dflags, ldflags => [@driver_ld_flags, defined($deps) ? @$deps : ()]);
}

sub link_lib
{
	my $fn = $_[0];
	
	my ($filename,$dir,undef) = fileparse($fn);
	
	return ("-L$dir", "-l:$filename");
}

sub build {
    my %args = @_;
	my $src = $args{src};
    my $name = $args{name};
    my $flags = $args{flags};
    my $ldflags = $args{ldflags};
	my $static = $args{static};
	
	my ($filename,$dir,$ext) = fileparse($name, qr/\.[^.]*/);
	
	my $outdir = "$builddir/$dir/$filename";
	
	foreach my $file (@$src) {
		
		my $outfile = "$outdir/$file.o";

		mkpath(dirname($outfile));

		if($file =~ /\.c$/i)
		{		
			system($cc, "-c", $file, "-o", $outfile, @c_flags, @$flags);
		}
		elsif($file =~ /\.cpp$/i)
		{		
			system($cpp, "-c", $file, "-o", $outfile, @cpp_flags, @$flags);
		}
		elsif($file =~ /\.asm$/i)
		{		
			system($asm, $file, "-o", $outfile, @asm_flags);
		}
	}
	
	my $output_file = "$builddir/$name";
	
	my @linker_cmd = (defined($static) and !!$static) ? ($ar, "rcs", $output_file) : ($ld, "-o", $output_file);
	
	foreach my $file (@$src) {
		my $outfile = "$outdir/$file.o";
	
		push @linker_cmd, $outfile;
	}
		
	system(@linker_cmd, @$ldflags);

	return $output_file;
}


