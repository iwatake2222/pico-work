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

#ifndef RING_BLOCK_BUFFER_H_
#define RING_BLOCK_BUFFER_H_

#include <cstdint>
#include <cstdio>
#include <vector>

/*** Buffer structure
 *   |------------------------------------------|
 *   | block[0]             : data of blockSize |
 *   | block[1]             : data of blockSize |  <--- RP (ptr to be read at the next READ)
 *   | block[2]             : data of blockSize |
 *   | block[3]             : data of blockSize |  <--- WP (ptr to be written at the next WRITE)
 *   | block[bufferSize - 1]: data of blockSize |
 *   |------------------------------------------|
 ***/

 /*** Notice
  * Mutex is not implemented!!!
  ***/

template<class T>
class RingBlockBuffer
{
public:
    RingBlockBuffer()
        : wp_(0)
        , rp_(0)
        , stored_data_num_(0)
        , accumulated_stored_data_num_(0)
        , accumulated_read_data_num_(0) {
    };

    ~RingBlockBuffer() {
    };

    void Initialize(int32_t buffer_size, int32_t block_size) {
        buffer_.resize(buffer_size);
        for (auto& block : buffer_) {
            block.resize(block_size);
        }

        wp_ = 0;
        rp_ = 0;
        stored_data_num_ = 0;
        accumulated_stored_data_num_ = 0;
        accumulated_read_data_num_ = 0;
    }

    void Finalize() {
        for (auto& block : buffer_) {
            block.clear();
        }
        buffer_.clear();
    }

    void Write(const std::vector<T>& data) {
        if (IsOverflow()) return;
        buffer_[wp_] = data;
        IncrementWp();
    }

    T* WritePtr() {
        if (IsOverflow()) return NULL;
        T* ptr = buffer_[wp_].data();
        IncrementWp();
        return ptr;
    }


    T* GetLatestWritePtr() {
        int32_t previous_wp = wp_ - 1;
        if (previous_wp < 0) previous_wp += buffer_.size();
        T* ptr = buffer_[previous_wp].data();
        return ptr;
    }

    std::vector<T>& Read() {
        int32_t previous_rp = rp_;
        IncrementRp();
        return buffer_[previous_rp];
    }

    // do not update RP. 0 = current, 1 = next, 2 = next next (NOTE: underflow is not checked)
    std::vector<T>& Refer(int32_t pos) {
        int32_t index = rp_ + pos;
        if (index >= buffer_.size()) index -= buffer_.size();
        std::vector<T>& read_data = buffer_[index];
        return read_data;
    }

    T* ReadPtr() {
        std::vector<T>& read_data = buffer_[rp_];
        IncrementRp();
        return read_data.data();
    }

    // do not update RP. 0 = current, 1 = next, 2 = next next (NOTE: underflow is not checked)
    T* ReferPtr(int32_t pos) {
        int32_t index = rp_ + pos;
        if (index >= buffer_.size()) index -= buffer_.size();
        std::vector<T>& readData = buffer_[index];
        return readData.data();
    }

    bool IsOverflow() const {
        if (wp_ == rp_ && stored_data_num_ > 0) {
            return true;
        } else {
            return false;
        }
    }

    bool IsUnderflow() const {
        return stored_data_num_ == 0;
    }

    int32_t stored_data_num() const {
        return stored_data_num_;
    }

    int32_t accumulated_stored_data_num() const {
        return accumulated_stored_data_num_;
    }

    int32_t accumulated_read_data_num() const {
        return accumulated_read_data_num_;
    }

private:
    void IncrementWp() {
        accumulated_stored_data_num_++;
        stored_data_num_++;
        wp_++;
        if (wp_ >= buffer_.size()) wp_ = 0;
    }

    void IncrementRp() {
        if (!IsUnderflow()) {
            accumulated_read_data_num_++;
            stored_data_num_--;
            rp_++;
            if (rp_ >= buffer_.size()) rp_ = 0;

        }
    }

private:
    std::vector<std::vector<T>> buffer_;
    int32_t wp_;
    int32_t rp_;
    int32_t stored_data_num_;
    int32_t accumulated_stored_data_num_;
    int32_t accumulated_read_data_num_;
};

#endif  // RING_BLOCK_BUFFER_H_
