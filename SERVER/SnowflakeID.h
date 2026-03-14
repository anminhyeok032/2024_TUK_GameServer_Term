#pragma once
#include <atomic>
#include <chrono>
#include <mutex>

// Snowflake ID Generator
// 아이템의 uid를 생성하기 위해 사용
// 이론상 1ms에 131071개까지 생성 가능 (충돌 없이)
// 64-bit: [1 sign][41 timestamp(ms)][5 server_id][17 sequence]
class SnowflakeIDGenerator
{
	static constexpr long long EPOCH = 1704067200000LL; // 2024-01-01 00:00:00 UTC (ms)
	static constexpr int SERVER_BITS = 5;
	static constexpr int SEQ_BITS = 17;
	static constexpr int SEQ_MASK = (1 << SEQ_BITS) - 1; // 131071

	int server_id_;
	long long last_ms_;
	int sequence_;
	std::mutex mut_;

	long long CurrentMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

public:
	SnowflakeIDGenerator(int server_id = 0)
		: server_id_(server_id & ((1 << SERVER_BITS) - 1))
		, last_ms_(0)
		, sequence_(0)
	{
	}

	long long Generate()
	{
		std::lock_guard<std::mutex> lock(mut_);

		long long now = CurrentMs();

		if (now == last_ms_)
		{
			sequence_ = (sequence_ + 1) & SEQ_MASK;
			if (sequence_ == 0)
			{
				// 같은 ms 내 시퀀스 소진 → 다음 ms까지 대기
				while (now <= last_ms_)
					now = CurrentMs();
			}
		}
		else
		{
			sequence_ = 0;
		}

		last_ms_ = now;

		return ((now - EPOCH) << (SERVER_BITS + SEQ_BITS))
			| ((long long)server_id_ << SEQ_BITS)
			| (long long)sequence_;
	}
};
