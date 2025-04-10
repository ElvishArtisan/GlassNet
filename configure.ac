dnl configure.ac
dnl
dnl Autoconf configuration
dnl
dnl Use autoconf to process this into a configure script
dnl
dnl   (C) Copyright 2014-2025 Fred Gleason <fredg@paravelsystems.com>
dnl
dnl   This program is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License version 2 as
dnl   published by the Free Software Foundation.
dnl
dnl   This program is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public
dnl   License along with this program; if not, write to the Free Software
dnl   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
dnl

AC_INIT(glassnet,m4_esyscmd(cat PACKAGE_VERSION))
AM_INIT_AUTOMAKE
AC_SUBST(RPM_RELEASE,1)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

#
# Basic Compiler Checks
#
AC_PROG_CXX
AC_LANG(C++)

# to avoid rpath usage :
# http://wiki.debian.net/index.cgi?RpathIssue
case ${host} in
  *-pc-linux-gnu)
    AC_MSG_RESULT([Fixing libtool for -rpath problems.])
    sed < libtool > libtool-2 \
    's/^hardcode_libdir_flag_spec.*$/hardcode_libdir_flag_spec=" -D__LIBTOOL_IS_A_FOOL__ "/'
    mv libtool-2 libtool
    chmod 755 libtool
  ;;
esac

#
# Option Switches
#
dnl Example of default-disabled feature
AC_ARG_ENABLE(docbook,[  --disable-docbook       disable building of DocBook documentation],[DOCBOOK_DISABLED=yes],[])

AC_ARG_ENABLE([gncd],
    AS_HELP_STRING([--disable-gncd], [Do not build receiver component.]))
AS_IF([test "x$enable_gncd" != "xno"], [
  BUILD_GNCD=yes
])
AM_CONDITIONAL([GNCD_AM],[test "x$enable_gncd" != "xno"])

AC_ARG_ENABLE([gnmc],
    AS_HELP_STRING([--disable-gnmc], [Do not build management component.]))
AS_IF([test "x$enable_gnmc" != "xno"], [
  BUILD_GNMC=yes
])
AM_CONDITIONAL([GNMC_AM],[test "x$enable_gnmc" != "xno"])

AC_ARG_ENABLE([gnmd],
    AS_HELP_STRING([--disable-gnmd], [Do not build server component.]))
AS_IF([test "x$enable_gnmd" != "xno"], [
  BUILD_GNMD=yes
])
AM_CONDITIONAL([GNMD_AM],[test "x$enable_gnmd" != "xno"])

#
# Check for Qt (Mandatory)
#
AQ_FIND_QT6(QT6,Core Gui Widgets Network Sql,[],[AC_MSG_ERROR([*** Qt6 not found ***])])

#
# Check for OpenSSL (Mandatory)
#
PKG_CHECK_MODULES(OPENSSL,openssl,[],[AC_MSG_ERROR([*** OpenSSL not found ***])])

#
# Check for DocBook Toolchain
#
if test -z $DOCBOOK_DISABLED ; then
  USING_DOCBOOK=yes
fi
AM_CONDITIONAL([DOCBOOK_AM], [test "$USING_DOCBOOK" = yes])

#
# Determine the target architecture
#
AR_GCC_TARGET()
AC_SUBST(VENDOR,$ar_gcc_distro)
AC_SUBST(ARCH,$ar_gcc_arch)

#
# Determine number of available CPU cores for building packages
#
AX_COUNT_CPUS([],[])
if test $CPU_COUNT -lt 2 ; then
   AC_SUBST(CPUS_AVAIL,1)
else
   if test $CPU_COUNT -gt 16 ; then
      AC_SUBST(CPUS_AVAIL,16)
   else
      AC_SUBST(CPUS_AVAIL,$(($CPU_COUNT-1)))
   fi
fi

#
# Determine Distro
#
AR_GET_DISTRO()
AC_SUBST(DISTRO,$ar_gcc_distro)

#
# Configure RPM Build
#
AC_CHECK_PROG(RPMBUILD_FOUND,rpmbuild,[yes],[])
if test -z $RPMBUILD_FOUND ; then
  AC_SUBST(RPMBUILD,rpm)
else
  AC_SUBST(RPMBUILD,rpmbuild)
fi
if test -d /usr/src/redhat ; then
  AC_SUBST(RPM_ROOT,/usr/src/redhat)
  AC_SUBST(VENDOR,redhat)
else
  AC_SUBST(RPM_ROOT,/usr/src/packages)
  AC_SUBST(VENDOR,suse)
fi 

#
# Generated File Warnings
#
AC_SUBST_FILE(GENERATED_C_FILE_WARNING)
GENERATED_C_FILE_WARNING=$srcdir/helpers/c_file_warning

AC_SUBST_FILE(GENERATED_SCRIPT_FILE_WARNING)
GENERATED_SCRIPT_FILE_WARNING=$srcdir/helpers/script_file_warning

AC_SUBST_FILE(GENERATED_XML_FILE_WARNING)
GENERATED_XML_FILE_WARNING=$srcdir/helpers/xml_file_warning

AC_SUBST_FILE(GENERATED_INI_FILE_WARNING)
GENERATED_INI_FILE_WARNING=$srcdir/helpers/ini_file_warning

AC_CONFIG_FILES([ \
    conf/Makefile \
    debian/Makefile \
    debian/source/Makefile \
    docs/Makefile \
    docs/client_tables/Makefile \
    docs/management_tables/Makefile \
    helpers/Makefile \
    icons/Makefile \
    src/Makefile \
    src/common/Makefile \
    src/gncd/Makefile \
    src/gnmc/Makefile \
    src/gnmd/Makefile \
    systemd/gncd.service \
    systemd/gnmd.service \
    systemd/Makefile \
    xdg/Makefile \
    build_debs.sh \
    glassnet.spec \
    Makefile ])
AC_OUTPUT()
chmod 755 build_debs.sh

#
# Link Common Elements
#
./link_common.sh gncd
./link_common.sh gnmc
./link_common.sh gnmd

AC_MSG_NOTICE()
AC_MSG_NOTICE("|---------------------------------------------------------|")
AC_MSG_NOTICE("|         *** GLASSNET CONFIGURATION SUMMARY ***          |")
AC_MSG_NOTICE("|---------------------------------------------------------|")
AC_MSG_NOTICE("|                                                         |")
if test -z $BUILD_GNCD ; then
AC_MSG_NOTICE("|      Build Stream Receiver Component (gncd(1)) ... No   |")
else
AC_MSG_NOTICE("|      Build Stream Receiver Component (gncd(1)) ... Yes  |")
fi
if test -z $BUILD_GNMC ; then
AC_MSG_NOTICE("|           Build Management Component (gnmc(1)) ... No   |")
else
AC_MSG_NOTICE("|           Build Management Component (gnmc(1)) ... Yes  |")
fi
if test -z $BUILD_GNMD ; then
AC_MSG_NOTICE("|               Build Server Component (gnmd(8)) ... No   |")
else
AC_MSG_NOTICE("|               Build Server Component (gnmd(8)) ... Yes  |")
fi
AC_MSG_NOTICE("|---------------------------------------------------------|")
AC_MSG_NOTICE()
AC_MSG_NOTICE()
AC_MSG_NOTICE(Now enter 'make' to build the software.)
AC_MSG_NOTICE()
