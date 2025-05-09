// gncd.cpp
//
// gncd(1) client daemon for GlassNet
//
//   (C) Copyright 2016-2025 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <map>

#include <QCoreApplication>
#include <QDateTime>
#include <QStringList>
#include <QTcpServer>
#include <QUrl>

#include "cmdswitch.h"
#include "gncd.h"
#include "db.h"
#include "paths.h"
#include "tzmap.h"

#define GNCD_UPGRADE_CRON_ENTRY "/etc/cron.d/gncd-upgrade"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  int startup_delay=30;
  bool ok=false;

  gncd_player_process=NULL;
  gncd_active_guid=-1;
  gncd_update_pass=0;

  CmdSwitch *cmd=new CmdSwitch("gncd",GNCD_USAGE);
  for(unsigned i=0;i<(cmd->keys());i++) {
    if(cmd->key(i)=="--ipv4-address") {
      bool ok=false;
      QStringList f0=cmd->value(i).split("/");
      if(f0.size()!=2) {
	fprintf(stderr,"gncd: invalid --ipv4-address argument\n");
	exit(255);
      }
      gncd_forced_ipv4_address.setAddress(f0.at(0));
      if(gncd_forced_ipv4_address.isNull()) {
	fprintf(stderr,"gncd: invalid --ipv4-address argument\n");
	exit(255);
      }
      uint16_t mask=f0.at(1).toUInt(&ok);
      if((!ok)||(mask>32)) {
	fprintf(stderr,"gncd: invalid --ipv4-address argument\n");
	exit(255);
      }
      uint32_t addr=0;
      for(uint16_t i=0;i<32;i++) {
	addr=addr<<1;
	if(i<mask) {
	  addr=addr|1;
	}
      }
      gncd_forced_ipv4_netmask.setAddress(addr);
      fprintf(stderr,"WARNING: using forced IPv4 address %s/%u!\n",
	     gncd_forced_ipv4_address.toString().toUtf8().constData(),mask);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--startup-delay") {
      startup_delay=cmd->value(i).toInt(&ok);
      if((!ok)||(startup_delay<0)) {
	fprintf(stderr,"invalid --startup-delay value\n");
	exit(255);
      }
      cmd->setProcessed(i,true);
    }

    if(!cmd->processed(i)) {
      fprintf(stderr,"gncd: unknown option\n");
      exit(256);
    }
  }

  gncd_config=new Config();
  gncd_config->load();

  openlog("gncd",LOG_PERROR,LOG_USER);

  //
  // Startup Timer
  //
  gncd_startup_timer=new QTimer(this);
  gncd_startup_timer->setSingleShot(true);
  connect(gncd_startup_timer,SIGNAL(timeout()),this,SLOT(startupData()));
  gncd_startup_timer->start(1000*startup_delay);

  syslog(LOG_DEBUG,"phase 1 started (delay=%d seconds)",startup_delay);
}


