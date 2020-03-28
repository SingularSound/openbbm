#include <QTimer>
#include <QDebug>

#include "bbmanagerapplication.h"
#include "mainwindow.h"
#include "debug.h"
#include "stylesheethelper.h"
#include "versioninfo.h"
#include "workspace/settings.h"
#include "workspace/libcontent.h"
#include "workspace/contentlibrary.h"

BBManagerApplication::BBManagerApplication( int &argc, char **argv )
    : QApplication(argc, argv)
    , m_projectBeingOpened(false)
{
    VersionInfo ravi = VersionInfo::RunningAppVersionInfo();
    setOrganizationName(ravi.companyName());
    setOrganizationDomain(ravi.companyDomain());
    setApplicationName(ravi.productName());
    setApplicationVersion(ravi.toQString());
    setWindowIcon(QIcon(":/images/images/App_Icon_64_2.png"));
    setupDebugging();
    qDebug() << "\t|---> Beatbuddy Manager is Starting Up <---|";

    // Make sure the software is uniquely identified for current user
    if(!Settings::softwareUuidExists()){
        Settings::setSoftwareUuid(QUuid::createUuid());
    }
    setStyleSheet(StyleSheetHelper::getStyleSheet()); // Load and set style sheet

    if (arguments().size() > 1){
        mp_mainWindow = new MainWindow(arguments().at(1));
    } else {
        mp_mainWindow = new MainWindow();
        QTimer::singleShot(500, this, SLOT(slotOpenDefaultProject()));
    }
    mp_mainWindow->show();
}

BBManagerApplication::~BBManagerApplication(){
    delete mp_mainWindow;
}

bool BBManagerApplication::event(QEvent *ev)
{
    if(ev->type()==QEvent::FileOpen) {
        QString fileName = static_cast<QFileOpenEvent *>(ev)->file();
        if(!m_projectBeingOpened) {
            m_projectBeingOpened = true;
            mp_mainWindow->slotProcessOpenFileEvent(fileName);
            m_projectBeingOpened = false;
        }
        return true;
    } else {
        return QApplication::event(ev);
    }
}
void BBManagerApplication::slotOpenDefaultProject(){
    if(mp_mainWindow && !mp_mainWindow->hasOpenedProject()
            && !m_projectBeingOpened && Settings::lastProjectExists()){
        m_projectBeingOpened = true;
        mp_mainWindow->slotProcessOpenFileEvent(Settings::getLastProject());
        m_projectBeingOpened = false;
    }
}

