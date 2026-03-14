#pragma once
#include "global.h"

// 아이템 종류
enum ItemType {
	ITEM_NONE = 0,
	ITEM_WEAPON,
	ITEM_ARMOR,
	ITEM_POTION,
	ITEM_ETC
};

// 변하지 않는 아이템의 기본 정보
struct ItemTemplate {
	int template_id;
	char name[NAME_SIZE];
	ItemType type;
	int width;      // 가로 크기 (Grid)
	int height;     // 세로 크기 (Grid)
	int max_stack;  // 한 칸에 겹칠 수 있는 최대 개수
};

// 실제 유저가 가진 아이템 정보
class Item {
public:
	long long item_uid;  // 고유 ID (DB Primary Key)
	int template_id;     // 어떤 아이템인가?

	// 위치 정보
	short x;
	short y;
	bool is_rotated;     // 회전 여부 (가로/세로 바뀜)

public:
	Item() : item_uid(0), template_id(0), x(-1), y(-1), is_rotated(false) {}
	~Item() {}

	// 회전 상태를 고려한 실제 너비 반환
	int GetWidth(const ItemTemplate& info) const {
		return is_rotated ? info.height : info.width;
	}
	// 회전 상태를 고려한 실제 높이 반환
	int GetHeight(const ItemTemplate& info) const {
		return is_rotated ? info.width : info.height;
	}
};
