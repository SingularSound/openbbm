/*
 * Re-implementation of QSound keeping only required functionalities and adding volume control implementation
 *
 */

#ifndef MYQSOUND_H
#define MYQSOUND_H

#include <QObject>
#include <QSoundEffect>

class MyQSound : public QObject
{
   Q_OBJECT
public:
   static void play(const QString& filename, uint volume = 100);
   explicit MyQSound(const QString& filename, QObject* parent = nullptr);
   ~MyQSound();

signals:

public slots:
   void play();
   void stop();
   void setVolume(uint volume);
private slots:
   void deleteOnComplete();
private:
   QSoundEffect *mp_soundEffect;

};

#endif // MYQSOUND_H
