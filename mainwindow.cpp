#include <QDebug>
#include <QMessageBox>
#include <QTableWidget>
#include <QTimer>

#include <functional>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "workthread.h"

namespace {
constexpr auto CONNECTION_TIMEOUT = 1500;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initTable();

    mCrawlStatus = STATUS_STOPPED;
    setStatus(mCrawlStatus);

    mProgressBar = this->findChild<QProgressBar*>("progressBar");

    mTimeOutTimer = new QTimer(this);
    mTimeOutTimer->setInterval(CONNECTION_TIMEOUT);

    this->setFixedSize(500,480);

    QObject::connect(this, SIGNAL(updateProgressBar(int)), mProgressBar, SLOT(setValue(int)));
    QObject::connect(this, SIGNAL(updateStatus(int)),
                     SLOT(setStatus(int)));
    QObject::connect(mTimeOutTimer, SIGNAL(timeout()),
                     SLOT(timeOutCheck()));
}

MainWindow::~MainWindow()
{
    mCrawlStatus = STATUS_STOPPED;
    mThreadPool.waitForDone();
    delete ui;
}

/*
 * Checks if there access to initial page
 */
void MainWindow::timeOutCheck()
{
    //Dirty hack to force QTableWidget to redraw and show upcomig elements
    //somehow works much better that redraw() or update()
    mTableWidget->selectAll();
    mTableWidget->setCurrentCell(0,0);

    if (mAnalyzedUrlNum == 1) {
        emit setStatus(STATUS_STOPPED);
        QMessageBox::critical(nullptr, "Critical", invalidUrlErr);
    }
}

/* Calculate how much nodes can graph contain
 * Graph representation to describe logic:
 *
 *     +      +      +      +   <--- 4 Threads, 3 Depth Level
 *     +      +      +      +   <--- 4 Threads, 2 Depth Level
 *     +      +      +      +   <--- 4 Threads, 1 Depth Level
 *               + <-- initial node
 */
int MainWindow::calculateMaxUrls()
{
    return mThreadsNum * mSearchDepth;
}

int MainWindow::getInput()
{
    mUrl = this->findChild<QLineEdit*>("urlEdit")->text();
    mKeyword = this->findChild<QLineEdit*>("keywordEdit")->text();
    mThreadsNum = this->findChild<QSpinBox*>("threadsBox")->value();
    mSearchDepth = this->findChild<QSpinBox*>("depthBox")->value();

    if (mUrl.isEmpty() || mKeyword.isEmpty()) {
        QMessageBox::critical(nullptr, "Critical", invalidInpStringErr);
        return 0;
    }

    if (!mThreadsNum || !mSearchDepth) {
        QMessageBox::critical(nullptr, "Critical", invalidInpIntErr);
        return 0;
    }

    mProgressBar->setMaximum(calculateMaxUrls());
    return 1;
}

void MainWindow::onThreadStarted(std::string url)
{
    if (mCrawlStatus == STATUS_WORKING) {
        mResultMutex.lock();
        mUrlPositionMap.emplace(url, mCurrentUrlIndex);
        mResultMutex.unlock();
        mTableWidget->setItem(mCurrentUrlIndex, 0, new QTableWidgetItem(QString::fromStdString(url)));
        mTableWidget->setItem(mCurrentUrlIndex, 1, new QTableWidgetItem(QString::fromStdString(workingOnThatStatus)));
        ++mCurrentUrlIndex;
    }
}


void MainWindow::onThreadFinished(URL_PAIR url_status)
{
    if (mCrawlStatus == STATUS_WORKING) {
        mResultMutex.lock();
        mCrawlResult.emplace(url_status.first, url_status.second);
        mResultMutex.unlock();
        int urlPosition = mUrlPositionMap.find(url_status.first)->second;
        mTableWidget->item(urlPosition, 1)->setText(QString::fromStdString(url_status.second));
        if (QString::fromStdString(url_status.second).contains(errorPrefix)) {
            mTableWidget->item(urlPosition, 1)->setBackground(Qt::red);
        }

        ++mAnalyzedUrlNum;
        emit updateProgressBar(mAnalyzedUrlNum);

        if (mAnalyzedUrlNum == calculateMaxUrls()) {
            emit updateStatus(STATUS_STOPPED);
        }
    }
}

