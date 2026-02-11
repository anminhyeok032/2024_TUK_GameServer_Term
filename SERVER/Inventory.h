#pragma once
#include "Item.h"
#include <unordered_map>
#include <vector>

class Inventory
{
private:
	int owner_id_; // 이 인벤토리의 주인 (Player ID)

	// 해당 칸을 차지하고 있는 item_uid를 저장 (0이면 빈칸)
	long long grid_[INV_MAX_ROW][INV_MAX_COL];

	// 빠른 접근을 위한 맵: item_uid -> Item 객체 포인터
	std::unordered_map<long long, Item*> items_;

public:
	Inventory(int owner_id);
	~Inventory();

	// 아이템 추가 (빈 공간 자동 탐색)
	bool AddItem(Item* item);

	// 아이템 추가 (특정 위치 지정 - DB 로드 or 이동 시)
	bool PlaceItem(Item* item, int x, int y, bool rotated);

	// 아이템 이동 (인벤토리 내에서 위치 변경)
	bool MoveItem(long long item_uid, int new_x, int new_y, bool new_rotated);

	// 아이템 제거
	bool RemoveItem(long long item_uid);

	// 해당 위치에 아이템을 놓을 수 있는지 검사
	bool CanPlace(int x, int y, int w, int h, long long exclude_item_uid = 0);

	// 템플릿 정보 가져오기
	ItemTemplate GetItemTemplate(int template_id); 

	// Redis 저장을 위한 데이터 반환 (items_ 캡슐화 유지)
	// 반환값: vector of {ItemUID, ValueString}
	std::vector<std::pair<std::string, std::string>> GetInventoryDataForRedis();

	// 디버깅용: 콘솔에 그리드 상태 출력
	void PrintGrid();
};
