//
// Created by Alex2772 on 6/7/2022.
//

#include "ACurlMulti.h"
#include "AUI/Util/kAUI.h"
#include <curl/curl.h>
#include <AUI/Thread/ACutoffSignal.h>

ACurlMulti::ACurlMulti() noexcept:
    mMulti(curl_multi_init())
{
}

ACurlMulti::~ACurlMulti() {
    if (mMulti) {
        clear();
        curl_multi_cleanup(mMulti);
    }
}

void ACurlMulti::run(bool infinite) {
    setThread(AThread::current());
    int isStillRunning;
    while(!mCancelled && (!mEasyCurls.empty() || infinite)) {
        auto status = curl_multi_perform(mMulti, &isStillRunning);

        if (status) { // failure
            for (const auto&[handle, curl] : mEasyCurls) {
                removeCurl(curl);
                curl->reportFail(0);
            }
            continue;
        }

        AThread::processMessages();
        status = curl_multi_poll(mMulti, nullptr, 0, 100, nullptr);
        AThread::interruptionPoint();

        if (status) {
            throw ACurl::Exception("curl poll failed: {}"_format(status));
        }

        int messagesLeft;
        for (CURLMsg* msg; (msg = curl_multi_info_read(mMulti, &messagesLeft));) {
            if (msg->msg == CURLMSG_DONE) {
                if (auto c = mEasyCurls.contains(msg->easy_handle)) {
                    auto s = std::move(c->second);
                    removeCurl(s);

                    if (msg->data.result != CURLE_OK) {
                        s->reportFail(msg->data.result);
                    } else {
                        s->reportSuccess();
                    }
                }
            }
        }
    }
}

ACurlMulti& ACurlMulti::operator<<(_<ACurl> curl) {
    ui_threadX [this, curl = std::move(curl)]() mutable {
        connect(curl->closeRequested, [this, curl = curl.weak()] {
            if (auto c = curl.lock()) {
                *this >> c;
            }
        });
        auto c = curl_multi_add_handle(mMulti, curl->handle());
        assert(c == CURLM_OK);
        mEasyCurls[curl->handle()] = std::move(curl);
    };
    return *this;
}

ACurlMulti& ACurlMulti::operator>>(const _<ACurl>& curl) {
    ui_thread {
        removeCurl(curl);
    };
    return *this;
}

void ACurlMulti::removeCurl(const _<ACurl>& curl) {
    curl_multi_remove_handle(mMulti, curl->handle());
    mEasyCurls.erase(curl->handle());
    curl->closeRequested.clearAllConnectionsWith(curl.get());
}

void ACurlMulti::clear() {
    ui_thread {
        assert(mMulti);
        for (const auto& [handle, acurl]: mEasyCurls) {
            curl_multi_remove_handle(mMulti, handle);
        }
        mEasyCurls.clear();
    };
}

ACurlMulti& ACurlMulti::global() noexcept {
    static struct Instance {
        ACurlMulti multi;
        _<AThread> thread = _new<AThread>([this] {
            AThread::setName("AUI CURL IO");
            multi.run(true);
        });

        Instance() {
            thread->start();

            ACutoffSignal cs;
            thread->enqueue([&] {
                cs.makeSignal();
            });
            cs.waitForSignal();
        }
    } instance;

    return instance.multi;
}
