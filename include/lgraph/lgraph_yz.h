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

/**
 * @brief Implemnets the DateTime, Date and TimeZone classes.
 */

#pragma once
#include <cstdint>
#include <map>
#include <atomic>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>

namespace lgraph_api {

namespace yz_logger {
static std::map<std::string, std::atomic<size_t>> call_counts_yz;
enum operation_type {
    complex_read_1 = 1,
    complex_read_2 = 2,
    complex_read_3 = 3,
    complex_read_4 = 4,
    complex_read_5 = 5,
    complex_read_6 = 6,
    complex_read_7 = 7,
    complex_read_8 = 8,
    complex_read_9 = 9,
    complex_read_10 = 10,
    complex_read_11 = 11,
    complex_read_12 = 12,
    complex_read_13 = 13,
    complex_read_14 = 14,
    short_read_1 = 14 + 1,
    short_read_2 = 14 + 2,
    short_read_3 = 14 + 3,
    short_read_4 = 14 + 4,
    short_read_5 = 14 + 5,
    short_read_6 = 14 + 6,
    short_read_7 = 14 + 7,
    update_1 = 21 + 1,
    update_2 = 21 + 2,
    update_3 = 21 + 3,
    update_4 = 21 + 4,
    update_5 = 21 + 5,
    update_6 = 21 + 6,
    update_7 = 21 + 7,
    update_8 = 21 + 8,
};

void log_breakdown(std::string& log_info);
void profl_init(std::string& log_info);

std::string& get_call_desc();

bool set_call_desc(const std::string& call_desc_new);

void set_call_id(size_t call_id);

bool add_transaction_id();
size_t get_transaction_id();

bool add_step_id();
size_t get_step_id();

class Profl {
 private:
    std::atomic<bool> exit_flag_;
    std::string log_dir_;
    // the log_file records txn and kv operations
    uint64_t next_log_file_id_ = 0;
    // this file records only meta-operations like opening and dropping tables
    std::unique_ptr<std::ofstream> log_file_;

    std::mutex mutex_;
    std::condition_variable cond_;
    std::deque<std::string> waiting_logs_;
    size_t log_rotate_size_max_ = 1024 * 1024;
    size_t log_rotate_size_curr_ = 0;
    size_t batch_size_ = 1024;
    size_t batch_time_ms_ = 500;
    size_t log_count = 0;

    // The flusher thread flushes wal on constant intervals and notifies waiting
    // txns. It also prepares new log files for rotation.

    std::thread profl_flusher_;

 public:
    Profl();

    Profl(const std::string& log_dir, const std::string header);

    ~Profl();

    int AppendGolobalLog(const std::string logs_local);
    size_t Formalize(char* buf, const std::string& module, const std::string& log);

 private:
    // Called on transaction commit, when we flush the wal. This guarantees that each
    // log file will contain a complete transaction.
    // This function checks whether the last flush time is too long ago. If that is the
    // case, close current log file and create a new one. It also schedules an asynchronous
    // task to flush the kv store and delete the old log file.
    void FlusherThread();

    // open next log file, changing curr_log_path_ and next_log_file_id
    void OpenNextLogForWrite();

    // Get log path from id
    std::string GetLogFilePathFromId(uint64_t log_file_id) const;

    // Get dbi file path
    std::string GetDbiFilePath() const;

    inline size_t PrintModuleName(char* buf, size_t off, const std::string& module);

    inline size_t GetSystemTime();
};

}  // namespace yz_logger
}  // namespace lgraph_api
