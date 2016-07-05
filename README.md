# json(1) in C

Firstly, this doesn't actually work. I just started on it while drinking.

Since JSON is such an indispensable part of the modern internet (and modern 
computing infrastructure), I think it makes sense to reimagine many parts of 
the system with this in mind. Rather than using V8 and Node for systems level 
software (not limited to `json`), I think it makes more sense to use the 
embeddable [Duktape](duktape.org) Javascript interpreter.

I'd really like to see this taken further with a `/usr/bin/duktape` interpreter 
in the base Illumos system for small scripting jobs and have duktape used 
thoughout the Illumos system to have JSON entirely replace XML config files.

Javascript is pretty awesome. Deal with it!

## Supported Systems

This should work on pretty much any UNIX that has a C99 compiler and GNU make.

If it doesn't, please submit a pull request!
