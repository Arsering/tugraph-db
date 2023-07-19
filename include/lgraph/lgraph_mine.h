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

namespace lgraph_api {

static std::map<std::string, std::atomic<size_t>> call_counts_yz;

void log_breakdown(std::string& log_info);
void log_breakdown_head(std::string& log_info);

std::string& get_call_desc();

bool set_call_desc(const std::string& call_desc_new);

void set_call_id(size_t call_id);

bool add_transaction_id();
size_t get_transaction_id();

bool add_step_id();
size_t get_step_id();

}  // namespace lgraph_api
