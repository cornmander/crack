# Copyright 2012 Google Inc.
# 
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# check for OpenGL

AC_DEFUN([AM_PATH_JACK], [dnl
    AC_MSG_CHECKING(for JACK)
    JACK_LIBS=-ljack
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $JACK_LIBS"
    AC_TRY_RUN([
        #include <jack/jack.h>
        
        int main(int argc, const char **argv) {
            int major, minor, micro, proto;
            jack_get_version(&major, &minor, &micro, &proto);
            return 0;
        }
    ], [
        got_jack=yes
        AC_MSG_RESULT(yes)
        AC_SUBST(JACK_LIBS)
    ], [
        AC_MSG_RESULT(no)
    ])
    LIBS="$ac_save_LIBS"
])