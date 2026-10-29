#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t Human_Inventory_GetGoodInSlot(const void* human, std::uint32_t slot);
	std::uint32_t Human_GetCarriedGood(const void* human);
	std::int32_t Human_GetPercentageConditionOfTool(const void* human);
	bool Human_IsAbleToEquipGoodNow(const void* human, std::uint32_t good, bool anyPlayer);
	bool Remaster_Human_TryEquipAvailableWeapon(void* human);
	void Human_ClearCarriedGood(void* human);
	std::uint32_t Tool_GoodEquipmentType_Convert(std::uint32_t good);
	bool Human_l_Item_Set(void* human, std::uint32_t good);
	void Human_l_Item_Clear(void* human, std::int32_t slot);
	bool Human_CanEquipGoodType(const void* human, std::uint32_t good);
	std::int32_t Human_l_Item_GetItemSlotIndex(const void* human, std::uint32_t good);
	std::int32_t Human_l_Item_GetFreeSlotIndex(const void* human);
	std::uint32_t Human_GetEquipmentType_Weapon(const void* human);
	std::uint32_t Human_GetEquipmentType_Shoes(const void* human);
	std::uint32_t Human_GetEquipmentType_Tool(const void* human);
	std::uint32_t Human_GetEquipmentType_Armor(const void* human);
	const char* Human_GetEquipmentType_Shoes_GetGuiNamePtr(const void* human);
	const char* Human_GetEquipmentType_Tool_GetGuiNamePtr(const void* human);
	std::uint32_t Human_Inventory_GetPercentageConditionOfItemInSlot(const void* human, std::int32_t slot);
	void Human_Equipment_SetToolType(void* human, std::uint32_t good);
	void Human_Equipment_SetArmorType(void* human, std::uint32_t armor);
	void Human_Equipment_SetWeaponType(void* human, std::uint32_t weapon);
	void Human_Equipment_UnEquipItem(void* human, std::uint32_t good);
	void Human_l_StartTask_Equip(void* human);
	void Human_l_StartTask_Equip_Shoes(void* human);
	void Human_l_StartTask_Equip_Tool(void* human);
	void Human_l_StartTask_Equip_Item(void* human);
	void Human_l_StartTask_Equip_Armor(void* human, std::uint32_t armor);
	void Human_l_StartTask_Equip_Weapon(void* human, std::uint32_t weapon);
	void Human_l_StartTask_Equip_FetchWantedItem(void* human);
	void Human_l_StartTask_Equip_DropItem(void* human, std::uint32_t good);
	void Human_l_FinishTask_Equip(void* human);
	void Human_l_FailTask_Equip(void* human);
	void Human_l_FailTask_Drop(void* human);
	std::uint32_t Human_DoExecuteUserCommand_Equipment_Change(void* human, std::uint32_t good, std::int32_t slot);
	std::uint32_t Human_DoExecuteUserCommand_Equipment_Drop(void* human, std::uint32_t good, std::int32_t slot);

	bool Human_HasEquippedWeapon(const void* human);
	bool Human_HasEquippedArmor(const void* human);
	const char* Human_GetReallyProducedGoodType_GetGuiNamePtr(const void* human);
	std::uint32_t Human_Equipment_GetArmorType(const void* human);
	void Human_Equipment_ResetShoeType(void* human);
	void Human_Equipment_ResetToolType(void* human);
	void Human_Equipment_ResetArmorType(void* human);
	bool Human_CanEquipArmorType(const void* human, std::uint32_t good);
}