void MainObject::startupData()
{
  OpenDb();

  //
  // Command Server
  //
  QTcpServer *server=new QTcpServer(this);
  server->listen(QHostAddress::Any,gncd_config->commandPort());
  std::map<int,QString> cmds;
  std::map<int,int> upper_limits;
  std::map<int,int> lower_limits;

  cmds[MainObject::Delete]="DELETE";
  upper_limits[MainObject::Delete]=1;
  lower_limits[MainObject::Delete]=1;

  cmds[MainObject::Exit]="EXIT";
  upper_limits[MainObject::Exit]=0;
  lower_limits[MainObject::Exit]=0;
  cmds[MainObject::List]="LIST";
  upper_limits[MainObject::List]=1;
  lower_limits[MainObject::List]=0;

  cmds[MainObject::Set]="SET";
  upper_limits[MainObject::Set]=11;
  lower_limits[MainObject::Set]=11;

  cmds[MainObject::Event]="EVENT";
  upper_limits[MainObject::Set]=11;
  lower_limits[MainObject::Set]=11;

  cmds[MainObject::Addr]="ADDR";
  upper_limits[MainObject::Addr]=3;
  lower_limits[MainObject::Addr]=3;

  cmds[MainObject::Clear]="CLEAR";
  upper_limits[MainObject::Clear]=0;
  lower_limits[MainObject::Clear]=0;

  cmds[MainObject::Update]="UPDATE";
  upper_limits[MainObject::Update]=0;
  lower_limits[MainObject::Update]=0;

  cmds[MainObject::Playstart]="PLAYSTART";
  upper_limits[MainObject::Playstart]=1;
  lower_limits[MainObject::Playstart]=1;

  cmds[MainObject::Playstop]="PLAYSTOP";
  upper_limits[MainObject::Playstop]=0;
  lower_limits[MainObject::Playstop]=0;

  cmds[MainObject::Timezone]="TIMEZONE";
  upper_limits[MainObject::Timezone]=1;
  lower_limits[MainObject::Timezone]=1;

  gncd_cmd_server=
    new StreamCmdServer(cmds,upper_limits,lower_limits,server,this);
  connect(gncd_cmd_server,SIGNAL(commandReceived(int,int,const QStringList &)),
	  this,SLOT(commandReceivedData(int,int,const QStringList &)));
  connect(gncd_cmd_server,SIGNAL(connected(int)),this,SLOT(connectedData(int)));

  //
  // Time Engine
  //
  gncd_time_engine=new TimeEngine(this);
  connect(gncd_time_engine,SIGNAL(eventTriggered(unsigned)),
	  this,SLOT(eventTriggeredData(unsigned)));

  gncd_time_engine->reload();

  //
  // Timers
  //
  gncd_stop_timer=new QTimer(this);
  gncd_stop_timer->setSingleShot(true);
  connect(gncd_stop_timer,SIGNAL(timeout()),this,SLOT(stopData()));

  gncd_ping_timer=new QTimer(this);
  connect(gncd_ping_timer,SIGNAL(timeout()),this,SLOT(pingData()));

  //
  // Connect to Management Server
  //
  /*
  printf("host: %s:%d\n",(const char *)gncd_config->callbackHostname().toUtf8(),
				 gncd_config->callbackPort());
  */
  gncd_cmd_server->connectToHost(gncd_config->callbackHostname(),
				 gncd_config->callbackPort());
  syslog(LOG_DEBUG,"phase 2 started");
}


void MainObject::connectedData(int id)
{
  QStringList args;

  ReadInterface();
  args.push_back(gncd_mac_address);
  args.push_back(gncd_ipv4_address.toString());
  args.push_back(VERSION);
  gncd_cmd_server->sendCommand(id,MainObject::Addr,args);

  args.clear();
  if(gncd_active_guid<0) {
    gncd_cmd_server->sendCommand(id,MainObject::Playstop);
  }
  else {
    args.push_back(QString::asprintf("%d",gncd_active_guid));
    gncd_cmd_server->sendCommand(id,MainObject::Playstart,args);
  }
  gncd_ping_timer->start(GLASSNET_RECEIVER_PING_INTERVAL);
}


void MainObject::commandReceivedData(int id,int cmd,const QStringList &args)
{
  /*
  printf("CMD: %d\n",cmd);
  for(int i=0;i<args.size();i++) {
    printf("[%d]: %s\n",i,(const char *)args[i].toUtf8());
  }
  printf("\n");
  */
  switch((MainObject::Commands)cmd) {
  case MainObject::Delete:
    ProcessDelete(id,args);
    break;

  case MainObject::Exit:
    gncd_cmd_server->closeConnection(id);
    break;

  case MainObject::List:
    ProcessList(id,args);
    break;

  case MainObject::Set:
    ProcessSet(id,args);
    break;

  case MainObject::Clear:
    ProcessClear(id);
    break;

  case MainObject::Update:
    ProcessUpdate(id);
    break;

  case MainObject::Playstop:
    if(gncd_player_process!=NULL) {
      gncd_player_process->terminate();
    }
    break;

  case MainObject::Playstart:
    ProcessPlaystart(id,args);
    break;

  case MainObject::Timezone:
    ProcessTimezone(id,args);
    break;

  case MainObject::Event:
  case MainObject::Addr:
    break;
  }
}


