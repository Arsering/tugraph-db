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
#include "core/value.h"

#include "lgraph/lgraph_yz.h"

namespace lgraph_api {
static std::unique_ptr<yz_logger::Profl> profl_logger;

namespace yz_logger {

thread_local std::string call_desc_local = "u_i";
thread_local size_t call_count = 0;
thread_local int transaction_count = 0;
thread_local int step_count = 0;

const static size_t buf_capacity = 4096;
thread_local char* buf = (char*)malloc(buf_capacity);
volatile thread_local size_t buf_size = 0;

void print_a(int a) { volatile size_t b = a; }

void log_breakdown(char* log_info) {
    if (call_desc_local == "u_i") return;
    volatile size_t now = lgraph_api::profl_logger->GetSystemTime();
    buf_size += sprintf(buf + buf_size, "%zu: [%s] C%zu-T%zu-S%zu:%s\n", now, call_desc_local,
                        call_count, transaction_count, step_count, log_info);

    if (buf_size + 120 > buf_capacity) {
        //     lgraph::Value logs;
        //     logs.TakeOwnershipFrom(buf, buf_size);
        //     lgraph_api::profl_logger->AppendGolobalLog(std::move(logs));
        //     buf = (char*)malloc(buf_capacity);
        buf_size = 0;
    }
}

void profl_init(std::string& log_dir) {
    char* buf = (char*)malloc(buf_capacity);
    size_t buf_size = 0;
    auto t = std::chrono::system_clock::now();
    time_t tnow = std::chrono::system_clock::to_time_t(t);
    tm* date = std::localtime(&tnow);
    buf_size += std::strftime(buf, 24, "%Y%m%d%H%M%S: ", date);
    buf_size += sprintf(buf + buf_size, "%s\n",
                        "Log Format = {day-hour-minute-second-microsecond}: {[plugin name]} "
                        "{callID-TransactionID-StepID: start(1)/end(0)}");

    lgraph::Value header;
    header.TakeOwnershipFrom(buf, buf_size);
    lgraph_api::profl_logger.reset(new Profl(log_dir, std::move(header)));
}

bool set_call_desc(const std::string& call_desc_new) {
    call_desc_local = call_desc_new;
    return true;
}

void set_call_id(size_t call_id) {
    transaction_count = 0;
    step_count = 0;
    call_count = call_id;
}

bool add_transaction_id() {
    step_count = 0;
    transaction_count += 1;
    return true;
}

Profl::~Profl() {
    exit_flag_ = true;
    profl_flusher_.join();
    for (auto& log : waiting_logs_) {
        log_file_->write(log.Data(), log.Size());
    }
    log_file_->flush();
    log_file_->close();
}

Profl::Profl(const std::string& log_dir, lgraph::Value header)
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
            std::deque<lgraph::Value> logs = std::move(waiting_logs_);
            waiting_logs_ = std::deque<lgraph::Value>();
            log_count += logs.size();
            lock.unlock();

            for (auto& log : logs) {
                log_file_->write(log.Data(), log.Size());
            }

            log_rotate_size_curr_ += logs.size();
            if (log_rotate_size_curr_ >= log_rotate_size_max_) {
                OpenNextLogForWrite();
            }
        }
    }
}

int Profl::AppendGolobalLog(const lgraph::Value logs_local) {
    std::lock_guard<std::mutex> lock(mutex_);
    waiting_logs_.emplace_back(std::move(logs_local));
    volatile size_t a = 0;
    cond_.notify_one();
    return 0;
}

__always_inline size_t Profl::GetSystemTime() {
    size_t hi, lo;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi));
    return ((size_t)lo) | (((size_t)hi) << 32);
}

}  // namespace yz_logger

}  // namespace lgraph_api
