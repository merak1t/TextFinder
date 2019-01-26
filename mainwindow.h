#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileInfoList>
#include <QMainWindow>
#include <memory>
#include <finder.h>
#include <QFuture>
#include <QFileSystemWatcher>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void stop_all();

private slots:
    void scan_dir();
    void find();
    void set_progress_bar(int progress);
    void add_to_group(QString const& file, QList<QString> const& enteries);
    void finish_find();
    void finish_scan();
    void show_about_dialog();
private:
    std::unique_ptr<Ui::MainWindow> ui;
    QString dir;
    Finder finder;
    QFuture<void> futureScan, futureFind;
};

#endif // MAINWINDOW_H
