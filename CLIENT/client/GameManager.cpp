#include "GameManager.h"
#include "protocol.h"
#include "InventoryUI.h"
#include <iostream>
#include <cmath>
#include <algorithm> // for remove_if

using namespace std;

GameManager g_gameManager;

// 전역 함수 래퍼 (InventoryUI 등에서 사용)
void send_packet(void* packet) {
	g_gameManager.SendPacket(packet);
}

GameManager::GameManager() : window_(nullptr), myId_(-1), leftX_(0), topY_(0), 
isChatActive_(false), isRankingActive_(false), rankingScrollIndex_(0)
{
}

GameManager::~GameManager()
{
	ClientFinish();
	if (window_) delete window_;
}

bool GameManager::Initialize()
{
	if (!font_.loadFromFile("Resources/Fonts/cour.ttf")) {
		cout << "Font Loading Error!\n";
		return false;
	}
	return true;
}

bool GameManager::Connect(const char* ip)
{
	if (socket_.connect(ip, PORT_NUM) == sf::Socket::Done) {
		socket_.setBlocking(false);
		cout << "Connect Success with : " << ip << endl;
		return true;
	}
	return false;
}

void GameManager::ClientInitialize()
{
	boardTex_ = new sf::Texture;
	pieceTex_ = new sf::Texture;
	npcTex_ = new sf::Texture;
	boardTex_->loadFromFile("Resources/Textures/chessmap.bmp");
	pieceTex_->loadFromFile("Resources/Textures/chess2.png");
	npcTex_->loadFromFile("Resources/Textures/chess2_zombie.png");

	whiteTile_ = OBJECT{ *boardTex_, 5, 5, 64, 64 };
	blackTile_ = OBJECT{ *boardTex_, 69, 5, 64, 64 };
	avatar_ = OBJECT{ *pieceTex_, 128, 0, 64, 64 };
	avatar_.move(4, 4);

	// Map
	mapRect_.setSize(sf::Vector2f(200, 200)); // MAP_SIZE
	mapRect_.setPosition(WINDOW_WIDTH - 200, 0); // MAP_WIDTH
	mapRect_.setFillColor(sf::Color(255, 255, 255, 128));

	playerDot_.setRadius(5.f);
	playerDot_.setFillColor(sf::Color::Red);

	// HP/EXP Bar
	hpBar_.setFillColor(sf::Color::Red);
	hpBar_.setOutlineColor(sf::Color::Black);
	hpBar_.setPosition(WINDOW_WIDTH / 2 - 100, 10);

	expBar_.setFillColor(sf::Color::White);
	expBar_.setOutlineColor(sf::Color::Black);
	expBar_.setPosition(WINDOW_WIDTH / 2 - 100, 40);

	levelText_.setFont(font_);
	levelText_.setCharacterSize(20);
	levelText_.setFillColor(sf::Color::White);
	levelText_.setPosition(WINDOW_WIDTH / 2 - 50, 10);

	// Chat
	chatBox_.setSize(sf::Vector2f(800, 30));
	chatBox_.setFillColor(sf::Color(0, 0, 0, 150));
	chatBox_.setPosition(0, 570);
	chatText_.setFont(font_);
	chatText_.setCharacterSize(20);
	chatText_.setFillColor(sf::Color::White);
	chatText_.setPosition(5, 575);

	// 인벤토리 초기화
	g_inventoryUI.Initialize(window_, &font_);
}

void GameManager::ClientFinish()
{
	players_.clear();
	if (boardTex_) delete boardTex_;
	if (pieceTex_) delete pieceTex_;
	if (npcTex_) delete npcTex_;
}

