/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_MOBYGAMESFETCHER_H
#define TELLICO_MOBYGAMESFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QLineEdit>
#include <QPointer>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for mobygames.com
 *
 * @author Robby Stephenson
 */
class MobyGamesFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  MobyGamesFetcher(QObject* parent);
  /**
   */
  virtual ~MobyGamesFetcher();

  /**
   */
  virtual QString source() const Q_DECL_OVERRIDE;
  virtual QString attribution() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return MobyGames; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;
  virtual void continueSearch() Q_DECL_OVERRIDE;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const MobyGamesFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) Q_DECL_OVERRIDE;
    virtual QString preferredName() const Q_DECL_OVERRIDE;

  private:
    QLineEdit* m_apiKeyEdit;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  Data::EntryList createEntries(Data::CollPtr coll, const QVariantMap& resultMap);
  void populateHashes();

  bool m_started;

  QString m_apiKey;
  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;

  QHash<int, QString> m_esrbHash;
};

  } // end namespace
} // end namespace
#endif
