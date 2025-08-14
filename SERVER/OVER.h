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
		wsabuf_.len = packet[0];
		wsabuf_.buf = send_buf_;
		ZeroMemory(&over_, sizeof(over_));
		comp_key_ = KEY_SEND;
		memcpy(send_buf_, packet, wsabuf_.len);
	}
};


template<typename T>
class ObjectPool
{
private:
    concurrency::concurrent_queue<T*> pool;
    const size_t max_size;
    std::atomic<size_t> current_size;

public:
    ObjectPool(size_t maxSize)
        : max_size(maxSize), current_size(0) { }

    // 객체 가져오기
    T* Acquire()
    {
        T* obj = nullptr;
        if (pool.try_pop(obj))
        {
            //Log(0); // 재사용된 객체 사용
            return obj;
        }

        //Log(1); // 객체가 max_size를 초과하여 새로 생성됨
        current_size.fetch_add(1, std::memory_order_relaxed);
        return new T();
    }

    // 객체 반환 (max_size 초과 시 바로 삭제)
    void Release(T* obj)
    {
        if (current_size.load(std::memory_order_relaxed) <= max_size)
        {
            //Log(2); // 객체 반환
            pool.push(obj);
        }
        else
        {
            //Log(3); // 객체가 max_size를 초과하여 삭제됨
            delete obj;
            current_size.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    // 주기적 Trim - max_size보다 많이 쌓인 경우 정리
    void Trim() 
    {
        T* obj = nullptr;
        int delete_count = 0;
        while (current_size.load(std::memory_order_relaxed) > max_size && pool.try_pop(obj))
        {
            delete obj;
            current_size.fetch_sub(1, std::memory_order_relaxed);
            delete_count++;
        }
        if (delete_count > 0)
        {
            //std::cout << "주기적 정리 - " << delete_count << "개 삭제";
            //Log(delete_count); // 주기적 정리 - delete_count개 삭제
        }
    }

    size_t Size() const
    {
        return current_size.load(std::memory_order_relaxed);
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