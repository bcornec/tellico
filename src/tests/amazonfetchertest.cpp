/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#undef QT_NO_CAST_FROM_ASCII

#include "amazonfetchertest.h"
#include "amazonfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/amazonfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/musiccollection.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include  "../images/imagefactory.h"

#include <KStandardDirs>
#include <KConfigGroup>

QTEST_KDEMAIN( AmazonFetcherTest, GUI )

AmazonFetcherTest::AmazonFetcherTest() : m_loop(this), m_hasConfigFile(false)
    , m_config(QString::fromLatin1(KDESRCDIR)  + "/amazonfetchertest.config", KConfig::SimpleConfig) {
}

void AmazonFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "music");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "mvideo");
  // since we use an XSL file
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();

  m_hasConfigFile = QFile::exists(QString::fromLatin1(KDESRCDIR)  + "/amazonfetchertest.config");

  QHash<QString, QString> practicalRdf;
  practicalRdf.insert(QLatin1String("title"), QLatin1String("Practical RDF"));
  practicalRdf.insert(QLatin1String("isbn"), QLatin1String("0-596-00263-7"));
  practicalRdf.insert(QLatin1String("author"), QLatin1String("Shelley Powers"));
  practicalRdf.insert(QLatin1String("binding"), QLatin1String("Paperback"));
  practicalRdf.insert(QLatin1String("publisher"), QLatin1String("O'Reilly Media"));
  practicalRdf.insert(QLatin1String("pages"), QLatin1String("331"));

  QHash<QString, QString> gloryRevealed;
  gloryRevealed.insert(QLatin1String("title"), QLatin1String("Glory Revealed II"));
  gloryRevealed.insert(QLatin1String("medium"), QLatin1String("Compact Disc"));
//  gloryRevealed.insert(QLatin1String("artist"), QLatin1String("Various Artists"));
  gloryRevealed.insert(QLatin1String("label"), QLatin1String("Reunion"));
  gloryRevealed.insert(QLatin1String("year"), QLatin1String("2009"));

  QHash<QString, QString> incredibles;
  incredibles.insert(QLatin1String("title"), QLatin1String("Incredibles"));
  incredibles.insert(QLatin1String("medium"), QLatin1String("DVD"));
//  incredibles.insert(QLatin1String("certification"), QLatin1String("PG (USA)"));
//  incredibles.insert(QLatin1String("studio"), QLatin1String("Walt Disney Home Entertainment"));
  incredibles.insert(QLatin1String("year"), QLatin1String("2004"));
  incredibles.insert(QLatin1String("widescreen"), QLatin1String("true"));
  incredibles.insert(QLatin1String("director"), QLatin1String("Brad Bird; Bud Luckey; Roger Gould"));

  QHash<QString, QString> pacteDesLoups;
  pacteDesLoups.insert(QLatin1String("title"), QLatin1String("Le Pacte des Loups"));
  pacteDesLoups.insert(QLatin1String("medium"), QLatin1String("Blu-ray"));
//  pacteDesLoups.insert(QLatin1String("region"), QLatin1String("Region 2"));
  pacteDesLoups.insert(QLatin1String("studio"), QLatin1String("StudioCanal"));
  pacteDesLoups.insert(QLatin1String("year"), QLatin1String("2001"));
  pacteDesLoups.insert(QLatin1String("director"), QLatin1String("Christophe Gans"));
//  pacteDesLoups.insert(QLatin1String("format"), QLatin1String("PAL"));

  m_fieldValues.insert(QLatin1String("practicalRdf"), practicalRdf);
  m_fieldValues.insert(QLatin1String("gloryRevealed"), gloryRevealed);
  m_fieldValues.insert(QLatin1String("incredibles"), incredibles);
  m_fieldValues.insert(QLatin1String("pacteDesLoups"), pacteDesLoups);
}

void AmazonFetcherTest::testTitle() {
  QFETCH(QString, locale);
  QFETCH(int, collType);
  QFETCH(QString, searchValue);
  QFETCH(QString, resultName);

  QString groupName = QLatin1String("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(collType, Tellico::Fetch::Title, searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QVERIFY(!m_results.isEmpty());

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues.value(resultName));
  while(i.hasNext()) {
    i.next();
    // a known bug is CA video titles result in music results, so only title matches
    if(i.key() != QLatin1String("title")) {
      QEXPECT_FAIL("CA video title", "CA video titles show music results for some reason", Continue);
    }
    QString result = entry->field(i.key()).toLower();
    // CA and FR titles have edition info in the title
    if(collType == Tellico::Data::Collection::Video &&
       (locale == QLatin1String("CA") || locale == QLatin1String("FR"))) {
      QVERIFY2(result.contains(i.value(), Qt::CaseInsensitive), i.key().toAscii());
    } else {
      QCOMPARE(result, i.value().toLower());
    }
  }
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void AmazonFetcherTest::testTitle_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<int>("collType");
  QTest::addColumn<QString>("searchValue");
  QTest::addColumn<QString>("resultName");

  QTest::newRow("US book title") << QString::fromLatin1("US")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QString::fromLatin1("Practical RDF")
                                 << QString::fromLatin1("practicalRdf");
  QTest::newRow("UK book title") << QString::fromLatin1("UK")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QString::fromLatin1("Practical RDF")
                                 << QString::fromLatin1("practicalRdf");
//  QTest::newRow("DE") << QString::fromLatin1("DE");
//  QTest::newRow("JP") << QString::fromLatin1("JP");
//  QTest::newRow("FR") << QString::fromLatin1("FR");
  QTest::newRow("CA book title") << QString::fromLatin1("CA")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QString::fromLatin1("Practical RDF")
                                 << QString::fromLatin1("practicalRdf");
    // a known bug is CA video titles result in music results, so only title matches
//  QTest::newRow("CA video title") << QString::fromLatin1("CA")
//                                  << static_cast<int>(Tellico::Data::Collection::Video)
//                                  << QString::fromLatin1("Le Pacte des Loups")
//                                  << QString::fromLatin1("pacteDesLoups");
  QTest::newRow("FR video title") << QString::fromLatin1("FR")
                                  << static_cast<int>(Tellico::Data::Collection::Video)
                                  << QString::fromLatin1("Le Pacte des Loups")
                                  << QString::fromLatin1("pacteDesLoups");
}