void MainObject::eventTriggeredData(unsigned guid)
{
  //  printf("eventTriggeredData(%u)\n",guid);

  if((gncd_player_process!=NULL)&&
     (gncd_player_process->state()!=QProcess::NotRunning)) {
    return;
  }

  QString sql;
  SqlQuery *q=NULL;

  sql=QString("select ")+
    "URL,"+
    "LENGTH "+
    "from EVENTS where "+
    QString::asprintf("GUID=%u",guid);
  q=new SqlQuery(sql);
  if(q->first()) {
    QStringList args;

    args.push_back("--stats-out");
    args.push_back("--audio-device="+gncd_config->audioDevice());
    args.push_back("--alsa-device="+gncd_config->alsaDevice());
    args.push_back(q->value(0).toString());
    if(gncd_player_process!=NULL) {
      delete gncd_player_process;
    }
    gncd_player_process=new QProcess(this);
    connect(gncd_player_process,SIGNAL(started()),
	    this,SLOT(playerStartedData()));
    connect(gncd_player_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	    this,SLOT(playerFinishedData(int,QProcess::ExitStatus)));
    connect(gncd_player_process,SIGNAL(error(QProcess::ProcessError)),
	    this,SLOT(playerErrorData(QProcess::ProcessError)));

    gncd_player_process->start("/usr/bin/glassplayer",args);
    gncd_active_guid=guid;
    gncd_stop_timer->start(q->value(1).toInt());
    syslog(LOG_DEBUG,"player starting: /usr/bin/glassplayer %s",
	   args.join(" ").toUtf8().constData());
  }
  delete q;
}


void MainObject::playerStartedData()
{
  QStringList args;

  args.push_back(QString::asprintf("%d",gncd_active_guid));
  gncd_cmd_server->sendCommand(MainObject::Playstart,args);
  syslog(LOG_DEBUG,"player start acknowledged");
}


void MainObject::playerFinishedData(int exit_code,QProcess::ExitStatus status)
{
  if(status!=QProcess::NormalExit) {
    syslog(LOG_WARNING,"glassplayer process crashed");
  }
  else {
    if(exit_code!=0) {
      syslog(LOG_WARNING,
	     "glassplayer process returned non-zero exit code %d [%s]",
	     exit_code,
	     gncd_player_process->readAllStandardError().constData());
    }
  }
  syslog(LOG_DEBUG,"player finish acknowledged");
  gncd_active_guid=-1;
  gncd_cmd_server->sendCommand(MainObject::Playstop);
  gncd_stop_timer->stop();
}


void MainObject::playerErrorData(QProcess::ProcessError err)
{
  gncd_active_guid=-1;
  syslog(LOG_WARNING,"glassplayer process error %d",err);
}


void MainObject::updateFinishedData(int exit_code,QProcess::ExitStatus status)
{
  QStringList args;
  QProcess *p=NULL;

  if(gncd_update_pass>0) {
    QTimer *timer=new QTimer(this);
    timer->setSingleShot(true);
    connect(timer,SIGNAL(timeout()),this,SLOT(rebootData()));
    timer->start(60000);
  }

#ifdef HAVE_DEB
  args.push_back("-y");
  args.push_back("upgrade");
  p=new QProcess(this);
  connect(p,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(updateFinishedData(int,QProcess::ExitStatus)));
  connect(p,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(updateErrorData(QProcess::ProcessError)));
  gncd_update_pass=1;
  p->start("/usr/bin/apt",args);
#endif  // HAVE_DEB

#ifdef HAVE_RPM
  args.push_back("-q");
  args.push_back("-y");
  args.push_back("update");
  p=new QProcess(this);
  connect(p,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(updateFinishedData(int,QProcess::ExitStatus)));
  connect(p,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(updateErrorData(QProcess::ProcessError)));
  gncd_update_pass=1;
  p->start("/usr/bin/yum",args);
#endif  // HAVE_RPM
}


