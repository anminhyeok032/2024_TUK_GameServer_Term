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
	// [스프라이트 실제 크기 (JSON 기준 매핑)]
	// Bone_14 : 10x47px  (대검, 1x2)
	// Bone_12 : 10x46px  (장검, 1x2)
	// Bone_5  : 12x41px  (창, 1x2)
	// Bone_4  : 10x39px  (단창, 1x2)
	// Bone_11 : 26x12px  (완드, 2x1)
	// Bone_6  : 18x24px  (방패, 1x1)
	// Bone_7  : 16x30px  (단검, 1x1)
	// Bone_17 : 16x16px  (표창, 1x1)
	// Bone_10 : 10x23px  (단도, 1x1)
	// Bone_3  : 10x32px  (곡검, 1x1)
	// Bone_8  :  6x27px  (송곳, 1x1)
	// Bone_18 :  6x16px  (바늘, 1x1)
	// Bone_0  : 16x31px  (건틀릿/방어구, 1x1)
	// Bone_9  : 14x30px  (막대/몽둥이, 1x1)
	// Bone_13 : 11x13px  (부적, 1x1)
	// Bone_16 : 14x16px  (반지, 1x1)
	// Bone_15 : 14x16px  (메달, 1x1)
	// Bone_1  : 16x31px  (둔기, 1x1)
	// Bone_2  : 12x32px  (활/뼈대, 1x1)
	// ================================================================

	// --- 무기 ---
	db_[1001] = { "Bone_14", 1, 2, "대검" };  // 10x47
	db_[1002] = { "Bone_6",  1, 1, "방패" };  // 18x24
	db_[1003] = { "Bone_12", 1, 2, "장검" };  // 10x46
	db_[1004] = { "Bone_7",  1, 1, "단검" };  // 16x30
	db_[1005] = { "Bone_5",  1, 2, "창" };  // 12x41
	db_[1006] = { "Bone_4",  1, 2, "단창" };  // 10x39
	db_[1007] = { "Bone_17", 1, 1, "표창" };  // 16x16
	db_[1008] = { "Bone_10", 1, 1, "단도" };  // 10x23
	db_[1009] = { "Bone_3",  1, 1, "곡검" };  // 10x32
	db_[1010] = { "Bone_8",  1, 1, "송곳" };  // 6x27
	db_[1011] = { "Bone_18", 1, 1, "바늘" };  // 6x16

	// --- 방어구 / 장비 ---
	db_[1012] = { "Bone_0",  1, 1, "건틀릿" };  // 16x31
	db_[1013] = { "Bone_11", 2, 1, "완드" };  // 26x12
	db_[1014] = { "Bone_9",  1, 1, "막대" };  // 14x30

	// --- 소비 / 기타 ---
	db_[1015] = { "Bone_13", 1, 1, "부적" };  // 11x13
	db_[1016] = { "Bone_16", 1, 1, "반지" };  // 14x16
	db_[1017] = { "Bone_15", 1, 1, "메달" };  // 14x16
	db_[1018] = { "Bone_1",  1, 1, "둔기" };  // 16x31 
	db_[1019] = { "Bone_2",  1, 1, "활" };  // 12x32

	std::cout << "[ItemDatabase] 초기화 완료 - "
		<< db_.size() << "개 항목" << std::endl;
}

const ItemInfo* ItemDatabase::Get(int template_id) const
{
	auto it = db_.find(template_id);
	if (it == db_.end()) return nullptr;
	return &it->second;
}
