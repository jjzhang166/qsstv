/**************************************************************************
*   Copyright (C) 2000-2012 by Johan Maes                                 *
*   on4qz@telenet.be                                                      *
*   http://users.telenet.be/on4qz                                         *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "mainwindow.h"
#include "appglobal.h"
#include "utils/logging.h"
#include "dispatch/dispatcher.h"
#include "ui_mainwindow.h"
#include "soundpulse.h"
#include "soundalsa.h"
#include "configdialog.h"
#include "configparams.h"
#include "rigcontrol.h"
#include "waterfalltext.h"
#include "calibration.h"
#include "frequencyselectwidget.h"
#include "hexconvertor.h"
#ifndef QT_NO_DEBUG
#include "scope/scopeview.h"
#endif
#include "scope/scopeoffset.h"
#include "rxfunctions.h"
#include "logbook.h"
#include "testpatternselection.h"
#include <QFont>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScreen>
#include <QApplication>


/**
 * @brief
 *
 * @param parent
 */
mainWindow::mainWindow(QWidget *parent) : QMainWindow(parent),  ui(new Ui::MainWindow)
{
  QApplication::instance()->thread()->setObjectName("qsstv_main");
  wfTextPushButton=new QPushButton("WF Text",this);
  bsrPushButton=new QPushButton("BSR",this);

  freqComboBox=new QComboBox(this);
  idPushButton=new QPushButton("ID",this);
  QFont f;

  freqDisplay = new QLabel(this);
  f.setBold(true);
  f.setPixelSize(15);
  freqDisplay->setFont(f);
  freqDisplay->setObjectName("freqDisplay");
  freqDisplay->setMinimumSize(QSize(100, 0));
  freqDisplay->setMaximumSize(QSize(100, 16777215));
  freqDisplay->setFrameShape(QFrame::Box);
  freqDisplay->setFrameShadow(QFrame::Raised);
  freqDisplay->setLineWidth(2);
  pttText.setText("   PTT");
  pttIcon=new QLabel(this);
  pttIcon->setFixedSize(16,16);
  pttIcon->setPixmap(*greenPXMPtr);
  pttIcon->setFrameShape(QFrame::Panel);
  pttIcon->setFrameShadow(QFrame::Raised);
  pttIcon->setLineWidth(2);
  rigControllerPtr=new rigControl(1); // must preceed configDialog construction
  configDialogPtr=new configDialog(this);
  configDialogPtr->readSettings();

  ui->setupUi(this);
  setWindowTitle(qsstvVersion);
  setWindowIcon(QPixmap(":/icons/qsstv.png"));
  ui->maintabWidget->setCurrentIndex(0);
  ui->statusBar->addPermanentWidget(freqDisplay);
  ui->statusBar->addPermanentWidget(freqComboBox);
  ui->statusBar->addPermanentWidget(wfTextPushButton);
  ui->statusBar->addPermanentWidget(bsrPushButton);
  ui->statusBar->addPermanentWidget(idPushButton);
  ui->statusBar->addPermanentWidget(&pttText);
  ui->statusBar->addPermanentWidget(pttIcon);
  statusBarPtr=statusBar(); // must be after setup UI
  spectrumFramePtr=ui->spectrumFrame;


  // setting up pointers
  soundIOPtr=NULL;


  rxWidgetPtr=ui->rxWindow;
  txWidgetPtr=ui->txWindow;
  galleryWidgetPtr=ui->galleryWindow;
  readSettings();
  if(pulseSelected) soundIOPtr=new soundPulse;
  else  soundIOPtr=new soundAlsa;

  dispatcherPtr=new dispatcher;
  waterfallPtr=new waterfallText;
  xmlIntfPtr=new xmlInterface;
  logBookPtr=new logBook;

  // setup connections

  connect(ui->actionExit,SIGNAL(triggered()),this, SLOT(slotExit()));
  connect(ui->actionConfigure,SIGNAL(triggered()),this, SLOT(slotConfigure()));
  connect(ui->actionCalibrate,SIGNAL(triggered()),this, SLOT(slotCalibrate()));
  connect(ui->actionAboutQSSTV, SIGNAL(triggered()),SLOT(slotAboutQSSTV()));
  connect(ui->actionAboutQt, SIGNAL(triggered()),SLOT(slotAboutQt()));
  connect(ui->actionUsersGuide, SIGNAL(triggered()),SLOT(slotDocumentation()));
  connect(idPushButton, SIGNAL(clicked()), this, SLOT(slotSendID()));
  connect(bsrPushButton, SIGNAL(clicked()), this, SLOT(slotSendBSR()));
  connect(freqComboBox,SIGNAL(activated(int)),SLOT(slotSetFrequency(int)));
  connect(wfTextPushButton, SIGNAL(clicked()), this, SLOT(slotSendWfText()));
  connect(rxWidgetPtr,SIGNAL(modeSwitch(int)),this, SLOT(slotModeChange(int)));
  connect(txWidgetPtr,SIGNAL(modeSwitch(int)),this, SLOT(slotModeChange(int)));

#ifdef ENABLELOGGING
  connect(ui->actionLogSettings, SIGNAL(triggered()),SLOT(slotLogSettings()));
  connect(ui->actionResetLog, SIGNAL(triggered()),SLOT(slotResetLog()));
#else
  ui->menuOptions->removeAction(ui->actionLogSettings);
  ui->menuOptions->removeAction(ui->actionResetLog);
#endif
#ifndef QT_NO_DEBUG

  connect(ui->actionShowDataScope, SIGNAL(triggered()),SLOT(slotShowDataScope()));
  connect(ui->actionShowSyncScopeNarrow, SIGNAL(triggered()),SLOT(slotShowSyncScopeNarrow()));
  connect(ui->actionShowSyncScopeWide, SIGNAL(triggered()),SLOT(slotShowSyncScopeWide()));
  connect(ui->actionScopeOffset,SIGNAL(triggered()),this, SLOT(slotScopeOffset()));
  connect(ui->actionClearScope,SIGNAL(triggered()),this, SLOT(slotClearScope()));
  connect(ui->actionDumpSamplesPerLine,SIGNAL(triggered()),this, SLOT(slotDumpSamplesPerLine()));
  connect(ui->actionTxTestPattern,SIGNAL(triggered()),this, SLOT(slotTxTestPattern()));



#else
  ui->menuOptions->removeAction(ui->actionDumpSamplesPerLine);
  ui->menuScope->removeAction(ui->actionShowDataScope);
  ui->menuScope->removeAction(ui->actionShowSyncScopeNarrow);
  ui->menuScope->removeAction(ui->actionShowSyncScopeWide);

  ui->menuScope->removeAction(ui->actionScopeOffset);
  ui->menuScope->menuAction()->setVisible(false);

#endif
}

