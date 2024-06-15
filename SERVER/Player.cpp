#include "Player.h"

void print_error(const char* msg, int err_no)
{
	WCHAR* msg_buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&msg_buf), 0, NULL);
	std::cout << msg;
	std::wcout << L" : 에러 : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

void Player::DoReceive()
{
	DWORD recv_flag = 0;
	memset(&recv_over_.over_, 0, sizeof(recv_over_.over_));
	recv_over_.wsabuf_.buf = recv_over_.send_buf_ + prev_packet_.size();
	recv_over_.wsabuf_.len = BUF_SIZE - prev_packet_.size();
	WSARecv(socket_, &recv_over_.wsabuf_, 1, 0, &recv_flag, 
		&recv_over_.over_, 0);
	
}

void Player::SendLoginInfoPacket()
{
	SC_LOGIN_INFO_PACKET packet;
	packet.id = id_;
	packet.size = sizeof(SC_LOGIN_INFO_PACKET);
	packet.type = SC_LOGIN_INFO;
	packet.exp = exp_;
	packet.hp = hp_;
	packet.level = level_;
	packet.max_hp = max_hp_;
	packet.x = x_;
	packet.y = y_;
	packet.visual = OT_PLAYER;
	DoSend(&packet);
}

void Player::SendLoginFailPacket()
{
	SC_LOGIN_FAIL_PACKET packet;
	packet.size = sizeof(SC_LOGIN_FAIL_PACKET);
	packet.type = SC_LOGIN_FAIL;
	DoSend(&packet);
}

void Player::DoSend(void* packet)
{
	OVER* sdata = new OVER{ reinterpret_cast<char*>(packet) };
	WSASend(socket_, &sdata->wsabuf_, 1, 0, 0, &sdata->over_, 0);
}

void Player::SendMovePacket(int c_id)
{
	SC_MOVE_OBJECT_PACKET packet;
	packet.size = sizeof(SC_MOVE_OBJECT_PACKET);
	packet.type = SC_MOVE_OBJECT;
	packet.id = c_id;
	packet.x = objects[c_id]->x_;
	packet.y = objects[c_id]->y_;
	packet.move_time = objects[c_id]->last_move_time_;
	DoSend(&packet);
}

void Player::SendAddObjectPacket(int c_id)
{
	// 자신의 뷰리스트 갱신
	mut_view_.lock();
	view_list_.insert(c_id);
	mut_view_.unlock();

	SC_ADD_OBJECT_PACKET packet;
	packet.size = sizeof(SC_ADD_OBJECT_PACKET);
	packet.type = SC_ADD_OBJECT;
	packet.id = c_id;
	packet.x = objects[c_id]->x_;
	packet.y = objects[c_id]->y_;
	packet.visual = objects[c_id]->visual_;
	strcpy_s(packet.name, objects[c_id]->name_);
	DoSend(&packet);
}

void Player::SendRemoveObjectPacket(int c_id)
{
	mut_view_.lock();
	view_list_.erase(c_id);
	mut_view_.unlock();

	SC_REMOVE_OBJECT_PACKET packet;
	packet.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	packet.type = SC_REMOVE_OBJECT;
	packet.id = c_id;
	DoSend(&packet);
}

void Player::SendChatPacket(int c_id, char mess[CHAT_SIZE])
{
	SC_CHAT_PACKET packet;
	packet.size = sizeof(SC_CHAT_PACKET);
	packet.type = SC_CHAT;
	packet.id = c_id;
	strcpy_s(packet.mess, mess);
	DoSend(&packet);
}

void Player::SendStatChangePacket()
{
	SC_STAT_CHANGE_PACKET packet;
	packet.size = sizeof(SC_STAT_CHANGE_PACKET);
	packet.type = SC_STAT_CHANGE;
	packet.hp = hp_;
	packet.max_hp = max_hp_;
	packet.exp = exp_;
	packet.level = level_;
	DoSend(&packet);
}