void MainWindow::initTable()
{
    mTableWidget = this->findChild<QTableWidget*>("tableWidget");
    mTableWidget->setColumnCount(2);
    mTableWidget->setSortingEnabled(false);
    mTableWidget ->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mTableWidget->setAutoScroll(false);
    QStringList header;
    header << "URL" << "Result";
    mTableWidget->setHorizontalHeaderLabels(header);
}

void MainWindow::enableControls()
{
    this->findChild<QLineEdit*>("urlEdit")->setReadOnly(false);
    this->findChild<QLineEdit*>("keywordEdit")->setReadOnly(false);
    this->findChild<QSpinBox*>("threadsBox")->setEnabled(true);
    this->findChild<QSpinBox*>("depthBox")->setEnabled(true);
    this->findChild<QPushButton*>("stopButton")->setEnabled(false);
    this->findChild<QPushButton*>("pauseButton")->setEnabled(false);
    this->findChild<QPushButton*>("startButton")->setEnabled(true);
}

void MainWindow::disableControls()
{
    this->findChild<QLineEdit*>("urlEdit")->setReadOnly(true);
    this->findChild<QLineEdit*>("keywordEdit")->setReadOnly(true);
    this->findChild<QSpinBox*>("threadsBox")->setEnabled(false);
    this->findChild<QSpinBox*>("depthBox")->setEnabled(false);
    this->findChild<QPushButton*>("stopButton")->setEnabled(true);
    this->findChild<QPushButton*>("pauseButton")->setEnabled(true);
    this->findChild<QPushButton*>("startButton")->setEnabled(false);
}

void MainWindow::setStatus(int status)
{
    mCrawlStatus = status;
    if (mCrawlStatus == STATUS_WORKING) {
        this->findChild<QLabel*>("statusLabel")->setText("Running");
        mTableWidget->clear();
        emit updateProgressBar(0);
        mCrawlResult.clear();
        mUrlPositionMap.clear();
        disableControls();
    } else
    if (mCrawlStatus == STATUS_PAUSED) {
        this->findChild<QLabel*>("statusLabel")->setText("Paused");
        this->findChild<QPushButton*>("startButton")->setEnabled(true);
    } else
    if (mCrawlStatus == STATUS_STOPPED) {
        this->findChild<QLabel*>("statusLabel")->setText("Stopped");
        mThreadPool.waitForDone();
        mUrlsQueue.clear();
        mAnalyzedUrlNum = 0;
        mCurrentUrlIndex = 0;
        emit updateProgressBar(0);
        enableControls();
    }
}

void MainWindow::on_startButton_released()
{
    if (mCrawlStatus == STATUS_STOPPED) {
        if (getInput()) {
            emit setStatus(STATUS_WORKING);
            mTableWidget->setRowCount(calculateMaxUrls());
            initTable();

            mUrlsQueue.push(mUrl.toStdString());
            mThreadPool.setMaxThreadCount(mThreadsNum);

            std::function<void(std::string)> start_cb = [this](std::string retval) {onThreadStarted(retval);};
            std::function<void(URL_PAIR)> finish_cb = [this](URL_PAIR retval) {onThreadFinished(retval);};

            qInfo() << "Starting " << mThreadsNum << " threads";
            qInfo() << "Initial URL: " << mUrl;
            qInfo() << "Depth: " << mSearchDepth;

            mTimeOutTimer->start();

            for (int i = 0; i < mThreadsNum; ++i) {
                WorkThread *thread = new WorkThread(mUrlsQueue, mKeyword, mCrawlStatus, mSearchDepth,
                                                    start_cb, finish_cb);
                thread->setAutoDelete(true);
                mThreadPool.start(thread);
            }
        }
    } else {
        emit setStatus(STATUS_WORKING);
    }
}

void MainWindow::on_stopButton_released()
{
    emit setStatus(STATUS_STOPPED);
}

void MainWindow::on_pauseButton_released()
{
    emit setStatus(STATUS_PAUSED);
}
