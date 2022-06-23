use File::Basename;
use File::Path;
use File::Copy;

my $toolsdir = dirname(__FILE__);

my $imgfile = "";
my $output_path = "";

my @args = ();

my $skip_next = 0;

foreach $argument (0 .. $#ARGV) 
{
    if($skip_next)
    {
        $skip_next = 0;
        next;
    }

    if($ARGV[$argument] eq '-o')
    {
        $imgfile = $ARGV[$argument + 1];
        $skip_next = 1;
        next;
    }

    if($ARGV[$argument] eq '-d')
    {
        $output_path = $ARGV[$argument + 1];
        $skip_next = 1;
        next;
    }

    push @args, $ARGV[$argument];
}

$output_path = "$output_path/$imgfile.out";

rmtree $output_path;

mkpath($output_path);

foreach my $argument (@args) {  

    my @parts = split('=', $argument);

    my $dir = dirname(@parts[0]);
    mkpath("$output_path/$dir");
    copy(@parts[1], "$output_path/@parts[0]")
}

mkpath("$output_path/boot");
copy("$toolsdir/limine/LICENSE.md", "$output_path/boot/LICENSE.md");
copy("$toolsdir/limine/limine.sys", "$output_path/boot/limine.sys");
copy("$toolsdir/limine/limine-cd.bin", "$output_path/boot/limine-cd.bin");
copy("$toolsdir/limine/limine-cd-efi.bin", "$output_path/boot/limine-cd-efi.bin");

if ($^O eq "linux") {
	system("xorriso -as mkisofs -b boot/limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot boot/limine-cd-efi.bin -efi-boot-part --efi-boot-image --protective-msdos-label \"$output_path\" -o $imgfile");
} else {
	system("$toolsdir/xorriso/xorriso -as mkisofs -b boot/limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot boot/limine-cd-efi.bin -efi-boot-part --efi-boot-image --protective-msdos-label \"$output_path\" -o $imgfile");
}

system("tools/limine-deploy \"$imgfile\"");