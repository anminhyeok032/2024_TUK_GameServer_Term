#include "GameManager.h"
#include <iostream>


int main()
{
	// 콘솔 한글 설정
	std::wcout.imbue(std::locale("korean"));

	// 게임 실행
	g_gameManager.Run();

	return 0;
}
