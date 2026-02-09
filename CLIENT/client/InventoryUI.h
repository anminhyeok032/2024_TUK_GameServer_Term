#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <chrono>
#include "Constants.h"
#include "protocol.h"

// [Inventory UI System]
// 인벤토리 화면 그리기, 마우스 입력 처리, 서버 동기화(Lazy Sync)를 담당

struct ClientItem {
	long long item_uid;
	int template_id;
	int count;
	int x, y;
	bool is_rotated;
	sf::Sprite sprite;
};

// 아이템 크기 정보
struct ClientItemTemplate {
	int w, h;
};

class InventoryUI
{
private:
	sf::RenderWindow* window_;
	sf::Font* font_;

	bool isActive_;
	std::vector<ClientItem> myItems_;

	// 드래그 관련
	bool isDragging_;
	long long draggingItemUID_;
	sf::Vector2f dragOffset_;

	// 동기화 관련
	bool isDirty_;
	std::chrono::system_clock::time_point lastSyncTime_;

	// 상수
	static constexpr int UI_X = 100;
	static constexpr int UI_Y = 100;
	static constexpr int SLOT_SIZE = 40;
	static constexpr int MARGIN = 2;

public:
	InventoryUI();
	~InventoryUI();

	void Initialize(sf::RenderWindow* window, sf::Font* font);
	
	// 토글 및 상태 확인
	void Toggle();
	bool IsActive() const { return isActive_; }

	// 그리기 및 업데이트
	void Draw();
	void UpdateSync(); // 5초 주기 체크

	// 입력 처리
	void HandleInput(sf::Event& event);

	// 아이템 관리
	void AddItem(long long uid, int tid, int cnt, int x, int y, bool rot);
	ClientItem* FindItem(long long uid);
	ClientItemTemplate GetItemTemplate(int tid);
	bool CanPlace(int startX, int startY, int w, int h, long long excludeUID);
	
	// 서버로 변경사항 전송
	void SyncToServer();
};

extern InventoryUI g_inventoryUI; // 전역 인스턴스 선언
