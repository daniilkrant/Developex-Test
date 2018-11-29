#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include <QRunnable>
#include <QString>

#include <atomic>
#include <set>

#include "shared_queue.h"
#include "types.h"

class QNetworkReply;

class WorkThread : public QRunnable
{
public:
    WorkThread(shQueue<std::string> &urls_queue,
               QString keyword,
               std::atomic<int> &status,
               int depth,
               std::function<void(std::string)> on_start_cb,
               std::function<void(URL_PAIR)> on_finish_cb);

protected:
    void run();

private:
    void lookForUrls(QString pageHtml);
    std::string lookForKeyword(QString pageHtml);

    shQueue<std::string> &mUrlsQueue;
    std::string mUrl;
    QString mKeyword;
    std::atomic<int> &mThreadStatus;
    int mSearchDepth{};
    int mCurrentSearchDepth{};
    std::function<void(std::string)> mThreadStarted;
    std::function<void(URL_PAIR)> mThreadFinished;
};

#endif // WORKTHREAD_H
