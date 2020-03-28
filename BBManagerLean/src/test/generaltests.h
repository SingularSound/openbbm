#ifndef GENERALTESTS_H
#define GENERALTESTS_H

#include <QtTest/QTest>
#include "../bbmanagerapplication.h"

class GeneralTests : public QObject{
Q_OBJECT
public:
    GeneralTests(BBManagerApplication* app){
        this->app = app;
    }
private:
    BBManagerApplication* app;
private slots:
    void initTestCase();
    void appExists();
    void mainWindowExists();
};
#endif // GENERALTESTS_H