void MainObject::updateErrorData(QProcess::ProcessError err)
{
  syslog(LOG_WARNING,"update process error %d",err);
}


void MainObject::stopData()
{
  gncd_player_process->terminate();
}


void MainObject::pingData()
{
  QStringList args;

  if(ReadInterface()) {  // Reset needed
    gncd_cmd_server->closeAllConnections();
    gncd_cmd_server->connectToHost(gncd_config->callbackHostname(),
				   gncd_config->callbackPort());
  }
  else {
    args.push_back(gncd_mac_address);
    args.push_back(gncd_ipv4_address.toString());
    args.push_back(VERSION);
    gncd_cmd_server->sendCommand(MainObject::Addr,args);
  }
}


void MainObject::rebootData()
{
  Reboot();
  exit(256);
}


bool MainObject::ProcessDelete(int id,const QStringList &args)
{
  bool ok=false;
  unsigned guid=args[0].toUInt(&ok);

  if(!ok) {
    return false;
  }
  QString sql=QString("delete from EVENTS where ")+
    QString::asprintf("GUID=%u",guid);
  SqlQuery::run(sql);
  gncd_time_engine->reload();

  return true;
}


void MainObject::ProcessList(int id,const QStringList &args)
{
  QString sql=QString("select ")+
    "GUID,"+        // 00
    "START_TIME,"+  // 01
    "LENGTH,"+      // 02
    "SUN,"+         // 03
    "MON,"+         // 04
    "TUE,"+         // 05
    "WED,"+         // 06
    "THU,"+         // 07
    "FRI,"+         // 08
    "SAT,"+         // 09
    "URL "+         // 10
    "from EVENTS";
  if(args.size()>0) {
    sql+=" where GUID="+args[0];
  }
  SqlQuery *q=new SqlQuery(sql);
  while(q->next()) {
    QStringList resp;
    resp.push_back(QString::asprintf("%d",q->value(0).toInt()));
    resp.push_back(QTime(0,0,0).addSecs(q->value(1).toInt()).toString());
    resp.push_back(QString::asprintf("%d",q->value(2).toInt()/1000));
    resp.push_back(QString::asprintf("%d",q->value(3).toInt()));
    resp.push_back(QString::asprintf("%d",q->value(4).toInt()));
    resp.push_back(QString::asprintf("%d",q->value(5).toInt()));
    resp.push_back(QString::asprintf("%d",q->value(6).toInt()));
    resp.push_back(QString::asprintf("%d",q->value(7).toInt()));
    resp.push_back(QString::asprintf("%d",q->value(8).toInt()));
    resp.push_back(QString::asprintf("%d",q->value(9).toInt()));
    resp.push_back(q->value(10).toString());
    gncd_cmd_server->sendCommand(id,(int)MainObject::Event,resp);
  }
  delete q;
}


