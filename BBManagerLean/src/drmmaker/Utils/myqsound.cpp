#include "myqsound.h"
#include <QCoreApplication>
/*
 * Re-implementation of QSound keeping only required functionalities and adding volume control implementation
 *
 */


void MyQSound::play(const QString& filename, uint volume)
{
   // Object destruction is generaly handled via deleteOnComplete
   // Unexpected cases will be handled via parenting of QSound objects to qApp
   MyQSound *sound = new MyQSound(filename, qApp);
   sound->connect(sound->mp_soundEffect, SIGNAL(playingChanged()), SLOT(deleteOnComplete()));
   sound->setVolume(volume);
   sound->play();
}

MyQSound::MyQSound(const QString& filename, QObject* parent) :
   QObject(parent)
{
   mp_soundEffect = new QSoundEffect(this);
   mp_soundEffect->setSource(QUrl::fromLocalFile(filename));
}
MyQSound::~MyQSound()
{
   if(mp_soundEffect->isPlaying()){
      stop();
   }
}
void MyQSound::play()
{
   mp_soundEffect->play();
}
void MyQSound::stop()
{
   mp_soundEffect->stop();
}

void MyQSound::setVolume(uint volume)
{
   if(volume > 100){
      volume = 100;
   }
   mp_soundEffect->setVolume((qreal)volume / 100.0);
}

void MyQSound::deleteOnComplete()
{
   if (!mp_soundEffect->isPlaying())
      deleteLater();
}
