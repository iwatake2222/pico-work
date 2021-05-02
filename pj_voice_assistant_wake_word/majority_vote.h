/* Copyright 2021 iwatake2222

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef MAJORITY_VOTE_H_
#define MAJORITY_VOTE_H_

#include <cstdint>
#include <array>

template<class T>
class MajorityVote
{
private:
    static constexpr int32_t kCategoryNum = 5;
    static constexpr int32_t kHistoryNum = 10;

public:
    MajorityVote() {
        for (auto& score_list : score_list_history_) {
            for (auto& score : score_list) {
                score = 0;
            }
        }
        history_index_ = 0;
        is_history_full_ = false;
    }

    ~MajorityVote() {}

    void vote(const std::array<T, kCategoryNum>& new_score_list, int32_t& first_index, T& score) {
        auto& current_score_list = score_list_history_[history_index_];
        for (int32_t category = 0; category < kCategoryNum; category++) {
            current_score_list[category] = new_score_list[category];
        }
        
        history_index_++;
        if (history_index_ >= kHistoryNum) {
            history_index_ = 0;
            is_history_full_ = true;
        }

        int32_t history_num = (is_history_full_ ? kHistoryNum : history_index_);
        std::array<T, kCategoryNum> accumulated_score_list = { 0 };
        for (int32_t history = 0; history < history_num; history++) {
            auto& score_list = score_list_history_[history];
            for (int32_t category = 0; category < kCategoryNum; category++) {
                accumulated_score_list[category] += score_list[category];
            }
        }

        auto max = std::max_element(accumulated_score_list.begin(), accumulated_score_list.end());
        size_t max_index = std::distance(accumulated_score_list.begin(), max);
        first_index = static_cast<int32_t>(max_index);
        score = *max / history_num;
    }

private:
    std::array<std::array<T, kCategoryNum>, kHistoryNum> score_list_history_;
    int32_t history_index_;
    bool is_history_full_;
};

#endif  // MAJORITY_VOTE_H_
