#ifndef FINDER_H
#define FINDER_H
#include <QSet>
#include <QPair>
#include <QList>
#include <QFileInfoList>
#include <QMutex>
#include <QFileSystemWatcher>
#include <QFuture>

class Finder : public QObject {
    Q_OBJECT

signals:
    void progress_changed(int progress);
    void file_finished(QString const& file, QList<QString> const& enteries);
    void search_finished();
    void indexing_finished();

public slots:
    void stop();
    void file_changes(QString dir);

private:
    QList<QPair<QVector<qint64>, QFileInfo>> all_trigrams;
    QFileSystemWatcher watcher;
    std::atomic_bool run;
    void add_file(QFileInfo const& file_info);
    void file_search(QFileInfo const& file_info, QString const& text);
    qint64 get_trigram(QChar const trigram[3]);
    QMutex mutex;
    QString dir;
    QList<QFuture<void>> future_index;
public:
    void find(QString const& text);
    void index_dir(QString dir);
    Finder();
    ~Finder();

};


#endif // FINDER_H
