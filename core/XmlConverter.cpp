#include "XmlConverter.h"
#include <QtXml>

QHash<QString,int> XmlConverter::typeHash;

void XmlConverter :: initTypes()
{
    typeHash.insert("whole",  64);
    typeHash.insert("half",   32);
    typeHash.insert("quarter",16);
    typeHash.insert("eighth",  8);
    typeHash.insert("16th",    4);
    typeHash.insert("32th",    2);
    typeHash.insert("64th",    1);
}


XmlConverter :: XmlConverter(QString outputFile, int t, int b, int bType) :
        currentMeasure(0), currentBarCount(0), startTieSet(false), endTieSet(false), slurSet(false), errorCode(0), outputFileName(outputFile)
{
    QFile file("skeleton.xml");
    if (!file.open(QIODevice::ReadOnly))
        errorCode = 1;                                      // Config File Not Present/No Open Permissions

    QByteArray content = file.readAll();

    QString errorMessage;


    int line, col;
    if (!doc.setContent(content, &errorMessage, &line, &col))
    {
        errorCode = 2;                                      // Config File Xml Parse Error
    }

    tempo = t;
    beats = b;
    beatType = bType;

    setTempo();
    setBeatBeatType();

    maxBarCount = beats * 64 / beatType;
}

void XmlConverter :: setTempo()
{
    QDomElement soundElement  = doc.createElement("sound");
    soundElement.setAttribute("pan","8");
    soundElement.setAttribute("tempo",tempo);

    QDomDocumentFragment fragment = doc.createDocumentFragment();
    fragment.appendChild(soundElement);

    QDomNode node = doc.elementsByTagName("measure").at(0);
    node.insertBefore(fragment, node.firstChild());
}

void XmlConverter :: setBeatBeatType()
{
    QDomElement attributesElement  = doc.createElement("attributes");
    QDomElement timeElement = doc.createElement("time");
    QDomElement beatsElement  = doc.createElement("beats");
    QDomElement beatTypeElement= doc.createElement("beat-type");

    QDomText beatsText = doc.createTextNode(QString::number(beats));
    QDomText beatTypeText = doc.createTextNode(QString::number(beatType));

    beatsElement.appendChild(beatsText);
    beatTypeElement.appendChild(beatTypeText);

    timeElement.appendChild(beatsElement);
    timeElement.appendChild(beatTypeElement);
    attributesElement.appendChild(timeElement);

    QDomDocumentFragment fragment = doc.createDocumentFragment();
    fragment.appendChild(attributesElement);

    QDomNode node = doc.elementsByTagName("measure").at(0);
    node.insertBefore(fragment, node.firstChild());
}


void XmlConverter::addPlainNote(QString step, QString octave, QString type)
{
    if(currentBarCount >= maxBarCount)
        addMeasure();
    currentBarCount += typeHash[type];

    QDomDocumentFragment fragment = doc.createDocumentFragment();

    QDomElement noteElement  = doc.createElement("note");
    QDomElement pitchElement = doc.createElement("pitch");
    QDomElement stepElement  = doc.createElement("step");
    QDomElement octaveElement= doc.createElement("octave");
    QDomElement typeElement  = doc.createElement("type");

    QDomText stepText = doc.createTextNode(step);
    QDomText octaveText = doc.createTextNode(octave);
    QDomText typeText = doc.createTextNode(type);

    stepElement.appendChild(stepText);
    octaveElement.appendChild(octaveText);
    typeElement.appendChild(typeText);

    pitchElement.appendChild(stepElement);
    pitchElement.appendChild(octaveElement);
    noteElement.appendChild(pitchElement);

    if(endTieSet)
    {
        QDomElement tieStopElement = doc.createElement("tie");
        tieStopElement.setAttribute("type","stop");
        noteElement.appendChild(tieStopElement);

        endTieSet=false;
    }
    if(startTieSet)
    {
        QDomElement tieStartElement = doc.createElement("tie");
        tieStartElement.setAttribute("type","start");
        noteElement.appendChild(tieStartElement);

        endTieSet=true;
    }
    if(slurSet)
    {
        QDomElement slurElement = doc.createElement("tie");
        slurElement.setAttribute("type","stop");
        noteElement.appendChild(slurElement);

        slurSet=false;
    }

    noteElement.appendChild(typeElement);

    fragment.appendChild(noteElement);

    QDomNode node = doc.elementsByTagName("measure").at(currentMeasure);
    node.insertAfter(fragment, node.lastChild());
}

