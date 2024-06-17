#include "Player.h"

void print_error(const char* msg, int err_no)
{
	WCHAR* msg_buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&msg_buf), 0, NULL);
	std::cout << msg;
	std::wcout << L" : ���� : " << msg_buf;
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

void Player::SendAttackPacket(int attacker_id, int damaged_id)
{
	SC_ATTACK_PACKET packet;
	packet.size = sizeof(SC_ATTACK_PACKET);
	packet.type = SC_ATTACK;
	packet.attacker_id = attacker_id;
	packet.damaged_id = damaged_id;
	packet.hp = objects[damaged_id]->hp_;
	packet.damaged_state = (objects[damaged_id]->hp_ <= 0) ? 1 : 0;
	DoSend(&packet);
}

void Player::ProcessPacket(char* packet)
{
	switch (packet[2])
	{
		// �α��� ��Ŷ ó��
		case CS_LOGIN:
		{
			CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
			strcpy_s(name_, p->name);

			// �ڽſ��� login ����
			// TODO : DB����� �����ÿ��� ����
			SendLoginInfoPacket();
			// �ڽ��� ��ġ ���Ϳ� ����
			PutInSector();
			// �ش� ��ü INGAME ���·� ����
			{
				std::lock_guard<std::mutex> lock(mut_state_);
				state_ = OS_INGAME;
			}

			for (auto& sector : around_sector_)
			{
				{
					// ���Ϳ� ���� lock
					std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
					for (auto& id : g_ObjectSector[sector].sec_id_)
					{
						{
							std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
							if (OS_INGAME != objects[id]->state_) continue;
						}

						if (false == CanSee(id_, objects[id]->id_))	continue;
						if (objects[id]->id_ == id_)	continue;	// �ڱ��ڽ��϶�
						objects[id]->SendAddObjectPacket(id_);
						SendAddObjectPacket(objects[id]->id_);
					}
				}
			}

			std::cout << "Login : [" << name_ << "]" << std::endl;
			break;
		}
		// �̵� ��Ŷ ó��
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
			// TODO : ��ֹ� üũ �ؾ���
			x_ = x;
			y_ = y;

			// Sector �̵�
			PutInSector();

			// ���� �þ߿� �ִ� �÷��̾�� ���ο� �þ߿� �ִ� �÷��̾� ��
			mut_view_.lock();
			std::unordered_set<int> prev_viewlist = view_list_;
			mut_view_.unlock();
			std::unordered_set<int> curr_viewlist;

			// �ڽ��� around_sector�� �ִ� object�� �þ߿� ���̴��� �˻�->curr_viewlist�� ����
			for (auto& sector : around_sector_)
			{
				{
					// ���Ϳ� ���� lock
					std::lock_guard<std::mutex> sec_l(g_ObjectSector[sector].mut_sector_);
					for (auto& id : g_ObjectSector[sector].sec_id_)
					{
						{
							std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
							if (OS_INGAME != objects[id]->state_) continue;
						}

						if (false == CanSee(id_, objects[id]->id_))	continue;
						if (objects[id]->id_ == id_)	continue;	// �ڱ��ڽ��϶�
						curr_viewlist.insert(objects[id]->id_);

					}
				}
			}
			// �ڽſ��� �̵� ����
			SendMovePacket(id_);
			// �˻��� �þ߸� �̿��� �� ������Ʈ�� �̵�
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
		// ä�� ��Ŷ ó��
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
		// ����ó��
		case CS_ATTACK:
		{
			CS_ATTACK_PACKET* p = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
			std::vector<std::pair<short, short>> attack_coord;
			short attack_x = x_;
			short attack_y = y_;
			// �⺻ ���� ���� ������ 10, �������� ������ 5
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
			case 4:		// 4���� ����
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
			
			// ���� ����
			for(const auto& coord : attack_coord)
			{
				// ���� ��ġ�� ���� ���͸� ���鼭 �˻���
				std::pair<int, int> sector_key = { coord.first / SEC_ROW, coord.second / SEC_COL };
				auto& sector = g_ObjectSector[sector_key];
				std::lock_guard<std::mutex> lock(sector.mut_sector_);
				for (auto& id : sector.sec_id_)
				{
					if (id == id_) continue;
					{
						std::lock_guard<std::mutex> ll(objects[id]->mut_state_);
						if (OS_INGAME != objects[id]->state_) continue;
					}
					if (objects[id]->x_ == coord.first && objects[id]->y_ == coord.second)
					{
						// ���� ����
						objects[id]->hp_ -= damage;
						objects[id]->SendStatChangePacket();

						if (objects[id]->hp_ <= 0)
						{
							// ���� ó��
							objects[id]->hp_ = 0;
							objects[id]->state_ = OS_DEAD;
							objects[id]->SendRemoveObjectPacket(id);
							SendRemoveObjectPacket(id);
						}
						// TODO: ����ġ ������ ������
						// ���� ���� ��ε� ĳ����
						for (auto& player : g_player_list)
						{
							objects[player]->SendAttackPacket(id_, objects[id]->id_);
						}
							
					}
				}
			}

			break;
		}
	}
}

