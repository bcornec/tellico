/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "textimporter.h"
#include "../filehandler.h"

using Tellico::Import::TextImporter;

TextImporter::TextImporter(const KURL& url_) : Import::Importer(url_) {
  // FIXME: error handling?
  if(url_.isValid()) {
    m_text = FileHandler::readTextFile(url_);
  }
}

#include "textimporter.moc"
