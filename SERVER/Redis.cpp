#include "global.h"
#include "Session.h"

std::unique_ptr<cpp_redis::client> g_redis_client;



std::string WStringToString(const std::wstring& wstr) 
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}
std::string WStringToString(const SQLWCHAR* wstr)
{
	if (!wstr) return "";

	// 1. 필요한 버퍼 크기 계산 (널 문자 제외)
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr, -1, NULL, 0, NULL, NULL);

	// 2. 변환 (size_needed에는 널 문자가 포함되어 있음)
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr, -1, &strTo[0], size_needed, NULL, NULL);

	// ★ [핵심] C++ string은 널 문자를 실제 데이터로 가질 수 있으므로, 
	// 변환 후 맨 뒤에 \0이 들어갔다면 제거해야 함.
	if (!strTo.empty() && strTo.back() == '\0') {
		strTo.pop_back();
	}

	return strTo;
}

bool ConnectWithRedis()
{
	std::cout << "=====Connecting to Redis=====\n";

	try {
		// 기본 포트 6379, localhost
		g_redis_client->connect("127.0.0.1", 6379, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) 
			{
				if (status == cpp_redis::client::connect_state::dropped) {
					std::cout << "Redis client disconnected\n";
				}
			});

		std::cout << "=====Redis Connected=====\n";
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Redis Connect Fail: " << e.what() << "\n";
		return false;
	}
}