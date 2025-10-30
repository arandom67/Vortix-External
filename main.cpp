#include <Windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <random>

#include "resource.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include "inc/dependencies/json.hpp"

#include "inc/snowflake/Snowflake.hpp"

#include "inc/Renderer.hpp"
#include "inc/rbx.hpp"
#include "inc/utils.hpp"

#include "inc/offsets.hpp"

using json = nlohmann::json;

#define SNOW_LIMIT 200

std::vector<RBX::Instance> playersList;
std::vector<std::string> aimbotLockPartsR6{ "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg" };
std::vector<std::string> aimbotLockPartsR15{ "Head", "UpperTorso", "LeftUpperArm", "RightUpperArm", "LeftUpperLeg", "RightUpperLeg" };
std::vector<std::string> tracerTypes{ "Mouse", "Corner", "Top", "Bottom" };
std::vector<std::string> espTypes{ "Square", "Skeleton", "Corners" };

std::vector<Snowflake::Snowflake> snow;
ImColor featureBGColor{ 17, 17, 17, 204 };
ImColor glowColor{ 1.0f, 0.0f, 0.0f, 0.8f };

namespace Settings
{
	bool imguiVisible{ true };
	bool mainMenuVisible{ true };
	bool explorerWinVisible{ false };
	bool keybindListVisible{ false };
	int toggleGuiKey{ 45 };
	std::string currentTab{ "Aiming" };

	bool aimbotEnabled{ false };
	bool aimbotFOVEnabled{ false };
	bool aimbotPredictionEnabled{ false };
	float aimbotFOVRadius{ 100.0f };
	float aimbotStrenght{ 0.2f };
	float aimbotPredictionX{ 10.0f };
	float aimbotPredictionY{ 10.0f };
	std::string aimbotLockPart{ "Torso" };
	int aimbotKey{ 2 };
	ImVec4 aimbotFovColor{ 1.0f, 0.0f, 0.0f, 1.0f };

	bool silentAimEnabled{ false };
	float silentAimFOVRadius{ 500.0f };
	std::string silentAimLockPart{ "Torso" };

	bool triggerbotEnabled{ false };
	bool triggerbotIndicateClicking{ false };
	float triggerbotDetectionRadius{ 20.0f };
	std::string triggerbotTriggerPart{ "Torso" };
	int triggerbotKey{ 0 };

	bool espEnabled{ false };
	bool espFilled{ false };
	bool espShowDistance{ false };
	bool espShowName{ false };
	bool espShowHealth{ false };
	bool espIgnoreDeadPlrs{ true };
	int espDistance{ 0 };
	std::string espType{ "Square" };
	ImVec4 espColor{ 1.0f, 0.0f, 0.0f, 1.0f };
	bool tracersEnabled{ false };
	std::string tracerType{ "Mouse" };
	ImVec4 tracerColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	char configFileName[30]{};
	char themeFileName[30]{};
	bool streamproofEnabled{ false };
	bool rbxWindowNeedsToBeSelected{ true };
	int mainLoopDelay{ 1 };

	bool noclipEnabled{ false };
	bool flyEnabled{ false };
	int flyKey{ 0 };
	bool flyKeyToggled{ false };
	bool orbitEnabled{ false };
	int walkSpeedSet{};
	int jumpPowerSet{};
	char othersRobloxPlr[30]{};
	RBX::Vector3 othersTeleportPos{};

	int gamblingSlotsNumber1{ 0 };
	int gamblingSlotsNumber2{ 0 };
	int gamblingSlotsNumber3{ 0 };

	bool espPreviewOpened{ false };
}

