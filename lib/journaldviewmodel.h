/*
    SPDX-License-Identifier: LGPL-2.1-or-later OR MIT
    SPDX-FileCopyrightText: 2021 Andreas Cord-Landwehr <cordlandwehr@kde.org>
*/

#ifndef JOURNALDVIEWMODEL_H
#define JOURNALDVIEWMODEL_H

#include "kjournald_export.h"
#include <QAbstractItemModel>
#include <ijournal.h>
#include <memory>

class JournaldViewModelPrivate;

/**
 * @brief Item model class that provides convienence access to journald database
 *
 * This QAbstractItemModel derived class provides a model/view abstraction for the journald API
 * with the goal to ease integration in Qt based applications.
 */
class KJOURNALD_EXPORT JournaldViewModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString journalPath WRITE setJournaldPath RESET setSystemJournal)
    /**
     * Configure model to only provide messages for stated systemd units
     **/
    Q_PROPERTY(QStringList systemdUnitFilter WRITE setSystemdUnitFilter READ systemdUnitFilter NOTIFY systemdUnitFilterChanged)
    /**
     * Configure model to only provide messages for stated boot ids.
     **/
    Q_PROPERTY(QStringList bootFilter WRITE setBootFilter READ bootFilter NOTIFY bootFilterChanged)
    /**
     * Configure model to only show messages of stated executables (see journald '_EXE' field)
     **/
    Q_PROPERTY(QStringList exeFilter WRITE setExeFilter READ exeFilter NOTIFY exeFilterChanged)
    /**
     * if set to true, Kernel messages are added to the log output
     **/
    Q_PROPERTY(bool kernelFilter WRITE setKernelFilter READ isKernelFilterEnabled NOTIFY kernelFilterChanged)
    /**
     * Configure model to only provide messages with stated priority or higher. Default: no filter is set.
     **/
    Q_PROPERTY(int priorityFilter WRITE setPriorityFilter READ priorityFilter NOTIFY priorityFilterChanged RESET resetPriorityFilter)