/**
 * @brief
 *
 */
mainWindow::~mainWindow()
{
  delete ui;
}

/**
 * @brief initialize sound device and dispatcher
 *
 */
void mainWindow::init()
{
  // starting threads
  // todo  ftp threads
  cleanUpCache(rxSSTVImagesPath);
  cleanUpCache(rxDRMImagesPath);
  cleanUpCache(txSSTVImagesPath);
  cleanUpCache(txDRMImagesPath);
  //start rx and tx threads
  rxWidgetPtr->functionsPtr()->start();
  txWidgetPtr->functionsPtr()->start();
  restartSound(true);
  dispatcherPtr->init();
  galleryWidgetPtr->init();
  txWidgetPtr->init();
  waterfallPtr->init();
  rigControllerPtr->init();
  rxWidgetPtr->init();
  if(!rigControllerPtr->initError.isEmpty())
    {
      splashStr+=rigControllerPtr->initError.rightJustified(25,' ')+"\n";
      splashPtr->showMessage(splashStr ,Qt::AlignLeft,Qt::white);
    }
  startTimer(1000);
}

void mainWindow::restartSound(bool inStartUp)
{
  //first check if sound
  if(soundIOPtr!=NULL)
    {
      soundIOPtr->stopSoundThread();
      delete soundIOPtr;
      soundIOPtr=NULL;
    }
  if(pulseSelected) soundIOPtr=new soundPulse;
  else soundIOPtr=new soundAlsa;
  if(!soundIOPtr->init(BASESAMPLERATE))
    {
      if(inStartUp)
        {
          splashStr+=QString("Soundcard error: %1").arg(soundIOPtr->getLastError()).rightJustified(25,' ')+"\n";;
          splashPtr->showMessage(splashStr ,Qt::AlignLeft,Qt::white);
        }
      else
        {
          QMessageBox::critical(this, tr("Soundcard error"),soundIOPtr->getLastError());
        }
    }
  soundIOPtr->start();
}


