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

#include "core/transaction.h"
// #include "core/type_convert.h"

#include "lgraph/lgraph_mine.h"

namespace lgraph_api {
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


thread_local std::string call_desc_local = "fdfad";
thread_local size_t call_count;
thread_local Count transaction_count;
thread_local Count step_count;

void log_breakdown(std::string& log_info) {
    fma_common::LoggerStream(fma_common::Logger::GetMine(get_call_desc()), fma_common::LL_INFO)
        << "C" + std::to_string(call_count) + "-T" +
               std::to_string(get_transaction_id()) + "-S" + std::to_string(get_step_id()) +
               ":" + log_info;
}
void log_breakdown_head(std::string& log_info) {
    fma_common::LoggerStream(fma_common::Logger::GetMine("Log Meta Data"), fma_common::LL_INFO)
        << log_info;
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

}  // namespace lgraph_api
