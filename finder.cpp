#include "finder.h"
#include <QtDebug>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QQueue>
#include <QtDebug>
#include <QtConcurrent/QtConcurrent>

const int MAX_SIZET = 600000;
const int FULL = 100;

Finder::Finder() : mutex(QMutex::Recursive)
{
    connect(&watcher, &QFileSystemWatcher::directoryChanged, this, &Finder::file_changes);
}

void Finder::find(QString const& text)
{
    run = true;
    if (!run)
    {
        return;
    }
    if (all_trigrams.empty())
    {
        emit progress_changed(FULL);
        emit search_finished();
        return;
    }
    QVector<qint64> trigrams;
    QChar trigram[3];
    for (int i = 0; i + 2 < text.size(); i++)
    {
        trigram[0] = text[i];
        trigram[1] = text[i + 1];
        trigram[2] = text[i + 2];
        trigrams.push_back(get_trigram(trigram));
    }
    qint64 cnt = 0;
    for (auto const& file_info : all_trigrams)
    {
        if (!run)
        {
            break;
        }
        bool find = true;
        for (auto const& trigram : trigrams)
        {
            if (!std::binary_search(file_info.first.begin(), file_info.first.end(),trigram))
            {
                find = false;
                break;
            }
        }
        if (find)
        {
            file_search(file_info.second, text);
        }
        cnt++;
        emit progress_changed(int(FULL * cnt / all_trigrams.size()));
    }
    emit search_finished();
}

void Finder::index_dir(QString dir)
{
    QMutexLocker locker(&mutex);
    watcher.directories().clear();
    dir = dir;
    run = true;
    all_trigrams.clear();
    QDirIterator it(dir, QDir::Files | QDir::Dirs| QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);
    QFileInfoList all;
    while (it.hasNext())
    {
        if (!run)
        {
            break;
        }
        it.next();
        if (it.fileInfo().isDir())
            watcher.addPath(it.fileInfo().filePath());
        else
            all.append(it.fileInfo());

    }
    if (all.empty())
    {
        emit progress_changed(FULL);
    }
    qint64 cnt = 0;
    for (auto const& file_info : all)
    {
        if(!run)
        {
            break;
        }
        add_file(file_info);
        cnt++;
        emit progress_changed(int(FULL * cnt / all.size()));
    }
    emit indexing_finished();
}

void Finder::stop()
{
    run = false;
}

void Finder::add_file(QFileInfo const& file_info)
{
    if (file_info.size() < 3)
    {
        return;
    }
    QFile file(file_info.filePath());
    QSet<qint64> set_tr;
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        QChar trigram[3];
        for (int i = 0; i < 3; i++)
        {
            QChar sym;
            in >> sym;
            trigram[i] = sym;
            if (!sym.isPrint() && !sym.isSpace())
            {
                return;
            }
        }
        set_tr.insert(get_trigram(trigram));
        while (!in.atEnd())
        {
            if (!run)
            {
                break;
            }
            for (int i = 0; i < 2; i++)
            {
                trigram[i] = trigram[i + 1];
            }
            QChar sym;
            in >> sym;
            if (!sym.isPrint() && !sym.isSpace())
            {
                return;
            }
            trigram[2] = sym;
            set_tr.insert(get_trigram(trigram));
            if (set_tr.size() > MAX_SIZET)
            {
                return;
            }
        }
    }
    QVector<qint64> trigrams;
    for (auto trigram : set_tr)
    {
        trigrams.append(trigram);
    }
    std::sort(trigrams.begin(), trigrams.end());
    all_trigrams.push_back(qMakePair(trigrams, file_info));
}

void Finder::file_search(const QFileInfo &file_info, const QString &text)
{
    QMutexLocker locker(&mutex);
    QVector<int> pref(text.size());
    for (int i = 1; i < text.size(); i++)
    {
        int j = pref[i - 1];
        while (j > 0 && text.at(i) != text.at(j))
        {
            j = pref[j - 1];
        }
        if (text.at(i) == text.at(j))
        {
            j++;
        }
        pref[i] = j;
    }
    pref.append(0);
    int prev = 0;
    QFile file(file_info.filePath());
    QList<QChar> before;
    QQueue<QString> result;
    qint64 pos = 0;
    if (file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QChar sym;
            in >> sym;
            before += sym;
            if (before.size() > text.size() + 20)
            {
                before.pop_front();
            }
            for (auto i = result.size() - 1; i >= 0; i--)
            {
                if (result[i].size() < text.size() + 40)
                {
                    result[i] += sym;
                }
            }
            int j = prev;
            while (j > 0 && (j == text.size() || sym != text.at(j)))
            {
                j = pref[j - 1];
            }
            if (j < text.size() && sym == text.at(j))
            {
                j++;
            }
            if (j  == text.size())
            {
                QString tmp;
                for (auto c : before)
                {
                    tmp += c;
                }
                result.append(tmp);
            }
            prev = j;
            pos++;
        }
    }
    if (result.size())
    {
        emit file_finished(file_info.filePath(), result);
    }
}

void Finder::file_changes(QString dir)
{
    future_index.push_back(QtConcurrent::run(this, &Finder::index_dir, dir));
}

qint64 Finder::get_trigram(QChar const trigram[3])
{
    qint64 res = 0;
    for (int i = 0; i < 3; i++)
    {
        res += trigram[i].unicode();
        res <<= 16;
    }
    return res;
}

Finder::~Finder()
{
    for (auto & future : future_index)
    {
        future.waitForFinished();
    }
}