void mainWindow::startRunning()
{
  dispatcherPtr->startRX();
}


void mainWindow::readSettings()
{


  QSettings qSettings;
  qSettings.beginGroup("MAIN");
  int windowWidth = qSettings.value("windowWidth", 460 ).toInt();
  int windowHeight = qSettings.value("windowHeight", 530 ).toInt();
  int windowX = qSettings.value( "windowX", -1 ).toInt();
  int windowY = qSettings.value( "windowY", -1 ).toInt();
  resize( windowWidth, windowHeight );
  if ( windowX != -1 || windowY != -1 )
    {
      move( windowX, windowY );
    }
  transmissionModeIndex=(etransmissionMode)qSettings.value("transmissionModeIndex",0).toInt();
  qSettings.endGroup();
  ui->spectrumFrame->readSettings();
  //  configDialogPtr->readSettings();
  logFilePtr->readSettings();

}

void mainWindow::writeSettings()
{
  QSettings qSettings;
  qSettings.beginGroup("MAIN");
  qSettings.setValue( "windowWidth", width() );
  qSettings.setValue( "windowHeight", height() );
  qSettings.setValue( "windowX", x() );
  qSettings.setValue( "windowY", y() );
  qSettings.setValue("transmissionModeIndex",(int)transmissionModeIndex);
  logFilePtr->writeSettings();
  galleryWidgetPtr->writeSettings();
  rxWidgetPtr->writeSettings();
  txWidgetPtr->writeSettings();
  configDialogPtr->writeSettings();
  logFilePtr->writeSettings();
  qSettings.endGroup();
  ui->spectrumFrame->writeSettings();
}


/**
 *\todo fontselection
 */
void mainWindow::setNewFont()
{
  //  QFont fnt;
  //  fnt.fromString(fontString);
  //  setFont(fnt);
  //  galMW->setFont(fnt);
  //  rxMW->setFont(fnt);
  //  txMW->setFont(fnt);
}

void mainWindow::slotExit()
{
  int exit;
  exit=QMessageBox::information(this, tr("Quit..."),tr("Do you really want to quit QSSTV?"), QMessageBox::Ok, QMessageBox::Cancel);
  if(exit==QMessageBox::Ok)
    {
      dispatcherPtr->idleAll();
      rxWidgetPtr->functionsPtr()->stopThread();
      txWidgetPtr->functionsPtr()->stopThread();
      if(soundIOPtr) soundIOPtr->stopSoundThread();
      writeSettings();
      QApplication::quit();
    }
}

void  mainWindow::closeEvent ( QCloseEvent *e )
{
  slotExit();
  e->ignore();
}

void mainWindow::slotConfigure()
{

  if(configDialogPtr->exec()==QDialog::Accepted)
    {
      if(configDialogPtr->soundNeedsRestart)
        {
          restartSound(false);
        }
      dispatcherPtr->init();
    }
}

void mainWindow::slotLogSettings()
{
  logFilePtr->maskSelect(this);
}

void mainWindow::slotResetLog()
{
  logFilePtr->reset();
}

void mainWindow::slotDocumentation()
{
  QDesktopServices::openUrl(docURL);

}



void mainWindow::slotAboutQSSTV()
{
  QString temp=tr("QSSTV\nVersion: ") + MAJORVERSION + MINORVERSION;
  temp += "\n http://users.telenet.be/on4qz \n(c) 2000-2016 -- Johan Maes - ON4QZ\n HAMDRM Software based on RX/TXAMADRM\n from PA0MBO";
  QMessageBox::about(this,tr("About..."),temp);

}

void mainWindow::slotAboutQt()
{
  QMessageBox::aboutQt(this,tr("About..."));

}
void mainWindow::setPTT(bool p)
{
  if(p) pttIcon->setPixmap(*redPXMPtr);
  else pttIcon->setPixmap(*greenPXMPtr);
}

void mainWindow::setSSTVDRMPushButton(bool inDRM)
{
  int i;
  QString modeStr;
  bsrPushButton->setEnabled(inDRM);
  freqComboBox->clear();
  if(inDRM) modeStr="DRM";
  else modeStr="SSTV";
  modModeList.clear();
  for(i=0;i<freqList.count();i++)
    {
      if(modeList.at(i)==modeStr)
        {
          freqComboBox->addItem(freqList.at(i));
          modModeList.append(sbModeList.at(i));
        }
    }
}

