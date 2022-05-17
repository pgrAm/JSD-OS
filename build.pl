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

my $cwd = getcwd;

if($#ARGV >= 1)
{
	$cwd = $ARGV[0]
}

my $builddir = $cwd . "/build2";

if($#ARGV >= 2)
{
	$builddir = $ARGV[1]
}

=begin
my @common_flags = qw(--target=i386-baremetal-elf -Wuninitialized -Wall -fno-unwind-tables -fno-asynchronous-unwind-tables -march=i386 -O2 -mno-sse -mno-mmx -fomit-frame-pointer -I ./ -Werror=implicit-function-declaration -flto -I clib/include -D__I386_ONLY);
my @cpp_flags = qw(-std=c++20 -fno-rtti -fno-exceptions -I cpplib/include);
my @c_flags = qw(-std=c99 -Wc++-compat);
my @asm_flags = qw(-f elf);

my @kernel_flags = qw(-D __KERNEL -mno-implicit-float);
my @driver_flags = (@kernel_flags, qw(-fPIC -fno-function-sections));
my @shlib_flags = qw(-fPIC);
my @user_flags = qw(-I api/ -nodefaultlibs);

my @user_ld_flags = qw(-L./ -l:libclang_rt.builtins-i386.a -mllvm -align-all-nofallthru-blocks=2 -O2 --lto-O2 --gc-sections);
my @shlib_ld_flags = qw(-O2 -shared --lto-O3 --gc-sections);
my @driver_ld_flags = qw(-L./ -l:libclang_rt.builtins-i386.a -O2 -shared --lto-O2 --gc-sections -T drivers/driver.ld);
my @kernel_ld_flags = qw(-L./ -l:libclang_rt.builtins-i386.a --lto-O2 -N -O2 -Ttext=0xF000 -T linker.ld -mllvm -align-all-nofallthru-blocks=2);

mkpath("$builddir/tools");
if ($^O eq "linux") {
	system("clang++ tools/rdfs.cpp -o $builddir/tools/rdfs -std=c++20 -O2");
	system("clang tools/limine/limine-install.c -o $builddir/tools/limine-install -O2");
} else {
	system("clang++ tools/rdfs.cpp -o $builddir/tools/rdfs.exe -std=c++20 -O2");
	system("clang -D_CRT_SECURE_NO_WARNINGS tools/limine/limine-install.c -o $builddir/tools/limine-install.exe -O2");
}

my @kernel_src = qw(	
	kernel/kernel.asm
	kernel/interrupt.asm
	kernel/paging.asm
	kernel/syscall.asm

	kernel/kernel.c
	kernel/memorymanager.cpp
	kernel/physical_manager.cpp
	kernel/filesystem/drives.cpp
	kernel/filesystem/directory.cpp
	kernel/filesystem/streams.cpp
	kernel/elf.cpp
	kernel/interrupt.cpp
	kernel/syscall.c
	kernel/task.cpp
	kernel/locks.cpp
	kernel/driver_loader.cpp
	kernel/display.cpp
	kernel/sysclock.cpp
	kernel/gdt.cpp
	kernel/boot_info.c
	kernel/rt_device.cpp		
	kernel/input.cpp		
	kernel/kassert.cpp		
	kernel/shared_mem.cpp		

	drivers/display/basic_text/basic_text.cpp
	drivers/formats/rdfs.cpp
	drivers/ramdisk.cpp
	drivers/cmos.cpp
	drivers/pit.cpp		
);

my @clib_src = qw(	
	clib/string.c
	clib/stdio.c
	clib/ctype.c
	clib/time.c
	clib/stdlib.cpp
	clib/liballoc.cpp
	clib/string.asm
);

my $boot_mapper = build(name => "boot_mapper.a", 
						static => 'true',
						src => ["kernel/boot_mapper.cpp"], 
						flags => [@common_flags, @kernel_flags, "-fno-lto", "-fPIC"], 
						ldflags => []);

build(	name => "kernal.elf", 
		src => [@kernel_src, @clib_src], 
		flags => [@common_flags, @kernel_flags], 
		ldflags => [@kernel_ld_flags, $boot_mapper]);

=cut
system("objcopy -x -S -O binary $builddir/kernal.elf $builddir/kernal.sys --set-section-flags .bss=alloc,load,contents --gap-fill 0");

my $kb_drv = "$builddir/AT_kbrd.drv";
#build_driver("AT_kbrd.drv", ["drivers/at_kbrd.cpp"]);
my $isa_dma =  "$builddir/isa_dma.drv";
#build_driver("isa_dma.drv", ["drivers/isa_dma.cpp"]);
my $drv_lib = "$builddir/drvlib.drv";
#build_driver("drvlib.lib", ["drivers/drvlib.cpp"]);
my $pci_drv = "$builddir/pci.drv";
#build_driver("pci.drv", ["drivers/pci.cpp"], [link_lib($drv_lib)]);

my $floppy_drv = "$builddir/floppy.drv";#build_driver("floppy.drv", ["drivers/floppy.cpp"], [link_lib($drv_lib), link_lib($isa_dma)]);
my $vga_drv = "$builddir/vga.drv";#build_driver("vga.drv", 		["drivers/display/vga/vga.cpp"], [link_lib($drv_lib)]);
my $mbr_drv = "$builddir/mbr.drv";#build_driver("mbr.drv", 		["drivers/formats/mbr.cpp"], 	[link_lib($drv_lib)]);
my $ata_drv = "$builddir/ata.drv";#build_driver("ata.drv", 		["drivers/ata.cpp"], 			[link_lib($drv_lib), link_lib($pci_drv)]);
#my $ahci_drv = build_driver("ahci.drv", 	["drivers/ahci.cpp"], 			[link_lib($drv_lib), link_lib($pci_drv)]);
my $fat_drv = "$builddir/fat.drv";#build_driver("fat.drv", 		["drivers/formats/fat.cpp"], 	[link_lib($drv_lib)]);
my $iso_drv = "$builddir/iso9660.drv";#build_driver("iso9660.drv",	["drivers/formats/iso9660.cpp"],[link_lib($drv_lib)]);

my $i8042_drv = "$builddir/i8042.drv";#build_driver("i8042.drv",	["drivers/i8042.cpp"], [link_lib($drv_lib)]);
my $ps2mouse_drv = "$builddir/ps2mouse.drv";#build_driver("ps2mouse.drv",	["drivers/ps2mouse.cpp"], [link_lib($drv_lib)]);

=begin
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
=cut

my $vesa_drv = "$builddir/vesa.drv"; # build_driver("vesa.drv", 
							#["drivers/display/vesa/vesa.cpp"], 
							#[$libemu, link_lib($drv_lib)]);

my $clib = "$builddir/clib.lib"; # build_shared("clib.lib", \@clib_src);
#my $kb = build_static("keyboard.a", ["api/keyboard.c"]);
#my $cppr = build_static("cppruntime.a", ["api/cppruntime.cpp"]);
my $terminal = "$builddir/terminal.lib";
# build_shared("terminal.lib", ["api/terminal/terminal.cpp", "api/cppruntime.cpp"], [link_lib($clib)]);

my $shell = "$builddir/shell.elf"; #build(name => "shell.elf", src => ["api/crt0.c", "api/crti.asm", "shell/commands.cpp", "shell/shell.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($terminal), $kb, $cppr]);

my $primes =  "$builddir/primes.elf";#build(name => "primes.elf", src => ["api/crt0.c", "api/crti.asm", "apps/primes.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($terminal), $kb, $cppr]);

my $bkgrndtest = "$builddir/bkgrnd.elf"; #build(name => "bkgrnd.elf", src => ["api/crt0.c", "api/crti.asm", "apps/bkgrnd.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($terminal), $kb, $cppr]);

my $listmode = "$builddir/listmode.elf"; #build(name => "listmode.elf", src => ["api/crt0.c", "api/crti.asm", "apps/listmode.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($terminal), $kb, $cppr]);

my $graphicstest = "$builddir/graphics.elf"; #build(name => "graphics.elf", src => ["api/crt0.c", "api/crti.asm", "apps/graphics.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), link_lib($terminal), $kb, $cppr]);

my $fwritetest = "$builddir/fwrite.elf"; #build(name => "fwrite.elf", src => ["api/crt0.c", "api/crti.asm", "apps/fwrite.cpp", "api/crtn.asm"], flags => [@common_flags, @user_flags], ldflags => [@user_ld_flags, "--image-base=0x8000000", link_lib($clib), $cppr]);

#mkpath("$builddir/fdboot");
#system("$builddir/tools/rdfs", "configs/cdboot/init.sys", $drv_lib, $iso_drv, $ata_drv, $kb_drv, $pci_drv, "-o", "$builddir/cdboot/init.rfs");
#mkpath("$builddir/cdboot");
#system("$builddir/tools/rdfs", "configs/fdboot/init.sys", $drv_lib, $fat_drv, $floppy_drv, $kb_drv, $isa_dma, "-o", "$builddir/fdboot/init.rfs");
#mkpath("$builddir/netboot");
#system("$builddir/tools/rdfs", "configs/netboot/init.sys", $ps2mouse_drv, $i8042_drv, $listmode, $vesa_drv, $drv_lib, $kb_drv, $shell, $graphicstest, $clib, $terminal, "-o", "$builddir/netboot/init.rfs");

#system("nasm boot/boot_sect.asm -i boot -f bin -o $builddir/boot_sect.bin");

build_fdimage(
	name => "os.vfd",
	boot_file => "$builddir/boot_sect.bin", 
	files => [
		"configs/fdboot/fdboot.sys",
		"$builddir/configs/fdboot/init.rfs",
		"$builddir/kernal.sys",
		$shell,
		"fonts/font08.psf",
		"fonts/font16.psf",
		"test.bat",
		"readme.txt",
		"LICENSE.txt",
		$primes,
		$ata_drv,
		$vga_drv,
		$pci_drv,
		$iso_drv,
		$mbr_drv,
		$terminal,
		$clib,
		$i8042_drv,
		$ps2mouse_drv,
		$fwritetest,
		$bkgrndtest
	]
);

my @iso_files = (
	"/" => [
		"LICENSE.txt",
		"configs/cdboot/cdboot.sys",
		$graphicstest,
		$clib,
		$terminal,
		$shell,
		$primes,
		$listmode,
		$fwritetest,
		$bkgrndtest,
	],
	"/drivers" => [
		$fat_drv, 		
		$floppy_drv, 
		$isa_dma, 		
		$vga_drv, 		
		$vesa_drv, 	
		$mbr_drv, 		
		$i8042_drv, 	
		$ps2mouse_drv,
		"fonts/font08.psf",
		"fonts/font16.psf"
	]
);

mkpath("$builddir/iso");

if ($^O ne "linux") {
	build_fdimage(
		name => "iso/isoboot.img",
		boot_file => "$builddir/boot_sect.bin", 
		files => [
			"$builddir/configs/cdboot/init.rfs",
			"$builddir/kernal.sys",
		]
	);
}

build_cdimage(
	name => "boot.iso",
	boot_file => "$builddir/iso/isoboot.img", 
	files => [@iso_files]
);

build_cdimage(
	name => "jsd-os+limine.iso",
	boot_file => ":limine:", 
	files => [
		@iso_files, 
		"boot" => ["limine.cfg"], 
		"system" => [
			"$builddir/configs/cdboot/init.rfs",
			"$builddir/kernal.sys"
		]
	]
);

mkpath("$builddir/limine-iso");

sub build_fdimage
{
	my %args = @_;
	my $files = $args{files};
    my $name = $args{name};
	my $boot_file = $args{boot_file};
	
	my $folder = "$builddir/fd/$name.bld";
	my $imgfile = "$builddir/$name";

	mkpath("$folder");

	foreach my $file (@$files) 
	{
		my ($filename,$dir,undef) = fileparse($file);
		copy($file, "$folder/$filename");
	}

	unlink("$builddir/$name");
	system("tools/bfi -t=144 -f=\"$imgfile\" -b=$boot_file $folder");
}

sub build_cdimage
{
	my %args = @_;
	my $files = $args{files};
    my $name = $args{name};
	my $boot_file = $args{boot_file};
	
	my $folder = "$builddir/cd/$name.bld";
	my $imgfile = "$builddir/$name";

	mkpath("$folder");

	my %file_hash = @$files;

	keys %file_hash; # reset the internal iterator so a prior each() doesn't affect the loop
	while(my($name, $dir) = each %file_hash) 
	{
		foreach my $file (@$dir) 
		{
			my ($filename, $d, undef) = fileparse($file);
			mkpath("$folder/$name");
			copy($file, "$folder/$name/$filename");
		}
	}

	if($boot_file eq ":limine:")
	{
		mkpath("$folder/boot");
		copy("tools/limine/LICENSE.md", "$folder/boot/LICENSE.md");
		copy("tools/limine/limine.sys", "$folder/boot/limine.sys");
		copy("tools/limine/limine-cd.bin", "$folder/boot/limine-cd.bin");
		copy("tools/limine/limine-eltorito-efi.bin", "$folder/boot/limine-eltorito-efi.bin");

		my $reldir = $folder;
		$reldir =~ s/${cwd}//;
		$reldir =~ s/\///;
		if ($^O eq "linux") {
			system("xorriso -as mkisofs -b boot/limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot boot/limine-eltorito-efi.bin -efi-boot-part --efi-boot-image --protective-msdos-label \"$reldir\" -o $imgfile");
		} else {
			system("tools/xorriso/xorriso -as mkisofs -b boot/limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot boot/limine-eltorito-efi.bin -efi-boot-part --efi-boot-image --protective-msdos-label \"$reldir\" -o $imgfile");
		}
	
		system("$builddir/tools/limine-install \"$imgfile\"");
	}
	else
	{
		if ($^O eq "linux") {
		} else {
			my ($bt_filename,$dir,undef) = fileparse($boot_file);
			copy($boot_file, "$folder/$bt_filename");

			unlink("$builddir/$name");
			system("tools/mkisofs -U -b $bt_filename -hide $bt_filename -V \"JSD-OS\" -iso-level 3 -o $imgfile \"$folder\"");
		}
	}

}
=begin

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
	
	foreach my $file (@$src)
	{
		my $outfile = "$outdir/$file.o";

		mkpath(dirname($outfile));

		if($file =~ /\.c$/i)
		{		
			system($cc, "-c", $file, "-o", $outfile, @c_flags, @$flags);
		}
		elsif($file =~ /\.cpp$/i)
		{		
			system($cpp, "-c", $file, "-o", $outfile, @cpp_flags, @$flags);
			
			#print ("$cpp -c $file -o $outfile @cpp_flags @$flags\n");
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

	if(!$static)
	{
		system("strip --strip-unneeded $output_file");
	}

	return $output_file;
}
=cut


