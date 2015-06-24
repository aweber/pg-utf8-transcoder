#!/bin/sh -e
aclocal
automake --add-missing --copy --foreign
autoconf