void mainWindow::slotCalibrate()
{
  calibration calib(this);
  if(calib.exec()==QDialog::Accepted)
    {
      rxClock=calib.getRXClock();
      txClock=calib.getTXClock();
    }
  writeSettings();
}

void mainWindow::slotModeChange(int rxtxMode)
{
  txWidgetPtr->changeTransmissionMode(rxtxMode);
  rxWidgetPtr->changeTransmissionMode(rxtxMode);
  lastReceivedCall.clear();
}

void mainWindow::slotSendBSR()
{
  txWidgetPtr->sendBSR();
}


void mainWindow::slotSendID()
{
  txWidgetPtr->sendID();
}



void mainWindow::slotSendWfText()
{
  txWidgetPtr->sendWfText();
}

void mainWindow::slotSetFrequency(int freqIndex)
{
  QByteArray ba;
  QString freqStr,mode;
  freqStr=freqComboBox->itemText(freqIndex);
  mode=modModeList.at(freqIndex);
  if(freqStr.isEmpty()) return;
  double fr=freqStr.toDouble()*1000000.;
  rigControllerPtr->setFrequency(fr);
  rigControllerPtr->setMode(mode);
  QString s=additionalCommand;
  //FEF7AE01A060101FD
  if(!s.isEmpty() && !rigControllerPtr->params()->enableXMLRPC)
    {
      if(hexFromString(s,ba,additionalCommandHex))
        {
          rigControllerPtr->rawCommand(ba);
        }
      else
        {
          QMessageBox::critical(this,"Cat Error","Advanced command invalid");
        }
    }
}




void mainWindow::timerEvent(QTimerEvent *)
{
  double fr;
  if(rigControllerPtr->getFrequency(fr))
    {
      fr/=1000000.;
      if(fr>1) freqDisplay->setText(QString::number(fr,'f',6));
    }
}

void mainWindow::cleanUpCache(QString dirPath)
{
  int i,j;
  bool found;
  QFileInfoList orgFileList;
  QFileInfoList cacheFileList;
  QFileInfoList removeFileList;
  QDir dirOrg(dirPath);
  dirOrg.setFilter(QDir::Files | QDir::NoSymLinks);
  orgFileList = dirOrg.entryInfoList();
  // get a orgiriginal filelist
  QDir dirCache(dirPath+"/cache");
  dirCache.setFilter(QDir::Files | QDir::NoSymLinks);
  cacheFileList = dirCache.entryInfoList();
  // get a orgiriginal filelist
  for(i=0;i<cacheFileList.count();i++)
    {
      found=false;
      for(j=0;j<orgFileList.count();j++)
        {

          if(cacheFileList.at(i).baseName()==QString(orgFileList.at(j).baseName()+orgFileList.at(j).created().toString()))
            {
              found=true;
              break;
            }
        }
      if(!found)
        {
          removeFileList.append(cacheFileList.at(i));
        }
    }
  for(i=0;i<removeFileList.count();i++)
    {
      QFile fi(removeFileList.at(i).absoluteFilePath());
      fi.remove();
    }
}


#ifndef QT_NO_DEBUG
void mainWindow::slotShowDataScope()
{
  scopeViewerData->show(true,true,true,true);
}
void mainWindow::slotShowSyncScopeNarrow()
{
  scopeViewerSyncNarrow->show(true,true,true,true);
}

void mainWindow::slotShowSyncScopeWide()
{
  scopeViewerSyncWide->show(true,true,true,true);
}

void mainWindow::slotScopeOffset()
{
  dataScopeOffset=rxWidgetPtr->functionsPtr()->setOffset(dataScopeOffset,true);
}



void mainWindow::slotDumpSamplesPerLine()
{
  dumpSamplesPerLine();
}

void mainWindow::slotClearScope()
{
  scopeViewerData->clear();
  scopeViewerSyncNarrow->clear();
  scopeViewerSyncWide->clear();
}

void mainWindow::slotTxTestPattern()
{
  etpSelect sel;
  testPatternSelection tpsel;
  if(tpsel.exec()==QDialog::Accepted)
    {
      sel=tpsel.getSelection();
      txWidgetPtr->txTestPattern(sel);
    }
}

#endif