bool MainObject::ProcessSet(int id,const QStringList &args)
{
  QString sql;
  SqlQuery *q=NULL;
  bool ok=false;

  //
  // Parse Command
  //
  unsigned guid=args[0].toInt(&ok);
  if(!ok) {
    return false;
  }
  QTime start_time=QTime::fromString(args[1],"hh:mm:ss");
  if(!start_time.isValid()) {
    return false;
  }
  unsigned length=args[2].toUInt(&ok);
  if(!ok) {
    return false;
  }
  bool dow[7];
  for(unsigned i=0;i<7;i++) {
    dow[i]=args[3+i]!="0";
  }
  QUrl url(args[10]);
  if(!url.isValid()) {
    return false;
  }

  //
  // Process
  //
  sql=QString("select GUID from EVENTS where ")+
    QString::asprintf("GUID=%u",guid);
  q=new SqlQuery(sql);
  if(q->first()) {
    sql=QString("update EVENTS set ")+
      QString::asprintf("START_TIME=%u,",QTime(0,0,0).secsTo(start_time))+
      QString::asprintf("LENGTH=%u,",1000*length)+
      QString::asprintf("SUN=%u,",dow[0])+
      QString::asprintf("MON=%u,",dow[1])+
      QString::asprintf("TUE=%u,",dow[2])+
      QString::asprintf("WED=%u,",dow[3])+
      QString::asprintf("THU=%u,",dow[4])+
      QString::asprintf("FRI=%u,",dow[5])+
      QString::asprintf("SAT=%u,",dow[6])+
      "URL='"+SqlQuery::escape(url.toString())+"' where "+
      QString::asprintf("GUID=%u",guid);
  }
  else {
    sql=QString("insert into EVENTS (")+
      "GUID,"+        // 00
      "START_TIME,"+  // 01
      "LENGTH,"+      // 02
      "SUN,"+         // 03
      "MON,"+         // 04
      "TUE,"+         // 05
      "WED,"+         // 06
      "THU,"+         // 07
      "FRI,"+         // 08
      "SAT,"+         // 09
      "URL) "+         // 10
      "VALUES("+
      QString::asprintf("%u,",guid)+                             // 00
      QString::asprintf("%u,",QTime(0,0,0).secsTo(start_time))+  // 01
      QString::asprintf("%u,",1000*length)+                      // 02
      QString::asprintf("%d,",dow[0])+                           // 03
      QString::asprintf("%d,",dow[1])+                           // 04
      QString::asprintf("%d,",dow[2])+                           // 05
      QString::asprintf("%d,",dow[3])+                           // 06
      QString::asprintf("%d,",dow[4])+                           // 07
      QString::asprintf("%d,",dow[5])+                           // 08
      QString::asprintf("%d,",dow[6])+                           // 09
      "'"+SqlQuery::escape(url.toString())+"'"+                  // 10
      ")";
  }
  delete q;
  SqlQuery::run(sql);
  gncd_time_engine->reload();

  return true;
}


void MainObject::ProcessClear(int id)
{
  QString sql=QString("delete from EVENTS");
  SqlQuery::run(sql);
  gncd_time_engine->reload();
}


void MainObject::ProcessPlaystart(int id,const QStringList &args)
{
  bool ok;
  int guid;

  if(args.size()==1) {
    guid=args.at(0).toInt(&ok);
    if(ok&&(guid>0)) {
      eventTriggeredData(guid);
    }
  }
}


void MainObject::ProcessUpdate(int id)
{
#ifdef HAVE_DEB
  //
  // Maintainer's Note
  //
  // The 'upgrade' command for apt(1) must not be issued by a process
  // inherited from the 'glassnet-receiver' package, otherwise the upgrade
  // will be interrupted if that package attempts to upgrade itself, resulting
  // in a corrupt installation. Hence, we schedule a cron job to trigger
  // the upgrade.
  //

  //
  // Create Upgrade Script
  //
  char temp_dir[]={"/tmp/upgrade_gncd_XXXXXX"};
  if(mkdtemp(temp_dir)!=NULL) {
    QString temppath=QString(temp_dir)+"/upgrade_gncd.sh";
    FILE *f=fopen(temppath.toUtf8(),"w");
    if(f!=NULL) {
      fprintf(f,"#!/bin/bash\n");
      fprintf(f,"\n");
      fprintf(f,"/usr/bin/apt -y update\n");
      fprintf(f,"/usr/bin/apt -y upgrade\n");
      fprintf(f,"/usr/sbin/reboot\n");
      fprintf(f,"/bin/unlink %s\n",GNCD_UPGRADE_CRON_ENTRY);

      fclose(f);
      chmod(temppath.toUtf8(),0775);
    }

    //
    // Create cron(5) Entry
    //
    if((f=fopen(GNCD_UPGRADE_CRON_ENTRY,"w"))!=NULL) {
      QDateTime dt=QDateTime::currentDateTime().addSecs(60);
      fprintf(f,"SHELL=/bin/bash\n");
      fprintf(f,"\n");
      fprintf(f,"%d %d * * *\troot\t%s\n",dt.time().minute(),dt.time().hour(),
	      temppath.toUtf8().constData());
      fclose(f);
    }
  }
#endif  // HAVE_DEB

#ifdef HAVE_RPM
  QStringList args;
  QProcess *p=NULL;

  args.push_back("-q");
  args.push_back("-y");
  args.push_back("clean");
  args.push_back("expire-cache");
  p=new QProcess(this);
  connect(p,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(updateFinishedData(int,QProcess::ExitStatus)));
  connect(p,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(updateErrorData(QProcess::ProcessError)));
  gncd_update_pass=0;
  p->start("/usr/bin/yum",args);
#endif  // HAVE_RPM
}


