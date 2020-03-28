#ifndef BBMANAGERAPPLICATION_H
#define BBMANAGERAPPLICATION_H

#include <QApplication>

class MainWindow;

class BBManagerApplication : public QApplication
{
    Q_OBJECT
public:
    BBManagerApplication( int &argc, char **argv );
    ~BBManagerApplication();
    inline MainWindow* getMainWindow(){
        return this->mp_mainWindow;
    }
private slots:
    void slotOpenDefaultProject();

protected:
    bool event(QEvent *ev);
private:
    bool m_projectBeingOpened;
    MainWindow* mp_mainWindow;
};

#endif // BBMANAGERAPPLICATION_H
