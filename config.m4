dnl $Id$
dnl config.m4 for extension tp

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(tp, for tp support,
dnl Make sure that the comment is aligned:
dnl [  --with-tp             Include tp support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(tp, whether to enable tp support,
Make sure that the comment is aligned:
[  --with-tp           Include tp support])

if test "$PHP_TP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-tp -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/tp.h"  # you most likely want to change this
  dnl if test -r $PHP_TP/$SEARCH_FOR; then # path given as parameter
  dnl   TP_DIR=$PHP_TP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for tp files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       TP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$TP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the tp distribution])
  dnl fi

  dnl # --with-tp -> add include path
  dnl PHP_ADD_INCLUDE($TP_DIR/include)

  dnl # --with-tp -> check for lib and symbol presence
  dnl LIBNAME=tp # you may want to change this
  dnl LIBSYMBOL=tp # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $TP_DIR/lib, TP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_TPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong tp lib version or lib not found])
  dnl ],[
  dnl   -L$TP_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(TP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(tp, tp.c, $ext_shared)
fi
