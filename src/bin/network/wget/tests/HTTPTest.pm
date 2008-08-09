#!/usr/bin/perl -w

package HTTPTest;

use strict;

use HTTPServer;
use WgetTest;

our @ISA = qw(WgetTest);
my $VERSION = 0.01;


{
    my %_attr_data = ( # DEFAULT
    );
    
    sub _default_for
    {
        my ($self, $attr) = @_;
        return $_attr_data{$attr} if exists $_attr_data{$attr};
        return $self->SUPER::_default_for($attr);
    }

    sub _standard_keys 
    {
        my ($self) = @_;
        ($self->SUPER::_standard_keys(), keys %_attr_data);
    }
}
    

sub _setup_server {}


sub _launch_server {
    my $self = shift;
    my $synch_func = shift;

    my $server = HTTPServer->new (LocalAddr => 'localhost',
                                  LocalPort => '8080',
                                  ReuseAddr => 1) or die "Cannot create server!!!";
    $server->run ($self->{_input}, $synch_func);
}

1;

# vim: et ts=4 sw=4

