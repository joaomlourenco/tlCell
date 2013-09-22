#! /usr/bin/perl -W

use strict;

my @options;

if ($#ARGV < 0)
{
	die("Sintaxe: my_indent <file>");
}

#NOTE: Many default are set here, intentionally

@options=("-kr",    # K&R style
	  "-i8",    # 8 character indents
	  "-fc1",   # Always align comments with code
	  "-c1",    # Column where comments to the right of code will appear
	  "-cd1",   # Column where comments to the right of declarations 
	            # will appear
	  "-cp1",   # Column where comments to the right of 
                    # preprocessor directives will appear
	  "-d0",    # Places comments n spaces to the left of code
	  "-bl",    # Put braces in a empty line
	  "-bli0",  # Don't indent the braces
	  "-cdw",   # In a do-while put the while right after the close brace
	  "-cbi0",  # Indentation of the braces below a case statement
	  "-ss",    # If a semicolon is on the same line as a `for' or `while',
                    # this cause a space to be placed before the semicolon.
	            # NOTE: In this case I prefer the semicolon in a empty line
	  "-npcs",  # Don't put a space between the name of a function and the (
	  "-ncs",   # Don't put a space after a cast
	  "-nprs",  # Do not put a space after every '(' and before every ')'
	  "-saf",   # Put a space between an `for' and the following parenthesis
	  "-sai",   # Put a space between an `if' and the following parenthesis
	  "-saw",   # Put a space between an `while' and the 
	            # following parenthesis
	  "-di1",   # Align declaration identifiers at least in the first column
#	  "-psl",   # Causes the type of a function to be placed on the line
                    # before the name of the function
	  "-bls",   # Put the open brace of a struct in a empty line
	  "-ci0",   # Additional indent when a statement is split across lines
	  "-lp",    # If a line has a left parenthesis which is not closed on
                    # that line, then continuation lines will be lined up to 
	            # start at the character position just after the
	            # left parenthesis
#	  "-ppi2",  # Indent, with 2 spaces, the preprocessor statements
	  "-bbo",   # Prefer to break long lines before the boolean operators
#	  "-hnl",   # Honours newlines in the input file (indent 
	            # of boolean operators)
	  "-bap",   # Forces a blank line after every procedure body
	  "-il0",   # Goto labels indent
    );

foreach my $filename (@ARGV)
{
	system("indent",@options,$filename);
}
