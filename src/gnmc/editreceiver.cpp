// editreceiver.cpp
//
// Edit a GlassNet Receiver
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

#include <QMessageBox>

#include "db.h"
#include "editreceiver.h"
#include "globals.h"
#include "receiver.h"

EditReceiver::EditReceiver(QWidget *parent)
  : QDialog(parent)
{
  QFont bold_font(font().family(),font().pointSize(),QFont::Bold);
  setMinimumSize(sizeHint());

  //
  // Type
  //
  edit_type_label=new QLabel(tr("Type")+":",this);
  edit_type_label->setFont(bold_font);
  edit_type_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  edit_type_box=new ComboBox(this);
  for(int i=1;i<Receiver::TypeLast;i++) {
    edit_type_box->insertItem(-1,Receiver::typeString((Receiver::Type)i),
			      (Receiver::Type)i);
  }

  //
  // MAC Address
  //
  edit_mac_label=new QLabel(tr("MAC Address")+":",this);
  edit_mac_label->setFont(bold_font);
  edit_mac_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  edit_mac_edit=new QLineEdit(this);
  edit_mac_edit->setMaxLength(17);

  //
  // Default Feed
  //
  edit_default_feed_label=new QLabel(tr("Default Feed")+":",this);
  edit_default_feed_label->setFont(bold_font);
  edit_default_feed_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  edit_default_feed_box=new ComboBox(this);
  edit_default_feed_model=new SqlTableModel(true,this);
  QString sql=QString("select ")+
    "`ID`,"+    // 00
    "`NAME` "+  // 01
    "from `FEEDS` order by `NAME`";
  edit_default_feed_model->setQuery(sql);
  edit_default_feed_box->setModel(edit_default_feed_model);
  edit_default_feed_box->setModelColumn(1);
  
  //
  // Remarks
  //
  edit_remarks_label=new QLabel(tr("Notes"),this);
  edit_remarks_label->setFont(bold_font);
  edit_remarks_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  edit_remarks_textedit=new QTextEdit(this);

  //
  // Ok Button
  //
  edit_ok_button=new QPushButton(tr("OK"),this);
  edit_ok_button->setFont(bold_font);
  connect(edit_ok_button,SIGNAL(clicked()),this,SLOT(okData()));

  //
  // Cancel Button
  //
  edit_cancel_button=new QPushButton(tr("Cancel"),this);
  edit_cancel_button->setFont(bold_font);
  connect(edit_cancel_button,SIGNAL(clicked()),this,SLOT(cancelData()));
}


EditReceiver::~EditReceiver()
{
}


QSize EditReceiver::sizeHint() const
{
  return QSize(640,480);
}
  

int EditReceiver::exec(int *receiver_id)
{
  edit_receiver_id=receiver_id;
  if(*receiver_id>0) {
    setWindowTitle(tr("GlassNet - Edit Receiver")+
		   QString::asprintf(" %d",*receiver_id));
    Receiver *rcvr=new Receiver(*receiver_id);
    edit_type_box->setCurrentItemData(rcvr->type());
    edit_mac_edit->setText(rcvr->macAddress());
    edit_default_feed_box->
     setCurrentIndex(edit_default_feed_model->rowNumber(rcvr->defaultFeedId()));
    edit_remarks_textedit->setHtml(rcvr->remarks());
    delete rcvr;
  }
  else {
    setWindowTitle(tr("GlassNet - Edit Receiver [new]"));
    edit_mac_edit->setText("00:00:00:00:00:00");
    edit_remarks_textedit->clear();
  }
  return QDialog::exec();
}


void EditReceiver::okData()
{
  if(!Receiver::isMacAddress(edit_mac_edit->text())) {
    QMessageBox::warning(this,tr("GlassNet - Format Error"),
			 tr("The MAC address entry")+
			 " \""+edit_mac_edit->text()+"\" "+
			 tr("is mal-formatted."));
    return;
  }
  if(*edit_receiver_id<0) {
    *edit_receiver_id=Receiver::create((Receiver::Type)edit_type_box->currentItemData().toInt(),edit_mac_edit->text().toUpper());
  }
  else {
    Receiver *rcvr=new Receiver(*edit_receiver_id);
    rcvr->setType((Receiver::Type)edit_type_box->currentItemData().toInt());
    rcvr->setMacAddress(edit_mac_edit->text().toUpper());
    rcvr->setDefaultFeedId(edit_default_feed_model->
			   id(edit_default_feed_box->currentIndex()).toInt());
    rcvr->setRemarks(edit_remarks_textedit->toHtml());
    delete rcvr;
  }

  done(true);
}


void EditReceiver::cancelData()
{
  done(false);
}


void EditReceiver::resizeEvent(QResizeEvent *e)
{
  edit_type_label->setGeometry(10,10,120,20);
  edit_type_box->setGeometry(135,10,size().width()-380,20);

  edit_mac_label->setGeometry(size().width()-265,10,120,20);
  edit_mac_edit->setGeometry(size().width()-140,10,130,20);

  edit_default_feed_label->setGeometry(10,32,120,20);
  edit_default_feed_box->setGeometry(135,32,size().width()-145,20);
  
  edit_remarks_label->setGeometry(15,54,size().width()-25,20);
  edit_remarks_textedit->
    setGeometry(10,74,size().width()-20,size().height()-140);

  edit_ok_button->setGeometry(size().width()-180,size().height()-60,80,50);
  edit_cancel_button->setGeometry(size().width()-90,size().height()-60,80,50);
}
