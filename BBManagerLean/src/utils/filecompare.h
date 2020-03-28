#ifndef FILECOMPARE_H
#define FILECOMPARE_H
#include <QFile>
#include <stdint.h>

class FileCompare
{
public:
   FileCompare();
   FileCompare(const QString &path1, const QString &path2);
   bool areIdentical();

   void setPath1(const QString &file1);
   void setFile1(QIODevice &file1);
   void setPath2(const QString &file2Path);
   void setFile2(QIODevice &file2);

private:
   uint32_t m_file1Crc;
   uint32_t m_file2Crc;
   qint64 m_file1size;
   qint64 m_file2size;
private:
   static uint32_t computeCRC(const QString &filePath);
   static uint32_t computeCRC(QIODevice &file);

};

#endif // FILECOMPARE_H
