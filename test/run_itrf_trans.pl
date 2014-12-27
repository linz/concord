use strict;

my ($concord,$inf)=@ARGV;
die "Need input file, output file, concord executable parameters\n" if ! -x $concord;

my $params="-INZGD2000:NEH:D -OITRF2008:NEH:D -P9";

open(my $fi, $inf) || die;
while( my $line = <$fi> )
{
    my( $lon,$lat,$hgt,$year) = split(' ',$line);
    if($lon !~ /^\d+(\.\d+)?$/){ print "\n$line"; next; }
    printf "\nNZGD: %.9f %.9f %.4f\nEpoch: $year\nITRF: ",$lat,$lon,$hgt;
    my $command="$concord $params -Y$year";
    open( my $cp, "| $command");
    print $cp "$lat $lon $hgt";
    close($cp);
    print "\n";
}







