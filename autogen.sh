#!/bin/sh

autoheader && \
touch NEWS README AUTHORS ChangeLog stamp-h && \
aclocal && \
autoconf && \
automake -a && \
automake && \
./configure CFLAGS="-Wall -Werror -g"
