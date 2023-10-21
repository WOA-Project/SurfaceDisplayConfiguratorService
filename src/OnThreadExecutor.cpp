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
#include "pch.h"

#include "on_thread_executor.h"

OnThreadExecutor::OnThreadExecutor() :
    _shutdown_request{ false }, _worker_thread{ [this] { worker_thread(); } }
{
}

std::future<void> OnThreadExecutor::submit(task_t task)
{
    auto future = task.get_future();
    std::lock_guard lock{ _task_mutex };
    _task_queue.emplace(std::move(task));
    _task_cv.notify_one();
    return future;
}

void OnThreadExecutor::cancel()
{
    std::lock_guard lock{ _task_mutex };
    _task_queue = {};
    _task_cv.notify_one();
}


void OnThreadExecutor::worker_thread()
{
    while (!_shutdown_request)
    {
        task_t task;
        {
            std::unique_lock task_lock{ _task_mutex };
            _task_cv.wait(task_lock, [this] { return !_task_queue.empty() || _shutdown_request; });
            if (_shutdown_request)
            {
                return;
            }
            task = std::move(_task_queue.front());
            _task_queue.pop();
        }
        task();
    }
}

OnThreadExecutor::~OnThreadExecutor()
{
    _shutdown_request = true;
    _task_cv.notify_one();
    _worker_thread.join();
}