void GameManager::Run()
{
	char SERVER_ADDR[BUF_SIZE];
	char PLAYER_ID[BUF_SIZE];
	cout << "Enter IP Address : ";
	cin.getline(SERVER_ADDR, BUF_SIZE);
	cout << "Enter Player ID : ";
	cin.getline(PLAYER_ID, BUF_SIZE);

	if (!Initialize()) return;
	if (!Connect(SERVER_ADDR)) return;

	window_ = new sf::RenderWindow(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	ClientInitialize();

	// 로그인 패킷 전송
	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;
	strcpy_s(p.name, PLAYER_ID);
	SendPacket(&p);

	avatar_.set_name(p.name, font_);

	while (window_->isOpen())
	{
		HandleInput();
		
		// 네트워크 처리
		char net_buf[BUF_SIZE];
		size_t received;
		auto status = socket_.receive(net_buf, BUF_SIZE, received);
		if (status == sf::Socket::Done) {
			ProcessData(net_buf, received);
		}
		else if (status == sf::Socket::Disconnected) {
			cout << "Disconnected\n";
			break;
		}

		// 인벤토리 동기화 체크
		g_inventoryUI.UpdateSync();

		// 렌더링
		window_->clear();
		Draw();
		DrawRanking();
		g_inventoryUI.Draw();
		window_->display();
	}
}

void GameManager::HandleInput()
{
	sf::Event event;
	bool enterConsumed = false;

	while (window_->pollEvent(event))
	{
		if (event.type == sf::Event::Closed) window_->close();

		// 인벤토리 입력 처리
		g_inventoryUI.HandleInput(event);

		// 랭킹 스크롤
		if (isRankingActive_ && event.type == sf::Event::MouseWheelScrolled) {
			if (event.mouseWheelScroll.delta > 0) rankingScrollIndex_--;
			else rankingScrollIndex_++;
			if (rankingScrollIndex_ < 0) rankingScrollIndex_ = 0;
		}

		if (event.type == sf::Event::KeyPressed) {
			// 이동/공격 로직 (채팅 중 아닐 때)
			if (!isChatActive_) {
				if (event.key.code == sf::Keyboard::I) {
					g_inventoryUI.Toggle();
				}
				// G키로 아이템 줍기
				if (event.key.code == sf::Keyboard::G) {
					CS_ITEM_PICKUP_PACKET p;
					p.size = sizeof(p);
					p.type = CS_ITEM_PICKUP;
					SendPacket(&p);
				}
				if (event.key.code == sf::Keyboard::P) {
					isRankingActive_ = !isRankingActive_;
					if (isRankingActive_) {
						CS_RANKING_REQ_PACKET p;
						p.size = sizeof(p);
						p.type = CS_RANKING_REQ;
						SendPacket(&p);
						rankingScrollIndex_ = 0;
					}
				}
				// 이동 (Arrow Keys)
				int dir = -1;
				if (event.key.code == sf::Keyboard::Left) dir = 2;
				if (event.key.code == sf::Keyboard::Right) dir = 3;
				if (event.key.code == sf::Keyboard::Up) dir = 0;
				if (event.key.code == sf::Keyboard::Down) dir = 1;
				if (dir != -1) {
					lastDirection_ = dir; // 이동 방향 저장
					CS_MOVE_PACKET p; p.size = sizeof(p); p.type = CS_MOVE; p.direction = dir;
					SendPacket(&p);
				}
				// 공격 (A: 범위, S: 평타)
				int attack_type = -1;
				if (event.key.code == sf::Keyboard::S) attack_type = 0; // 평타
				if (event.key.code == sf::Keyboard::A) attack_type = 1; // 범위

				if (attack_type != -1) {
					CS_ATTACK_PACKET p; 
					p.size = sizeof(p); 
					p.type = CS_ATTACK; 
					p.attack_type = (char)attack_type; 
					
					if (attack_type == 0) p.attack_direction = lastDirection_; // 저장된 방향
					else p.attack_direction = 4; // 범위

					SendPacket(&p);
				}
			}
			
			// 채팅 (Enter)
			if (event.key.code == sf::Keyboard::Enter) {
				if (isChatActive_) {
					CS_CHAT_PACKET p; p.size = sizeof(p); p.type = CS_CHAT;
					strcpy_s(p.mess, chatInput_.c_str());
					SendPacket(&p);
					avatar_.set_chat(p.mess, font_);
					
					// 내 채팅도 히스토리에 추가
					string msg = "[" + string(avatar_.name) + "] : " + p.mess;
					chatHistory_.push_back(msg);
					if (chatHistory_.size() > 5) chatHistory_.erase(chatHistory_.begin());

					chatInput_.clear();
					isChatActive_ = false;
				} else {
					isChatActive_ = true;
				}

				enterConsumed = true;
			}
		}

		// 채팅 입력
		if (!enterConsumed && isChatActive_ && event.type == sf::Event::TextEntered) {
			if (event.text.unicode == '\b') {
				if (!chatInput_.empty()) chatInput_.pop_back();
			} else if (event.text.unicode < 128 && event.text.unicode != '\r') {
				chatInput_ += static_cast<char>(event.text.unicode);
			}
		}
	}
}

void GameManager::ProcessPacket(char* ptr)
{
	switch (ptr[2])
	{
	case SC_LOGIN_INFO: {
		SC_LOGIN_INFO_PACKET* p = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		myId_ = p->id;
		avatar_.m_x = p->x;
		avatar_.m_y = p->y;
		leftX_ = p->x - SCREEN_WIDTH / 2;
		topY_ = p->y - SCREEN_HEIGHT / 2;
		avatar_.hp = p->hp; avatar_.max_hp = p->max_hp;
		avatar_.exp = p->exp; avatar_.level = p->level;
		
		// UI 갱신
		hpBar_.setSize(sf::Vector2f(((float)avatar_.hp / avatar_.max_hp) * 200, 30));
		levelText_.setString("Level : " + to_string(avatar_.level));
		avatar_.show();
		break;
	}
	case SC_ADD_OBJECT: {
		SC_ADD_OBJECT_PACKET* p = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		if (p->id == myId_) {
			avatar_.move(p->x, p->y);
			leftX_ = p->x - SCREEN_WIDTH / 2;
			topY_ = p->y - SCREEN_HEIGHT / 2;
			avatar_.show();
		} else {
			// visual == -1은 맵 아이템: SC_ADD_MAP_ITEM으로 별도 처리되므로 여기서 무시
			if (p->visual == -1) break;

			if (p->visual == 0) players_[p->id] = OBJECT{ *pieceTex_, 192, 0, 64, 64 };
			else if (p->visual == 1) players_[p->id] = OBJECT{ *npcTex_, 128, 0, 64, 64 }; // Agro
			else players_[p->id] = OBJECT{ *npcTex_, 0, 0, 64, 64 }; // Peace
			
			players_[p->id].move(p->x, p->y);
			players_[p->id].set_name(p->name, font_);
			players_[p->id].show();
		}
		break;
	}
	case SC_MOVE_OBJECT: {
		SC_MOVE_OBJECT_PACKET* p = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		if (p->id == myId_) {
			avatar_.move(p->x, p->y);
			leftX_ = p->x - SCREEN_WIDTH / 2;
			topY_ = p->y - SCREEN_HEIGHT / 2;
		} else {
			players_[p->id].move(p->x, p->y);
		}
		break;
	}
	case SC_REMOVE_OBJECT: {
		SC_REMOVE_OBJECT_PACKET* p = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		if (p->id == myId_) avatar_.hide();
		else players_.erase(p->id);
		break;
	}
	case SC_CHAT: {
		SC_CHAT_PACKET* p = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
		if (p->id == myId_) {
			avatar_.set_chat(p->mess, font_);
		}
		else {
			// players_에 있는 경우에만 set_chat 호출
			if (players_.count(p->id)) {
				players_[p->id].set_chat(p->mess, font_);
			}
		}

		// 발신자 이름 안전 조회 (myId_면 avatar_, 없으면 Unknown)
		string senderName;
		if (p->id == myId_) {
			senderName = string(avatar_.name);
		}
		else if (players_.count(p->id)) {
			senderName = string(players_[p->id].name);
		}
		else {
			senderName = "Unknown";
		}

		// 내 채팅은 Enter 키 입력 시 이미 추가했으므로 중복 추가 방지
		if (p->id != myId_) {
			string msg = "[" + senderName + "] : " + p->mess;
			chatHistory_.push_back(msg);
			if (chatHistory_.size() >= 5) chatHistory_.erase(chatHistory_.begin());
		}
		break;
	}
	case SC_STAT_CHANGE: {
		SC_STAT_CHANGE_PACKET* p = reinterpret_cast<SC_STAT_CHANGE_PACKET*>(ptr);
		avatar_.hp = p->hp; avatar_.max_hp = p->max_hp;
		avatar_.exp = p->exp; avatar_.level = p->level;
		hpBar_.setSize(sf::Vector2f(((float)avatar_.hp / avatar_.max_hp) * 200, 30));
		expBar_.setSize(sf::Vector2f(static_cast<float>((double)avatar_.exp / (100 * pow(2, avatar_.level - 1))) * 200, 10)); // warning C4244 해결
		levelText_.setString("Level : " + to_string(avatar_.level));
		break;
	}
	// 공격 처리 및 시각화
	case SC_ATTACK: {
		SC_ATTACK_PACKET* p = reinterpret_cast<SC_ATTACK_PACKET*>(ptr);
		string msg;

		int damage = p->damage;

		// 이름 안전 조회 헬퍼 람다
		auto getName = [&](int id) -> string {
			if (id == myId_) return string(avatar_.name);
			if (players_.count(id)) return string(players_[id].name);
			return "Unknown";
		};

		// 공격 이펙트 생성
		auto now = chrono::system_clock::now();
		if (p->attack_type == 0) { // 평타
			int tx = p->center_x;
			int ty = p->center_y;
			switch (p->direction) {
			case 0: ty--; break; // UP
			case 1: ty++; break; // DOWN
			case 2: tx--; break; // LEFT
			case 3: tx++; break; // RIGHT
			}
			attackEffects_.push_back({ tx, ty, now });
		}
		else if (p->attack_type == 1) { // 범위
			int cx = p->center_x;
			int cy = p->center_y;
			attackEffects_.push_back({ cx, cy - 1, now });
			attackEffects_.push_back({ cx, cy + 1, now });
			attackEffects_.push_back({ cx - 1, cy, now });
			attackEffects_.push_back({ cx + 1, cy, now });
		}

		if (p->exp == 0) { // 공격
			if (p->attacker_id == myId_) {
				msg = "You attack " + getName(p->damaged_id) + " to give " + to_string(damage) + " damage.";
			} else if (p->damaged_id == myId_) {
				msg = getName(p->attacker_id) + " attack you to give " + to_string(damage) + " damage.";
			} else {
				msg = getName(p->attacker_id) + " attack " + getName(p->damaged_id) + " to give " + to_string(damage) + " damage.";
			}

			// HP 갱신 (players_에 있는 경우만)
			if (players_.count(p->damaged_id)) {
				players_[p->damaged_id].hp = p->hp;
				players_[p->damaged_id].hp_bar.setSize(sf::Vector2f(((float)p->hp / p->max_hp) * TILE_WIDTH, 5));
			}
		} else { // 사망/킬
			if (p->attacker_id == myId_) {
				avatar_.exp += p->exp;
				msg = "You killed " + getName(p->damaged_id) + " and get EXP : " + to_string(p->exp);
			} else if (p->damaged_id == myId_) {
				avatar_.exp -= avatar_.exp / 2;
				msg = getName(p->attacker_id) + " killed you and lose EXP - " + to_string(avatar_.exp);
			} else {
				msg = getName(p->attacker_id) + " killed " + getName(p->damaged_id) + " and get EXP - " + to_string(p->exp);
			}
		}
		chatHistory_.push_back(msg);
		if (chatHistory_.size() >= 5) chatHistory_.erase(chatHistory_.begin());
		break;
	}
	case SC_RANKING: {
		SC_RANKING_PACKET* p = reinterpret_cast<SC_RANKING_PACKET*>(ptr);
		rankingData_.clear();
		for (int i = 0; i < p->count; ++i) {
			rankingData_.push_back({ p->ranks[i].name, p->ranks[i].rank, p->ranks[i].level });
		}
		break;
	}
	// 필드 아이템 생성 알림
	case SC_ADD_MAP_ITEM: {
		SC_ADD_MAP_ITEM_PACKET* p = reinterpret_cast<SC_ADD_MAP_ITEM_PACKET*>(ptr);
		MapItemInfo info;
		info.object_id = p->object_id;
		info.template_id = p->template_id;
		info.count = p->count;
		info.x = p->x;
		info.y = p->y;
		// 템플릿 ID에 따른 스프라이트 설정 (일단 체스말로 임시 사용)
		info.sprite.setTexture(*pieceTex_);
		info.sprite.setTextureRect(sf::IntRect(0, 0, 64, 64)); // 임시
		info.sprite.setScale(0.3f, 0.3f); // 좀 작게
		mapItems_[p->object_id] = info;
		break;
	}
	// 필드 아이템 삭제 알림
	case SC_REMOVE_MAP_ITEM: {
		SC_REMOVE_MAP_ITEM_PACKET* p = reinterpret_cast<SC_REMOVE_MAP_ITEM_PACKET*>(ptr);
		mapItems_.erase(p->object_id);
		break;
	}
	// 아이템 획득 (인벤토리 추가)
	case SC_GET_ITEM: {
		SC_GET_ITEM_PACKET* p = reinterpret_cast<SC_GET_ITEM_PACKET*>(ptr);
		g_inventoryUI.AddItem(
			p->item_uid,
			p->template_id,
			p->count,
			p->x,
			p->y,
			p->is_rotated
		);
		break;
	}
	}
}

void GameManager::ProcessData(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) {
			// saved_packet_size > 0 이면 packet_buffer에 이미 첫 바이트가 있으므로
			// packet_buffer + 미수신분을 합쳐서 패킷 크기를 읽어야 한다.
			if (saved_packet_size + io_byte < 2) {
				// 아직 2바이트도 안 됨 → 그냥 누적만
				memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
				saved_packet_size += io_byte;
				io_byte = 0;
				break;
			}
			// packet_buffer에 남은 바이트 + 새 데이터를 합쳐 패킷 크기 계산
			if (saved_packet_size == 0) {
				// 누적 없음 → ptr에서 직접 읽기
				in_packet_size = reinterpret_cast<unsigned short*>(ptr)[0];
			}
			else {
				// 1바이트 누적 → packet_buffer[0] + ptr[0] 조합
				unsigned char lo = static_cast<unsigned char>(packet_buffer[0]);
				unsigned char hi = static_cast<unsigned char>(ptr[0]);
				in_packet_size = static_cast<size_t>(lo) | (static_cast<size_t>(hi) << 8);
			}
		}
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		} else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void GameManager::SendPacket(void* packet)
{
	unsigned short size = reinterpret_cast<unsigned short*>(packet)[0];
	size_t sent;
	socket_.send(packet, size, sent);
}

