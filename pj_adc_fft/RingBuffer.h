#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <cstdio>
#include <cstdint>
#include <vector>

/*** Buffer structure
 *   |------------------------------------------|
 *   | buffer[0]             : data of dataSize |
 *   | buffer[1]             : data of dataSize |  <--- RP (ptr to be read at the next READ)
 *   | buffer[2]             : data of dataSize |
 *   | buffer[3]             : data of dataSize |  <--- WP (ptr to be written at the next WRITE)
 *   | buffer[bufferSize - 1]: data of dataSize |
 *   |------------------------------------------|
 ***/

/*** Notice
 * Mutex is not implemented!!!
 ***/

template<class T>
class RingBuffer
{
public:
	RingBuffer() 
	{
	};

	~RingBuffer()
	{
	};

	void initialize(int32_t bufferSize, int32_t dataSize)
	{
		m_buffer.resize(bufferSize);
		for (int i = 0; i < bufferSize; i++) {
			m_buffer[i].resize(dataSize);
		}

		m_wp = 0;
		m_rp = 0;
		m_sotedDataNum = 0;
	}

	void finalize()
	{
		for (int i = 0; i < m_buffer.size(); i++) {
			m_buffer[i].clear();
		}
		m_buffer.clear();
	}

	void write(const std::vector<T>& data)
	{
		if (isOverflow()) return;
		m_buffer[m_wp] = data;
		incrementWp();
	}

	T* writePtr()
	{
		if (isOverflow()) return NULL;
		T* ptr = m_buffer[m_wp].data();
		incrementWp();
		return ptr;
	}


	T* getLatestWritePtr()
	{
		int32_t previousWp = m_wp - 1;
		if (previousWp < 0) previousWp += m_buffer.size();
		T* ptr = m_buffer[previousWp].data();
		return ptr;
	}

	std::vector<T>& read()
	{
		std::vector<T>& readData = m_buffer[m_rp];
		incrementRp();
		return readData;
	}

	// do not update RP. 0 = current, 1 = next, 2 = next next (NOTE: underflow is not checked)
	std::vector<T>& refer(int32_t next)
	{
		int32_t index = m_rp + next;
		if (index >= m_buffer.size()) index -= m_buffer.size();
		std::vector<T>& readData = m_buffer[index];
		return readData;
	}

	T* readPtr()
	{
		std::vector<T>& readData = m_buffer[m_rp];
		incrementRp();
		return readData.data();
	}

	// do not update RP. 0 = current, 1 = next, 2 = next next (NOTE: underflow is not checked)
	T* referPtr(int32_t next)
	{
		int32_t index = m_rp + next;
		if (index >= m_buffer.size()) index -= m_buffer.size();
		std::vector<T>& readData = m_buffer[index];
		return readData.data();
	}

	bool isOverflow()
	{
		if (m_wp == m_rp && m_sotedDataNum > 0) {
			return true;
		} else {
			return false;
		}
	}


	bool isUnderflow()
	{
		return m_sotedDataNum == 0;
	}

	int32_t getStoredDataNum()
	{
		return m_sotedDataNum;
	}

private:
	void incrementWp()
	{
		m_sotedDataNum++;
		m_wp++;
		if (m_wp >= m_buffer.size()) m_wp = 0;
	}

	void incrementRp()
	{
		if (!isUnderflow()) {
			m_rp++;
			if (m_rp >= m_buffer.size()) m_rp = 0;
			m_sotedDataNum--;
		}
	}

private:
	std::vector<std::vector<T>> m_buffer;
	int32_t m_wp;
	int32_t m_rp;
	int32_t m_sotedDataNum;
};

#endif
