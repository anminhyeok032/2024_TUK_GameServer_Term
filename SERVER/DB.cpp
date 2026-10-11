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

// 연결 실패 시 기다리면 나아질 수 있는 경우의 시도 횟수 (로그인 시간 제한 5초 x 3회)
constexpr int DB_CONNECT_TRY = 3;

// 진단 레코드의 SQLSTATE (IM002 = DSN 없음, 08001 = 서버에 닿지 않음, HYT00 = 시간 초과)
static std::wstring GetSqlState(SQLSMALLINT hType, SQLHANDLE hHandle)
{
	SQLWCHAR state[SQL_SQLSTATE_SIZE + 1] = {};
	SQLINTEGER native = 0;
	SQLSMALLINT len = 0;
	SQLRETURN ret = SQLGetDiagRec(hType, hHandle, 1, state, &native, NULL, 0, &len);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return L"";
	return state;
}

// 서버 코드가 쓰는 테이블/프로시저가 모두 있는지 확인 -> 배포 실행용 오프라인 모드
// TODO : 서버가 새 테이블/프로시저를 쓰게 되면 이 목록에도 추가해야 함
static bool HasRequiredSchema(SQLHDBC hdbc)
{
	SQLHSTMT hstmt = AllocateStatement(hdbc);
	if (!hstmt) return false;

	SQLWCHAR query[] =
		L"SELECT v.n FROM (VALUES "
		L"(N'dbo.user_term_table'), (N'dbo.user_inventory_table'), "
		L"(N'dbo.sp_UserLogin'), (N'dbo.sp_UserLogout'), (N'dbo.sp_LoadInventory'), "
		L"(N'dbo.sp_SaveInventory'), (N'dbo.sp_DeleteItem')"
		L") AS v(n) WHERE OBJECT_ID(v.n) IS NULL";	// 없는 객체의 이름만 돌려준다

	SQLRETURN ret = SQLExecDirect(hstmt, query, SQL_NTS);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		DisplayDBError(hstmt, SQL_HANDLE_STMT, ret);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		return false;
	}

	bool ok = true;
	SQLWCHAR missing[128];
	SQLLEN cb = 0;
	SQLBindCol(hstmt, 1, SQL_C_WCHAR, missing, sizeof(missing), &cb);
	while (SQLFetch(hstmt) == SQL_SUCCESS)
	{
		ok = false;
		std::wcout << L"[DB] 없음: " << missing << std::endl;
	}
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	return ok;
}

// SQL에 연결 - SQL을 쓸 수 없으면 SQL_NULL_HDBC를 돌려주고, 서버는 오프라인 모드로 실행
//  - force_offline (--offline)  : 연결하지 않음
//  - DSN 없음 등                 : 재시도 없이 오프라인
//  - 서버 응답 없음 / 시간 초과   : DB_CONNECT_TRY회 시도 후 오프라인
//  - 연결됐지만 테이블/프로시저 없음 : 오프라인
SQLHDBC ConnectWithDataBase(bool force_offline)
{
	setlocale(LC_ALL, "korean");

	if (force_offline)
	{
		std::cout << "[DB] --offline: SQL에 연결하지 않습니다.\n";
		return SQL_NULL_HDBC;
	}

	std::cout << "=====Connecting to SQL Server=====\n";

	SQLHENV henv = SQL_NULL_HENV;
	SQLHDBC hdbc = SQL_NULL_HDBC;
	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
		std::cout << "[DB] ODBC 초기화 실패\n";
		if (henv != SQL_NULL_HENV) SQLFreeHandle(SQL_HANDLE_ENV, henv);
		return SQL_NULL_HDBC;
	}

	// Set login timeout to 5 seconds
	SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

	for (int attempt = 1; attempt <= DB_CONNECT_TRY; ++attempt)
	{
		retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2024_GameServer", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			if (HasRequiredSchema(hdbc))
			{
				std::cout << "=====Connect=====\n";
				return hdbc;
			}
			std::cout << "[DB] 서버가 쓰는 테이블/프로시저가 DB에 없습니다.\n";
			SQLDisconnect(hdbc);
			break;
		}

		std::wstring state = GetSqlState(SQL_HANDLE_DBC, hdbc);
		DisplayDBError(hdbc, SQL_HANDLE_DBC, retcode);

		bool retryable = (state.compare(0, 2, L"08") == 0) || (state == L"HYT00");
		if (!retryable || attempt == DB_CONNECT_TRY) break;
		std::cout << "[DB] 연결 재시도 (" << attempt << "/" << DB_CONNECT_TRY << ")\n";
	}

	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, henv);
	return SQL_NULL_HDBC;
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
	
	case DBRequest::SAVE_REDIS: // Redis 저장 처리
		// dynamic_cast 체크나 유효성 검사 필요
		if (objects[request.id]->state_ == OS_INGAME)
			objects[request.id]->SaveToRedis();
		break;
	case DBRequest::SAVE_ITEM:
		objects[request.id]->DBSaveItem(hdbc, request.item_uid);
		break;
	case DBRequest::DELETE_ITEM:
		objects[request.id]->DBDeleteItem(hdbc, request.item_uid);
		break;
	}
}