void Player::DBLogin(SQLHDBC& hdbc)
{
	SQLHSTMT hstmt = AllocateStatement(hdbc);
	SQLRETURN retcode;
	SQLWCHAR dId[NAME_LEN];
	SQLSMALLINT  d_x, d_y, d_max_hp, d_level, d_visual;
	SQLINTEGER d_exp;
	SQLLEN cbId = 0, cb_x = 0, cb_y = 0, cb_max_hp = 0, cb_exp = 0, cb_level = 0, cb_visual = 0;

	std::string user_id(name_);

	std::wstring sql_query = L"SELECT * FROM user_term_table WHERE user_id = \'"
		+ std::wstring(user_id.begin(), user_id.end()) + L"\'";

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)sql_query.c_str(), SQL_NTS);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		// Bind columns
		SQLBindCol(hstmt, 1, SQL_C_WCHAR, dId, NAME_LEN * sizeof(SQLWCHAR), &cbId);
		SQLBindCol(hstmt, 2, SQL_C_SSHORT, &d_x, 0, &cb_x);
		SQLBindCol(hstmt, 3, SQL_C_SSHORT, &d_y, 0, &cb_y);
		SQLBindCol(hstmt, 4, SQL_C_SSHORT, &d_max_hp, 0, &cb_max_hp);
		SQLBindCol(hstmt, 5, SQL_C_SLONG, &d_exp, 0, &cb_exp);
		SQLBindCol(hstmt, 6, SQL_C_SSHORT, &d_level, 0, &cb_level);
		SQLBindCol(hstmt, 7, SQL_C_SSHORT, &d_visual, 0, &cb_visual);


		
		for (auto player : g_player_list)
		{
			if (true == strcmp(name_,objects[player]->name_))
			{
				std::wcerr << L"Login Failed: Already logged in." << std::endl;
				name_[0] = { 0, };
				SendLoginFailPacket();
				return;
			}
		}
		retcode = SQLFetch(hstmt);
		if (retcode == SQL_SUCCESS)
		{
			wprintf(L"Login Success : User ID: %s, Location X: %d, Location Y: %d\n", dId, d_x, d_y);
			// DB에서 받은 정보 초기화
			x_ = d_x;
			y_ = d_y;
			max_hp_ = d_max_hp;
			hp_ = max_hp_;
			exp_ = d_exp;
			level_ = d_level;
			visual_ = d_visual;

			//===============
			// Login 로직 시작
			//===============
			SendLoginInfoPacket();
			// 자신의 위치 섹터에 저장
			PutInSector();
			// 해당 객체 INGAME 상태로 변경
			{
				std::lock_guard<std::mutex> lock(mut_state_);
				state_ = OS_INGAME;
			}

			for (auto& sector : around_sector_)
			{
				{
					// 섹터에 대한 lock
					std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
					for (auto& id : g_ObjectSector[sector].sec_id_)
					{
						{
							std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
							if (OS_INGAME != objects[id]->state_) continue;
						}

						if (false == CanSee(id_, objects[id]->id_))	continue;
						if (objects[id]->id_ == id_)	continue;	// 자기자신일때
						objects[id]->SendAddObjectPacket(id_);
						SendAddObjectPacket(objects[id]->id_);
					}
				}
			}
			std::cout << "Login : [" << name_ << "]" << std::endl;
		}
		else
		{
			std::wcerr << L"Login Failed: No such user ID found." << std::endl;
			SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

			// 새로운 사용자 정보를 삽입하는 쿼리 작성
			hstmt = AllocateStatement(hdbc);
			std::wstring insert_query = L"INSERT INTO user_term_table (user_id, user_x, user_y, user_max_hp, user_exp, user_level, user_visual) VALUES (\'"
				+ std::wstring(user_id.begin(), user_id.end()) + L"\', 0, 0, 100, 0, 1, 0)";

			retcode = SQLExecDirect(hstmt, (SQLWCHAR*)insert_query.c_str(), SQL_NTS);

			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				std::wcout << L"New user inserted successfully: User ID: " << user_id.c_str() << std::endl;
				// 초기화된 사용자 정보 설정
				x_ = 0;
				y_ = 0;
				max_hp_ = 100;
				hp_ = max_hp_;
				exp_ = 0;
				level_ = 1;
				visual_ = 0;

				// 다시 로그인 로직 시작
				SendLoginInfoPacket();
				PutInSector();
				{
					std::lock_guard<std::mutex> lock(mut_state_);
					state_ = OS_INGAME;
				}

				for (auto& sector : around_sector_)
				{
					std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
					for (auto& id : g_ObjectSector[sector].sec_id_)
					{
						std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
						if (OS_INGAME != objects[id]->state_) continue;
						if (false == CanSee(id_, objects[id]->id_)) continue;
						if (objects[id]->id_ == id_) continue; // 자기자신일때
						objects[id]->SendAddObjectPacket(id_);
						SendAddObjectPacket(objects[id]->id_);
					}
				}
				std::cout << "Login : [" << name_ << "]" << std::endl;
			}
			else
			{
				std::wcerr << L"Failed to insert new user: " << user_id.c_str() << std::endl;
				name_[0] = { 0, };
				SendLoginFailPacket();
			}
		}
	}
	else
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
		name_[0] = { 0, };
		SendLoginFailPacket();
	}

	if (hstmt) 
	{
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}
}

