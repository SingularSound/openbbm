/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "InstrumentLabel.h"
#include "../../Model/drmmakermodel.h"
#include "math.h"

// ********************************************************************************************* //
// **************************************** CONSTRUCTOR **************************************** //
// ********************************************************************************************* //

InstrumentLabel::InstrumentLabel(DrmMakerModel *model, const QString& text, int midiId, int chokeGroup, int nonPercussion, QWidget * parent)
    : BigTextClickableLabelWithErrorIndicator(text,parent)
{
    mp_Model = model;

    // Init members
    mMidiID = mp_Model->getNextAvailableId(midiId);
    mChokeGroup = chokeGroup;

    setNonPercussion(nonPercussion);

    // Refresh text with proper values
    refreshText();
}

void InstrumentLabel::refreshText(){

    // If there is no choke group, don't display it
    setText((mChokeGroup
        ? tr("%1-%2 (vol %3 db) - %4\nChoke Group %5")
        : tr("%1-%2 (vol %3 db) - %4\nNo choke Group%5"))
            .arg(mMidiID)
            .arg(mName)
            .arg(QString::number(- 20.0 * log10(100.0/mVolume),'g',2))
            .arg(1==getNonPercussion() ? "Non-Percussion" : "Percussion")
            .arg((mChokeGroup?QString(mChokeGroup):"")));
}

// Setters and getters
uint InstrumentLabel::getMidiId() {
    return mMidiID;
}

uint InstrumentLabel::getChokeGroup() {
    return mChokeGroup;
}

void InstrumentLabel::setMidiId(uint id){
    mp_Model->switchMidiIds(mMidiID,id);
    mMidiID = id;
    refreshText();
}

void InstrumentLabel::setChokeGroup(uint chokeGroup){
    mChokeGroup = chokeGroup;
    refreshText();
}

void InstrumentLabel::setVolume(uint volume){
   if(mVolume != volume){
      mVolume = volume;
      refreshText();
      emit sigVolumeChanged(mVolume);
   }
}

uint InstrumentLabel::getVolume(){
   return mVolume;
}

void InstrumentLabel::setName(QString name){
    BigTextClickableLabelWithErrorIndicator::setName(name);
    refreshText();
}

void InstrumentLabel::setFillChokeGroup(uint fillChokeGroup)
{
    mFillChokeGroup = fillChokeGroup;
}

uint InstrumentLabel::getFillChokeGroup()
{
    return mFillChokeGroup;
}

void InstrumentLabel::setFillChokeDelay(uint fillChokeDelay)
{
    mFillChokeDelay = fillChokeDelay;
}

uint InstrumentLabel::getFillChokeDelay()
{
    return mFillChokeDelay;
}

uint InstrumentLabel::getPolyphony() {
    return mPolyPhony;
}

void InstrumentLabel::setPolyphony(uint polyPhony){
    mPolyPhony = polyPhony;
}

uint InstrumentLabel::getNonPercussion() {
    return nonPercussion;
}

void InstrumentLabel::setNonPercussion(uint nonPerc){
    nonPercussion = nonPerc;
}

