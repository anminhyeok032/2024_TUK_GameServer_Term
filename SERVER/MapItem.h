#pragma once
#include "Session.h"

// [MapItem]
// 필드에 떨어진 아이템을 나타내는 객체
// Session을 상속받아 Sector 관리 및 시야 처리

class MapItem : public SESSION
{
public:
	long long item_uid;  // 실제 아이템 데이터 ID
	int template_id;     // 아이템 종류
		
	// 생성 시간 (나중에 일정 시간 지나면 사라지게)
	std::chrono::system_clock::time_point drop_time;

public:
	MapItem() : item_uid(0), template_id(0) {
		drop_time = std::chrono::system_clock::now();
		visual_ = OT_MAP_ITEM;
	}
	~MapItem() {}

	// Session 가상 함수
	void SendAddObjectPacket(int c_id) override;
	void SendRemoveObjectPacket(int c_id) override;
};