void MainObject::ProcessTimezone(int id,const QStringList &args)
{
  QStringList pargs;

  if(args.size()==1) {
    if(args.at(0)!=TzMap::localTzid()) {
      pargs.push_back("set-timezone");
      pargs.push_back(args.at(0));
      QProcess *proc=new QProcess(this);
      proc->start("/bin/timedatectl",pargs);
      proc->waitForFinished();
      Reboot();
      exit(256);
    }
  }
}


bool MainObject::ReadInterface()
{
  struct ifreq ifr;
  int index=0;
  int sock;
  bool reset_needed=false;
  QHostAddress addr;

  if((sock=socket(PF_INET,SOCK_DGRAM,IPPROTO_IP))<0) {
    syslog(LOG_ERR,"gncd: unable to detect interface");
    exit(256);
  }
  memset(&ifr,0,sizeof(ifr));
  index=1;
  ifr.ifr_ifindex=index;
  while(ioctl(sock,SIOCGIFNAME,&ifr)==0) {
    if(ioctl(sock,SIOCGIFHWADDR,&ifr)==0) {
      if(QString(ifr.ifr_name)==gncd_config->networkInterface()) {
	gncd_mac_address=
	  QString::asprintf("%02X:%02X:%02X:%02X:%02X:%02X",
			    0xff&ifr.ifr_ifru.ifru_hwaddr.sa_data[0],
			    0xff&ifr.ifr_ifru.ifru_hwaddr.sa_data[1],
			    0xff&ifr.ifr_ifru.ifru_hwaddr.sa_data[2],
			    0xff&ifr.ifr_ifru.ifru_hwaddr.sa_data[3],
			    0xff&ifr.ifr_ifru.ifru_hwaddr.sa_data[4],
			    0xff&ifr.ifr_ifru.ifru_hwaddr.sa_data[5]);
	if(gncd_forced_ipv4_address.isNull()) {
	  if(ioctl(sock,SIOCGIFADDR,&ifr)==0) {
	    struct sockaddr_in sa=*(sockaddr_in *)(&ifr.ifr_addr);
	    addr.setAddress(ntohl(sa.sin_addr.s_addr));
	    if(addr!=gncd_ipv4_address) {
	      gncd_ipv4_address=addr;
	      reset_needed=true;
	    }
	  }
	  if(ioctl(sock,SIOCGIFNETMASK,&ifr)==0) {
	    struct sockaddr_in sa=*(sockaddr_in *)(&ifr.ifr_netmask);
	    gncd_ipv4_netmask.setAddress(ntohl(sa.sin_addr.s_addr));
	  }
	}
	else {
	  gncd_ipv4_address=gncd_forced_ipv4_address;
	  gncd_ipv4_netmask=gncd_forced_ipv4_netmask;
	}
      }
    }
    ifr.ifr_ifindex=++index;
  }
  close(sock);

  return reset_needed;
}


void MainObject::Reboot()
{
  QProcess *p=new QProcess(this);
  p->start("/sbin/reboot",QStringList(),QIODevice::ReadWrite);
  p->waitForFinished();
  delete p;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