public:
    enum Roles {
        MESSAGE = Qt::DisplayRole, //!< journal entry's message text
        MESSAGE_ID = Qt::UserRole + 1, //!< ID of log entry in journald DB (might not exist for non systemd services)
        DATE, //!< date of journal entry
        DATETIME, //!< date and time of journal entry
        MONOTONIC_TIMESTAMP, //!< monotonic timestamp in miliseconds for journal entry
        PRIORITY, //!< priority of journal entry
        SYSTEMD_UNIT, //!< systemd unit name of journal entry
        SYSTEMD_UNIT_CHANGED_SUBSTRING, //!< changed part of systemd unit string when compared to previous line
        BOOT_ID, //!< boot ID of journal entry
        SYSTEMD_UNIT_COLOR_BACKGROUND, //!< convenience rainbow color that is hashed for systemd unit, lighter variant
        SYSTEMD_UNIT_COLOR_FOREGROUND, //!< convenience rainbow color that is hashed for systemd unit, darker variant
        EXE_COLOR_BACKGROUND, //!< convenience rainbow color that is hashed for the process, lighter variant
        EXE_COLOR_FOREGROUND, //!< convenience rainbow color that is hashed for the process, darker variant
        EXE, //!< executable path, when available; field "_EXE"
        EXE_CHANGED_SUBSTRING, //!< changed part of EXE string when compared to previous line
        CURSOR, //!< journald internal unique identifier for a log entry
    };
    Q_ENUM(Roles);

    enum Direction {
        FORWARD, //!< interaction with log view leads to browsing
        BACKWARD, //!< interaction with log view leads to text selection
    };
    Q_ENUM(Direction);

    /**
     * @brief Construct model from the default local journald database
     *
     * @param parent the QObject parent
     */
    explicit JournaldViewModel(QObject *parent = nullptr);

    /**
     * @brief Construct model from a specific journal database
     *
     * This constructor works similar to "journalctl -D" and allows to use a custom path to the
     * journald database.
     *
     * @param journalPath path to the journald database
     * @param parent the QObject parent
     */
    JournaldViewModel(const QString &journalPath, QObject *parent = nullptr);

    /**
     * @brief Construct model for given journal object
     *
     * @note API requires a unique_ptr because journald documentation explicitly states that (even though it works
     * at the moment) one cannot assume that using differen requests for the same journal has no side effects.
     * A prominent example would be to use the same journal object for unique query requests and for obtaining the log
     * content.
     *
     * @param journal object that contains a journald database object
     * @param parent the QObject parent
     */
    JournaldViewModel(std::unique_ptr<IJournal> journal, QObject *parent = nullptr);

    /**
     * Destroys JournaldViewModel
     */
    ~JournaldViewModel() override;

    /**
     * Reset model by reading from a new journald database
     *
     * @param path The path to directory that obtains the journald DB, usually ending with "journal".
     * @return true if path could be found and opened, otherwise false
     */
    bool setJournaldPath(const QString &path);

    /**
     * Switch to local system's default journald database
     *
     * For details regarding preference, see journald documentation.
     * @return true if journal was loaded correctly
     */
    bool setSystemJournal();

    /**
     * Reset model by using given journal object
     *
     * @param journal The journald access wrapper
     * @return true if path could be opened, otherwise false
     */
    bool setJournal(std::unique_ptr<IJournal> journal);

    /**
     * @copydoc QAbstractItemModel::rolesNames()
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @copydoc QAbstractItemModel::headerData()
     *
     * @note NOT IMPLEMENTED YET
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @copydoc QAbstractItemModel::index()
     */
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @copydoc QAbstractItemModel::parent()
     */
    QModelIndex parent(const QModelIndex &index) const override;

    /**
     * @copydoc QAbstractItemModel::rowCount()
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @copydoc QAbstractItemModel::columnCount()
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @copydoc QAbstractItemModel::data()
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Convenience method that returns date for a given model row
     * @param indexRow the index row
     * @return the QDateItem object for the entry found
     */
    Q_INVOKABLE QDateTime datetime(int indexRow) const;

    /**
     * @copydoc QAbstractItemModel::canFetchMore()
     */
    bool canFetchMore(const QModelIndex &parent) const override;

    /**
     * @copydoc QAbstractItemModel::fetchMore()
     */
    void fetchMore(const QModelIndex &parent) override;

    /**
     * @brief Configure for which systemd units messages shall be shown
     *
     * If no unit is configured, this filter is deactivated. The given values are compared
     * to the _SYSTEMD_UNIT journal value.
     *
     * @param systemdUnitFilter list of system units
     */
    void setSystemdUnitFilter(const QStringList &systemdUnitFilter);

    /**
     * @return list of currently set systemd services for filtering
     */
    QStringList systemdUnitFilter() const;

    /**
     * @brief Configure for which boots messages shall be shown
     *
     * If no boot id is configured, this filter is deactivated. The given values are compared
     * to the _BOOT_ID journal value.
     *
     * @param bootFilter list of boot ids
     */
    void setBootFilter(const QStringList &bootFilter);

    /**
     * @return list of currently set boot ids for filtering
     */
    QStringList bootFilter() const;

    /**
     * @brief Configure for which executable messages shall be shown
     *
     * If no executable is configured, this filter is deactivated. The given values are compared
     * to the _EXE journal value.
     *
     * @param exeFilter list of executable paths
     */
    void setExeFilter(const QStringList &exeFilter);

    /**
     * @return list of currently set executables for filtering
     */
    QStringList exeFilter() const;

    /**
     * @brief Configure if Kernel messages shall be included in model
     *
     * Per default, Kernel messages are deactivated.
     *
     * @param showKernelMessages parameter that defines if Kernel messages shall be shown
     */
    void setKernelFilter(bool showKernelMessages);

    /**
     * @return true if Kernel messages are included in model, otherwise false
     */
    bool isKernelFilterEnabled() const;

    /**
     * @brief Filter messages such that only messages with this and higher priority are provided
     *
     * @note Non-systemd services may not follow systemd's priority values
     *
     * @param priority the minimal priority for messages that shall be provided by model
     */
    void setPriorityFilter(int priority);

    /**
     * @return currently set priority filter value or -1 if not set
     */
    int priorityFilter() const;

    /**
     * @brief Discard priority filter and display all messages
     */
    void resetPriorityFilter();

    /**
     * @return row index of searched string
     */
    Q_INVOKABLE int search(const QString &searchString, int startRow, JournaldViewModel::Direction direction = FORWARD);

    /**
     * @brief Format time into string
     * @param datetime the datetime object
     * @param utc if set to true, the string will be UTC time, otherwise according to the current local
     * @return formatted string
     */
    Q_INVOKABLE QString formatTime(const QDateTime &datetime, bool utc) const;

    /**
     * @brief Reset model and start reading from head
     *
     * This method can be used to position the log at the beginning and clearing all existing
     * data in the log cache.
     */
    Q_INVOKABLE void seekHead();

    /**
     * @brief Reset model and start reading from tail
     *
     * This method can be used to position the log at the end and clearing all existing
     * data in the log cache.
     */
    Q_INVOKABLE void seekTail();

    /**
     * @brief Return closest index row for given date
     *
     * This method always returns a valid index then the model is not empty. If the model is empty,
     * return value is -1;
     *
     * @return row index for closest entry
     */
    Q_INVOKABLE int closestIndexForData(const QDateTime &datetime);

    /**
     * @brief Set how many log entries shall be read on each request of read-mode.
     * @param size
     * @note the initial value is 500 and changing this value only affects future reads.
     */
    void setFetchMoreChunkSize(quint32 size);

private Q_SLOTS:
    /**
     * Decoupled fetching for log entries that can enforce sequence of fetching calls.
     * @return pair of fetched entries, first are entries at head, second are entries at tail
     */
    std::pair<int, int> fetchMoreLogEntries();

Q_SIGNALS:
    /**
     * Signal is emitted when Kernel message filter is changed
     */
    void kernelFilterChanged();
    /**
     * Signal is emitted when boot id filter is changed
     */
    void bootFilterChanged();
    /**
     * Signal is emitted when Systemd service name filter is changed
     */
    void systemdUnitFilterChanged();
    /**
     * Signal is emitted when executable path filter is changed
     */
    void exeFilterChanged();
    /**
     * Signal is emitted when log level priority filter is changed
     */
    void priorityFilterChanged();

private:
    std::unique_ptr<JournaldViewModelPrivate> d;
};

#endif // JOURNALDVIEWMODEL_H
