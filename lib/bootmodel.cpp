/*
    SPDX-License-Identifier: LGPL-2.1-or-later OR MIT
    SPDX-FileCopyrightText: 2021 Andreas Cord-Landwehr <cordlandwehr@kde.org>
*/

#include "bootmodel.h"
#include "bootmodel_p.h"
#include "loggingcategories.h"

BootModelPrivate::BootModelPrivate(std::unique_ptr<IJournal> journal)
    : mJournal(std::move(journal))
{
}

BootModel::BootModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new BootModelPrivate(std::make_unique<LocalJournal>()))
{
    beginResetModel();
    d->mBootInfo = JournaldHelper::queryOrderedBootIds(*d->mJournal.get());
    d->sort(Qt::SortOrder::DescendingOrder);
    endResetModel();
}

BootModel::BootModel(const QString &journaldPath, QObject *parent)
    : QAbstractListModel(parent)
    , d(new BootModelPrivate(std::make_unique<LocalJournal>(journaldPath)))
{
    beginResetModel();
    d->mBootInfo = JournaldHelper::queryOrderedBootIds(*d->mJournal.get());
    d->sort(Qt::SortOrder::DescendingOrder);
    endResetModel();
}

BootModel::BootModel(std::unique_ptr<IJournal> journal, QObject *parent)
    : QAbstractListModel(parent)
    , d(new BootModelPrivate(std::move(journal)))
{
    beginResetModel();
    d->mBootInfo = JournaldHelper::queryOrderedBootIds(*d->mJournal.get());
    d->sort(Qt::SortOrder::DescendingOrder);
    endResetModel();
}

BootModel::~BootModel() = default;

bool BootModel::setJournaldPath(const QString &path)
{
    qCDebug(KJOURNALD_DEBUG) << "load journal from path" << path;
    bool success{true};
    beginResetModel();
    d->mJournaldPath = path;
    d->mJournal = std::make_unique<LocalJournal>(path);
    success = d->mJournal->isValid();
    if (success) {
        d->mBootInfo = JournaldHelper::queryOrderedBootIds(*d->mJournal.get());
        d->sort(Qt::SortOrder::DescendingOrder);
    }
    endResetModel();
    return success;
}

QString BootModel::journaldPath() const
{
    return d->mJournaldPath;
}

void BootModel::setSystemJournal()
{
    qCDebug(KJOURNALD_DEBUG) << "load system journal";
    beginResetModel();
    d->mJournaldPath = QString();
    d->mJournal = std::make_unique<LocalJournal>();
    d->mBootInfo = JournaldHelper::queryOrderedBootIds(*d->mJournal.get());
    d->sort(Qt::SortOrder::DescendingOrder);
    endResetModel();
}

QHash<int, QByteArray> BootModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[BootModel::BOOT_ID] = "bootid";
    roles[BootModel::CURRENT] = "current";
    roles[BootModel::SINCE] = "since";
    roles[BootModel::UNTIL] = "until";
    roles[BootModel::DISPLAY_SHORT_UTC] = "displayshort_utc";
    roles[BootModel::DISPLAY_SHORT_LOCALTIME] = "displayshort_localtime";
    return roles;
}

int BootModel::rowCount(const QModelIndex &parent) const
{
    return d->mBootInfo.size();
}

int BootModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QModelIndex BootModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column);
}

QModelIndex BootModel::parent(const QModelIndex &index) const
{
    // no tree model, thus no parent
    return QModelIndex();
}

QVariant BootModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= d->mBootInfo.size()) {
        return QVariant();
    }
    switch (role) {
    case BootModel::BOOT_ID:
        return d->mBootInfo.at(index.row()).mBootId;
    case BootModel::SINCE:
        return d->mBootInfo.at(index.row()).mSince;
    case BootModel::UNTIL:
        return d->mBootInfo.at(index.row()).mUntil;
    case BootModel::DISPLAY_SHORT_UTC:
        return d->prettyPrintBoot(d->mBootInfo.at(index.row()), BootModelPrivate::TIME_FORMAT::UTC);
    case BootModel::DISPLAY_SHORT_LOCALTIME:
        return d->prettyPrintBoot(d->mBootInfo.at(index.row()), BootModelPrivate::TIME_FORMAT::LOCALTIME);
    case BootModel::CURRENT:
        return QVariant::fromValue<bool>(d->mJournal->currentBootId() == d->mBootInfo.at(index.row()).mBootId);
    }

    return QVariant();
}

QString BootModel::bootId(int row) const
{
    if (row < 0 || row >= d->mBootInfo.size()) {
        return QString();
    }
    return d->mBootInfo.at(row).mBootId;
}
