/**
 * Copyright 2022 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include "fma-common/string_formatter.h"
#include "fma-common/logger.h"

#include "lgraph/lgraph_yz.h"

namespace lgraph_api {
static std::unique_ptr<yz_logger::Profl> profl_logger;

namespace yz_logger {

struct Count {
    Count() : id_(0) {}
    size_t get_id() {
        std::lock_guard<std::mutex> l(visit_lock_);
        return id_;
    }
    bool add_id() {
        std::lock_guard<std::mutex> l(visit_lock_);
        id_++;
        return true;
    }
    bool set_zero() {
        std::lock_guard<std::mutex> l(visit_lock_);
        id_ = 0;
        return true;
    }

 private:
    std::mutex visit_lock_;
    size_t id_;
};
// enum operation_type {

// };

// struct log_info {
//     size_t time_stamp;
//     union {}
// };
thread_local std::string call_desc_local = "u_i";
thread_local size_t call_count;
thread_local Count transaction_count;
thread_local Count step_count;

const static size_t buf_capacity = 1024;
thread_local char buf[buf_capacity];
volatile thread_local size_t buf_size = 0;

void log_breakdown(std::string& log_info) {
    if (call_desc_local == "u_i") return;
    std::string log = "C" + std::to_string(call_count) + "-T" +
                      std::to_string(get_transaction_id()) + "-S" + std::to_string(get_step_id()) +
                      ":" + log_info + "\n";

    buf_size += lgraph_api::profl_logger->Formalize(buf + buf_size, get_call_desc(), log);
    bool mark = true;
    if (buf_size + 120 > buf_capacity) {
        std::string logs(buf, buf_size);
        lgraph_api::profl_logger->AppendGolobalLog(std::move(logs));
        buf_size = 0;
    }
}

void profl_init(std::string& log_dir) {
    char head_t[1024];
    auto t = std::chrono::system_clock::now();
    time_t tnow = std::chrono::system_clock::to_time_t(t);
    tm* date = std::localtime(&tnow);
    size_t s = std::strftime(head_t, 24, "%Y%m%d%H%M%S: ", date);
    std::string header = head_t;
    header +=
        "Log Format = {day-hour-minute-second-microsecond}: {[plugin name]} "
        "{callID-TransactionID-StepID: "
        "start(1)/end(0)}\n";
    header = "" + header;
    lgraph_api::profl_logger.reset(new Profl(log_dir, std::move(header)));
}

std::string& get_call_desc() { return call_desc_local; }

bool set_call_desc(const std::string& call_desc_new) {
    call_desc_local = call_desc_new;
    return true;
}

void set_call_id(size_t call_id) {
    transaction_count.set_zero();
    call_count = call_id;
}

bool add_transaction_id() {
    step_count.set_zero();
    return transaction_count.add_id();
}

size_t get_transaction_id() { return transaction_count.get_id(); }

bool add_step_id() { return step_count.add_id(); }
size_t get_step_id() { return step_count.get_id(); }

Profl::~Profl() {
    exit_flag_ = true;
    profl_flusher_.join();
    for (auto& log : waiting_logs_) {
        *log_file_ << log;
    }
    log_file_->flush();
}

Profl::Profl(const std::string& log_dir, const std::string header)
    : log_dir_(log_dir), exit_flag_(false) {
    // now open log file for write
    OpenNextLogForWrite();

    AppendGolobalLog(std::move(header));

    // ok, start wal flusher
    profl_flusher_ = std::thread([this]() { FlusherThread(); });
}

void Profl::OpenNextLogForWrite() {
    if (next_log_file_id_ != 0) {
        log_file_->flush();
        log_file_->close();
    }
    log_rotate_size_curr_ = 0;
    log_file_.reset(new std::ofstream(GetLogFilePathFromId(next_log_file_id_++), std::ios::out));
}

std::string Profl::GetLogFilePathFromId(uint64_t log_file_id) const {
    return FMA_FMT("{}/{}{}", log_dir_, "breakdown.log.", log_file_id);
}

void Profl::FlusherThread() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (exit_flag_) break;

        bool need_flush = true;
        if (waiting_logs_.size() < batch_size_) {
            // wait for new txns
            cond_.wait_for(lock, std::chrono::milliseconds(batch_time_ms_));
            need_flush = (waiting_logs_.size() >= batch_size_);
        }
        if (need_flush) {
            std::deque<std::string> logs = std::move(waiting_logs_);
            waiting_logs_ = std::deque<std::string>();
            log_count += logs.size();
            lock.unlock();

            for (auto& log : logs) {
                *log_file_ << log;
            }
            log_file_->flush();

            log_rotate_size_curr_ += logs.size();
            if (log_rotate_size_curr_ >= log_rotate_size_max_) {
                OpenNextLogForWrite();
            }
        }
    }
}
inline size_t Profl::PrintModuleName(char* buf, size_t off, const std::string& module) {
    if (module.empty()) return 0;
    buf[off++] = '[';
    memcpy(buf + off, module.data(), module.size());
    off += module.size();
    buf[off++] = ']';
    buf[off++] = ' ';
    return module.size() + 3;
}

int Profl::AppendGolobalLog(const std::string logs_local) {
    std::lock_guard<std::mutex> lock(mutex_);
    waiting_logs_.emplace_back(std::move(logs_local));
    cond_.notify_one();
    return 0;
}
inline size_t Profl::GetSystemTime() {
    size_t hi, lo;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi));
    return ((size_t)lo) | (((size_t)hi) << 32);
}
size_t Profl::Formalize(char* buf, const std::string& module, const std::string& log) {
    size_t hs = 0;
    size_t buf_size = 60;
    // auto t = std::chrono::system_clock::now();
    // volatile time_t tnow = std::chrono::system_clock::to_time_t(t);
    // volatile tm* date = std::localtime(&tnow);
    // volatile size_t s = std::strftime(buf, buf_size, "%d%H%M%S", date);
    // assert(s < buf_size);

    // date->tm_hour = 0;
    // date->tm_min = 0;
    // date->tm_sec = 0;
    // auto midnight = std::chrono::system_clock::from_time_t(std::mktime(date));
    // size_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t -
    // midnight).count();

    // size_t hs = s + snprintf(buf + s, buf_size - s, ".%09d: ", (int)(elapsed_ns % 1000000000));
    // size_t hs = s;

    volatile size_t now = GetSystemTime();
    hs += sprintf(buf + hs, "%zu: ", now);
    size_t s_module = PrintModuleName(buf, hs, module);
    memcpy(buf + hs + s_module, log.data(), log.size());
    return hs + s_module + log.size();
}

}  // namespace yz_logger

}  // namespace lgraph_api
