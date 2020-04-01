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
#include <QApplication>
#include <QFile>
#include "stylesheethelper.h"



StyleSheetHelper * StyleSheetHelper::sp_Instance;

StyleSheetHelper::StyleSheetHelper(QObject *parent) :
   QObject(parent)
{
   QFile cssFile(":/main.css");
   cssFile.open(QIODevice::ReadOnly);
   mp_MainStyleSheet = new QString(cssFile.readAll());
   cssFile.close();

   cssFile.setFileName(":/selected.css");
   cssFile.open(QIODevice::ReadOnly);
   mp_SelectedStyleSheet = new QString(cssFile.readAll());
   cssFile.close();

   cssFile.setFileName(":/unselected.css");
   cssFile.open(QIODevice::ReadOnly);
   mp_UnselectedStyleSheet = new QString(cssFile.readAll());
   cssFile.close();
}

StyleSheetHelper::~StyleSheetHelper(){
   delete mp_MainStyleSheet;
   delete mp_SelectedStyleSheet;
   delete mp_UnselectedStyleSheet;
}

const QString & StyleSheetHelper::getStyleSheet()
{
   if(StyleSheetHelper::sp_Instance == nullptr){
      sp_Instance = new StyleSheetHelper(QApplication::instance());
   }

   return *sp_Instance->mp_MainStyleSheet;
}

const QString & StyleSheetHelper::getStateStyleSheet(bool selected)
{
   if(StyleSheetHelper::sp_Instance == nullptr){
      sp_Instance = new StyleSheetHelper(QApplication::instance());
   }

   if (selected){
      return *sp_Instance->mp_SelectedStyleSheet;
   } else {
      return *sp_Instance->mp_UnselectedStyleSheet;
   }

}


