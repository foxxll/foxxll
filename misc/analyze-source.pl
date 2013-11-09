#!/usr/bin/perl -w
############################################################################
#  misc/analyze-source.pl
#
#  Perl script to test source header files, license headers and write
#  AUTHORS from copyright statements.
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

# print multiple email addresses
my $email_multimap = 0;

# launch emacsen for each error
my $launch_emacs = 0;

# write changes to files (dangerous!)
my $write_changes = 0;

use strict;
use warnings;

my %includemap;
my %authormap;

sub expect_error($$$$) {
    my ($path,$ln,$str,$expect) = @_;

    print("Bad header line $ln in $path\n");
    print("Expected $expect\n");
    print("Got      $str\n");

    system("emacsclient -n $path") if $launch_emacs;
}

sub expect($$\$$) {
    my ($path,$ln,$str,$expect) = @_;

    if ($$str ne $expect) {
        expect_error($path,$ln,$$str,$expect);
        $$str = $expect;
    }
}
sub expect_re($$\$$) {
    my ($path,$ln,$str,$expect) = @_;

    if ($$str !~ m/$expect/) {
        expect_error($path,$ln,$$str,"/$expect/");
    }
}

sub process_cpp {
    my ($path) = @_;

    # special files
    return if $path eq "tools/benchmarks/app_config.h";

    # read file
    open(F, $path) or die("Cannot read file $path: $!");
    my @data = <F>;
    close(F);

    # put all #include lines into the includemap
    foreach my $ln (@data)
    {
        if ($ln =~ m!\s*#\s*include\s*([<"]\S+[">])!) {
            $includemap{$1}{$path} = 1;
        }
    }

    # check source header
    my $i = 0;
    if ($data[$i] =~ m!// -.*- mode:!) { ++$i; } # emacs mode line
    expect($path, $i, $data[$i], "/".('*'x75)."\n"); ++$i;
    expect($path, $i, $data[$i], " *  $path\n"); ++$i;
    expect($path, $i, $data[$i], " *\n"); ++$i;

    # skip over comment
    while ($data[$i] !~ /^ \*  Part of the STXXL/) {
        expect_re($path, $i, $data[$i], '^ \*(  .*)?\n$');
        return unless ++$i < @data;
    }

    # check "Part of STXXL"
    expect($path, $i-1, $data[$i-1], " *\n");
    expect($path, $i, $data[$i], " *  Part of the STXXL. See http://stxxl.sourceforge.net\n"); ++$i;
    expect($path, $i, $data[$i], " *\n"); ++$i;

    # read authors
    while ($data[$i] =~ /^ \*  Copyright \(C\) ([0-9-]+(, [0-9-]+)*) (?<name>[^0-9<]+)( <(?<mail>[^>]+)>)?\n/) {
        #print "Author: $+{name} - $+{mail}\n";
        $authormap{$+{name}}{$+{mail} || ""} = 1;
        return unless ++$i < @data;
    }

    # otherwise check license
    expect($path, $i, $data[$i], " *\n"); ++$i;
    expect($path, $i, $data[$i], " *  Distributed under the Boost Software License, Version 1.0.\n"); ++$i;
    expect($path, $i, $data[$i], " *  (See accompanying file LICENSE_1_0.txt or copy at\n"); ++$i;
    expect($path, $i, $data[$i], " *  http://www.boost.org/LICENSE_1_0.txt)\n"); ++$i;
    expect($path, $i, $data[$i], " ".('*'x74)."/\n"); ++$i;

    # check include guard name
    if ($path =~ m!^include/stxxl/bits/.*\.(h|h.in)$!)
    {
        expect($path, $i, $data[$i], "\n"); ++$i;

        # construct include guard macro name: STXXL_FILE_NAME_HEADER
        my $guard = $path;
        $guard =~ s!include/stxxl/bits/!stxxl/!;
        $guard =~ tr!/!_!;
        $guard =~ s!\.h(\.in)?$!!;
        $guard = uc($guard)."_HEADER";
        #print $guard."\n";

        expect($path, $i, $data[$i], "#ifndef $guard\n"); ++$i;
        expect($path, $i, $data[$i], "#define $guard\n"); ++$i;

        my $n = scalar(@data)-1;
        if ($data[$n] =~ m!// vim:!) { --$n; } # skip vim
        expect($path, $n, $data[$n], "#endif // !$guard\n");
    }

    if ($write_changes)
    {
        open(F, "> $path") or die("Cannot write $path: $!");
        print(F join("", @data));
        close(F);
    }
}

sub process_pl_cmake {
    my ($path) = @_;

    open(F, $path) or die("Cannot read file $path: $!");
    my @data = <F>;
    close(F);

    # check source header
    my $i = 0;
    if ($data[$i] =~ m/#!/) { ++$i; } # bash line
    expect($path, $i, $data[$i], ('#'x76)."\n"); ++$i;
    expect($path, $i, $data[$i], "#  $path\n"); ++$i;
    expect($path, $i, $data[$i], "#\n"); ++$i;

    # skip over comment
    while ($data[$i] !~ /^#  Part of the STXXL/) {
        expect_re($path, $i, $data[$i], '^#(  .*)?\n$');
        return unless ++$i < @data;
    }

    # check "Part of STXXL"
    expect($path, $i-1, $data[$i-1], "#\n");
    expect($path, $i, $data[$i], "#  Part of the STXXL. See http://stxxl.sourceforge.net\n"); ++$i;
    expect($path, $i, $data[$i], "#\n"); ++$i;

    # read authors
    while ($data[$i] =~ /^#  Copyright \(C\) ([0-9-]+(, [0-9-]+)*) (?<name>[^0-9<]+)( <(?<mail>[^>]+)>)?\n/) {
        #print "Author: $+{name} - $+{mail}\n";
        $authormap{$+{name}}{$+{mail} || ""} = 1;
        return unless ++$i < @data;
    }

    # otherwise check license
    expect($path, $i, $data[$i], "#\n"); ++$i;
    expect($path, $i, $data[$i], "#  Distributed under the Boost Software License, Version 1.0.\n"); ++$i;
    expect($path, $i, $data[$i], "#  (See accompanying file LICENSE_1_0.txt or copy at\n"); ++$i;
    expect($path, $i, $data[$i], "#  http://www.boost.org/LICENSE_1_0.txt)\n"); ++$i;
    expect($path, $i, $data[$i], ('#'x76)."\n"); ++$i;
}

### Main ###

foreach my $arg (@ARGV) {
    if ($arg eq "-w") { $write_changes = 1; }
    elsif ($arg eq "-e") { $launch_emacs = 1; }
    elsif ($arg eq "-m") { $email_multimap = 1; }
    else {
        print "Unknown parameter: $arg\n";
    }
}

(-e "include/stxxl.h")
    or die("Please run this script in the STXXL source base directory.");

use File::Find;
my @filelist;
find(sub { !-d && push(@filelist, $File::Find::name) }, ".");

foreach my $file (@filelist)
{
    $file =~ s!./!! or die("File does not start ./");

    if ($file =~ /\.(h|cpp|h.in)$/) {
        process_cpp($file);
    }
    elsif ($file =~ m!^doc/[^/]*\.dox$!) {
        process_cpp($file);
    }
    elsif ($file =~ m!^include/stxxl/[^/]*$!) {
        process_cpp($file);
    }
    elsif ($file =~ m!^include/stxxl/[^/]*$!) {
        process_cpp($file);
    }
    elsif ($file =~ m!\.pl$!) {
        process_pl_cmake($file);
    }
    elsif ($file =~ m!/CMakeLists\.txt$!) {
        process_pl_cmake($file);
    }
    # recognize further files
    elsif ($file =~ m!^[^/]*$!) { # files in source root
    }
    elsif ($file =~ m!^\.git/!) {
    }
    elsif ($file =~ m!^misc/!) {
    }
    elsif ($file =~ m!^doc/images/.*\.(png|pdf|svg)$!) {
    }
    elsif ($file =~ m!^doc/[^/]*\.(xml|css|bib)$!) {
    }
    elsif ($file =~ m!^doxygen-html!) {
    }
    elsif ($file =~ m!README$!) {
    }
    else {
        print "Unknown file type $file\n";
    }
}

# print includes to includemap.txt
if (0)
{
    print "Writing includemap:\n";
    foreach my $inc (sort keys %includemap)
    {
        print "$inc => ".scalar(keys %{$includemap{$inc}})." [";
        print join(",", sort keys %{$includemap{$inc}}). "]\n";
    }
}

# check includemap for C-style headers
{

    my @cheaders = qw(assert.h ctype.h errno.h fenv.h float.h inttypes.h
                      limits.h locale.h math.h signal.h stdarg.h stddef.h
                      stdlib.h stdio.h string.h time.h);

    foreach my $ch (@cheaders)
    {
        $ch = "<$ch>";
        next if !$includemap{$ch};
        print "Replace c-style header $ch in\n";
        print "    [".join(",", sort keys %{$includemap{$ch}}). "]\n";
    }
}

# print authors to AUTHORS
print "Writing AUTHORS:\n";
open(A, "> AUTHORS");
foreach my $a (sort keys %authormap)
{
    my $mail = $authormap{$a};
    if ($email_multimap) {
        $mail = join(",", sort keys %{$mail});
    }
    else {
        $mail = (sort keys(%{$mail}))[0]; # pick first
    }
    $mail = $mail ? " <$mail>" : "";

    print "  $a$mail\n";
    print A "$a$mail\n";
}
close(A);