void XmlConverter :: addChord(QList<QString> step, QList<QString> octave, QString type)
{
    if(currentBarCount >= maxBarCount)
        addMeasure();
    currentBarCount += typeHash[type];

    int length = step.length() < octave.length() ? step.length() : octave.length();

    QDomDocumentFragment fragment = doc.createDocumentFragment();

    for(int i=0; i<length; ++i)
    {
        QDomElement noteElement  = doc.createElement("note");
        QDomElement chordElement = doc.createElement("chord");
        QDomElement pitchElement = doc.createElement("pitch");
        QDomElement stepElement  = doc.createElement("step");
        QDomElement octaveElement= doc.createElement("octave");
        QDomElement typeElement  = doc.createElement("type");

        QDomText stepText = doc.createTextNode(step[i]);
        QDomText octaveText = doc.createTextNode(octave[i]);
        QDomText typeText = doc.createTextNode(type);

        stepElement.appendChild(stepText);
        octaveElement.appendChild(octaveText);
        typeElement.appendChild(typeText);

        pitchElement.appendChild(stepElement);
        pitchElement.appendChild(octaveElement);
        if(i!=0)
            noteElement.appendChild(chordElement);
        noteElement.appendChild(pitchElement);

        if(endTieSet)
        {
            QDomElement tieStopElement = doc.createElement("tie");
            tieStopElement.setAttribute("type","stop");
            noteElement.appendChild(tieStopElement);

            if(i==length-1)
                endTieSet=false;
        }
        if(startTieSet)
        {
            QDomElement tieStartElement = doc.createElement("tie");
            tieStartElement.setAttribute("type","start");
            noteElement.appendChild(tieStartElement);

            if(i==length-1)
                endTieSet=true;
        }
        if(slurSet)
        {
            QDomElement slurElement = doc.createElement("tie");
            slurElement.setAttribute("type","stop");
            noteElement.appendChild(slurElement);

            if(i==length-1)
                slurSet=false;
        }

        noteElement.appendChild(typeElement);

        fragment.appendChild(noteElement);
    }

    QDomNode node = doc.elementsByTagName("measure").at(currentMeasure);
    node.insertAfter(fragment, node.lastChild());
}

void XmlConverter::addMeasure()
{
    ++currentMeasure;
    currentBarCount = 0;

    QDomDocumentFragment fragment = doc.createDocumentFragment();

    QDomElement measureElement  = doc.createElement("measure");
    measureElement.setAttribute("number",QString::number(currentMeasure+1));

    fragment.appendChild(measureElement);

    QDomNode node = doc.elementsByTagName("part").at(0);
    node.insertAfter(fragment, node.lastChild());
}

void XmlConverter :: enableTie()
{
    startTieSet = true;
}

void XmlConverter :: disableTie()
{
    startTieSet = false;
}

void XmlConverter::endSlur()
{
    if(endTieSet)
        errorCode = 4;                                      // Some idiot trying to end a tie when a slur is supposed to end
    slurSet = true;
}

void XmlConverter :: DomTreeToXmlString(QString&string)
{
    string = qPrintable(doc.toString(4));
}

void XmlConverter :: DomTreeToXmlFile()
{
    if(errorCode)
    {
        return;
    }

    QFile outfile(outputFileName);
    if (!outfile.open(QIODevice::WriteOnly))
    {
        errorCode = 3;                                      //  Output file open failed
        return;
    }

    QByteArray data = doc.toByteArray(4);
    outfile.write(data);
    outfile.close();
}

int XmlConverter :: getErrorCode() const
{
    return errorCode;
}
