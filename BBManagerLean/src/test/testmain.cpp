#include <QTest>
#include "generaltests.h"
#include "../bbmanagerapplication.h"

int main(int argc, char** argv){
    BBManagerApplication app(argc, argv);
    GeneralTests gt(&app);
    return QTest::qExec(&gt,argc,argv);
}
