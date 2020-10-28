#!/bin/perl -pi.bak
BEGIN { @ARGV=map(glob,  @ARGV); }
s/(\s)\-(0\.0+)(?![0-9])/$1 $2/g;
s/\\coordsys/\/coordsys/;
s/version\s+\d+\.\d+(\.\d+)?\,?\s+dated\s+\w+\s+\d+\s+\d+(\s+\d\d\:\d\d\:\d\d)?/version 0.0 dated 1 April 1066/;
s/\s+$/\n/;
