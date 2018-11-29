#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThreadPool>
#include <QProgressBar>

#include <atomic>
#include <mutex>
#include <map>

#include "shared_queue.h"
#include "types.h"

namespace Ui {
class MainWindow;
}
class urlTableModel;
class QTableWidget;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startButton_released();

    void on_stopButton_released();

    void on_pauseButton_released();

    void setStatus(int status);

    void timeOutCheck();

signals:
    void updateProgressBar(int val);
    void updateStatus(int status);

private:
    int getInput();
    void initTable();
    void enableControls();
    void disableControls();
    void onThreadStarted(std::string url);
    void onThreadFinished(URL_PAIR url_status);
    int calculateMaxUrls();

    Ui::MainWindow *ui;
    int mThreadsNum{};
    int mAnalyzedUrlNum{};
    int mCurrentUrlIndex{};
    int mSearchDepth{};
    std::atomic<int> mCrawlStatus;

    QProgressBar *mProgressBar;
    QString mUrl;
    QString mKeyword;
    QTableWidget *mTableWidget;
    QThreadPool mThreadPool;
    QTimer *mTimeOutTimer;
    std::mutex mResultMutex;
    shQueue<std::string> mUrlsQueue;
    std::map<std::string, std::string> mCrawlResult;
    std::map<std::string, int> mUrlPositionMap;
};

#endif // MAINWINDOW_H
