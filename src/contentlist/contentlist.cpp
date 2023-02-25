// SPDX-FileCopyrightText: 2015 Dan Leinir Turthra Jensen <admin@leinir.dk>
// SPDX-License-Identifier: LGPL-2.1-only or LGPL-3.0-only or LicenseRef-KDE-Accepted-LGPL

#include "contentlist.h"
#ifdef HAVE_BALOO
#include "baloocontentlister.h"
#endif
#include "filesystemcontentlister.h"
#include "manualcontentlister.h"

#include <QDebug>
#include <QMimeDatabase>
#include <QSet>
#include <QTimer>
#include <QUrl>

struct ContentEntry {
    QString filename;
    QUrl filePath;
    QVariantMap metadata;
};

class ContentList::Private
{
public:
    typedef QQmlListProperty<ContentQuery> QueryListProperty;

    Private()
        : actualContentList(nullptr)
        , manualContentLister(new ManualContentLister())
    {
    }
    QList<ContentEntry *> entries;
    ContentListerBase *actualContentList;
    ManualContentLister *manualContentLister;

    QList<ContentQuery *> queries;
    QueryListProperty listProperty;

    QSet<QString> knownFiles;

    bool autoSearch = false;
    bool cacheResults = false;
    bool completed = false;

    static void appendToList(QueryListProperty *property, ContentQuery *value);
    static ContentQuery *listValueAt(QueryListProperty *property, int index);
    static void clearList(QueryListProperty *property);
    static int countList(QueryListProperty *property);

    static QStringList cachedFiles;
};

QStringList ContentList::Private::cachedFiles;

ContentList::ContentList(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private)
{
#ifdef HAVE_BALOO
    auto baloo = new BalooContentLister(this);
    if (baloo->balooEnabled()) {
        d->actualContentList = baloo;
    } else {
        baloo->deleteLater();
        d->actualContentList = new FilesystemContentLister(this);
    }
#else
    d->actualContentList = new FilesystemContentLister(this);
#endif

    connect(d->actualContentList, &ContentListerBase::fileFound, this, &ContentList::fileFound);
    connect(d->actualContentList, &ContentListerBase::searchCompleted, this, &ContentList::searchCompleted);

    connect(d->manualContentLister, &ContentListerBase::fileFound, this, &ContentList::fileFound);
    connect(d->manualContentLister, &ContentListerBase::searchCompleted, this, &ContentList::searchCompleted);

    d->listProperty = QQmlListProperty<ContentQuery>{this,
                                                     &d->queries,
                                                     &ContentList::Private::appendToList,
                                                     &ContentList::Private::countList,
                                                     &ContentList::Private::listValueAt,
                                                     &ContentList::Private::clearList};
}

ContentList::~ContentList() = default;

QQmlListProperty<ContentQuery> ContentList::queries()
{
    return d->listProperty;
}

bool ContentList::autoSearch() const
{
    return d->autoSearch;
}

bool ContentList::cacheResults() const
{
    return d->cacheResults;
}

QString ContentList::getMimetype(QString filePath)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filePath);
    return mime.name();
}

void ContentList::startSearch()
{
    QTimer::singleShot(1, [this]() {
        Q_EMIT searchStarted();
        qWarning() << "search started";
        d->actualContentList->knownFiles = d->knownFiles;
        d->actualContentList->startSearch(d->queries);
        d->manualContentLister->startSearch(d->queries);
    });
}

void ContentList::fileFound(const QString &filePath, const QVariantMap &metaData)
{
    if (d->knownFiles.contains(filePath))
        return;

    auto fileUrl = QUrl::fromLocalFile(filePath);

    ContentEntry *entry = new ContentEntry();
    entry->filename = fileUrl.fileName();
    entry->filePath = fileUrl;
    entry->metadata = metaData;

    int newRow = d->entries.count();
    beginInsertRows({}, newRow, newRow);
    d->entries.append(entry);
    endInsertRows();

    if (d->cacheResults)
        Private::cachedFiles.append(filePath);
}

void ContentList::setAutoSearch(bool autoSearch)
{
    if (autoSearch == d->autoSearch)
        return;

    d->autoSearch = autoSearch;
    Q_EMIT autoSearchChanged();
}

void ContentList::setCacheResults(bool cacheResults)
{
    if (cacheResults == d->cacheResults)
        return;

    d->cacheResults = cacheResults;

    if (d->cacheResults && d->completed && !Private::cachedFiles.isEmpty()) {
        setKnownFiles(Private::cachedFiles);
    }

    Q_EMIT cacheResultsChanged();
}

void ContentList::setKnownFiles(const QStringList &results)
{
    beginResetModel();
    d->entries.clear();
    d->knownFiles.clear();
    for (const auto &result : results) {
        auto entry = new ContentEntry{};
        auto url = QUrl::fromLocalFile(result);

        entry->filename = url.fileName();
        entry->filePath = url;
        entry->metadata = ContentListerBase::metaDataForFile(result);

        d->entries.append(entry);
        d->knownFiles.insert(result);
    }
    endResetModel();
}

QHash<int, QByteArray> ContentList::roleNames() const
{
    return {
        {FilenameRole, "filename"},
        {FilePathRole, "filePath"},
        {MetadataRole, "metadata"},
    };
}

QVariant ContentList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() <= -1 || index.row() >= d->entries.count()) {
        return {};
    }

    const ContentEntry *entry = d->entries[index.row()];
    switch (role) {
    case FilenameRole:
        return entry->filename;
    case FilePathRole:
        return entry->filePath;
    case MetadataRole:
        return entry->metadata;
    }
    return {};
}

int ContentList::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return d->entries.count();
}

void ContentList::classBegin()
{
}

void ContentList::componentComplete()
{
    d->completed = true;

    if (d->cacheResults && !Private::cachedFiles.isEmpty())
        setKnownFiles(Private::cachedFiles);

    if (d->autoSearch)
        d->actualContentList->startSearch(d->queries);
}

bool ContentList::isComplete() const
{
    return d->completed;
}

void ContentList::Private::appendToList(Private::QueryListProperty *property, ContentQuery *value)
{
    auto list = static_cast<QList<ContentQuery *> *>(property->data);
    auto model = static_cast<ContentList *>(property->object);
    list->append(value);
    if (model->autoSearch() && model->isComplete())
        model->startSearch();
}

ContentQuery *ContentList::Private::listValueAt(Private::QueryListProperty *property, int index)
{
    return static_cast<QList<ContentQuery *> *>(property->data)->at(index);
}

int ContentList::Private::countList(Private::QueryListProperty *property)
{
    return static_cast<QList<ContentQuery *> *>(property->data)->size();
}

void ContentList::Private::clearList(Private::QueryListProperty *property)
{
    auto list = static_cast<QList<ContentQuery *> *>(property->data);
    auto model = static_cast<ContentList *>(property->object);
    model->beginResetModel();
    list->clear();
    model->endResetModel();
}

void ContentList::addFile(const QUrl &filePath)
{
    d->manualContentLister->addFile(filePath);
}
