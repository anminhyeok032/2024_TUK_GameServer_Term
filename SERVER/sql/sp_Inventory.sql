-- ============================================
-- user_inventory_table
-- ============================================
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'user_inventory_table') AND type = 'U')
BEGIN
    CREATE TABLE user_inventory_table (
        user_id       NCHAR(20)  NOT NULL,
        item_uid      BIGINT     NOT NULL,  -- Snowflake ID (global.h ItemUIDGenerator)
        template_id   INT        NOT NULL,
        pos_x         SMALLINT   NOT NULL DEFAULT 0,
        pos_y         SMALLINT   NOT NULL DEFAULT 0,
        is_rotated    BIT        NOT NULL DEFAULT 0,
        created_at    DATETIME2  NOT NULL DEFAULT SYSDATETIME(),
 
        CONSTRAINT PK_Inventory
            PRIMARY KEY (user_id, item_uid),
 
        -- 유저 삭제 시 인벤토리 자동 삭제
        CONSTRAINT FK_Inventory_User
            FOREIGN KEY (user_id)
            REFERENCES user_term_table(user_id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
    );
END
GO
 
 
-- ============================================
-- InventoryItemType (TVP)
-- ============================================
-- sp_SaveInventoryBulk 전용 - 인벤토리 전체를 한 번에 전달
CREATE TYPE InventoryItemType AS TABLE (
    item_uid    BIGINT   NOT NULL,
    template_id INT      NOT NULL,
    pos_x       SMALLINT NOT NULL,
    pos_y       SMALLINT NOT NULL,
    is_rotated  BIT      NOT NULL
);
GO
 
 
-- ============================================
-- sp_LoadInventory
-- ============================================
-- 호출: DBLogin() - Redis 캐시 없을 때 SQL에서 복구
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'sp_LoadInventory') AND type = 'P')
    DROP PROCEDURE sp_LoadInventory;
GO
 
CREATE PROCEDURE sp_LoadInventory
    @user_id NCHAR(20)
AS
BEGIN
    SET NOCOUNT ON;
 
    SELECT item_uid, template_id, pos_x, pos_y, is_rotated
    FROM user_inventory_table
    WHERE user_id = @user_id
    ORDER BY pos_y ASC, pos_x ASC;  -- 좌상단부터 순서대로 반환
END
GO
 
 
-- ============================================
-- sp_SaveInventoryBulk
-- ============================================
-- 호출: DBLogout(), CS_ITEM_SORT
-- DELETE + INSERT로 현재 인벤 상태를 전체 교체
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'sp_SaveInventoryBulk') AND type = 'P')
    DROP PROCEDURE sp_SaveInventoryBulk;
GO
 
CREATE PROCEDURE sp_SaveInventoryBulk
    @user_id NCHAR(20),
    @items   InventoryItemType READONLY
AS
BEGIN
    SET NOCOUNT ON;
 
    BEGIN TRANSACTION;
    BEGIN TRY
        DELETE FROM user_inventory_table WHERE user_id = @user_id;
 
        INSERT INTO user_inventory_table (user_id, item_uid, template_id, pos_x, pos_y, is_rotated)
        SELECT @user_id, item_uid, template_id, pos_x, pos_y, is_rotated
        FROM @items;
 
        COMMIT TRANSACTION;
    END TRY
    BEGIN CATCH
        ROLLBACK TRANSACTION;
        THROW;
    END CATCH
END
GO
 
 
-- ============================================
-- sp_SaveInventory
-- ============================================
-- 호출: CS_ITEM_PICKUP (INSERT), CS_ITEM_MOVE (UPDATE pos)
-- MERGE: row exists -> UPDATE pos, row not exists -> INSERT
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'sp_SaveInventory') AND type = 'P')
    DROP PROCEDURE sp_SaveInventory;
GO
 
CREATE PROCEDURE sp_SaveInventory
    @user_id     NCHAR(20),
    @item_uid    BIGINT,
    @template_id INT,
    @pos_x       SMALLINT,
    @pos_y       SMALLINT,
    @is_rotated  BIT
AS
BEGIN
    SET NOCOUNT ON;
 
    MERGE user_inventory_table AS target
    USING (SELECT @user_id AS user_id, @item_uid AS item_uid) AS source
    ON target.user_id = source.user_id AND target.item_uid = source.item_uid
    WHEN MATCHED THEN
        UPDATE SET pos_x = @pos_x, pos_y = @pos_y, is_rotated = @is_rotated
    WHEN NOT MATCHED THEN
        INSERT (user_id, item_uid, template_id, pos_x, pos_y, is_rotated)
        VALUES (@user_id, @item_uid, @template_id, @pos_x, @pos_y, @is_rotated);
END
GO
 
 
-- ============================================
-- sp_DeleteItem
-- ============================================
-- 호출: CS_ITEM_DROP
-- deleted_count = 0 이면 이미 없는 아이템 (이중 드랍 감지용)
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'sp_DeleteItem') AND type = 'P')
    DROP PROCEDURE sp_DeleteItem;
GO
 
CREATE PROCEDURE sp_DeleteItem
    @user_id  NCHAR(20),
    @item_uid BIGINT
AS
BEGIN
    SET NOCOUNT ON;
 
    DELETE FROM user_inventory_table
    WHERE user_id = @user_id AND item_uid = @item_uid;
 
    SELECT @@ROWCOUNT AS deleted_count;
END
GO
 
 
-- ============================================
-- sp_DeleteInventory
-- ============================================
-- 호출: 계정 삭제 / 인벤 초기화
-- FK CASCADE로 user_term_table 삭제 시 자동 실행됨
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'sp_DeleteInventory') AND type = 'P')
    DROP PROCEDURE sp_DeleteInventory;
GO
 
CREATE PROCEDURE sp_DeleteInventory
    @user_id NCHAR(20)
AS
BEGIN
    SET NOCOUNT ON;
 
    DELETE FROM user_inventory_table WHERE user_id = @user_id;
 
    SELECT @@ROWCOUNT AS deleted_count;
END
GO