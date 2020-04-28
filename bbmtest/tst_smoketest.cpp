#include <QtTest>
#include <QCoreApplication>

// add necessary includes here

class SmokeTest : public QObject
{
    Q_OBJECT

public:
    SmokeTest();
    ~SmokeTest();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_case1();

};

SmokeTest::SmokeTest()
{

}

SmokeTest::~SmokeTest()
{

}

void SmokeTest::initTestCase()
{

}

void SmokeTest::cleanupTestCase()
{

}

void SmokeTest::test_case1()
{
    QVERIFY(true);
}

QTEST_MAIN(SmokeTest)

#include "tst_smoketest.moc"
