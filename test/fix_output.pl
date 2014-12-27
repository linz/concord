#!/bin/perl -pi.bak
BEGIN { @ARGV=map(glob,  @ARGV); }
s/(\s)\-(0\.0+)(?![0-9])/$1 $2/g;
s/\\coordsys/\/coordsys/;
s/\s+$/\n/;
s/version\s+[\d\.]+\s+dated\s+\w\w\w\s+\d\d?\s+2\d\d\d/version 0.0 dated 1 Jan 1970/;
