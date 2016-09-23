// sqltablemodel.h
//
// Two dimensional, SQL-based data model for GlassNet
//
//   (C) Copyright 2016 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <map>
#include <vector>

#include <QAbstractTableModel>
#include <QFont>
#include <QSize>
#include <QStringList>
#include <QVariant>

class SqlTableModel : public QAbstractTableModel
{
  Q_OBJECT
 public:
  enum FieldType {DefaultType=0,LengthType=1,ColorTextType=2,
		  AudioLevelType=3,BooleanType=4};
  SqlTableModel(QObject *parent=0);
  ~SqlTableModel();
  QFont font() const;
  void setFont(const QFont &font);
  int columnCount(const QModelIndex &index=QModelIndex()) const;
  int rowCount(const QModelIndex &index=QModelIndex()) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  QVariant data(int row,int column,int role=Qt::DisplayRole) const;
  void setQuery(const QString &sql);
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  bool setHeaderData(int section,Qt::Orientation orient,const QVariant &value,
  		     int role=Qt::EditRole);
  FieldType fieldType(int section) const;
  void setFieldType(int section,FieldType type,int key_col=-1);
  void update();
  bool insertRows(int row,const QString &sql);
  bool removeRows(int row,int count,const QModelIndex &parent=QModelIndex());

 private:
  QVariant GetHeader(int section) const;
  QFont model_font;
  int model_columns;
  QString model_sql;
  std::map<int,QVariant> model_headers;
  std::map<int,FieldType> model_field_types;
  std::map<int,int> model_field_key_columns;
  std::vector<std::vector<QVariant> > model_display_datas;
};


#endif  //  SQLTABLEMODEL_H