void Player::DBLogout(SQLHDBC& hdbc)
{
	SQLHSTMT hstmt = AllocateStatement(hdbc);
	SQLRETURN retcode;

	std::string user_id(name_);

	std::wstring sql_query = L"UPDATE user_term_table SET user_x = "
		+ std::to_wstring(x_)
		+ L", user_y = " + std::to_wstring(y_)
		+ L", user_max_hp = " + std::to_wstring(max_hp_)
		+ L", user_exp = " + std::to_wstring(exp_)
		+ L", user_level = " + std::to_wstring(level_)
		+ L" WHERE user_id = \'" + std::wstring(user_id.begin(), user_id.end()) + L"\'";

	std::wcout << L"Executing SQL Query: " << sql_query << std::endl;

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)sql_query.c_str(), SQL_NTS);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		std::wcout << "[" << name_ << "] : " << L"Logout successful, data saved. x = " << x_ << ", y = " << y_ << std::endl;
		name_[0] = 0;
	}
	else
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, retcode);
	}

	if (hstmt)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}
}

void Player::SendAttackPacket(int attacker_id, int damaged_id)
{
	SC_ATTACK_PACKET packet;
	packet.size = sizeof(SC_ATTACK_PACKET);
	packet.type = SC_ATTACK;
	packet.attacker_id = attacker_id;
	packet.damaged_id = damaged_id;
	packet.max_hp = objects[damaged_id]->max_hp_;
	packet.hp = objects[damaged_id]->hp_;
	packet.damaged_state = (objects[damaged_id]->hp_ <= 0) ? 1 : 0;
	DoSend(&packet);
}

