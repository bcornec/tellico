/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "onixexporter.h"
#include "xslthandler.h"
#include "tellicoxmlexporter.h"
#include "../document.h"
#include "../collection.h"
#include "../filehandler.h"
#include "../tellico_utils.h"
#include "../imagefactory.h"

#include <config.h>

#include <kstandarddirs.h>
#include <kapplication.h>
#include <kzip.h>
#include <kdebug.h>
#include <kconfig.h>

#include <qdom.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qbuffer.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qgroupbox.h>

using Tellico::Export::ONIXExporter;

ONIXExporter::ONIXExporter() : Tellico::Export::Exporter(),
    m_handler(0),
    m_xsltFile(QString::fromLatin1("tellico2onix.xsl")),
    m_includeImages(true),
    m_widget(0) {
}

ONIXExporter::ONIXExporter(const Data::Collection* coll_) : Tellico::Export::Exporter(coll_),
    m_handler(0),
    m_xsltFile(QString::fromLatin1("tellico2onix.xsl")),
    m_includeImages(true),
    m_widget(0) {
}

ONIXExporter::~ONIXExporter() {
  delete m_handler;
  m_handler = 0;
}

QString ONIXExporter::formatString() const {
  return i18n("ONIX Archive");
}

QString ONIXExporter::fileFilter() const {
  return i18n("*.zip|Zip Files (*.zip)") + QChar('\n') + i18n("*|All Files");
}

bool ONIXExporter::exec() {
  const Data::Collection* coll = Data::Document::self()->collection();
  if(!coll) {
    return false;
  }

  QCString xml = text().utf8(); // encoded in utf-8

  QByteArray data;
  QBuffer buf(data);

  KZip zip(&buf);
  zip.open(IO_WriteOnly);
  zip.writeFile(QString::fromLatin1("onix.xml"), QString::null, QString::null, xml.length(), xml);

  // use a dict for fast random access to keep track of which images were written to the file
  if(m_includeImages) { // for now, we're ignoring (options() & Export::ExportImages)
    const QString cover = QString::fromLatin1("cover");
    StringSet imageSet;
    for(Data::EntryVec::ConstIterator it = entries().begin(); it != entries().end(); ++it) {
      const Data::Image& img = ImageFactory::imageById(it->field(cover));
      if(!img.isNull() && !imageSet.has(img.id())
         && (img.format() == "JPEG" || img.format() == "JPG" || img.format() == "GIF")) { /// onix only understands jpeg and gif
        QByteArray ba = img.byteArray();
        zip.writeFile(QString::fromLatin1("images/") + it->field(cover),
                      QString::null, QString::null, ba.size(), ba);
        imageSet.add(img.id());
      }
    }
  }

  zip.close();
  return FileHandler::writeDataURL(url(), data, options() & Export::ExportForce);
//  return FileHandler::writeTextURL(url(), text(),  options() & Export::ExportUTF8, options() & Export::ExportForce);
}

QString ONIXExporter::text() {
  QString xsltfile = locate("appdata", m_xsltFile);
  if(xsltfile.isNull()) {
    kdDebug() << "ONIXExporter::text() - no xslt file for " << m_xsltFile << endl;
    return QString::null;
  }

  const Data::Collection* coll = collection();
  if(!coll) {
    kdDebug() << "ONIXExporter::text() - no collection pointer!" << endl;
    return QString::null;
  }

  // notes about utf-8 encoding:
  // all params should be passed to XSLTHandler in utf8
  // input string to XSLTHandler should be in utf-8, EVEN IF DOM STRING SAYS OTHERWISE

  KURL u;
  u.setPath(xsltfile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements in the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLFile(u, false);
  if(dom.isNull()) {
    kdDebug() << "ONIXExporter::text() - error loading xslt file: " << xsltfile << endl;
    return QString::null;
  }

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!(options() & Export::ExportUTF8)) {
    XSLTHandler::setLocaleEncoding(dom);
  }

  delete m_handler;
  m_handler = new XSLTHandler(dom, QFile::encodeName(xsltfile));

  QDateTime now = QDateTime::currentDateTime();
  m_handler->addStringParam("sentDate", now.toString(QString::fromLatin1("yyyyMMddhhmm")).utf8());

  m_handler->addStringParam("version", VERSION);

  GUI::CursorSaver cs(Qt::waitCursor);

  // now grab the XML
  TellicoXMLExporter exporter(coll);
  exporter.setEntries(entries());
  exporter.setIncludeImages(false); // do not include images in XML
// yes, this should be in utf8, always
  exporter.setOptions(options() | Export::ExportUTF8);
  QDomDocument output = exporter.exportXML();
#if 0
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t << output.toString();
  }
  f.close();
#endif
  return m_handler->applyStylesheet(output.toString());
}

QWidget* ONIXExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Horizontal, i18n("ONIX Archive Options"), m_widget);
  l->addWidget(box);

  m_checkIncludeImages = new QCheckBox(i18n("Include images in archive"), box);
  m_checkIncludeImages->setChecked(m_includeImages);
  QWhatsThis::add(m_checkIncludeImages, i18n("If checked, the images in the document will be included "
                                             "in the zipped ONIX archive."));

  return m_widget;
}

void ONIXExporter::readOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_includeImages = config_->readBoolEntry("Include Images", m_includeImages);
}

void ONIXExporter::saveOptions(KConfig* config_) {
  m_includeImages = m_checkIncludeImages->isChecked();

  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  config_->writeEntry("Include Images", m_includeImages);
}
