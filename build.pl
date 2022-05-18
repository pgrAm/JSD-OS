use strict;
use warnings;
use File::Basename;
use File::Path;
use File::Copy;
use Cwd;

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

my $kb_drv = "$builddir/AT_kbrd.drv";
my $isa_dma =  "$builddir/isa_dma.drv";
my $drv_lib = "$builddir/drvlib.drv";
my $pci_drv = "$builddir/pci.drv";
my $floppy_drv = "$builddir/floppy.drv";
my $vga_drv = "$builddir/vga.drv";
my $mbr_drv = "$builddir/mbr.drv";
my $ata_drv = "$builddir/ata.drv";
my $fat_drv = "$builddir/fat.drv";
my $iso_drv = "$builddir/iso9660.drv";
my $i8042_drv = "$builddir/i8042.drv";
my $ps2mouse_drv = "$builddir/ps2mouse.drv";
my $vesa_drv = "$builddir/vesa.drv";
my $clib = "$builddir/clib.lib";
my $terminal = "$builddir/terminal.lib";
my $shell = "$builddir/shell.elf";
my $primes =  "$builddir/primes.elf";
my $bkgrndtest = "$builddir/bkgrnd.elf";
my $listmode = "$builddir/listmode.elf";
my $graphicstest = "$builddir/graphics.elf";
my $fwritetest = "$builddir/fwrite.elf"; 

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

#build_cdimage(
#	name => "jsd-os+limine.iso",
#	boot_file => ":limine:", 
#	files => [
#		@iso_files, 
#		"boot" => ["limine.cfg"], 
#		"system" => [
#			"$builddir/configs/cdboot/init.rfs",
#			"$builddir/kernal.sys"
#		]
#	]
#);

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


