// accessor.cpp
//
// Abstract base class for a DB object accessor.
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

#include <stdio.h>

#include "accessor.h"
#include "db.h"

Accessor::Accessor()
{
}


Accessor::~Accessor()
{
}


QVariant Accessor::getRow(const QString &column) const
{
  QVariant ret;

  QString sql=QString("select ")+
    "`"+column+"` from "+
    "`"+tableName()+"` where "+
    whereClause();
  SqlQuery *q=new SqlQuery(sql);
  if(q->first()) {
    ret=q->value(0);
  }
  delete q;

  return ret;
}


void Accessor::setRow(const QString &column,const QString &value) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`='"+SqlQuery::escape(value)+"' where "+
    whereClause();
  SqlQuery::run(sql);
}


void Accessor::setRow(const QString &column,int value) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`="+QString::asprintf("%d",value)+" where "+
    whereClause();
  SqlQuery::run(sql);
}


void Accessor::setRow(const QString &column,unsigned value) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`="+QString::asprintf("%u",value)+" where "+
    whereClause();
  SqlQuery::run(sql);
}


void Accessor::setRow(const QString &column,bool value) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`="+QString::asprintf("%u",value)+" where "+
    whereClause();
  SqlQuery::run(sql);
}


void Accessor::setRow(const QString &column,const QTime &time) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`=\'"+time.toString("hh:mm:ss")+"' where "+
    whereClause();
  SqlQuery::run(sql);
}


void Accessor::setRow(const QString &column,const QDate &date) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`=\'"+date.toString("yyyy-MM-dd")+"' where "+
    whereClause();
  SqlQuery::run(sql);
}


void Accessor::setRow(const QString &column,const QDateTime &dt) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`=\'"+dt.toString("yyyy-MM-dd hh:mm:ss")+"' where "+
    whereClause();
  SqlQuery::run(sql);
}


void Accessor::setRowNull(const QString &column) const
{
  QString sql=QString("update ")+
    "`"+tableName()+"` set "+
    "`"+column+"`=NULL where "+
    whereClause();
  SqlQuery::run(sql);
}


QString Accessor::dowFieldName(int dow) const
{
  QString ret="BAD";

  switch(dow) {
  case 1:
    ret="MON";
    break;
  
  case 2:
    ret="TUE";
    break;
  
  case 3:
    ret="WED";
    break;
  
  case 4:
    ret="THU";
    break;
  
  case 5:
    ret="FRI";
    break;
  
  case 6:
    ret="SAT";
    break;

  case 7:
    ret="SUN";
    break;
  }
  return ret;
}
