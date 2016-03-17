use strict;
use warnings;
use LWP::Simple;
use Getopt::Long;
use Data::Dumper;
use Pithub;

my $milestone;

GetOptions ("milestone=s" => \$milestone);

my $target;
	
if ($milestone =~ /^(\d*):(.*)$/)
{
	$target = $1;
}

my $i = Pithub::Issues->new;
my $result = $i->list(
	repo  => 'Meridian59',
	user  => 'OpenMeridian105',
	params => {
		milestone => $target,
		state     => 'open',
	}
);

while ( my $row = $result->next ) 
{
        printf "%s,", $row->{number};		
}

