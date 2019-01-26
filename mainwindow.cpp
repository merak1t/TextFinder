#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "finder.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDirIterator>
#include <QtConcurrent/QtConcurrent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    qRegisterMetaType<QList<QString>>();
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    QCommonStyle style;
    ui->actionScan->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionSearch->setIcon(style.standardIcon(QCommonStyle::SP_FileDialogContentsView));
    ui->actionCancel->setIcon(style.standardIcon(QCommonStyle::SP_DialogCancelButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    ui->progressBar->setValue(0);

    connect(&finder, &Finder::progress_changed, this, &MainWindow::set_progress_bar);
    connect(&finder, &Finder::file_finished, this, &MainWindow::add_to_group);
    connect(&finder, &Finder::search_finished, this, &MainWindow::finish_find);
    connect(&finder, &Finder::indexing_finished, this, &MainWindow::finish_scan);

    connect(ui->actionScan, &QAction::triggered, this, &MainWindow::scan_dir);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::show_about_dialog);
    connect(ui->actionSearch, &QAction::triggered, this, &MainWindow::find);
    connect(ui->actionCancel, &QAction::triggered, this, &MainWindow::finish_find);
    connect(ui->actionCancel, &QAction::triggered, &finder, &Finder::stop);


    connect(this, &MainWindow::stop_all, &finder, &Finder::stop);
}

MainWindow::~MainWindow() {
    emit stop_all();
    futureScan.waitForFinished();
    futureFind.waitForFinished();
}

void MainWindow::show_about_dialog()
{
    QMessageBox::about(this, "About", "Program which indexes directory and find text substrings.");
}

void MainWindow::scan_dir() {
    dir = QFileDialog::getExistingDirectory(this, "Choose Directory");
    if (dir.size() == 0)
    {
        return;
    }
    ui->actionScan->setEnabled(false);
    ui->actionSearch->setEnabled(false);
    ui->actionCancel->setEnabled(true);
    ui->progressBar->setValue(0);
    ui->treeWidget->clear();
    futureScan = QtConcurrent::run(&finder, &Finder::index_dir, dir);
}

void MainWindow::find() {
    QString text = ui->lineEdit->text();
    if (text.size() >= 3)
    {
        ui->treeWidget->clear();
        ui->actionScan->setEnabled(false);
        ui->actionSearch->setEnabled(false);
        ui->actionCancel->setEnabled(true);
        ui->progressBar->setValue(0);
        futureFind = QtConcurrent::run(&finder, &Finder::find, text);
    } else
    {
        QMessageBox message;
        message.setText("Enter at least 3 letters");
        message.exec();
    }

}

void MainWindow::set_progress_bar(int progress)
{
    ui->progressBar->setValue(progress);
}

void MainWindow::add_to_group(QString const& file, QList<QString> const& result) {
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
    item->setExpanded(false);
    item->setText(0, file + " - " + QString::number(result.size()) + " enteries");
    for (auto const& it : result) {
        QTreeWidgetItem* child = new QTreeWidgetItem(item);
        child->setText(0, it);
        item->addChild(child);
    }
    ui->treeWidget->addTopLevelItem(item);
}

void MainWindow::finish_scan() {
    ui->actionScan->setEnabled(true);
    ui->actionSearch->setEnabled(true);
    ui->actionCancel->setEnabled(false);
}

void MainWindow::finish_find() {
    if (ui->treeWidget->topLevelItemCount() == 0) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, "Nothing was found\n Maybe you should change the key-word");
        ui->treeWidget->addTopLevelItem(item);
    }
    ui->actionScan->setEnabled(true);
    ui->actionSearch->setEnabled(true);
    ui->actionCancel->setEnabled(false);
}

