#pragma once
#include "global.h"
#include "protocol.h"

class OVER
{
public:
	WSAOVERLAPPED over_;
	WSABUF wsabuf_;
	char send_buf_[BUF_SIZE];
	COMP_KEY comp_key_;

	// ai_target_c_id_ : AI가 추적하는 대상의 client id
	int ai_target_c_id_;

	OVER()
	{
		wsabuf_.buf = send_buf_;
		wsabuf_.len = BUF_SIZE;
		comp_key_ = KEY_RECV;
		ZeroMemory(&over_, sizeof(over_));
	}
	OVER(char* packet)
	{
        unsigned short packet_size = reinterpret_cast<unsigned short*>(packet)[0];
		wsabuf_.len = packet_size;
		wsabuf_.buf = send_buf_;
		ZeroMemory(&over_, sizeof(over_));
		comp_key_ = KEY_SEND;
		memcpy(send_buf_, packet, wsabuf_.len);
	}
};


template<typename MemoryPool>
class ObjectPool
{
private:
    concurrency::concurrent_queue<MemoryPool*> pool_;   // 객체 풀
    std::atomic<size_t> max_size_;                      // 최대 객체 크기
    std::atomic<size_t> current_size_;                  // 현재 객체 크기
public:
    ObjectPool(size_t maxSize)
        : max_size_(maxSize), current_size_(0) { }

    // 객체 가져오기
    MemoryPool* Acquire()
    {
        MemoryPool* obj = nullptr;
        // 재사용 가능한 객체가 있으면 반환
        if (pool_.try_pop(obj))
            return obj;
        // 없으면 새로 생성
        current_size_.fetch_add(1, std::memory_order_relaxed);
        return new MemoryPool();
    }

    // 객체 반환 (max_size 초과 시 바로 삭제)
    void Release(MemoryPool* obj)
    {
        // 객체가 max_size 이하일 때만 풀에 반환
        if (current_size_.load(std::memory_order_relaxed) <= max_size_)
            pool_.push(obj);
        // 객체 반환
        else
            delete obj;
            current_size_.fetch_sub(1, std::memory_order_relaxed);
    }

    // 주기적 Trim - max_size보다 많이 쌓인 경우 정리
    void Trim() 
    {
        MemoryPool* obj = nullptr;
        int delete_count = 0;
        while (current_size_.load(std::memory_order_relaxed) > max_size_ && pool_.try_pop(obj))
        {
            delete obj;
            current_size_.fetch_sub(1, std::memory_order_relaxed);
            delete_count++;
        }
        if (delete_count > 0)
        {
            //std::cout << "주기적 정리 - " << delete_count << "개 삭제";
            //Log(delete_count); // 주기적 정리 - delete_count개 삭제
        }
    }

    // max_size를 플레이어 수에 맞춰 변경하여 메모리 낭비를 줄이기
    void SetMaxSize(size_t newMax)
    {
        max_size_.store(newMax, std::memory_order_relaxed);
    }

    size_t Size() const
    {
        return current_size_.load(std::memory_order_relaxed);
    }

    void Log(int type) const
    {
        switch (type) {
        case 0: 
            std::cout << "재사용된 객체 사용";                          
            break;
        case 1: 
            std::cout << "객체가 max_size를 초과하여 새로 생성됨";      
            break;
        case 2: 
            std::cout << "객체 반환";                                   
            break;
        case 3: 
            std::cout << "객체가 max_size를 초과하여 삭제됨";
            break;
        default:     
            break;
        }
        std::cout << ", Current Size: " << Size() << std::endl;
    }


};
extern ObjectPool<OVER> g_sendPool;