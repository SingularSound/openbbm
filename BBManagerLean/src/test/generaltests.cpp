#include "generaltests.h"
#include "../mainwindow.h"

void GeneralTests::initTestCase(){
    qDebug() << "Initializing!";
}

void GeneralTests::appExists(){
    QVERIFY(nullptr != this->app);
}

void GeneralTests::mainWindowExists(){
    QVERIFY(nullptr != this->app->getMainWindow());
}
