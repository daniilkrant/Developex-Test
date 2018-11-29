#include <QtNetwork>

#include <chrono>
#include <regex>

#include "workthread.h"

namespace {
constexpr auto SLEEP_TIMEOUT = 100;
}

WorkThread::WorkThread(shQueue<std::string> &urls_queue,
                       QString keyword,
                       std::atomic<int> &status,
                       int depth,
                       std::function<void(std::string)> on_start_cb,
                       std::function<void(URL_PAIR)> on_finish_cb) :
    mUrlsQueue(urls_queue),
    mKeyword(keyword),
    mThreadStatus(status),
    mSearchDepth(depth),
    mThreadStarted(on_start_cb),
    mThreadFinished(on_finish_cb)
{
}

void WorkThread::lookForUrls(QString pageHtml)
{
    std::string text = pageHtml.toStdString();
    const std::regex hl_regex("http[s]?:\\/\\/?[^\\s([\"<,>]*\\.[^\\s[\",><]*");

    std::sregex_iterator next(text.begin(), text.end(), hl_regex);
    std::sregex_iterator end;
    while (next != end) {
        std::smatch match = *next;
        auto val = match.str();
        mUrlsQueue.push(val);
        next++;
    }
}

std::string WorkThread::lookForKeyword(QString pageHtml)
{
    if (pageHtml.contains(mKeyword)) {
        return keywordFoundStatus;
    } else {
        return keywordNotFoundStatus;
    }
}

void WorkThread::run()
{
    while (1) {
        if (mThreadStatus == STATUS_PAUSED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIMEOUT));
        } else
        if (mThreadStatus == STATUS_STOPPED ||
            mCurrentSearchDepth == mSearchDepth) {
            return;
        } else
        if (mThreadStatus == STATUS_WORKING) {
            if (!mUrlsQueue.try_and_pop(mUrl)) {
                continue;
            } else {
                std::string threadResult = workingOnThatStatus;
                if (mThreadStarted) {
                    mThreadStarted(mUrl);
                }

                QNetworkAccessManager manager;
                QEventLoop event;
                QNetworkReply *response = manager.get(QNetworkRequest(QUrl(QString::fromStdString(mUrl))));
                QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
                event.exec();

                if (response->error() == QNetworkReply::NoError) {
                    QString page = response->readAll();
                    threadResult = lookForKeyword(page);
                    lookForUrls(page);
                } else {
                    threadResult = errorPrefix + response->errorString().toStdString();
                }

                if (mThreadFinished) {
                    mThreadFinished(std::make_pair(mUrl, threadResult));
                }

                ++mCurrentSearchDepth;

                response->deleteLater();
            }
        }
    }
}
