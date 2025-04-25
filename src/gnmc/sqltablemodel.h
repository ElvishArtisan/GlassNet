// sqltablemodel.h
//
// Two dimensional, SQL-based data model for GlassNet
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

#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QAbstractTableModel>
#include <QFont>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QSize>
#include <QStringList>
#include <QVariant>

class SqlTableModel : public QAbstractTableModel
{
  Q_OBJECT
 public:
  enum FieldType {DefaultType=0,LengthType=1,ColorTextType=2,
		  AudioLevelType=3,BooleanType=4,ChassisType=5,
		  ReceiverType=6,NumericType=7,TriStateType=8,
		  TimeType=9,BiStateType=10};
  enum TriState {Off=0,On=1,Disabled=2};
  SqlTableModel(bool incl_none,QObject *parent);
  ~SqlTableModel();
  QFont font() const;
  void setFont(const QFont &font);
  QString nullText(int section) const;
  void setNullText(int section,const QString &str);
  int columnCount(const QModelIndex &index=QModelIndex()) const;
  int rowCount(const QModelIndex &index=QModelIndex()) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  QVariant data(int row,int column,int role=Qt::DisplayRole) const;
  void setQuery(const QString &sql,int remarks_col=-1);
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  bool setHeaderData(int section,Qt::Orientation orient,const QVariant &value,
  		     int role=Qt::EditRole);
  FieldType fieldType(int section) const;
  void setFieldType(int section,FieldType type,int key_col=-1);
  QVariant id(int row) const;
  int rowNumber(const QVariant &id);

 public slots:
  void update();
  void setShowRemarks(bool state);

 private:
  QVariant GetHeader(int section) const;
  QFont model_font;
  int model_columns;
  bool model_include_none;
  QString model_sql;
  int model_remarks_column;
  QMap<int,QVariant> model_headers;
  QMap<int,FieldType> model_field_types;
  QMap<int,int> model_field_key_columns;
  QList<QList<QVariant> > model_display_datas;
  QList<QString> model_null_texts;
  QList<QVariant> model_remarks;
  bool model_show_remarks;
  QPixmap *model_greenball_map;
  QPixmap *model_redball_map;
  QPixmap *model_whiteball_map;
};


#endif  //  SQLTABLEMODEL_H