void GameManager::Draw()
{
	// 타일 그리기
	for (int i = 0; i < SCREEN_WIDTH; ++i) {
		for (int j = 0; j < SCREEN_HEIGHT; ++j) {
			int tileX = i + leftX_;
			int tileY = j + topY_;
			if (tileX < 0 || tileY < 0) continue;
			if ((tileX / 3 + tileY / 3) % 2 == 0) {
				whiteTile_.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				whiteTile_.a_draw(window_);
			} else {
				blackTile_.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				blackTile_.a_draw(window_);
			}
		}
	}

	// 공격 이펙트 그리기
	auto now = chrono::system_clock::now();
	sf::RectangleShape effectRect(sf::Vector2f(TILE_WIDTH, TILE_WIDTH));
	effectRect.setFillColor(sf::Color(255, 0, 0, 128)); // 반투명 빨강

	// 만료된 이펙트 제거 (remove_if)
	attackEffects_.erase(remove_if(attackEffects_.begin(), attackEffects_.end(), 
		[&](const AttackEffect& ef) {
			return now - ef.startTime > chrono::milliseconds(200); // 0.2초 지속
		}), attackEffects_.end());

	for (const auto& ef : attackEffects_) {
		// 화면 좌표로 변환 (타일 좌표 - 카메라 오프셋)
		float ex = (float)((ef.x - leftX_) * TILE_WIDTH);
		float ey = (float)((ef.y - topY_) * TILE_WIDTH);
		effectRect.setPosition(ex, ey);
		window_->draw(effectRect);
	}

	// 맵 아이템 그리기
	for (auto& pair : mapItems_) {
		auto& item = pair.second;
		float ix = (float)((item.x - leftX_) * TILE_WIDTH + 10); // 약간 오프셋
		float iy = (float)((item.y - topY_) * TILE_WIDTH + 10);
		item.sprite.setPosition(ix, iy);
		window_->draw(item.sprite);
	}

	avatar_.draw(window_, leftX_, topY_);
	for (auto& p : players_) p.second.draw(window_, leftX_, topY_);

	// UI
	window_->draw(mapRect_);
	playerDot_.setPosition((float)(MAP_WIDTH + (avatar_.m_x / (2.1 * (W_WIDTH / 400)))), (float)(MAP_HEIGHT + (avatar_.m_y / (2.1 * (W_HEIGHT / 400))))); // [Mod] double->float 형변환 경고 해결
	window_->draw(playerDot_);
	window_->draw(hpBar_);
	window_->draw(expBar_);
	window_->draw(levelText_);

	// Chat
	if (isChatActive_) {
		chatText_.setString(chatInput_);
		window_->draw(chatBox_);
		window_->draw(chatText_);
	}
	float yOffset = 530;
	for (auto it = chatHistory_.rbegin(); it != chatHistory_.rend() && yOffset > 0; ++it) {
		sf::Text t(*it, font_, 20);
		t.setFillColor(sf::Color::White); // 채팅 색상 흰색
		t.setPosition(5, yOffset);
		window_->draw(t);
		yOffset -= 25;
	}
}

void DrawRanking() { g_gameManager.DrawRanking(); }
void GameManager::DrawRanking()
{
	if (!isRankingActive_) return;
	
	sf::RectangleShape bg(sf::Vector2f(400, 400));
	bg.setFillColor(sf::Color(0, 0, 0, 200));
	bg.setPosition(WINDOW_WIDTH / 2 - 200, WINDOW_HEIGHT / 2 - 200);
	window_->draw(bg);

	sf::Text title("== RANKING ==", font_, 30);
	title.setFillColor(sf::Color::Yellow);
	title.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 190);
	window_->draw(title);
	
	int y = WINDOW_HEIGHT / 2 - 140;
	int count = 0;
	for (size_t i = rankingScrollIndex_; i < rankingData_.size(); ++i) {
		if (count++ >= 10) break;
		string line = to_string(rankingData_[i].rank) + ". " + rankingData_[i].name + " (Lv." + to_string(rankingData_[i].level) + ")";
		sf::Text t(line, font_, 20);
		t.setFillColor(sf::Color::White); // 랭킹 텍스트 색상
		t.setPosition(WINDOW_WIDTH / 2 - 180, (float)y);
		window_->draw(t);
		y += 30;
	}
}
