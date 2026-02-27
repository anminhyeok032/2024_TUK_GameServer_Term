#pragma once
#include <unordered_map>
#include <string>

// [ItemDatabase]
// template_id -> 아이템 정보(스프라이트 id, 인벤토리 grid 크기, 이름) 매핑
// 서버의 Item.h / Inventory.cpp 와 template_id를 반드시 동기화할 것

struct ItemInfo {
	std::string sprite_id;  // ItemSpriteSheet에서 사용하는 키 ("Bone_0" 등)
	int         grid_w;     // 인벤토리 가로 칸 수
	int         grid_h;     // 인벤토리 세로 칸 수
	std::string name;       // 표시 이름
};

class ItemDatabase
{
private:
	// template_id -> ItemInfo
	std::unordered_map<int, ItemInfo> db_;

	ItemDatabase() = default;
	ItemDatabase(const ItemDatabase&) = delete;
	ItemDatabase& operator=(const ItemDatabase&) = delete;

public:
	static ItemDatabase& GetInstance();

	// 앱 시작 시 1회 호출
	void Init();

	// template_id로 조회. 없으면 nullptr 반환
	const ItemInfo* Get(int template_id) const;
};
