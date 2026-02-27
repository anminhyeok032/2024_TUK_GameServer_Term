#include "ItemDatabase.h"
#include <iostream>

ItemDatabase& ItemDatabase::GetInstance()
{
	static ItemDatabase instance;
	return instance;
}

void ItemDatabase::Init()
{
	// ================================================================
	// template_id : { sprite_id, grid_w, grid_h, name }
	//
	// [서버 Inventory.cpp와 반드시 동기화]
	// grid_w / grid_h 는 서버 GetItemTemplate() 반환값과 일치해야 함
	//
	// [스프라이트 참고]
	// Bone_14 : 16x79px  (장창형)
	// Bone_8  : 14x63px  (장검형)
	// Bone_12 : 10x46px  (단검형)
	// Bone_4  : 12x41px  (단검형)
	// Bone_3  : 10x39px  (단검형)
	// Bone_1  : 16x31px  (단검형)
	// Bone_6  : 16x30px  (단검형)
	// Bone_2  : 12x32px  (단검형)
	// Bone_9  : 10x23px  (소형무기)
	// Bone_7  :  6x27px  (소형무기)
	// Bone_5  : 18x24px  (소형무기)
	// Bone_0  : 30x28px  (방패형)
	// Bone_11 : 26x12px  (가로형)
	// Bone_10 : 16x6px   (가로형)
	// Bone_13 : 11x13px  (소형)
	// Bone_15 : 14x16px  (소형)
	// Bone_16 : 14x16px  (소형)
	// Bone_17 : 16x16px  (소형)
	// Bone_18 :  6x16px  (소형)
	// ================================================================

	// --- 무기 ---
	db_[1001] = { "Bone_14", 2, 3, "대검"   };  // 16x79 / 서버 기존 정의 유지
	db_[1002] = { "Bone_0",  2, 2, "방패"   };  // 30x28 / 서버 기존 정의 유지
	db_[1003] = { "Bone_8",  1, 2, "장검"   };  // 14x63
	db_[1004] = { "Bone_12", 1, 2, "단검"   };  // 10x46
	db_[1005] = { "Bone_4",  1, 2, "창"     };  // 12x41
	db_[1006] = { "Bone_3",  1, 2, "단창"   };  // 10x39
	db_[1007] = { "Bone_2",  1, 1, "표창"   };  // 12x32
	db_[1008] = { "Bone_1",  1, 1, "단도"   };  // 16x31
	db_[1009] = { "Bone_6",  1, 1, "곡검"   };  // 16x30
	db_[1010] = { "Bone_9",  1, 1, "송곳"   };  // 10x23
	db_[1011] = { "Bone_7",  1, 1, "바늘"   };  // 6x27

	// --- 방어구 / 장비 ---
	db_[1012] = { "Bone_5",  1, 1, "건틀릿" };  // 18x24
	db_[1013] = { "Bone_11", 2, 1, "완드"   };  // 26x12
	db_[1014] = { "Bone_10", 2, 1, "막대"   };  // 16x6

	// --- 소비 / 기타 ---
	db_[1015] = { "Bone_13", 1, 1, "부적"   };  // 11x13
	db_[1016] = { "Bone_15", 1, 1, "반지"   };  // 14x16
	db_[1017] = { "Bone_16", 1, 1, "메달"   };  // 14x16
	db_[1018] = { "Bone_17", 1, 1, "구슬"   };  // 16x16
	db_[1019] = { "Bone_18", 1, 1, "핀"     };  // 6x16

	std::cout << "[ItemDatabase] 초기화 완료 - "
		<< db_.size() << "개 항목" << std::endl;
}

const ItemInfo* ItemDatabase::Get(int template_id) const
{
	auto it = db_.find(template_id);
	if (it == db_.end()) return nullptr;
	return &it->second;
}