int main()
{
	SetConsoleTitleA("Vortix");

	std::cout << "Fetching offsets...\n";

	if (!Offsets::fetchOffsets())
	{
		std::cout << "Failed to fetch offsets!\n";

		system("pause");
		return 1;
	}
	else
	{
		std::cout << "Fetched offsets.\n";
	}

	std::cout << "Press ENTER to attach.";
	std::cin.get();

	system("cls");

	if (RBX::Memory::attach())
	{
		std::cout << "Attached!\n\n";

		Sleep(1000);

		system("cls");
	}
	else
	{
		std::cout << "Failed to attach.\n";

		system("pause");

		return 1;
	}

	void* dataModelAddr;
	dataModelAddr = RBX::getDataModel();

	if (RBX::Memory::read<int>((void*)((uintptr_t)dataModelAddr + Offsets::GameId)) == 0)
	{
		std::cout << "You need to join a game.\n";

		while (RBX::Memory::read<int>((void*)((uintptr_t)dataModelAddr + Offsets::GameId)) == 0)
		{
			dataModelAddr = RBX::getDataModel();
			Sleep(1000);
		}

		system("cls");
	}

	FreeConsole();

	json settingsJ;

	if (!std::filesystem::exists("settings.json"))
	{
		std::ofstream oF("settings.json");

		json s;
		s["theme"] = "default";

		oF << s.dump(4);

		oF.close();
	}

	if (!std::filesystem::exists("themes/"))
	{
		std::filesystem::create_directory("themes");
	}

	std::ifstream settingsIF("settings.json");
	std::string settingsContents((std::istreambuf_iterator<char>(settingsIF)), std::istreambuf_iterator<char>());

	settingsJ = json::parse(settingsContents);

	int monitorWidth{ GetSystemMetrics(SM_CXSCREEN) };
	int monitorHeight{ GetSystemMetrics(SM_CYSCREEN) };

	// orbit vars
	int radius{ 8 };
	int eclipse{ 1 };

	double rotspeed{ 3.14159265358979323846 * 2 / 1 };
	eclipse = eclipse * radius;

	double rot{ 0 };
	// -------------

	Renderer renderer;
	renderer.Init();

	Snowflake::CreateSnowFlakes(snow, SNOW_LIMIT, 5.0f, 15.0f, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), Snowflake::vec3(0.0f, 0.005f), IM_COL32(255, 255, 255, 100));

	ID3D11ShaderResourceView* avatarImg{ nullptr };
	int avatarImgW, avatarImgH;

	avatarImg = renderer.LoadTextureFromResource(IDR_ESP_PREVIEW_IMG, avatarImgW, avatarImgH);

	ID3D11ShaderResourceView* logoImg{ nullptr };
	int logoImgW, logoImgH;

	logoImg = renderer.LoadTextureFromResource(IDR_VORTIX_LOGO, logoImgW, logoImgH);

	ID3D11ShaderResourceView* menuIconImg{ nullptr };
	int menuIconImgW, menuIconImgH;

	menuIconImg = renderer.LoadTextureFromResource(IDR_MENU_ICON, menuIconImgW, menuIconImgH);

	ID3D11ShaderResourceView* explorerIconImg{ nullptr };
	int explorerIconImgW, explorerIconImgH;

	explorerIconImg = renderer.LoadTextureFromResource(IDR_EXPLORER_ICON, explorerIconImgW, explorerIconImgH);

	ID3D11ShaderResourceView* aimingIconImg{ nullptr };
	int aimingIconImgW, aimingIconImgH;

	aimingIconImg = renderer.LoadTextureFromResource(IDR_BULLSEYE_ICON, aimingIconImgW, aimingIconImgH);

	ID3D11ShaderResourceView* visualsIconImg{ nullptr };
	int visualsIconImgW, visualsIconImgH;

	visualsIconImg = renderer.LoadTextureFromResource(IDR_ESP_PERSON_ICON, visualsIconImgW, visualsIconImgH);

	ID3D11ShaderResourceView* settingsIconImg{ nullptr };
	int settingsIconImgW, settingsIconImgH;

	settingsIconImg = renderer.LoadTextureFromResource(IDR_GEAR_ICON, settingsIconImgW, settingsIconImgH);

	ID3D11ShaderResourceView* miscIconImg{ nullptr };
	int miscIconImgW, miscIconImgH;

	miscIconImg = renderer.LoadTextureFromResource(IDR_DICE_ICON, miscIconImgW, miscIconImgH);

	ID3D11ShaderResourceView* keybindListIconImg{ nullptr };
	int keybindListIconImgW, keybindListIconImgH;

	keybindListIconImg = renderer.LoadTextureFromResource(IDR_PAINT_BOARD_ICON, keybindListIconImgW, keybindListIconImgH);

	ID3D11ShaderResourceView* gamblingIconImg{ nullptr };
	int gamblingIconImgW, gamblingIconImgH;

	gamblingIconImg = renderer.LoadTextureFromResource(IDR_CARDS_ICON, gamblingIconImgW, gamblingIconImgH);

	RBX::VisualEngine visualEngine{ RBX::Memory::read<void*>((void*)((uintptr_t)RBX::Memory::getRobloxBaseAddr() + Offsets::VisualEnginePointer)) };

	RBX::Instance dataModel{ dataModelAddr };
	RBX::Instance workspace{ dataModel.findFirstChild("Workspace") };
	RBX::Instance players{ dataModel.findFirstChild("Players") };

	RBX::Instance localPlayer{ RBX::Memory::read<void*>((void*)((uintptr_t)players.address + Offsets::LocalPlayer)) };
	RBX::Instance localPlayerModelInstance{ localPlayer.getModelInstance() };
	RBX::Instance humanoid{ localPlayerModelInstance.findFirstChild("Humanoid") };
	RBX::Instance hrp{ localPlayerModelInstance.findFirstChild("HumanoidRootPart") };

	RBX::Instance camera{ RBX::Memory::read<void*>((void*)((uintptr_t)workspace.address + Offsets::Camera)) };

	RBX::Instance lockedPlr{ nullptr };
	bool locked{ false };
	bool keybindPrevDown{ false };

	if (settingsJ["theme"] != "default")
	{
		loadTheme(featureBGColor, glowColor, settingsJ["theme"].get<std::string>());
	}
	else
	{
		ImGuiStyle& style{ ImGui::GetStyle() };

		style.Colors[ImGuiCol_Tab] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.5f, 0.0f, 0.0f, 0.2f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.2f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0627451f, 0.05882353f, 0.0627451f, 0.9f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.05f, 0.05f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.35f, 0.10f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.30f, 0.05f, 0.05f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.30f, 0.05f, 0.05f, 1.00f);

		//style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.25f, 0.25f, 1.00f);

		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.85f, 0.20f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.25f, 0.25f, 1.00f);

		style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.05f, 0.05f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.10f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f);

		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.05f, 0.05f, 0.94f);

		style.WindowPadding = ImVec2(12, 12);
		style.FramePadding = ImVec2(8, 4);
		style.ItemSpacing = ImVec2(10, 6);
		style.WindowRounding = 6.0f;
		style.FrameRounding = 6.0f;
		style.GrabRounding = 6.0f;
		style.TabRounding = 6.0f;
	}

	RBX::Instance silent_closestPlr{ nullptr };
	RBX::Vector2 silent_lastPos{};
	RBX::Vector2 silent_targetPos{};

	std::thread([&]() {
		while (true)
		{
			if (!Settings::silentAimEnabled)
			{
				Sleep(100);

				continue;
			}

			RBX::Instance bestPlr{ nullptr };
			RBX::Vector2 bestPos{};

			float closestDistance{ Settings::silentAimFOVRadius };
			silent_closestPlr = nullptr;

			POINT mousePos;
			GetCursorPos(&mousePos);

			RBX::Instance mouseService{ dataModel.findFirstChildOfClass("MouseService") };
			RBX::Instance inputObject{ RBX::Memory::read<void*>((void*)((uintptr_t)mouseService.address + Offsets::InputObject)) };

			for (RBX::Instance player : players.getChildren())
			{
				RBX::Instance plr{ RBX::Memory::read<void*>((void*)((uintptr_t)player.address + Offsets::ModelInstance)) };

				if (plr.name() == localPlayer.name())
				{
					continue;
				}

				RBX::Instance lockPart{ plr.findFirstChild(Settings::silentAimLockPart) };
				RBX::Instance torso{ plr.findFirstChild("Torso") };
				if (!torso.address) // if not R6
				{
					auto it{ std::find(aimbotLockPartsR6.begin(), aimbotLockPartsR6.end(), Settings::silentAimLockPart) };
					size_t index{ static_cast<size_t>(std::distance(aimbotLockPartsR6.begin(), it)) };

					lockPart = plr.findFirstChild(aimbotLockPartsR15[index]);
				}

				RBX::Vector3 lockPartPos{ lockPart.getPosition() };
				RBX::Vector2 screenPos{ visualEngine.worldToScreen(lockPartPos) };

				float dx{ screenPos.x - mousePos.x };
				float dy{ screenPos.y - mousePos.y };
				float dist{ sqrtf(dx * dx + dy * dy) };

				if (dist < Settings::silentAimFOVRadius && dist < closestDistance)
				{
					closestDistance = dist;
					bestPlr = plr;
					bestPos = screenPos;
				}
			}

			silent_closestPlr = bestPlr;
			silent_targetPos = bestPos;

			if (silent_closestPlr.address == nullptr)
			{
				silent_targetPos.x = static_cast<float>(mousePos.x);
				silent_targetPos.y = static_cast<float>(mousePos.y);
			}

			Sleep(1);
		}
	}).detach();

	std::thread([&]() {
		while (true)
		{
			if (!Settings::silentAimEnabled)
			{
				Sleep(100);

				continue;
			}

			RBX::Instance mouseService{ dataModel.findFirstChildOfClass("MouseService") };
			RBX::Instance inputObject{ RBX::Memory::read<void*>((void*)((uintptr_t)mouseService.address + Offsets::InputObject)) };

			RBX::Memory::write<RBX::Vector2>((void*)((uintptr_t)inputObject.address + Offsets::MousePosition), silent_targetPos);
			RBX::Memory::write<RBX::Vector2>((void*)((uintptr_t)inputObject.address + Offsets::MousePosition), silent_targetPos);
			RBX::Memory::write<RBX::Vector2>((void*)((uintptr_t)inputObject.address + Offsets::MousePosition), silent_targetPos);
			RBX::Memory::write<RBX::Vector2>((void*)((uintptr_t)inputObject.address + Offsets::MousePosition), silent_targetPos);
			RBX::Memory::write<RBX::Vector2>((void*)((uintptr_t)inputObject.address + Offsets::MousePosition), silent_targetPos);
			RBX::Memory::write<RBX::Vector2>((void*)((uintptr_t)inputObject.address + Offsets::MousePosition), silent_targetPos);
		}
	}).detach();

	ULONGLONG lastPlrRefresh{ GetTickCount64() };
	while (true)
	{
		if (RBX::Memory::read<int>((void*)((uintptr_t)dataModelAddr + Offsets::GameId)) == 0)
		{
			while (RBX::Memory::read<int>((void*)((uintptr_t)dataModelAddr + Offsets::GameId)) == 0)
			{
				dataModelAddr = RBX::getDataModel();
				Sleep(1000);
			}
		}

		HWND robloxHWND{ FindWindow(NULL, L"Roblox") };
		bool robloxFocused{ GetForegroundWindow() == robloxHWND };

		if (GetAsyncKeyState(Settings::toggleGuiKey) & 1)
		{
			Settings::imguiVisible = !Settings::imguiVisible;
		}

		dataModel = RBX::Instance(RBX::getDataModel());
		workspace = dataModel.findFirstChild("Workspace");
		players = dataModel.findFirstChild("Players");

		localPlayer = RBX::Instance(RBX::Memory::read<void*>((void*)((uintptr_t)players.address + Offsets::LocalPlayer)));
		localPlayerModelInstance = RBX::Instance(RBX::Memory::read<void*>((void*)((uintptr_t)localPlayer.address + Offsets::ModelInstance)));
		humanoid = localPlayerModelInstance.findFirstChild("Humanoid");
		hrp = localPlayerModelInstance.findFirstChild("HumanoidRootPart");

		camera = RBX::Memory::read<void*>((void*)((uintptr_t)workspace.address + Offsets::Camera));

		if (GetTickCount64() - lastPlrRefresh > 500)
		{
			playersList.clear();
			for (RBX::Instance plr : players.getChildren())
			{
				//std::cout << plr.name() << "\n";
				playersList.push_back(RBX::Instance(RBX::Memory::read<void*>((void*)((uintptr_t)plr.address + Offsets::ModelInstance))));
			}
			lastPlrRefresh = GetTickCount64();
		}

		renderer.StartRender();

		if (Settings::imguiVisible)
		{
			SetWindowLong(renderer.hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);

			ImGuiIO& imguiIo{ ImGui::GetIO() };
			ImVec2 mousePos{ imguiIo.MousePos };

			ImDrawList* drawList{ ImGui::GetBackgroundDrawList() };
			drawList->AddRectFilled({ 0.f, 0.f }, { static_cast<float>(monitorWidth), static_cast<float>(monitorHeight) }, IM_COL32(0, 0, 0, 128));

			POINT mouse;
			GetCursorPos(&mouse);

			RECT rc;
			GetWindowRect(renderer.hwnd, &rc);

			Snowflake::Update(snow, Snowflake::vec3(mouse.x, mouse.y), Snowflake::vec3(rc.left, rc.top));

			drawList->AddRectFilled({ (monitorWidth / 2) - 94.5f, monitorHeight - 80.0f }, { (monitorWidth / 2) + 94.5f, monitorHeight - 40.0f }, ImColor(0.05f, 0.05f, 0.05f, 1.0f), 6.0f);
			DrawGlow(drawList, { (monitorWidth / 2) - 94.5f, monitorHeight - 80.0f }, { (monitorWidth / 2) + 94.5f, monitorHeight - 40.0f }, glowColor, 5, 0.15f, 6.0f);

			// logo
			drawList->AddImage((void*)logoImg, { (monitorWidth / 2) + 54.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) + 84.5f, monitorHeight - 45.0f });
			drawList->AddLine({ (monitorWidth / 2) + 44.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) + 44.5f, monitorHeight - 45.0f }, IM_COL32_WHITE);

			// menu
			drawList->AddImage((void*)menuIconImg, { (monitorWidth / 2) - 79.5f, monitorHeight - 70.0f }, { (monitorWidth / 2) - 59.5f, monitorHeight - 50.0f });

			// explorer
			drawList->AddImage((void*)explorerIconImg, { (monitorWidth / 2) - 44.5f, monitorHeight - 70.0f }, { (monitorWidth / 2) - 24.5f, monitorHeight - 50.0f });

			// keybind list
			drawList->AddImage((void*)keybindListIconImg, { (monitorWidth / 2) - 9.5f, monitorHeight - 70.0f }, { (monitorWidth / 2) + 10.5f, monitorHeight - 50.0f });

			bool menuBtnHovered{ ImRect({ (monitorWidth / 2) - 84.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) - 54.5f, monitorHeight - 45.0f }).Contains(mousePos) };
			if (!menuBtnHovered)
				drawList->AddRect({ (monitorWidth / 2) - 84.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) - 54.5f, monitorHeight - 45.0f }, IM_COL32(50, 50, 50, 255), 6.0f);
			else
				drawList->AddRect({ (monitorWidth / 2) - 84.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) - 54.5f, monitorHeight - 45.0f }, IM_COL32(139, 50, 50, 255), 6.0f);

			bool explorerBtnHovered{ ImRect({ (monitorWidth / 2) - 49.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) - 19.5f, monitorHeight - 45.0f }).Contains(mousePos) };
			if (!explorerBtnHovered)
				drawList->AddRect({ (monitorWidth / 2) - 49.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) - 19.5f, monitorHeight - 45.0f }, IM_COL32(50, 50, 50, 255), 6.0f);
			else
				drawList->AddRect({ (monitorWidth / 2) - 49.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) - 19.5f, monitorHeight - 45.0f }, IM_COL32(139, 50, 50, 255), 6.0f);

			bool keybindlistBtnHovered{ ImRect({ (monitorWidth / 2) - 14.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) + 15.5f, monitorHeight - 45.0f }).Contains(mousePos) };
			if (!keybindlistBtnHovered)
				drawList->AddRect({ (monitorWidth / 2) - 14.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) + 15.5f, monitorHeight - 45.0f }, IM_COL32(50, 50, 50, 255), 6.0f);
			else
				drawList->AddRect({ (monitorWidth / 2) - 14.5f, monitorHeight - 75.0f }, { (monitorWidth / 2) + 15.5f, monitorHeight - 45.0f }, IM_COL32(139, 50, 50, 255), 6.0f);

			if (menuBtnHovered && imguiIo.MouseClicked[0])
			{
				Settings::mainMenuVisible = !Settings::mainMenuVisible;
			}

			if (menuBtnHovered)
			{
				drawList->AddText({ mousePos.x + 15.0f, mousePos.y + 15.0f }, IM_COL32_WHITE, "Menu");
			}

			if (explorerBtnHovered && imguiIo.MouseClicked[0])
			{
				Settings::explorerWinVisible = !Settings::explorerWinVisible;
			}

			if (explorerBtnHovered)
			{
				drawList->AddText({ mousePos.x + 15.0f, mousePos.y + 15.0f }, IM_COL32_WHITE, "Explorer");
			}

			if (keybindlistBtnHovered && imguiIo.MouseClicked[0])
			{
				Settings::keybindListVisible = !Settings::keybindListVisible;
			}

			if (keybindlistBtnHovered)
			{
				drawList->AddText({ mousePos.x + 15.0f, mousePos.y + 15.0f }, IM_COL32_WHITE, "Keybind List");
			}

			if (Settings::mainMenuVisible)
			{
				ImGui::SetNextWindowSize({ 1057, 600 });

				ImGui::Begin("Vortix", (bool*)0, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

				ImVec2 p{ ImGui::GetCursorScreenPos() };

				ImDrawList* drawList{ ImGui::GetWindowDrawList() };

				ImVec2 winPos{ ImGui::GetWindowPos() };
				ImVec2 winSize{ ImGui::GetWindowSize() };
				DrawGlow(ImGui::GetBackgroundDrawList(), winPos, { winPos.x + winSize.x, winPos.y + winSize.y }, glowColor, 10, 0.1f, ImGui::GetStyle().WindowRounding);

				// tab bar
				drawList->AddRectFilled({ p.x - 12.0f, p.y - 12.0f }, { p.x + 160.0f, p.y + 600.0f }, IM_COL32(17, 17, 17, 204), 6.0f);

				drawList->AddImage(logoImg, { winPos.x, winPos.y - 34.0f }, { winPos.x + 180.0f, winPos.y + 160.0f });
				drawList->AddText({ winPos.x + 94.0f, winPos.y + 96.0f }, IM_COL32_WHITE, "ortix");

				//ImGui::SetCursorPos({ 86.0f, 100.0f });
				if (DrawButtonWithImage(aimingIconImg, { 30.0f, 30.0f }, { 120.0f, 50.0f }, { 26.0f, 160.0f }, "Aiming", IM_COL32(30, 30, 30, 255), IM_COL32(70, 20, 20, 255), IM_COL32(100, 25, 25, 255), 6.0f))
				{
					Settings::currentTab = "Aiming";
				}

				if (DrawButtonWithImage(visualsIconImg, { 30.0f, 30.0f }, { 120.0f, 50.0f }, { 26.0f, 220.0f }, "Visuals", IM_COL32(30, 30, 30, 255), IM_COL32(70, 20, 20, 255), IM_COL32(100, 25, 25, 255), 6.0f))
				{
					Settings::currentTab = "Visuals";
				}

				if (DrawButtonWithImage(settingsIconImg, { 30.0f, 30.0f }, { 120.0f, 50.0f }, { 26.0f, 280.0f }, "Settings", IM_COL32(30, 30, 30, 255), IM_COL32(70, 20, 20, 255), IM_COL32(100, 25, 25, 255), 6.0f))
				{
					Settings::currentTab = "Settings";
				}

				if (DrawButtonWithImage(miscIconImg, { 30.0f, 30.0f }, { 120.0f, 50.0f }, { 26.0f, 340.0f }, "Misc", IM_COL32(30, 30, 30, 255), IM_COL32(70, 20, 20, 255), IM_COL32(100, 25, 25, 255), 6.0f))
				{
					Settings::currentTab = "Misc";
				}

				if (DrawButtonWithImage(gamblingIconImg, { 30.0f, 30.0f }, { 120.0f, 50.0f }, { 26.0f, 400.0f }, "Gambling", IM_COL32(30, 30, 30, 255), IM_COL32(70, 20, 20, 255), IM_COL32(100, 25, 25, 255), 6.0f))
				{
					Settings::currentTab = "Gambling";
				}

				ImGui::SetCursorPos({ 12.0f, 40.0f });

				//ImGui::Text("To toggle this menu press INSERT.\n\n");
				//ImGui::Text("TIPS:\nFeatures that write memory are detected!\nRoblox needs to be maximized for position accuracy!\n\n");

				if (Settings::currentTab == "Aiming")
				{
					drawList->AddRectFilled({ p.x + 170.0f, p.y }, { p.x + 454.0f, p.y + 360.0f }, featureBGColor, 6.0f);
					drawList->AddRect({ p.x + 170.0f, p.y }, { p.x + 454.0f, p.y + 360.0f }, IM_COL32(20, 20, 20, 255), 6.0f);

					drawList->AddRectFilled({ p.x + 460.0f, p.y }, { p.x + 744.0f, p.y + 250.0f }, featureBGColor, 6.0f);
					drawList->AddRect({ p.x + 460.0f, p.y }, { p.x + 744.0f, p.y + 250.0f }, IM_COL32(20, 20, 20, 255), 6.0f);

					drawList->AddRectFilled({ p.x + 750.0f, p.y }, { p.x + 1034.0f, p.y + 250.0f }, featureBGColor, 6.0f);
					drawList->AddRect({ p.x + 750.0f, p.y }, { p.x + 1034.0f, p.y + 250.0f }, IM_COL32(20, 20, 20, 255), 6.0f);
					ImGui::SetCursorPos({ 185.0f, 15.0f });

					ImGui::Checkbox("Toggle aimbot", &Settings::aimbotEnabled);
					ImGui::SetCursorPosX(185.0f);

					ImGui::Checkbox("Toggle FOV", &Settings::aimbotFOVEnabled);
					ImGui::SetCursorPosX(185.0f);

					ImGui::Checkbox("Toggle prediction", &Settings::aimbotPredictionEnabled);
					ImGui::SetCursorPosX(185.0f);

					ImGui::PushItemWidth(276);
					ImGui::Text("Lock Parts");
					ImGui::SetCursorPosX(185.0f);
					if (ImGui::BeginCombo("##Lock Parts", Settings::aimbotLockPart.c_str()))
					{
						for (const std::string& part : aimbotLockPartsR6)
						{
							bool selected{ Settings::aimbotLockPart == part };
							if (ImGui::Selectable(part.c_str(), selected))
							{
								Settings::aimbotLockPart = part;
							}

							if (selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::SetCursorPosX(185.0f);

					ImGui::Text("FOV radius");
					ImGui::SetCursorPosX(185.0f);
					ImGui::SliderFloat("##FOV radius", &Settings::aimbotFOVRadius, 1.0f, 400.0f, "%.1f");
					ImGui::SetCursorPosX(185.0f);

					ImGui::Text("Aimbot strenght");
					ImGui::SetCursorPosX(185.0f);
					ImGui::SliderFloat("##Aimbot strenght", &Settings::aimbotStrenght, 0.1f, 0.6f, "%.1f");
					ImGui::SetCursorPosX(185.0f);

					ImGui::Text("Prediction X");
					ImGui::SetCursorPosX(185.0f);
					ImGui::SliderFloat("##Prediction X", &Settings::aimbotPredictionX, 1.0f, 50.0f, "%.0f");
					ImGui::SetCursorPosX(185.0f);

					ImGui::Text("Prediction Y");
					ImGui::SetCursorPosX(185.0f);
					ImGui::SliderFloat("##Prediction Y", &Settings::aimbotPredictionY, 1.0f, 50.0f, "%.0f");
					ImGui::SetCursorPosX(185.0f);

					ImGui::Text("FOV color");
					ImGui::SetCursorPosX(185.0f);
					ImGui::ColorEdit4("##FOV color", (float*)&Settings::aimbotFovColor);

					ImGui::PopItemWidth();

					ImGui::SetCursorPos({ 360.0f, 40.0f });
					ImGui::Hotkey(&Settings::aimbotKey, { 100, 20 });

					ImGui::SetCursorPos({ 385.0f, 25.0f });
					ImGui::Text("[Aimbot]");

					ImGui::SetCursorPos({ 658.0f, 25.0f });
					ImGui::Text("[Triggerbot]");

					ImGui::SetCursorPos({ 475.0f, 15.0f });
					ImGui::Checkbox("Toggle triggerbot", &Settings::triggerbotEnabled);
					ImGui::SetCursorPosX(475.0f);
					ImGui::Checkbox("Indicate clicking", &Settings::triggerbotIndicateClicking);

					ImGui::PushItemWidth(276);
					ImGui::SetCursorPosX(475.0f);
					ImGui::Text("Trigger parts");
					ImGui::SetCursorPosX(475.0f);
					if (ImGui::BeginCombo("##Trigger parts", Settings::triggerbotTriggerPart.c_str()))
					{
						for (const std::string& part : aimbotLockPartsR6)
						{
							bool selected{ Settings::triggerbotTriggerPart == part };
							if (ImGui::Selectable(part.c_str(), selected))
							{
								Settings::triggerbotTriggerPart = part;
							}

							if (selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}

						ImGui::EndCombo();
					}
					ImGui::SetCursorPosX(475.0f);
					ImGui::Text("Detection radius");
					ImGui::SetCursorPosX(475.0f);
					ImGui::SliderFloat("##Detection radius", &Settings::triggerbotDetectionRadius, 1.0f, 100.0f, "%.1f");
					ImGui::PopItemWidth();

					ImGui::SetCursorPos({ 650.0f, 40.0f });
					ImGui::Hotkey(&Settings::triggerbotKey, { 100, 20 });

					ImGui::SetCursorPos({ 765.0f, 15.0f });
					ImGui::Checkbox("Toggle silent aim", &Settings::silentAimEnabled);

					ImGui::PushItemWidth(276);

					ImGui::SetCursorPosX(765.0f);
					ImGui::Text("Lock Parts");
					ImGui::SetCursorPosX(765.0f);
					if (ImGui::BeginCombo("##Silent Aim Lock Parts", Settings::silentAimLockPart.c_str()))
					{
						for (const std::string& part : aimbotLockPartsR6)
						{
							bool selected{ Settings::silentAimLockPart == part };
							if (ImGui::Selectable(part.c_str(), selected))
							{
								Settings::silentAimLockPart = part;
							}

							if (selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					ImGui::SetCursorPosX(765.0f);
					ImGui::Text("FOV Radius");
					ImGui::SetCursorPosX(765.0f);
					ImGui::SliderFloat("##Silent Aim FOV Radius", &Settings::silentAimFOVRadius, 1, 1000, "%.0f");

					ImGui::PopItemWidth();

					ImGui::SetCursorPos({ 950.0f, 25.0f });
					ImGui::Text("[Silent Aim]");
				}

				if (Settings::currentTab == "Visuals")
				{
					drawList->AddRectFilled({ p.x + 170.0f, p.y }, { p.x + 454.0f, p.y + 340.0f }, featureBGColor, 6.0f);
					drawList->AddRect({ p.x + 170.0f, p.y }, { p.x + 454.0f, p.y + 340.0f }, IM_COL32(20, 20, 20, 255), 6.0f);

					drawList->AddRectFilled({ p.x + 460.0f, p.y }, { p.x + 744.0f, p.y + 250.0f }, featureBGColor, 6.0f);
					drawList->AddRect({ p.x + 460.0f, p.y }, { p.x + 744.0f, p.y + 250.0f }, IM_COL32(20, 20, 20, 255), 6.0f);
					ImGui::SetCursorPos({ 185.0f, 15.0f });

					ImGui::PushItemWidth(276);
					ImGui::Checkbox("Toggle ESP", &Settings::espEnabled);

					ImGui::SetCursorPosX(185.0f);
					ImGui::Text("ESP Type");
					ImGui::SetCursorPosX(185.0f);
					if (ImGui::BeginCombo("##ESP Type", Settings::espType.c_str()))
					{
						for (const std::string& type : espTypes)
						{
							bool selected{ Settings::espType == type };
							if (ImGui::Selectable(type.c_str(), selected))
							{
								Settings::espType = type;
							}

							if (selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::SetCursorPosX(185.0f);
					ImGui::Checkbox("Filled ESP", &Settings::espFilled);
					ImGui::SetCursorPosX(185.0f);
					ImGui::Checkbox("Show name", &Settings::espShowName);
					ImGui::SetCursorPosX(185.0f);
					ImGui::Checkbox("Show health", &Settings::espShowHealth);
					ImGui::SetCursorPosX(185.0f);
					ImGui::Checkbox("Show distance", &Settings::espShowDistance);
					ImGui::SetCursorPosX(185.0f);
					if (ImGui::Button("Preview ESP"))
					{
						Settings::espPreviewOpened = !Settings::espPreviewOpened;
					}
					ImGui::SetCursorPosX(185.0f);
					ImGui::Checkbox("Ignore dead players", &Settings::espIgnoreDeadPlrs);
					ImGui::SetCursorPosX(185.0f);
					ImGui::Text("ESP distance");
					ImGui::SetCursorPosX(185.0f);
					ImGui::SliderInt("##ESP distance", &Settings::espDistance, 0, 500);
					ImGui::SetCursorPosX(185.0f);
					ImGui::Text("ESP color");
					ImGui::SetCursorPosX(185.0f);
					ImGui::ColorEdit4("##ESP color", (float*)&Settings::espColor);
					ImGui::PopItemWidth();

					ImGui::SetCursorPos({ 390.0f, 25.0f });
					ImGui::Text("[ESP]");

					ImGui::PushItemWidth(276);

					ImGui::SetCursorPos({ 475.0f, 15.0f });
					ImGui::Checkbox("Toggle tracers", &Settings::tracersEnabled);
					ImGui::SetCursorPosX(475.0f);
					ImGui::Text("Tracer Type");
					ImGui::SetCursorPosX(475.0f);
					if (ImGui::BeginCombo("##Tracer Type", Settings::tracerType.c_str()))
					{
						for (const std::string& type : tracerTypes)
						{
							bool selected{ Settings::tracerType == type };
							if (ImGui::Selectable(type.c_str(), selected))
							{
								Settings::tracerType = type;
							}

							if (selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::SetCursorPosX(475.0f);
					ImGui::Text("Tracer color");
					ImGui::SetCursorPosX(475.0f);
					ImGui::ColorEdit4("##Tracer color", (float*)&Settings::tracerColor);

					ImGui::PopItemWidth();

					ImGui::SetCursorPos({ 665.0f, 25.0f });
					ImGui::Text("[Tracers]");
				}

				if (Settings::currentTab == "Settings")
				{
					ImGui::SetCursorPosY(15.0f);
					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("Config name");
					ImGui::SetCursorPosX(175.0f);
					ImGui::InputText("##Config name", Settings::configFileName, 30);
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Load config"))
					{
						wchar_t fPath[MAX_PATH]{};

						OPENFILENAME ofn;
						ZeroMemory(&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = renderer.hwnd;
						ofn.lpstrFile = fPath;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrFilter = L"All FIles\0";
						ofn.nFilterIndex = 1;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

						if (GetOpenFileNameW(&ofn))
						{
							std::ifstream iF(ofn.lpstrFile);
							if (iF.is_open())
							{
								std::string contents((std::istreambuf_iterator<char>(iF)), std::istreambuf_iterator<char>());

								json config{ json::parse(contents) };

								Settings::silentAimEnabled = config["silentaim"]["enabled"].get<bool>();
								Settings::silentAimLockPart = config["silentaim"]["lockPart"].get<std::string>();
								Settings::silentAimFOVRadius = config["silentaim"]["FOVradius"].get<float>();

								Settings::aimbotEnabled = config["aimbot"]["enabled"].get<bool>();
								Settings::aimbotFOVEnabled = config["aimbot"]["FOVenabled"].get<bool>();
								Settings::aimbotFOVRadius = config["aimbot"]["FOVradius"].get<float>();
								Settings::aimbotStrenght = config["aimbot"]["strenght"].get<float>();
								Settings::aimbotLockPart = config["aimbot"]["lockPart"].get<std::string>();
								Settings::aimbotKey = config["aimbot"]["key"].get<int>();
								Settings::aimbotFovColor.x = config["aimbot"]["FOVcolor"][0].get<float>();
								Settings::aimbotFovColor.y = config["aimbot"]["FOVcolor"][1].get<float>();
								Settings::aimbotFovColor.z = config["aimbot"]["FOVcolor"][2].get<float>();
								Settings::aimbotFovColor.w = config["aimbot"]["FOVcolor"][3].get<float>();

								Settings::triggerbotEnabled = config["triggerbot"]["enabled"].get<bool>();
								Settings::triggerbotIndicateClicking = config["triggerbot"]["indicateClicking"].get<bool>();
								Settings::triggerbotDetectionRadius = config["triggerbot"]["detectionRadius"].get<float>();
								Settings::triggerbotTriggerPart = config["triggerbot"]["triggerPart"].get<std::string>();
								Settings::triggerbotKey = config["triggerbot"]["key"].get<int>();

								Settings::espEnabled = config["esp"]["enabled"].get<bool>();
								Settings::espFilled = config["esp"]["filled"].get<bool>();
								Settings::espShowDistance = config["esp"]["showDistance"].get<bool>();
								Settings::espShowName = config["esp"]["showName"].get<bool>();
								Settings::espShowHealth = config["esp"]["showHealth"].get<bool>();
								Settings::espIgnoreDeadPlrs = config["esp"]["ignoreDeadPlayers"].get<bool>();
								Settings::espDistance = config["esp"]["distance"].get<int>();
								Settings::espType = config["esp"]["type"].get<std::string>();
								Settings::espColor.x = config["esp"]["color"][0].get<float>();
								Settings::espColor.y = config["esp"]["color"][1].get<float>();
								Settings::espColor.z = config["esp"]["color"][2].get<float>();
								Settings::espColor.w = config["esp"]["color"][3].get<float>();

								Settings::tracersEnabled = config["tracers"]["enabled"].get<bool>();
								Settings::tracerType = config["tracers"]["type"].get<std::string>();
								Settings::tracerColor.x = config["tracers"]["color"][0].get<float>();
								Settings::tracerColor.y = config["tracers"]["color"][1].get<float>();
								Settings::tracerColor.z = config["tracers"]["color"][2].get<float>();
								Settings::tracerColor.w = config["tracers"]["color"][3].get<float>();

								Settings::rbxWindowNeedsToBeSelected = config["settings"]["rbxWindowNeedsToBeSelected"].get<bool>();
								Settings::mainLoopDelay = config["settings"]["mainLoopDelay"].get<int>();

								Settings::noclipEnabled = config["misc"]["noclipEnabled"].get<bool>();
								Settings::flyEnabled = config["misc"]["flyEnabled"].get<bool>();
								Settings::flyKey = config["misc"]["flyKey"].get<int>();

								iF.close();
							}
							else
							{
								MessageBoxA(renderer.hwnd, "Failed to open config file.", "Vortix Error", MB_ICONERROR | MB_OK);
							}
						}
					}
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Save config"))
					{
						json config;

						config["silentaim"]["enabled"] = Settings::silentAimEnabled;
						config["silentaim"]["lockPart"] = Settings::silentAimLockPart;
						config["silentaim"]["FOVradius"] = Settings::silentAimFOVRadius;

						config["aimbot"]["enabled"] = Settings::aimbotEnabled;
						config["aimbot"]["FOVenabled"] = Settings::aimbotFOVEnabled;
						config["aimbot"]["FOVradius"] = Settings::aimbotFOVRadius;
						config["aimbot"]["strenght"] = Settings::aimbotStrenght;
						config["aimbot"]["lockPart"] = Settings::aimbotLockPart;
						config["aimbot"]["key"] = Settings::aimbotKey;
						config["aimbot"]["FOVcolor"] = { Settings::aimbotFovColor.x, Settings::aimbotFovColor.y, Settings::aimbotFovColor.z, Settings::aimbotFovColor.w };

						config["triggerbot"]["enabled"] = Settings::triggerbotEnabled;
						config["triggerbot"]["indicateClicking"] = Settings::triggerbotIndicateClicking;
						config["triggerbot"]["detectionRadius"] = Settings::triggerbotDetectionRadius;
						config["triggerbot"]["triggerPart"] = Settings::triggerbotTriggerPart;
						config["triggerbot"]["key"] = Settings::triggerbotKey;

						config["esp"]["enabled"] = Settings::espEnabled;
						config["esp"]["filled"] = Settings::espFilled;
						config["esp"]["showDistance"] = Settings::espShowDistance;
						config["esp"]["showName"] = Settings::espShowName;
						config["esp"]["showHealth"] = Settings::espShowHealth;
						config["esp"]["ignoreDeadPlayers"] = Settings::espIgnoreDeadPlrs;
						config["esp"]["distance"] = Settings::espDistance;
						config["esp"]["type"] = Settings::espType;
						config["esp"]["color"] = { Settings::espColor.x, Settings::espColor.y, Settings::espColor.z, Settings::espColor.w };

						config["tracers"]["enabled"] = Settings::tracersEnabled;
						config["tracers"]["type"] = Settings::tracerType;
						config["tracers"]["color"] = { Settings::tracerColor.x, Settings::tracerColor.y, Settings::tracerColor.z, Settings::tracerColor.w };

						config["settings"]["rbxWindowNeedsToBeSelected"] = Settings::rbxWindowNeedsToBeSelected;
						config["settings"]["mainLoopDelay"] = Settings::mainLoopDelay;

						config["misc"]["noclipEnabled"] = Settings::noclipEnabled;
						config["misc"]["flyEnabled"] = Settings::flyEnabled;
						config["misc"]["flyKey"] = Settings::flyKey;

						std::ofstream oF(Settings::configFileName);
						if (oF.is_open())
						{
							oF << config.dump(4);
							oF.close();
						}
						else
						{
							MessageBoxA(renderer.hwnd, "Failed to write config file.", "Vortix Error", MB_ICONERROR | MB_OK);
						}
					}
					ImGui::Text("\n");

					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("Theme name");
					ImGui::SetCursorPosX(175.0f);
					ImGui::InputText("##Theme name", Settings::themeFileName, 30);
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Set theme"))
					{
						if (loadTheme(featureBGColor, glowColor, std::string(Settings::themeFileName)))
						{
							std::ofstream oF("settings.json");
							if (oF.is_open())
							{
								json settingsJ2{ settingsJ };
								settingsJ2["theme"] = Settings::themeFileName;

								oF << settingsJ2.dump(4);

								oF.close();
							}
						}
					}
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Reset theme"))
					{
						std::ofstream oF("settings.json");
						if (oF.is_open())
						{
							json settingsJ2{ settingsJ };
							settingsJ2["theme"] = "default";

							oF << settingsJ2.dump(4);

							oF.close();

							ImGuiStyle& style{ ImGui::GetStyle() };

							style.Colors[ImGuiCol_Tab] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
							style.Colors[ImGuiCol_TabHovered] = ImVec4(0.5f, 0.0f, 0.0f, 0.2f);
							style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
							style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
							style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.2f);
							style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0627451f, 0.05882353f, 0.0627451f, 0.9f);
							style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
							style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.05f, 0.05f, 1.00f);
							style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.35f, 0.10f, 0.10f, 1.00f);
							style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f);
							style.Colors[ImGuiCol_TitleBg] = ImVec4(0.30f, 0.05f, 0.05f, 1.00f);
							style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.30f, 0.05f, 0.05f, 1.00f);

							//style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.25f, 0.25f, 1.00f);

							style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.85f, 0.20f, 0.20f, 1.00f);
							style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.25f, 0.25f, 1.00f);

							style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.05f, 0.05f, 1.00f);
							style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.10f, 0.10f, 1.00f);
							style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.15f, 0.15f, 1.00f);

							style.Colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.05f, 0.05f, 0.94f);

							style.WindowPadding = ImVec2(12, 12);
							style.FramePadding = ImVec2(8, 4);
							style.ItemSpacing = ImVec2(10, 6);
							style.WindowRounding = 6.0f;
							style.FrameRounding = 6.0f;
							style.GrabRounding = 6.0f;
							style.TabRounding = 0.0f;

							featureBGColor = { 17, 17, 17, 204 };
							glowColor = { 1.0f, 0.0f, 0.0f, 0.8f };
						}
					}

					ImGui::Text("\n");

					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Checkbox("Streamproof", &Settings::streamproofEnabled))
					{
						if (Settings::streamproofEnabled)
						{
							SetWindowDisplayAffinity(renderer.hwnd, WDA_EXCLUDEFROMCAPTURE);
						}
						else
						{
							SetWindowDisplayAffinity(renderer.hwnd, WDA_NONE);
						}
					}
					ImGui::SetCursorPosX(175.0f);
					ImGui::Checkbox("Roblox window needs to be selected", &Settings::rbxWindowNeedsToBeSelected);
					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("Main loop delay (ms)");
					ImGui::SetCursorPosX(175.0f);
					ImGui::InputInt("##Main loop delay (ms)", &Settings::mainLoopDelay);

					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("Toggle GUI key");
					ImGui::SetCursorPosX(175.0f);
					ImGui::Hotkey(&Settings::toggleGuiKey);

					ImGui::Text("\n");
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Exit"))
					{
						RBX::Memory::detach();
						renderer.Shutdown();

						return 0;
					}
				}

				if (Settings::currentTab == "Misc")
				{
					ImGui::SetCursorPosY(15.0f);
					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("Player name");
					ImGui::SetCursorPosX(175.0f);
					ImGui::InputText("##Player name", Settings::othersRobloxPlr, 30);

					ImGui::Text("\n");
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Spectate player"))
					{
						RBX::Memory::write<void*>((void*)((uintptr_t)camera.address + Offsets::CameraSubject), workspace.findFirstChild(std::string(Settings::othersRobloxPlr)).address);
					}
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Stop spectating player"))
					{
						RBX::Memory::write<void*>((void*)((uintptr_t)camera.address + Offsets::CameraSubject), humanoid.address);
					}

					ImGui::Text("\n");
					ImGui::SetCursorPosX(175.0f);
					ImGui::InputFloat("X", &Settings::othersTeleportPos.x, 0.0f, 0.0f, "%.0f");
					ImGui::SetCursorPosX(175.0f);
					ImGui::InputFloat("Y", &Settings::othersTeleportPos.y, 0.0f, 0.0f, "%.0f");
					ImGui::SetCursorPosX(175.0f);
					ImGui::InputFloat("Z", &Settings::othersTeleportPos.z, 0.0f, 0.0f, "%.0f");
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Teleport to coordinates"))
					{
						RBX::Memory::write<RBX::Vector3>((void*)((uintptr_t)hrp.getPrimitive() + Offsets::Position), Settings::othersTeleportPos);
					}
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Teleport to player"))
					{
						RBX::Instance plr{ players.findFirstChild(Settings::othersRobloxPlr) };
						RBX::Instance plrMi{ plr.getModelInstance() };
						RBX::Instance plrHrp{ plrMi.findFirstChild("HumanoidRootPart") };

						RBX::Memory::write<RBX::Vector3>((void*)((uintptr_t)hrp.getPrimitive() + Offsets::Position), plrHrp.getPosition());
					}
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Orbit player"))
					{
						Settings::orbitEnabled = true;
					}
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Stop orbiting player"))
					{
						Settings::orbitEnabled = false;
					}

					ImGui::Text("\n");
					ImGui::SetCursorPosX(175.0f);
					ImGui::Checkbox("Toggle fly", &Settings::flyEnabled);
					ImGui::SetCursorPosX(175.0f);
					ImGui::Hotkey(&Settings::flyKey, { 100, 20 });
					ImGui::SetCursorPosX(175.0f);
					ImGui::Checkbox("Toggle noclip", &Settings::noclipEnabled);

					ImGui::Text("\n");
					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("WalkSpeed");
					ImGui::SetCursorPosX(175.0f);
					ImGui::SliderInt("##WalkSpeed", &Settings::walkSpeedSet, 0.0f, 1000.0f);
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Set Walk Speed"))
					{
						RBX::setWalkSpeed(humanoid, Settings::walkSpeedSet);
					}
					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("JumpPower");
					ImGui::SetCursorPosX(175.0f);
					ImGui::SliderInt("##JumpPower", &Settings::jumpPowerSet, 0.0f, 1000.0f);
					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Set Jump Power"))
					{
						RBX::setJumpPower(humanoid, Settings::jumpPowerSet);
					}
				}

				if (Settings::currentTab == "Gambling")
				{
					ImGui::SetCursorPosY(15.0f);
					ImGui::SetCursorPosX(175.0f);
					ImGui::Text("%d %d %d", Settings::gamblingSlotsNumber1, Settings::gamblingSlotsNumber2, Settings::gamblingSlotsNumber3);

					ImGui::SetCursorPosX(175.0f);
					if (ImGui::Button("Spin"))
					{
						Settings::gamblingSlotsNumber1 = getRandomNumber(0, 9);
						Settings::gamblingSlotsNumber2 = getRandomNumber(0, 9);
						Settings::gamblingSlotsNumber3 = getRandomNumber(0, 9);
					}
				}

				ImGui::End();
			}

			if (Settings::espPreviewOpened)
			{
				ImGui::SetNextWindowSize({ 290, 380 });
				ImGui::Begin("Vortix - ESP Preview", (bool*)0, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);

				ImVec2 p{ ImGui::GetCursorScreenPos() };
				ImDrawList* drawList{ ImGui::GetWindowDrawList() };

				ImVec2 winPos{ ImGui::GetWindowPos() };
				ImVec2 winSize{ ImGui::GetWindowSize() };
				DrawGlow(ImGui::GetBackgroundDrawList(), winPos, { winPos.x + winSize.x, winPos.y + winSize.y }, glowColor, 10, 0.1f, 6.0f);

				if (avatarImg)
				{
					ImGui::Image((void*)avatarImg, { (float)avatarImgW, (float)avatarImgH });
				}

				if (Settings::espEnabled)
				{
					if (Settings::espType == "Square")
					{
						if (!Settings::espFilled)
						{
							drawList->AddRect({ p.x + 50.0f, p.y + 70.0f }, { p.x + 220.0f, p.y + 260.0f }, ImColor(Settings::espColor));
						}
						else
						{
							drawList->AddRectFilled({ p.x + 50.0f, p.y + 70.0f }, { p.x + 220.0f, p.y + 260.0f }, ImColor(Settings::espColor));
						}
					}
					else if (Settings::espType == "Skeleton")
					{
						drawList->AddLine({ p.x + 132.0f, p.y + 70.0f }, { p.x + 132.0f, p.y + 150.0f }, ImColor(Settings::espColor));

						drawList->AddLine({ p.x + 50.0f, p.y + 150.0f }, { p.x + 132.0f, p.y + 150.0f }, ImColor(Settings::espColor));
						drawList->AddLine({ p.x + 220.0f, p.y + 150.0f }, { p.x + 132.0f, p.y + 150.0f }, ImColor(Settings::espColor));

						drawList->AddLine({ p.x + 105.0f, p.y + 270.0f }, { p.x + 132.0f, p.y + 150.0f }, ImColor(Settings::espColor));
						drawList->AddLine({ p.x + 157.0f, p.y + 270.0f }, { p.x + 132.0f, p.y + 150.0f }, ImColor(Settings::espColor));
					}
					else if (Settings::espType == "Corners")
					{
						drawList->AddLine({ p.x + 50.0f, p.y + 70.0f }, { p.x + 60.0f, p.y + 70.0f }, ImColor(Settings::espColor));
						drawList->AddLine({ p.x + 50.0f, p.y + 70.0f }, { p.x + 50.0f, p.y + 80.0f }, ImColor(Settings::espColor));

						drawList->AddLine({ p.x + 220.0f, p.y + 70.0f }, { p.x + 210.0f, p.y + 70.0f }, ImColor(Settings::espColor));
						drawList->AddLine({ p.x + 220.0f, p.y + 70.0f }, { p.x + 220.0f, p.y + 80.0f }, ImColor(Settings::espColor));

						drawList->AddLine({ p.x + 50.0f, p.y + 260.0f }, { p.x + 50.0f, p.y + 250.0f }, ImColor(Settings::espColor));
						drawList->AddLine({ p.x + 50.0f, p.y + 260.0f }, { p.x + 60.0f, p.y + 260.0f }, ImColor(Settings::espColor));

						drawList->AddLine({ p.x + 220.0f, p.y + 260.0f }, { p.x + 220.0f, p.y + 250.0f }, ImColor(Settings::espColor));
						drawList->AddLine({ p.x + 220.0f, p.y + 260.0f }, { p.x + 210.0f, p.y + 260.0f }, ImColor(Settings::espColor));
					}

					if (Settings::espShowName)
					{
						drawList->AddText({ p.x + 114.0f, p.y + 50.0f }, IM_COL32_WHITE, "abc123");
					}

					if (Settings::espShowHealth)
					{
						drawList->AddRectFilled({ p.x + 48.0f, p.y + 70.0f }, { p.x + 50.0f, p.y + 260.0f }, IM_COL32(0, 255, 0, 255));
					}

					if (Settings::espShowDistance)
					{
						drawList->AddText({ p.x + 120.0f, p.y + 240.0f }, IM_COL32_WHITE, "10");
					}
				}

				ImGui::End();
			}

			if (Settings::explorerWinVisible)
			{
				ImGui::SetNextWindowSize({ 500, 600 });

				ImGui::Begin("Vortix - Explorer", (bool*)0, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);

				ImVec2 winPos{ ImGui::GetWindowPos() };
				ImVec2 winSize{ ImGui::GetWindowSize() };
				DrawGlow(ImGui::GetBackgroundDrawList(), winPos, { winPos.x + winSize.x, winPos.y + winSize.y }, glowColor, 10, 0.1f, 6.0f);

				for (RBX::Instance chld : dataModel.getChildren())
				{
					showExplorerChildren(chld);
				}

				ImGui::End();
			}
		}
		else
		{
			SetWindowLong(renderer.hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED);
		}

		if (Settings::keybindListVisible)
		{
			ImGui::SetNextWindowSize({ 290, 320 });
			ImGui::Begin("Vortix - Keybind List", (bool*)0, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize);

			ImVec2 winPos{ ImGui::GetWindowPos() };
			ImVec2 winSize{ ImGui::GetWindowSize() };
			DrawGlow(ImGui::GetBackgroundDrawList(), winPos, { winPos.x + winSize.x, winPos.y + winSize.y }, glowColor, 10, 0.1f, 6.0f);

			if (Settings::aimbotEnabled && (GetAsyncKeyState(Settings::aimbotKey) & 0x8000))
			{
				ImGui::Text("[HOLD] Aimbot - %s", KeyNames[Settings::aimbotKey]);
			}

			if (Settings::triggerbotEnabled && (GetAsyncKeyState(Settings::triggerbotKey) & 0x8000))
			{
				ImGui::Text("[HOLD] Triggerbot - %s", KeyNames[Settings::triggerbotKey]);
			}

			if (Settings::flyEnabled && Settings::flyKeyToggled)
			{
				ImGui::Text("[TOGGLE] Fly - %s", KeyNames[Settings::flyKey]);
			}

			ImGui::End();
		}

		ImDrawList* drawList{ ImGui::GetBackgroundDrawList() };

		if (Settings::aimbotEnabled && (!Settings::rbxWindowNeedsToBeSelected || robloxFocused))
		{
			bool keybindDown{ static_cast<bool>(Settings::aimbotKey != 0 && GetAsyncKeyState(Settings::aimbotKey) & 0x8000) };

			if (keybindDown && !keybindPrevDown)
			{
				if (!locked)
				{
					float closestDistance{ Settings::aimbotFOVRadius };
					RBX::Instance closestPlr{ nullptr };
					RBX::Instance bestPlr{ nullptr };
					RBX::Vector2 bestPos{};

					for (RBX::Instance plr : playersList)
					{
						if (plr.name() == localPlayer.name())
						{
							continue;
						}

						RBX::Instance lockPart{ plr.findFirstChild(Settings::aimbotLockPart) };
						RBX::Instance torso{ plr.findFirstChild("Torso") };
						if (!torso.address) // if not R6
						{
							auto it{ std::find(aimbotLockPartsR6.begin(), aimbotLockPartsR6.end(), Settings::aimbotLockPart) };
							size_t index{ static_cast<size_t>(std::distance(aimbotLockPartsR6.begin(), it)) };

							lockPart = plr.findFirstChild(aimbotLockPartsR15[index]);
						}

						RBX::Vector3 lockPartPos{ lockPart.getPosition() };
						RBX::Vector2 screenPos{ visualEngine.worldToScreen(lockPartPos) };

						POINT mousePos;
						GetCursorPos(&mousePos);

						float dx{ screenPos.x - mousePos.x };
						float dy{ screenPos.y - mousePos.y };
						float dist{ sqrtf(dx * dx + dy * dy) };

						if (dist < Settings::aimbotFOVRadius && dist < closestDistance)
						{
							closestDistance = dist;
							closestPlr = plr;
						}
					}

					if (closestPlr.address != nullptr)
					{
						lockedPlr = closestPlr;
						locked = true;
					}
				}
			}

			if (keybindDown && locked && lockedPlr.address != nullptr)
			{
				RBX::Instance lockPart{ lockedPlr.findFirstChild(Settings::aimbotLockPart) };
				RBX::Instance torso{ lockedPlr.findFirstChild("Torso") };
				if (!torso.address) // if not R6
				{
					auto it{ std::find(aimbotLockPartsR6.begin(), aimbotLockPartsR6.end(), Settings::aimbotLockPart) };
					size_t index{ static_cast<size_t>(std::distance(aimbotLockPartsR6.begin(), it)) };

					lockPart = lockedPlr.findFirstChild(aimbotLockPartsR15[index]);
				}

				RBX::Vector3 lockPartPos{ lockPart.getPosition() };

				if (Settings::aimbotPredictionEnabled)
				{
					RBX::Vector3 lockPartVelocity{ RBX::Memory::read<RBX::Vector3>((void*)((uintptr_t)lockPart.getPrimitive() + Offsets::Velocity)) };
					RBX::Vector3 velocity{};
					velocity.x = lockPartVelocity.x / Settings::aimbotPredictionX;
					velocity.y = lockPartVelocity.y / Settings::aimbotPredictionY;
					velocity.z = lockPartVelocity.z / Settings::aimbotPredictionX;

					lockPartPos.x += velocity.x;
					lockPartPos.y += velocity.y;
					lockPartPos.z += velocity.z;
				}

				RBX::Vector2 screenPos{ visualEngine.worldToScreen(lockPartPos) };

				if (screenPos.x >= 0.0f && screenPos.y >= 0.0f && screenPos.x <= monitorWidth && screenPos.y <= monitorHeight)
				{
					POINT mousePos;
					GetCursorPos(&mousePos);

					MoveMouse((screenPos.x - mousePos.x) * Settings::aimbotStrenght, (screenPos.y - mousePos.y) * Settings::aimbotStrenght);
				}
			}

			if (!keybindDown && keybindPrevDown)
			{
				locked = false;
				lockedPlr = RBX::Instance(nullptr);
			}

			keybindPrevDown = keybindDown;
		}

		if (Settings::aimbotFOVEnabled && (!Settings::rbxWindowNeedsToBeSelected || robloxFocused))
		{
			POINT mousePos;
			GetCursorPos(&mousePos);

			drawList->AddCircle({ static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) }, Settings::aimbotFOVRadius, ImColor(Settings::aimbotFovColor));
		}

		if (Settings::triggerbotEnabled && (static_cast<bool>(Settings::triggerbotKey != 0 && GetAsyncKeyState(Settings::triggerbotKey) & 0x8000)) && (!Settings::rbxWindowNeedsToBeSelected || robloxFocused))
		{
			POINT mousePos;
			GetCursorPos(&mousePos);

			for (RBX::Instance plr : playersList)
			{
				if (plr.name() == localPlayer.name())
				{
					continue;
				}

				RBX::Instance triggerPart{ plr.findFirstChild(Settings::triggerbotTriggerPart) };
				RBX::Instance torso{ plr.findFirstChild("Torso") };
				if (!torso.address)
				{
					auto it{ std::find(aimbotLockPartsR6.begin(), aimbotLockPartsR6.end(), Settings::triggerbotTriggerPart) };
					size_t index{ static_cast<size_t>(std::distance(aimbotLockPartsR6.begin(), it)) };

					triggerPart = plr.findFirstChild(aimbotLockPartsR15[index]);
				}

				RBX::Vector3 triggerPartPos{ triggerPart.getPosition() };
				RBX::Vector2 screenPos{ visualEngine.worldToScreen(triggerPartPos) };

				float dx{ screenPos.x - mousePos.x };
				float dy{ screenPos.y - mousePos.y };
				float dist{ sqrtf(dx * dx + dy * dy) };

				if (dist < Settings::triggerbotDetectionRadius)
				{
					INPUT input{ 0 };
					input.type = INPUT_MOUSE;
					input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					SendInput(1, &input, sizeof(input));

					ZeroMemory(&input, sizeof(input));

					input.type = INPUT_MOUSE;
					input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(1, &input, sizeof(input));

					if (Settings::triggerbotIndicateClicking)
						drawList->AddCircleFilled({ static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) }, Settings::triggerbotDetectionRadius, IM_COL32(255, 0, 0, 255));
				}
			}
		}

		if (Settings::espEnabled && (!Settings::rbxWindowNeedsToBeSelected || robloxFocused))
		{
			for (RBX::Instance plr : playersList)
			{
				if (Settings::espIgnoreDeadPlrs)
				{
					float health{ RBX::Memory::read<float>((void*)((uintptr_t)plr.findFirstChild("Humanoid").address + Offsets::Health)) };

					if (health <= 0.0f)
					{
						continue;
					}
				}

				if (plr.name() == localPlayer.name())
				{
					continue;
				}

				RBX::Instance torso{ plr.findFirstChild("Torso") };
				RBX::Instance plrHrp{ plr.findFirstChild("HumanoidRootPart") };

				RBX::Vector3 BOX_TOP;
				RBX::Vector3 LEFT_ARM;
				RBX::Vector3 RIGHT_ARM;
				RBX::Vector3 BOX_BOTTOM;

				if (torso.address) // R6
				{
					BOX_TOP = plr.findFirstChild("Head").getPosition();
					LEFT_ARM = plr.findFirstChild("Left Arm").getPosition();
					RIGHT_ARM = plr.findFirstChild("Right Arm").getPosition();
					BOX_BOTTOM = plr.findFirstChild("Right Leg").getPosition();
				}
				else // R15
				{
					BOX_TOP = plr.findFirstChild("Head").getPosition();
					LEFT_ARM = plr.findFirstChild("LeftLowerArm").getPosition();
					RIGHT_ARM = plr.findFirstChild("RightLowerArm").getPosition();
					BOX_BOTTOM = plr.findFirstChild("RightFoot").getPosition();
				}

				RBX::Vector2 BOX_TOP_DRAW{ visualEngine.worldToScreen(BOX_TOP) };
				RBX::Vector2 BOX_BOTTOM_DRAW{ visualEngine.worldToScreen(BOX_BOTTOM) };
				RBX::Vector2 pos1{ visualEngine.worldToScreen({ LEFT_ARM.x, BOX_TOP.y, LEFT_ARM.z }) };
				RBX::Vector2 pos2{ visualEngine.worldToScreen({ RIGHT_ARM.x, BOX_BOTTOM.y, RIGHT_ARM.z }) };

				float dist{ hrp.getDistance(plrHrp.getPosition()) };

				if ((pos1.x == 0 && pos1.y == 0) && (pos2.x == 0 && pos2.y == 0)) continue;
				if (Settings::espDistance != 0 && dist > Settings::espDistance) continue;

				if (Settings::espType == "Square")
				{
					if (!Settings::espFilled)
						drawList->AddRect({ pos1.x, pos1.y }, { pos2.x, pos2.y }, ImColor(Settings::espColor));
					else
						drawList->AddRectFilled({ pos1.x, pos1.y }, { pos2.x, pos2.y }, ImColor(Settings::espColor));
				}
				else if (Settings::espType == "Skeleton")
				{
					RBX::Vector3 torsoPos;
					RBX::Vector3 leftLegPos;
					RBX::Vector3 rightLegPos;

					if (torso.address)
					{
						torsoPos = torso.getPosition();
						leftLegPos = plr.findFirstChild("Left Leg").getPosition();
						rightLegPos = plr.findFirstChild("Right Leg").getPosition();
					}
					else
					{
						torsoPos = plr.findFirstChild("UpperTorso").getPosition();
						leftLegPos = plr.findFirstChild("LeftUpperLeg").getPosition();
						rightLegPos = plr.findFirstChild("RightUpperLeg").getPosition();
					}

					RBX::Vector2 torsoDraw{ visualEngine.worldToScreen(torsoPos) };
					RBX::Vector2 leftLegDraw{ visualEngine.worldToScreen(leftLegPos) };
					RBX::Vector2 rightLegDraw{ visualEngine.worldToScreen(rightLegPos) };
					RBX::Vector2 rightArmDraw{ visualEngine.worldToScreen(RIGHT_ARM) };
					RBX::Vector2 leftArmDraw{ visualEngine.worldToScreen(LEFT_ARM) };

					drawList->AddLine({ BOX_TOP_DRAW.x, BOX_TOP_DRAW.y }, { torsoDraw.x, torsoDraw.y }, ImColor(Settings::espColor));
					drawList->AddLine({ rightArmDraw.x, rightArmDraw.y }, { torsoDraw.x, torsoDraw.y }, ImColor(Settings::espColor));
					drawList->AddLine({ leftArmDraw.x, leftArmDraw.y }, { torsoDraw.x, torsoDraw.y }, ImColor(Settings::espColor));
					drawList->AddLine({ rightLegDraw.x, rightLegDraw.y }, { torsoDraw.x, torsoDraw.y }, ImColor(Settings::espColor));
					drawList->AddLine({ leftLegDraw.x, leftLegDraw.y }, { torsoDraw.x, torsoDraw.y }, ImColor(Settings::espColor));
				}
				else if (Settings::espType == "Corners")
				{
					float left{ (std::min)(pos1.x, pos2.x) };
					float right{ (std::max)(pos1.x, pos2.x) };
					float top{ (std::min)(pos1.y, pos2.y) };
					float bottom{ (std::max)(pos1.y, pos2.y) };

					drawList->AddLine({ left, top }, { left + 10.0f, top }, ImColor(Settings::espColor));
					drawList->AddLine({ left, top }, { left, top + 10.0f }, ImColor(Settings::espColor));

					drawList->AddLine({ right, top }, { right - 10.0f, top }, ImColor(Settings::espColor));
					drawList->AddLine({ right, top }, { right, top + 10.0f }, ImColor(Settings::espColor));

					drawList->AddLine({ left, bottom }, { left + 10.0f, bottom }, ImColor(Settings::espColor));
					drawList->AddLine({ left, bottom }, { left, bottom - 10.0f }, ImColor(Settings::espColor));

					drawList->AddLine({ right, bottom }, { right - 10.0f, bottom }, ImColor(Settings::espColor));
					drawList->AddLine({ right, bottom }, { right, bottom - 10.0f }, ImColor(Settings::espColor));
				}

				if (Settings::espShowName)
					drawList->AddText({ BOX_TOP_DRAW.x - ImGui::CalcTextSize(plr.name().c_str()).x * 0.5f, BOX_TOP_DRAW.y - ImGui::CalcTextSize(plr.name().c_str()).y - 5.0f }, IM_COL32_WHITE, plr.name().c_str());

				if (Settings::espShowDistance)
				{
					drawList->AddText({ visualEngine.worldToScreen(plrHrp.getPosition()).x - ImGui::CalcTextSize(std::to_string((int)dist).c_str()).x * 0.5f, BOX_BOTTOM_DRAW.y - ImGui::CalcTextSize(std::to_string((int)dist).c_str()).y - 5.0f}, IM_COL32_WHITE, std::to_string((int)dist).c_str());
				}

				if (Settings::espShowHealth)
				{
					RBX::Instance plrHumanoid{ plr.findFirstChild("Humanoid") };
					float health{ RBX::Memory::read<float>((void*)((uintptr_t)plrHumanoid.address + Offsets::Health)) };
					float maxHealth{ RBX::Memory::read<float>((void*)((uintptr_t)plrHumanoid.address + Offsets::MaxHealth)) };

					float healthBar{ (health / maxHealth) * (BOX_BOTTOM_DRAW.y - BOX_TOP_DRAW.y) };
					float HEALTH_BAR_TOP{ BOX_BOTTOM_DRAW.y - healthBar };

					drawList->AddRectFilled({ pos1.x - 6.0f, BOX_TOP_DRAW.y }, { pos1.x - 2.0f, BOX_BOTTOM_DRAW.y }, IM_COL32(100, 100, 100, 200));
					drawList->AddRectFilled({ pos1.x - 5.0f, HEALTH_BAR_TOP }, { pos1.x - 3.0f, BOX_BOTTOM_DRAW.y }, IM_COL32(0, 255, 0, 255));
				}
			}
		}

		if (Settings::tracersEnabled && (!Settings::rbxWindowNeedsToBeSelected || robloxFocused))
		{
			for (RBX::Instance plr : playersList)
			{
				if (Settings::espIgnoreDeadPlrs)
				{
					float health{ RBX::Memory::read<float>((void*)((uintptr_t)plr.findFirstChild("Humanoid").address + Offsets::Health)) };

					if (health <= 0.0f)
					{
						continue;
					}
				}

				if (plr.name() == localPlayer.name())
				{
					continue;
				}

				RBX::Instance torso{ plr.findFirstChild("Torso") };
				if (!torso.address)
				{
					torso = plr.findFirstChild("UpperTorso");
				}

				RBX::Vector2 torsoDraw{ visualEngine.worldToScreen(torso.getPosition()) };
				if (torsoDraw.x == 0 && torsoDraw.y == 0) continue;

				if (Settings::tracerType == "Mouse")
				{
					POINT mousePos;
					GetCursorPos(&mousePos);

					drawList->AddLine({ torsoDraw.x, torsoDraw.y }, { static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) }, ImColor(Settings::tracerColor));
				}
				else if (Settings::tracerType == "Corner")
				{
					drawList->AddLine({ torsoDraw.x, torsoDraw.y }, { 0.0f, 0.0f }, ImColor(Settings::tracerColor));
				}
				else if (Settings::tracerType == "Top")
				{
					drawList->AddLine({ torsoDraw.x, torsoDraw.y }, { static_cast<float>(monitorWidth / 2), 0.0f }, ImColor(Settings::tracerColor));
				}
				else if (Settings::tracerType == "Bottom")
				{
					drawList->AddLine({ torsoDraw.x, torsoDraw.y }, { static_cast<float>(monitorWidth / 2), (float)monitorHeight }, ImColor(Settings::tracerColor));
				}
			}
		}

		if (Settings::flyEnabled)
		{
			if (GetAsyncKeyState(Settings::flyKey) & 1)
			{
				Settings::flyKeyToggled = !Settings::flyKeyToggled;
			}

			if (Settings::flyKeyToggled)
			{
				float flySpeed{ 5.0f };
				void* primitive{ hrp.getPrimitive() };

				RBX::Vector3 camPos{ RBX::Memory::read<RBX::Vector3>((void*)((uintptr_t)camera.address + Offsets::CameraPos)) };
				RBX::Matrix3 camRot{ RBX::Memory::read<RBX::Matrix3>((void*)((uintptr_t)camera.address + Offsets::CameraRotation)) };
				RBX::Vector3 pos{ RBX::Memory::read<RBX::Vector3>((void*)((uintptr_t)primitive + Offsets::Position)) };

				RBX::Vector3 lookVector{ -camRot.data[2], -camRot.data[5], -camRot.data[8] };
				RBX::Vector3 rightVector{ camRot.data[0], camRot.data[3], camRot.data[6] };

				RBX::Vector3 moveDirection{};

				if (GetAsyncKeyState('W') & 0x8000)
				{
					moveDirection.x += lookVector.x;
					moveDirection.y += lookVector.y;
					moveDirection.z += lookVector.z;
				}
				if (GetAsyncKeyState('S') & 0x8000)
				{
					moveDirection.x -= lookVector.x;
					moveDirection.y -= lookVector.y;
					moveDirection.z -= lookVector.z;
				}
				if (GetAsyncKeyState('A') & 0x8000)
				{
					moveDirection.x -= rightVector.x;
					moveDirection.y -= rightVector.y;
					moveDirection.z -= rightVector.z;
				}
				if (GetAsyncKeyState('D') & 0x8000)
				{
					moveDirection.x += rightVector.x;
					moveDirection.y += rightVector.y;
					moveDirection.z += rightVector.z;
				}
				if (GetAsyncKeyState(VK_SPACE) & 0x8000)
				{
					moveDirection.y += 1.0f;
				}
				if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
				{
					moveDirection.y -= 1.0f;
				}

				if (moveDirection.x != 0 && moveDirection.y != 0 && moveDirection.z != 0)
				{
					float len{ std::sqrt(moveDirection.x * moveDirection.x + moveDirection.y * moveDirection.y) };
					if (len == 0) continue;
					moveDirection.x /= len;
					moveDirection.y /= len;
					moveDirection.z /= len;

					moveDirection.x *= flySpeed;
					moveDirection.y *= flySpeed;
					moveDirection.z *= flySpeed;
				}

				RBX::Vector3 newPos{ pos.x + moveDirection.x, pos.y + moveDirection.y, pos.z + moveDirection.z };

				RBX::Memory::write<RBX::Vector3>((void*)((uintptr_t)primitive + Offsets::Position), newPos);
				RBX::Memory::write<RBX::Vector3>((void*)((uintptr_t)primitive + Offsets::Velocity), { 0.0f, 0.0f, 0.0f });
			}
		}

		if (Settings::noclipEnabled)
		{
			RBX::Instance head{ localPlayerModelInstance.findFirstChild("Head") };
			RBX::Instance torso{ localPlayerModelInstance.findFirstChild("Torso") };
			RBX::Instance torso2{ nullptr };
			if (!torso.address)
			{
				torso = localPlayerModelInstance.findFirstChild("UpperTorso");
				torso2 = localPlayerModelInstance.findFirstChild("LowerTorso");
			}

			RBX::Memory::write<int>((void*)((uintptr_t)head.getPrimitive() + Offsets::CanCollide), 0);
			RBX::Memory::write<int>((void*)((uintptr_t)torso.getPrimitive() + Offsets::CanCollide), 0);
			if (torso2.address)
				RBX::Memory::write<int>((void*)((uintptr_t)torso2.getPrimitive() + Offsets::CanCollide), 0);

			RBX::Memory::write<int>((void*)((uintptr_t)hrp.getPrimitive() + Offsets::CanCollide), 0);
		}

		if (Settings::orbitEnabled)
		{
			RBX::Instance plr{ players.findFirstChild(Settings::othersRobloxPlr) };
			RBX::Instance plrMi{ plr.getModelInstance() };
			RBX::Instance plrHrp{ plrMi.findFirstChild("HumanoidRootPart") };

			void* primitive{ hrp.getPrimitive() };

			rot += 0.016f * rotspeed;

			double offsetAngle{ rot };
			RBX::Vector3 offset{ sin(offsetAngle) * eclipse, 0, cos(offsetAngle) * radius };

			RBX::Vector3 targetPos{ RBX::Memory::read<RBX::Vector3>((void*)((uintptr_t)plrHrp.getPrimitive() + Offsets::Position))};
			RBX::Vector3 newPos{ targetPos.x + offset.x, targetPos.y + offset.y, targetPos.z + offset.z };

			RBX::Memory::write<RBX::Vector3>((void*)((uintptr_t)primitive + Offsets::Position), newPos);
		}

		renderer.EndRender();

		Sleep(Settings::mainLoopDelay);
	}

	renderer.Shutdown();

	return 0;
}