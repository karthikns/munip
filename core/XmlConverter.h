/*************************************************************************************************
Interface Version : 0.0
Interface Completion Date : 01/04/2010

Made specifically for MuNIP to generate MusicXML files readable in GuitarPro5

FEATURES:

-- Adds plain notes, chords
-- Automatic measure(bar) addition
-- Tempo, Beat, Beat-Style setting features
-- Tie and Slur addition feature (Check user manual at the end)
-- Xml file/text output of notes added can be obtained


INTERFACE USAGE SPECIFICS:

-- Call XmlConverter::initTypes() before any object of the class is instantiated
-- Error Code can be obtained anytime using XmlConverter::getErrorCode()
      >> 0 - No Error
      >> 1 - Configuration File Not Present/No Read Permissions
      >> 2 - Configuration File Xml Parse Error
      >> 3 - Output file creation failed
      >> 4 - Trying to end a tie when a slur is supposed to end
-- "step" takes values "C", "D", "E", "F", "G", "A", "B"
-- "octave" takes values "1" to "7"
-- "type" takes values "whole", "half", "quarter", "16th", "32th", "64th"

**************************************************************************************************/

#ifndef XMLCONVERTER_H
#define XMLCONVERTER_H

#include <QtXml>

class XmlConverter
{
public:
    static void initTypes();
private:
    static QHash<QString,int> typeHash;

public:
    XmlConverter(QString outputFile, int tempo=120, int b=4, int bType=4);

    void addPlainNote(QString step, QString octave, QString type);
    void addChord(QList<QString> step, QList<QString> octave, QString type);

    void enableTie();
    void disableTie();
    void endSlur();  // Auto Disables

    void domTreeToXmlString(QString &string);
    void domTreeToXmlFile();

    int getErrorCode() const;

private:
    void addMeasure();
    void setTempo();
    void setBeatBeatType();

    QDomDocument doc;
    int tempo;
    int beats;
    int beatType;

    int currentMeasure;
    int currentBarCount;
    int maxBarCount;

    bool startTieSet;
    bool endTieSet;
    bool slurSet;

    int errorCode;

    QString outputFileName;
};

/*
----------------------------------------
INTERFACE USER MANUAL FOR TIES AND SLURS
----------------------------------------
~~Ties~~
--------
Before adding the first note(chord) of tie, call XmlConverter::enableTie();
Before adding the last note(chord) of tie, call XmlConverter::disableTie();
However if the XmlConverter::disableTie() is not called, the tie mode remains active
-----------------------------------------
~~Slurs~~
---------
Call XmlConverter::endSlur before adding the last note(chord) of the slur
-----------------------------------------
*/

#endif // XMLCONVERTER_H
