## automake.am
##
## Makefile for the gnmc(1) management client for GlassNet
##
## (C) Copyright 2016-2022 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License version 2 as
##   published by the Free Software Foundation.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public
##   License along with this program; if not, write to the Free Software
##   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##
## Use automake to process this into a Makefile.in

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing -Wno-overloaded-virtual -std=c++17 -fPIC @QT6_CFLAGS@ @OPENSSL_CFLAGS@
MOC = @QT6_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


bin_PROGRAMS = gnmc

dist_gnmc_SOURCES = adduser.cpp adduser.h\
                    changepassword.cpp changepassword.h\
                    combobox.cpp combobox.h\
                    crypto.cpp crypto.h\
                    dayofweekwidget.cpp dayofweekwidget.h\
                    editchassis.cpp editchassis.h\
                    editevent.cpp editevent.h\
                    editfeed.cpp editfeed.h\
                    editreceiver.cpp editreceiver.h\
                    editsite.cpp editsite.h\
                    edituser.cpp edituser.h\
                    eventtablemodel.cpp eventtablemodel.h\
                    eventtableview.cpp eventtableview.h\
                    globals.h\
                    gnmc.cpp gnmc.h\
                    listchassis.cpp listchassis.h\
                    listdialog.cpp listdialog.h\
                    listevents.cpp listevents.h\
                    listfeeds.cpp listfeeds.h\
                    listreceivers.cpp listreceivers.h\
                    listsites.cpp listsites.h\
                    listusers.cpp listusers.h\
                    login.cpp login.h\
                    managementdb.cpp\
                    servicemonitor.cpp servicemonitor.h\
                    sqltablemodel.cpp sqltablemodel.h\
                    tableview.cpp tableview.h\
                    wallclock.cpp wallclock.h

nodist_gnmc_SOURCES = accessor.cpp accessor.h\
                      chassis.cpp chassis.h\
                      cmdswitch.cpp cmdswitch.h\
                      common.h\
                      db.cpp db.h\
                      event.h event.cpp\
                      feed.h feed.cpp\
                      managementconfig.cpp managementconfig.h\
                      moc_db.cpp\
                      moc_adduser.cpp\
                      moc_changepassword.cpp\
                      moc_combobox.cpp\
                      moc_dayofweekwidget.cpp\
                      moc_editchassis.cpp\
                      moc_editevent.cpp\
                      moc_editfeed.cpp\
                      moc_editreceiver.cpp\
                      moc_editsite.cpp\
                      moc_edituser.cpp\
                      moc_eventtablemodel.cpp\
                      moc_eventtableview.cpp\
                      moc_gnmc.cpp\
                      moc_listchassis.cpp\
                      moc_listdialog.cpp\
                      moc_listevents.cpp\
                      moc_listfeeds.cpp\
                      moc_listreceivers.cpp\
                      moc_listsites.cpp\
                      moc_listusers.cpp\
                      moc_login.cpp\
                      moc_servicemonitor.cpp\
                      moc_sqltablemodel.cpp\
                      moc_tableview.cpp\
                      moc_wallclock.cpp\
                      receiver.cpp receiver.h\
                      site.cpp site.h\
                      tzmap.cpp tzmap.h\
                      user.cpp user.h

gnmc_LDADD = @QT6_LIBS@ @OPENSSL_LIBS@

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk
DISTCLEANFILES = accessor.cpp accessor.h\
                 cmdswitch.cpp cmdswitch.h\
                 common.h\
                 chassis.cpp chassis.h\
                 db.cpp db.h\
                 event.h event.cpp\
                 feed.h feed.cpp\
                 receiver.cpp receiver.h\
                 site.cpp site.h\
                 user.cpp user.h\
                 managementconfig.cpp managementconfig.h\
                 streamcmdserver.cpp streamcmdserver.h\
                 tzmap.cpp tzmap.h

MAINTAINERCLEANFILES = *~\
                       Makefile.in