void AmazonFetcherTest::testIsbn() {
  QFETCH(QString, locale);
  QFETCH(QString, searchValue);
  QFETCH(QString, resultName);

  QString groupName = QLatin1String("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 2);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues.value(resultName));
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void AmazonFetcherTest::testIsbn_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<QString>("searchValue");
  QTest::addColumn<QString>("resultName");

  QTest::newRow("US isbn") << QString::fromLatin1("US")
                           << QString::fromLatin1("0-596-00263-7; 978-1-59059-831-3")
                           << QString::fromLatin1("practicalRdf");
  QTest::newRow("UK isbn") << QString::fromLatin1("UK")
                           << QString::fromLatin1("0-596-00263-7; 978-1-59059-831-3")
                           << QString::fromLatin1("practicalRdf");
//  QTest::newRow("DE") << QString::fromLatin1("DE");
//  QTest::newRow("JP") << QString::fromLatin1("JP");
//  QTest::newRow("FR") << QString::fromLatin1("FR");
  QTest::newRow("CA isbn") << QString::fromLatin1("CA")
                           << QString::fromLatin1("0-596-00263-7; 978-1-59059-831-3")
                           << QString::fromLatin1("practicalRdf");
}

void AmazonFetcherTest::testUpc() {
  QFETCH(QString, locale);
  QFETCH(int, collType);
  QFETCH(QString, searchValue);
  QFETCH(QString, resultName);

  QString groupName = QLatin1String("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(collType, Tellico::Fetch::UPC, searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QVERIFY(!m_results.isEmpty());

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues.value(resultName));
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    // exception for UK label different than US
    // and FR title having edition info
    if((i.key() == QLatin1String("label") && locale == QLatin1String("UK")) ||
       (i.key() == QLatin1String("title"))) {
      QVERIFY(result.contains(i.value(), Qt::CaseInsensitive));
    } else {
      QCOMPARE(result, i.value().toLower());
    }
  }
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  if(collType == Tellico::Data::Collection::Album) {
    QVERIFY(!entry->field(QLatin1String("genre")).isEmpty());
    QVERIFY(!entry->field(QLatin1String("track")).isEmpty());
  } else if(collType == Tellico::Data::Collection::Video) {
    QVERIFY(!entry->field(QLatin1String("cast")).isEmpty());
  }
}

void AmazonFetcherTest::testUpc_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<int>("collType");
  QTest::addColumn<QString>("searchValue");
  QTest::addColumn<QString>("resultName");

  QTest::newRow("US music upc") << QString::fromLatin1("US")
                                << static_cast<int>(Tellico::Data::Collection::Album)
                                << QString::fromLatin1("602341013727")
                                << QString::fromLatin1("gloryRevealed");
  QTest::newRow("UK music upc") << QString::fromLatin1("UK")
                                << static_cast<int>(Tellico::Data::Collection::Album)
                                << QString::fromLatin1("0602341013727")
                                << QString::fromLatin1("gloryRevealed");
  QTest::newRow("CA music upc") << QString::fromLatin1("CA")
                                << static_cast<int>(Tellico::Data::Collection::Album)
                                << QString::fromLatin1("0602341013727")
                                << QString::fromLatin1("gloryRevealed");

  QTest::newRow("US movie upc") << QString::fromLatin1("US")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QString::fromLatin1("786936244250")
                                << QString::fromLatin1("incredibles");
  QTest::newRow("UK movie upc") << QString::fromLatin1("UK")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QString::fromLatin1("0786936244250")
                                << QString::fromLatin1("incredibles");
  QTest::newRow("CA movie upc") << QString::fromLatin1("CA")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QString::fromLatin1("0786936244250")
                                << QString::fromLatin1("incredibles");
  QTest::newRow("FR movie upc") << QString::fromLatin1("FR")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QString::fromLatin1("5050582560985")
                                << QString::fromLatin1("pacteDesLoups");
}

void AmazonFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