void Player::ProcessPacket(char* packet)
{
	switch (packet[2])
	{
		// 로그인 패킷 처리
		case CS_LOGIN:
		{
			CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
			strcpy_s(name_, p->name);
			std::cout << name_ << " : name" << std::endl;
			g_db_request_queue.push({ DBRequest::LOGIN, id_ });
			// 자신에게 login 전송
			// TODO : DB연결시 성공시에만 전송
			//SendLoginInfoPacket();
			//// 자신의 위치 섹터에 저장
			//PutInSector();
			//// 해당 객체 INGAME 상태로 변경
			//{
			//	std::lock_guard<std::mutex> lock(mut_state_);
			//	state_ = OS_INGAME;
			//}

			//for (auto& sector : around_sector_)
			//{
			//	{
			//		// 섹터에 대한 lock
			//		std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
			//		for (auto& id : g_ObjectSector[sector].sec_id_)
			//		{
			//			{
			//				std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
			//				if (OS_INGAME != objects[id]->state_) continue;
			//			}

			//			if (false == CanSee(id_, objects[id]->id_))	continue;
			//			if (objects[id]->id_ == id_)	continue;	// 자기자신일때
			//			objects[id]->SendAddObjectPacket(id_);
			//			SendAddObjectPacket(objects[id]->id_);
			//		}
			//	}
			//}

			//std::cout << "Login : [" << name_ << "]" << std::endl;
			break;
		}
		// 이동 패킷 처리
		case CS_MOVE:
		{
			CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
			last_move_time_ = p->move_time;
			short x = x_;
			short y = y_;
			switch (p->direction) {
			case 0: if (y > 0) y--; break;
			case 1: if (y < W_HEIGHT - 1) y++; break;
			case 2: if (x > 0) x--; break;
			case 3: if (x < W_WIDTH - 1) x++; break;
			}
			// TODO : 장애물 체크 해야함
			x_ = x;
			y_ = y;

			// Sector 이동
			PutInSector();

			// 기존 시야에 있는 플레이어와 새로운 시야에 있는 플레이어 비교
			mut_view_.lock();
			std::unordered_set<int> prev_viewlist = view_list_;
			mut_view_.unlock();
			std::unordered_set<int> curr_viewlist;

			// 자신의 around_sector에 있는 object가 시야에 보이는지 검사->curr_viewlist에 삽입
			for (auto& sector : around_sector_)
			{
				{
					// 섹터에 대한 lock
					std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
					for (auto& id : g_ObjectSector[sector].sec_id_)
					{
						{
							std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
							if (OS_INGAME != objects[id]->state_) continue;
						}

						if (false == CanSee(id_, objects[id]->id_))	continue;
						if (objects[id]->id_ == id_)	continue;	// 자기자신일때
						curr_viewlist.insert(objects[id]->id_);

					}
				}
			}
			// 자신에게 이동 전송
			SendMovePacket(id_);
			// 검사한 시야를 이용해 각 오브젝트들 이동
			for (int ano_id : curr_viewlist)
			{
				if (0 == prev_viewlist.count(ano_id))
				{
					SendAddObjectPacket(ano_id);
					objects[ano_id]->SendAddObjectPacket(id_);
				}
				else
				{
					objects[ano_id]->SendMovePacket(id_);
				}
			}
			for (int ano_id : prev_viewlist)
			{
				if (0 == curr_viewlist.count(ano_id))
				{
					SendRemoveObjectPacket(ano_id);
					objects[ano_id]->SendRemoveObjectPacket(id_);
				}
			}

			break;
		}
		// 채팅 패킷 처리
		case CS_CHAT:
		{
			CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);
			std::cout << "Chat : [" << name_ << "] : " << p->mess << std::endl;
			
			for (auto& player : g_player_list)
			{
				if (player == id_) continue;
				objects[player]->SendChatPacket(id_, p->mess);
			}
			break;
		}
		// 공격처리
		case CS_ATTACK:
		{
			CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
			std::vector<std::pair<short, short>> attack_coord;
			short attack_x = x_;
			short attack_y = y_;
			// 기본 방향 공격 데미지 10, 범위공격 데미지 5
			int damage = 10;

			switch (p->attack_direction) {
			case 0:		// UP
				if (attack_y > 0)
				{
					attack_y--;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 1:		// DOWN
				if (attack_y < W_HEIGHT - 1)
				{
					attack_y++;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 2:		// LEFT
				if (attack_x > 0)
				{
					attack_x--;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 3:		// RIGHT
				if (attack_x < W_WIDTH - 1)
				{
					attack_x++;
					attack_coord.emplace_back(attack_x, attack_y);
				}
				break;
			case 4:		// 4방향 공격
			{
				const std::array<std::pair<short, short>, 4> directions = 
				{
				std::make_pair(0, -1),  // UP
				std::make_pair(0, 1),   // DOWN
				std::make_pair(-1, 0),  // LEFT
				std::make_pair(1, 0)    // RIGHT
				};
				damage = 5;
				for (const auto& direction : directions)
				{
					short x = attack_x + direction.first;
					short y = attack_y + direction.second;

					if (x >= 0 && x < W_WIDTH && y >= 0 && y < W_HEIGHT) 
					{
						attack_coord.emplace_back(x, y);
					}
				}
				break;
			}
			}
			
			// 공격 판정
			for(const auto& coord : attack_coord)
			{
				// 공격 위치에 대한 섹터를 돌면서 검사함
				std::pair<int, int> sector_key = { coord.first / SEC_ROW, coord.second / SEC_COL };
				auto& sector = g_ObjectSector[sector_key];
				std::lock_guard<std::mutex> lock(sector.mut_sector_);
				for (auto& id : sector.sec_id_)
				{
					if (id == id_) continue;
					if (true == IsPlayer(id)) continue;
					{
						std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
						if (OS_INGAME != objects[id]->state_) continue;
					}
					if (objects[id]->x_ == coord.first && objects[id]->y_ == coord.second)
					{
						// 공격 판정
						objects[id]->hp_ -= damage;
						objects[id]->SendStatChangePacket();

						// TODO: 경험치 판정이 들어가야함
						// 공격 판정 맞는 사람 입장 view_list 브로드 캐스팅
						for (auto& view_list : objects[id]->view_list_)
						{
							objects[view_list]->SendAttackPacket(id_, objects[id]->id_);
						}

						if (objects[id]->hp_ <= 0)
						{
							// 죽음 처리
							objects[id]->hp_ = 0;
							objects[id]->state_ = OS_DEAD;
							// Npc의 뷰리스트에 있는 Player한테만 보냄
							for (auto& view_list : objects[id]->view_list_)
							{
								objects[view_list]->SendRemoveObjectPacket(id);
							}
						}
						
							
					}
				}
			}
			break;
		}
	}
}

