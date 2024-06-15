#include "global.h"
#include "Session.h"

void DisplayDBError(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

SQLHDBC ConnectWithDataBase()
{
	// SQLBindCol_ref.cpp  
	// compile with: odbc32.lib  
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;


	setlocale(LC_ALL, "korean");

	std::cout << "=====Connecting to SQL Server=====\n";
	
	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				while (true)
				{
					retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2024_GameServer", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

					// Allocate statement handle  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
						std::cout << "=====Connect=====\n";
			
						return hdbc;
					}
					std::cout << "Connect Fail\n";
					std::cout << "Retry Connecting...\n";
				}
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);

	}

}

SQLHSTMT AllocateStatement(SQLHDBC hdbc) 
{
	SQLHSTMT hstmt;
	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
		DisplayDBError(hdbc, SQL_HANDLE_DBC, retcode);
		return nullptr;
	}
	return hstmt;
}

void DBWoker(SQLHDBC hdbc)
{
	while (true)
	{
		DBRequest request;
		if (g_db_request_queue.try_pop(request))
		{
			ProcessDBRequest(request, hdbc);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void ProcessDBRequest(const DBRequest& request, SQLHDBC& hdbc) 
{
	// 기존 DBCheckLogin 코드에서 switch 문을 추출하여 여기서 처리
	switch (request.db_type) {
	case DBRequest::LOGIN:
		// 로그인 처리 로직
		objects[request.id]->DBLogin(hdbc);
		break;
	case DBRequest::LOGOUT:
		// 로그아웃 처리 로직
		objects[request.id]->DBLogout(hdbc);
		break;
	}
}