/*

This code was adapted from Microsoft Power Toys' Fancy Zones Source Code
License is reproduced below:

The MIT License

Copyright (c) Microsoft Corporation. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#pragma once

#include <future>
#include <thread>
#include <functional>
#include <queue>
#include <atomic>

// OnThreadExecutor allows its caller to off-load some work to a persistently running background thread.
// This might come in handy if you use the API which sets thread-wide global state and the state needs
// to be isolated.

class OnThreadExecutor final
{
public:
    using task_t = std::packaged_task<void()>;

    OnThreadExecutor();
    ~OnThreadExecutor();
    std::future<void> submit(task_t task);
    void cancel();

private:
    void worker_thread();

    std::mutex _task_mutex;
    std::condition_variable _task_cv;
    std::atomic_bool _shutdown_request;
    std::queue<std::packaged_task<void()>> _task_queue;
    std::thread _worker_thread;
};
