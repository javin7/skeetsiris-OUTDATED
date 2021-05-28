#include <mutex>
#include <numeric>
#include <sstream>
#include <stdlib.h>
#include <time.h>

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "../Hooks.h"
#include "Misc.h"
#include "../SDK/ConVar.h"
#include "../SDK/Surface.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/WeaponData.h"
#include "EnginePrediction.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GameEvent.h"
#include "../SDK/FrameStage.h"
#include "../SDK/Client.h"
#include "../SDK/ItemSchema.h"
#include "../SDK/WeaponSystem.h"
#include "../SDK/WeaponData.h"
#include "../GUI.h"

static constexpr bool worldToScreen(const Vector& in, Vector& out) noexcept
{
    const auto& matrix = interfaces->engine->worldToScreenMatrix();
    float w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;

    if (w > 0.001f) {
        const auto [width, height] = interfaces->surface->getScreenSize();
        out.x = width / 2 * (1 + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w);
        out.y = height / 2 * (1 - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w);
        out.z = 0.0f;
        return true;
    }
    return false;
}

void Misc::removeClientSideChokeLimit() noexcept //may cause vaccccc
{
    static bool once = false;
    if (!once)
    {
        auto clMoveChokeClamp = memory->chokeLimit;

        unsigned long protect = 0;

        VirtualProtect((void*)clMoveChokeClamp, 4, PAGE_EXECUTE_READWRITE, &protect);
        *(std::uint32_t*)clMoveChokeClamp = 62;
        VirtualProtect((void*)clMoveChokeClamp, 4, protect, &protect);
        once = true;
    }
}

void Misc::edgejump(UserCmd* cmd) noexcept
{
    if (!config->misc.edgejump || !GetAsyncKeyState(config->misc.edgejumpkey))
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP)
        return;

    if ((EnginePrediction::getFlags() & 1) && !(localPlayer->flags() & 1))
        cmd->buttons |= UserCmd::IN_JUMP;
}

void Misc::slowwalk(UserCmd* cmd) noexcept
{
    if (!config->misc.slowwalk || !GetAsyncKeyState(config->misc.slowwalkKey))
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    const float maxSpeed = (localPlayer->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 3;

    if (cmd->forwardmove && cmd->sidemove) {
        const float maxSpeedRoot = maxSpeed * static_cast<float>(M_SQRT1_2);
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
    } else if (cmd->forwardmove) {
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeed : maxSpeed;
    } else if (cmd->sidemove) {
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeed : maxSpeed;
    }
}

void Misc::fastStop(UserCmd* cmd) noexcept
{
    if (!config->misc.fastStop)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER || !(localPlayer->flags() & 1) || cmd->buttons & UserCmd::IN_JUMP)
        return;

    if (cmd->buttons & (UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT | UserCmd::IN_FORWARD | UserCmd::IN_BACK))
        return;

    auto VectorAngles = [](const Vector& forward, Vector& angles)
    {
        if (forward.y == 0.0f && forward.x == 0.0f)
        {
            angles.x = (forward.z > 0.0f) ? 270.0f : 90.0f;
            angles.y = 0.0f;
        }
        else
        {
            angles.x = atan2(-forward.z, forward.length2D()) * -180.f / M_PI;
            angles.y = atan2(forward.y, forward.x) * 180.f / M_PI;

            if (angles.y > 90)
                angles.y -= 180;
            else if (angles.y < 90)
                angles.y += 180;
            else if (angles.y == 90)
                angles.y = 0;
        }

        angles.z = 0.0f;
    };
    auto AngleVectors = [](const Vector& angles, Vector* forward)
    {
        float	sp, sy, cp, cy;

        sy = sin(degreesToRadians(angles.y));
        cy = cos(degreesToRadians(angles.y));

        sp = sin(degreesToRadians(angles.x));
        cp = cos(degreesToRadians(angles.x));

        forward->x = cp * cy;
        forward->y = cp * sy;
        forward->z = -sp;
    };

    Vector velocity = localPlayer->velocity();
    Vector direction;
    VectorAngles(velocity, direction);
    float speed = velocity.length2D();
    if (speed < 15.f)
        return;

    direction.y = cmd->viewangles.y - direction.y;

    Vector forward;
    AngleVectors(direction, &forward);

    Vector negated_direction = forward * speed;

    cmd->forwardmove = negated_direction.x;
    cmd->sidemove = negated_direction.y;
}

void Misc::inverseRagdollGravity() noexcept
{
    static auto ragdollGravity = interfaces->cvar->findVar("cl_ragdoll_gravity");
    ragdollGravity->setValue(config->visuals.inverseRagdollGravity ? -600 : 600);
}

void Misc::clanTag() noexcept
{
    static std::string clanTag;
    static std::string oldClanTag;
    
    if (config->misc.clanTag) {
        switch (int(memory->globalVars->currenttime * 2.4) % 10)
        {
        case 0: clanTag = "";
            break;
        case 1: clanTag = "J";
            break;
        case 2: clanTag = "Ja";
            break;
        case 3: clanTag = "Jav";
            break;
        case 4: clanTag = "Javi";
            break;
        case 5: clanTag = "Javin";
            break;
        case 6: clanTag = "Javi";
            break;
        case 7: clanTag = "Jav";
            break;
        case 8: clanTag = "Ja";
            break;
        case 9: clanTag = "J";
            break;
        case 10: clanTag = "";
            break;
        }
    }
    
    if (!config->misc.clanTag)
        clanTag = "";
    
    if (oldClanTag != clanTag) {
        memory->setClanTag(clanTag.c_str(), clanTag.c_str());
        oldClanTag = clanTag;
    }
}

void Misc::spectatorList() noexcept
{
    if (!config->misc.spectatorList)
        return;

    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::SetNextWindowSize({ 250.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints({ 250.f, 0.f }, { 250.f, 1000.f });
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!gui->open)
        windowFlags |= ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Spectator list", nullptr, windowFlags);
    ImGui::PopStyleVar();

    if (interfaces->engine->isInGame() && localPlayer)
    {
        if (localPlayer->isAlive())
        {
            for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
                const auto entity = interfaces->entityList->getEntity(i);
                if (!entity || entity->isDormant() || entity->isAlive() || entity->getObserverTarget() != localPlayer.get())
                    continue;

                PlayerInfo playerInfo;

                if (!interfaces->engine->getPlayerInfo(i, playerInfo))
                    continue;

                auto obsMode{ "" };

                switch (entity->getObserverMode()) {
                case ObsMode::InEye:
                    obsMode = "1st";
                    break;
                case ObsMode::Chase:
                    obsMode = "3rd";
                    break;
                case ObsMode::Roaming:
                    obsMode = "Freecam";
                    break;
                default:
                    continue;
                }

                ImGui::TextWrapped("%s - %s", playerInfo.name, obsMode);
            }
        }
        else if (auto observer = localPlayer->getObserverTarget(); !localPlayer->isAlive() && observer && observer->isAlive())
        {
            for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
                const auto entity = interfaces->entityList->getEntity(i);
                if (!entity || entity->isDormant() || entity->isAlive() || entity == localPlayer.get() || entity->getObserverTarget() != observer)
                    continue;

                PlayerInfo playerInfo;

                if (!interfaces->engine->getPlayerInfo(i, playerInfo))
                    continue;

                auto obsMode{ "" };

                switch (entity->getObserverMode()) {
                case ObsMode::InEye:
                    obsMode = "1st";
                    break;
                case ObsMode::Chase:
                    obsMode = "3rd";
                    break;
                case ObsMode::Roaming:
                    obsMode = "Freecam";
                    break;
                default:
                    continue;
                }

                ImGui::TextWrapped("%s - %s", playerInfo.name, obsMode);
            }
        }
    }
    ImGui::End();
}



void Misc::debugwindow() noexcept
{


    if (!config->misc.debugwindow)
        return;

}



void Misc::sniperCrosshair() noexcept
{
    static auto showSpread = interfaces->cvar->findVar("weapon_debug_spread_show");
    showSpread->setValue(config->misc.sniperCrosshair && localPlayer && !localPlayer->isScoped() ? 3 : 0);
}

void Misc::recoilCrosshair() noexcept
{
    static auto recoilCrosshair = interfaces->cvar->findVar("cl_crosshair_recoil");
    recoilCrosshair->setValue(config->misc.recoilCrosshair ? 1 : 0);
}

void Misc::watermark() noexcept
{
    if (!config->misc.watermark)
        return;
    
    ImGui::SetNextWindowBgAlpha(1.f);
    ImGui::SetNextWindowSizeConstraints({ 0.f, 0.f }, { 1000.f, 1000.f });
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Watermark", nullptr, windowFlags);
    ImGui::PopStyleVar();
    
    const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();

    static auto frameRate = 1.0f;
    frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;

    float latency = 0.0f;
    if (auto networkChannel = interfaces->engine->getNetworkChannel(); networkChannel && networkChannel->getLatency(0) > 0.0f) {
        latency = networkChannel->getLatency(0);
    }

    std::string fps{  std::to_string(static_cast<int>(1 / frameRate)) + " fps" };
    std::string ping{ interfaces->engine->isConnected() ? std::to_string(static_cast<int>(latency * 1000)) + " ms" : "Not connected" };

    ImGui::Text("Javin | %s | %s", fps.c_str(), ping.c_str());
    ImGui::SetWindowPos({ screenWidth - ImGui::GetWindowSize().x - 15, 15 });
    
    ImGui::End();
}

void Misc::prepareRevolver(UserCmd* cmd) noexcept
{
    constexpr auto timeToTicks = [](float time) {  return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); };
    constexpr float revolverPrepareTime{ 0.234375f };

    static float readyTime;
    if (config->misc.prepareRevolver && localPlayer && (!config->misc.prepareRevolverKey || GetAsyncKeyState(config->misc.prepareRevolverKey))) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver) {
            if (!readyTime) readyTime = memory->globalVars->serverTime() + revolverPrepareTime;
            auto ticksToReady = timeToTicks(readyTime - memory->globalVars->serverTime() - interfaces->engine->getNetworkChannel()->getLatency(0));
            if (ticksToReady > 0 && ticksToReady <= timeToTicks(revolverPrepareTime))
                cmd->buttons |= UserCmd::IN_ATTACK;
            else
                readyTime = 0.0f;
        }
    }
}

void Misc::fastPlant(UserCmd* cmd) noexcept
{
    if (config->misc.fastPlant) {
        static auto plantAnywhere = interfaces->cvar->findVar("mp_plant_c4_anywhere");

        if (plantAnywhere->getInt())
            return;

        if (!localPlayer || !localPlayer->isAlive() || localPlayer->inBombZone())
            return;

        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (!activeWeapon || activeWeapon->getClientClass()->classId != ClassId::C4)
            return;

        cmd->buttons &= ~UserCmd::IN_ATTACK;

        constexpr float doorRange{ 200.0f };
        Vector viewAngles{ cos(degreesToRadians(cmd->viewangles.x)) * cos(degreesToRadians(cmd->viewangles.y)) * doorRange,
                           cos(degreesToRadians(cmd->viewangles.x)) * sin(degreesToRadians(cmd->viewangles.y)) * doorRange,
                          -sin(degreesToRadians(cmd->viewangles.x)) * doorRange };
        Trace trace;
        interfaces->engineTrace->traceRay({ localPlayer->getEyePosition(), localPlayer->getEyePosition() + viewAngles }, 0x46004009, localPlayer.get(), trace);

        if (!trace.entity || trace.entity->getClientClass()->classId != ClassId::PropDoorRotating)
            cmd->buttons &= ~UserCmd::IN_USE;
    }
}

void Misc::drawBombTimer() noexcept
{
    if (config->misc.bombTimer.enabled) {
        for (int i = interfaces->engine->getMaxClients(); i <= interfaces->entityList->getHighestEntityIndex(); i++) {
            Entity* entity = interfaces->entityList->getEntity(i);
            if (!entity || entity->isDormant() || entity->getClientClass()->classId != ClassId::PlantedC4 || !entity->c4Ticking())
                continue;

            interfaces->surface->setTextFont(hooks->verdanaExtraBoldAA);

            auto drawPositionY{ interfaces->surface->getScreenSize().second / 7 };
            auto bombText{ (std::wstringstream{ } << L"Bomb " << (!entity->c4BombSite() ? 'A' : 'B')).str() };
            auto bombText2{ (std::wstringstream{ } << std::fixed << std::showpoint << std::setprecision(1) << (std::max)(entity->c4BlowTime() - memory->globalVars->currenttime, 0.0f) << L" s").str() };
            const auto bombTextX{ interfaces->surface->getScreenSize().first / 2 - static_cast<int>((interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, bombText.c_str())).first / 2) };
            const auto bombText2X{ interfaces->surface->getScreenSize().first / 2 - static_cast<int>((interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, bombText2.c_str())).first / 2) };

            interfaces->surface->setTextPosition(bombTextX, drawPositionY);
            drawPositionY += interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, bombText.c_str()).second;
            interfaces->surface->setTextColor(242, 242, 199);
            interfaces->surface->printText(bombText.c_str());

            interfaces->surface->setTextPosition(bombText2X, drawPositionY);
            drawPositionY += interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, bombText2.c_str()).second;
            interfaces->surface->setTextColor(255, 255, 255);
            interfaces->surface->printText(bombText2.c_str());

            if (entity->c4Defuser() != -1) {
                if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(interfaces->entityList->getEntityFromHandle(entity->c4Defuser())->index(), playerInfo)) {
                    if (wchar_t name[128];  MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {

                        drawPositionY += (interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, L" ").second)*2;

                        const auto defusingText{ (std::wstringstream{ } << L"Defusing").str() };
                        const auto defusingText2{ (std::wstringstream{ } << std::fixed << std::showpoint << std::setprecision(1) << (std::max)(entity->c4DefuseCountDown() - memory->globalVars->currenttime, 0.0f) << L" s").str() };
                        
                        interfaces->surface->setTextPosition((interfaces->surface->getScreenSize().first - interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, defusingText.c_str()).first) / 2, drawPositionY);
                        drawPositionY += interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, L" ").second;
                        interfaces->surface->setTextColor(242, 141, 141);
                        interfaces->surface->printText(defusingText.c_str());

                        interfaces->surface->setTextPosition((interfaces->surface->getScreenSize().first - interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, defusingText2.c_str()).first) / 2, drawPositionY);
                        drawPositionY += interfaces->surface->getTextSize(hooks->verdanaExtraBoldAA, L" ").second;
                        interfaces->surface->setTextColor(242, 141, 141);
                        interfaces->surface->printText(defusingText2.c_str());
                    }
                }
            }
            break;
        }
    }
}

void Misc::stealNames() noexcept
{
    if (!config->misc.nameStealer)
        return;

    if (!localPlayer)
        return;

    static std::vector<int> stolenIds;

    for (int i = 1; i <= memory->globalVars->maxClients; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);

        if (!entity || entity == localPlayer.get())
            continue;

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(entity->index(), playerInfo))
            continue;

        if (playerInfo.fakeplayer || std::find(stolenIds.cbegin(), stolenIds.cend(), playerInfo.userId) != stolenIds.cend())
            continue;

        if (changeName(false, (std::string{ playerInfo.name } +'\x1').c_str(), 1.0f))
            stolenIds.push_back(playerInfo.userId);

        return;
    }
    stolenIds.clear();
}

void Misc::disablePanoramablur() noexcept
{
    static auto blur = interfaces->cvar->findVar("@panorama_disable_blur");
    blur->setValue(config->misc.disablePanoramablur);
}

bool Misc::changeName(bool reconnect, const char* newName, float delay) noexcept
{
    static auto exploitInitialized{ false };

    static auto name{ interfaces->cvar->findVar("name") };

    if (reconnect) {
        exploitInitialized = false;
        return false;
    }

    if (!exploitInitialized && interfaces->engine->isInGame()) {
        if (PlayerInfo playerInfo; localPlayer && interfaces->engine->getPlayerInfo(localPlayer->index(), playerInfo) && (!strcmp(playerInfo.name, "?empty") || !strcmp(playerInfo.name, "\n\xAD\xAD\xAD"))) {
            exploitInitialized = true;
        } else {
            name->onChangeCallbacks.size = 0;
            name->setValue("\n\xAD\xAD\xAD");
            return false;
        }
    }

    static auto nextChangeTime{ 0.0f };
    if (nextChangeTime <= memory->globalVars->realtime) {
        name->setValue(newName);
        nextChangeTime = memory->globalVars->realtime + delay;
        return true;
    }
    return false;
}

void Misc::bunnyHop(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    static auto wasLastTimeOnGround{ localPlayer->flags() & 1 };

    if (config->misc.bunnyHop && !(localPlayer->flags() & 1) && localPlayer->moveType() != MoveType::LADDER && !wasLastTimeOnGround)
        cmd->buttons &= ~UserCmd::IN_JUMP;

    wasLastTimeOnGround = localPlayer->flags() & 1;
}

void Misc::fakeBan(bool set) noexcept
{
    static bool shouldSet = false;

    if (set)
        shouldSet = set;

    if (shouldSet && interfaces->engine->isInGame() && changeName(false, std::string{ "\x1\xB" }.append(std::string{ static_cast<char>(config->misc.banColor + 1) }).append(config->misc.banText).append("\x1").c_str(), 5.0f))
        shouldSet = false;
}

void Misc::nadeTrajectory() noexcept
{
    static auto trajectoryVar{ interfaces->cvar->findVar("sv_grenade_trajectory") };
    static auto trajectoryTimeVar{ interfaces->cvar->findVar("sv_grenade_trajectory_time") };

    static auto timeBackup = trajectoryTimeVar->getFloat();

    trajectoryVar->onChangeCallbacks.size = 0;
    trajectoryVar->setValue(config->misc.nadeTrajectory);
    trajectoryTimeVar->onChangeCallbacks.size = 0;
    trajectoryTimeVar->setValue(config->misc.nadeTrajectory ? 4 : timeBackup);
}

void Misc::showImpacts() noexcept
{
    static auto impactsVar{ interfaces->cvar->findVar("sv_showimpacts") };

    impactsVar->onChangeCallbacks.size = 0;
    impactsVar->setValue(config->misc.showImpacts);
}

void Misc::fakePrime() noexcept
{
    static bool lastState = false;

    if (config->misc.fakePrime != lastState) {
        lastState = config->misc.fakePrime;

        if (DWORD oldProtect; VirtualProtect(memory->fakePrime, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            constexpr uint8_t patch[]{ 0x74, 0xEB };
            *memory->fakePrime = patch[config->misc.fakePrime];
            VirtualProtect(memory->fakePrime, 1, oldProtect, nullptr);
        }
    }
}

void Misc::killSay(GameEvent& event) noexcept
{
    if (!config->misc.killSay)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    srand(time(NULL));
    auto randomMessage = rand() % 3;

    std::string killMessage = "";
    switch (randomMessage) {
    case 0:
        killMessage = "1";
        break;
    case 1:
        killMessage = "? ";
        break;
    case 2:
        killMessage = "andy likes pp";
        break;
    }

    std::string cmd = "say \"";
    cmd += killMessage;
    cmd += '"';
    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
}

void Misc::chickenDeathSay(GameEvent& event) noexcept
{
    if (!config->misc.chickenDeathSay)
        return;

    if (!localPlayer)
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") == localUserId)
        return;

    std::string deathMessages[] = {
        "Chick-fil-A Chicken Sandwich $3.05",
        "Chick-fil-A Chicken Sandwich - Combo $5.95",
        "Chick-fil-A Chicken Deluxe Sandwich $3.65",
        "Chick-fil-A Chicken Deluxe Sandwich Combo $6.55",
        "Spicy Chicken Sandwich $3.29",
        "Spicy Chicken Sandwich – Combo $6.19",
        "Spicy Chicken Deluxe Sandwich $3.89",
        "Spicy Chicken Deluxe Sandwich – Combo $6.79",
        "Chick-fil-A Nuggets 8 Pc. $3.05",
        "Chick-fil-A Nuggets 12 Pc. $4.45",
        "Chick-fil-A Nuggets – Combo 8 Pc. $5.95",
        "Chick-fil-A Nuggets – Combo 12 Pc. $8.59",
        "Chick-fil-A Nuggets (Grilled) 8 Pc. $3.85",
        "Chick-fil-A Nuggets (Grilled) 12 Pc. $5.75",
        "Chick-fil-A Nuggets (Grilled) – Combo 8 Pc. $6.75",
        "Chick-fil-A Nuggets (Grilled) – Combo 12 Pc. $8.59",
        "Chick-n-Strips 3 Pc. $3.35",
        "Chick-n-Strips 4 Pc. $4.39",
        "Chick-n-Strips – Combo 3 Pc. $6.25",
        "Chick-n-Strips – Combo 4 Pc. $7.25",
        "Grilled Chicken Sandwich $4.39",
        "Grilled Chicken Sandwich – Combo $7.19",
        "Grilled Chicken Club Sandwich $5.59",
        "Grilled Chicken Club Sandwich – Combo $8.39",
        "Chicken Salad Sandwich $3.99",
        "Chicken Salad Sandwich – Combo $6.79",
        "Grilled Chicken Cool Wrap $5.19",
        "Grilled Chicken Cool Wrap – Combo $8.15",
        "Soup & Salad (Large Chicken Soup and Side Salad) $8.35",
        "Chilled Grilled Chicken Sub Sandwich (Limited Time) $4.79"
    };

    srand(time(NULL));
    int randomMessage = rand() % ARRAYSIZE(deathMessages);

    std::string cmd = "say \"";
    cmd += deathMessages[randomMessage];
    cmd += '"';
    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
}

void Misc::fixMovement(UserCmd* cmd, float yaw) noexcept
{
    float oldYaw = yaw + (yaw < 0.0f ? 360.0f : 0.0f);
    float newYaw = cmd->viewangles.y + (cmd->viewangles.y < 0.0f ? 360.0f : 0.0f);
    float yawDelta = newYaw < oldYaw ? fabsf(newYaw - oldYaw) : 360.0f - fabsf(newYaw - oldYaw);
    yawDelta = 360.0f - yawDelta;

    const float forwardmove = cmd->forwardmove;
    const float sidemove = cmd->sidemove;
    cmd->forwardmove = std::cos(degreesToRadians(yawDelta)) * forwardmove + std::cos(degreesToRadians(yawDelta + 90.0f)) * sidemove;
    cmd->sidemove = std::sin(degreesToRadians(yawDelta)) * forwardmove + std::sin(degreesToRadians(yawDelta + 90.0f)) * sidemove;
}

void Misc::antiAfkKick(UserCmd* cmd) noexcept
{
    if (config->misc.antiAfkKick && cmd->commandNumber % 2)
        cmd->buttons |= 1 << 26;
}

void Misc::autoPistol(UserCmd* cmd) noexcept
{
    if (config->misc.autoPistol && localPlayer) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->isPistol() && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime()) {
            if (activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
                cmd->buttons &= ~UserCmd::IN_ATTACK2;
            else
                cmd->buttons &= ~UserCmd::IN_ATTACK;
        }
    }
}

void Misc::revealRanks(UserCmd* cmd) noexcept
{
    if (config->misc.revealRanks && cmd->buttons & UserCmd::IN_SCORE)
        interfaces->client->dispatchUserMessage(50, 0, 0, nullptr);
}

float Misc::autoStrafe(UserCmd* cmd, const Vector& currentViewAngles) noexcept
{
    static float angle = 0.f;
    static float direction = 0.f;
    static float AutoStrafeAngle = 0.f;
    if (!config->misc.autoStrafe)
    {
        angle = 0.f;
        direction = 0.f;
        AutoStrafeAngle = 0.f;
        return AutoStrafeAngle;
    }
    if (!localPlayer || !localPlayer->isAlive())
        return 0.f;
    if (localPlayer->velocity().length2D() < 5.f)
    {
        angle = 0.f;
        direction = 0.f;
        AutoStrafeAngle = 0.f;
        return AutoStrafeAngle;
    }
    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER || (!(cmd->buttons & UserCmd::IN_JUMP)))
    {
        angle = 0.f;
        if (cmd->buttons & UserCmd::IN_FORWARD)
        {
            angle = 0.f;
            if (cmd->buttons & UserCmd::IN_MOVELEFT)
            {
                angle = 45.f;
            }
            else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
            {
                angle = -45.f;
            }
        }
        if (!(cmd->buttons & (UserCmd::IN_FORWARD | UserCmd::IN_BACK)))
        {
            if (cmd->buttons & UserCmd::IN_MOVELEFT)
            {
                angle = 90.f;
            }
            if (cmd->buttons & UserCmd::IN_MOVERIGHT)
            {
                angle = -90.f;
            }
        }
        if (cmd->buttons & UserCmd::IN_BACK)
        {
            angle = 180.f;
            if (cmd->buttons & UserCmd::IN_MOVELEFT)
            {
                angle = 135.f;
            }
            else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
            {
                angle = -135.f;
            }
        }
        direction = angle;
        AutoStrafeAngle = 0.f;
        return AutoStrafeAngle;
    }
    Vector base;
    interfaces->engine->getViewAngles(base);
    float delta = std::clamp(radiansToDegrees(std::atan2(15.f, localPlayer->velocity().length2D())), 0.f, 45.f);

    static bool flip = true;
    if (cmd->buttons & (UserCmd::IN_FORWARD | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT | UserCmd::IN_BACK))
    {
        cmd->forwardmove = 0;
        cmd->sidemove = 0;
        cmd->upmove = 0;
    }
    angle = 0.f;
    if (cmd->buttons & UserCmd::IN_FORWARD)
    {
        angle = 0.f;
        if (cmd->buttons & UserCmd::IN_MOVELEFT)
        {
            angle = 45.f;
        }
        else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
        {
            angle = -45.f;
        }
    }
    if (!(cmd->buttons & (UserCmd::IN_FORWARD | UserCmd::IN_BACK)))
    {
        if (cmd->buttons & UserCmd::IN_MOVELEFT)
        {
            angle = 90.f;
        }
        if (cmd->buttons & UserCmd::IN_MOVERIGHT)
        {
            angle = -90.f;
        }
    }
    if (cmd->buttons & UserCmd::IN_BACK)
    {
        angle = 180.f;
        if (cmd->buttons & UserCmd::IN_MOVELEFT)
        {
            angle = 135.f;
        }
        else if (cmd->buttons & UserCmd::IN_MOVERIGHT)
        {
            angle = -135.f;
        }
    }
    if (std::abs(direction - angle) <= 180)
    {
        if (direction < angle)
        {
            direction += delta;
        }
        else
        {
            direction -= delta;
        }
    }
    else {
        if (direction < angle)
        {
            direction -= delta;
        }
        else
        {
            direction += delta;
        }
    }
    direction = std::isfinite(direction) ? std::remainder(direction, 360.0f) : 0.0f;
    if (cmd->mousedx < 0)
    {
        cmd->sidemove = -450.0f;
    }
    else if (cmd->mousedx > 0)
    {
        cmd->sidemove = 450.0f;
    }
    flip ? base.y += direction + delta : base.y += direction - delta;
    flip ? AutoStrafeAngle = direction + delta : AutoStrafeAngle = direction - delta;
    if (cmd->viewangles.y == currentViewAngles.y)
    {
        cmd->viewangles.y = base.y;
    }
    flip ? cmd->sidemove = 450.f : cmd->sidemove = -450.f;
    flip = !flip;
    return AutoStrafeAngle;
}

struct customCmd
{
    float forwardmove;
    float sidemove;
    float upmove;
};

bool hasShot;
Vector quickPeekStartPos;
Vector drawPos;
std::vector<customCmd>usercmdQuickpeek;
int qpCount;

void Misc::drawQuickPeekStartPos() noexcept
{
    if (!worldToScreen(quickPeekStartPos, drawPos))
        return;

    if (quickPeekStartPos != Vector{ 0, 0, 0 }) {
        interfaces->surface->setDrawColor(255, 255, 255);
        interfaces->surface->drawCircle(drawPos.x, drawPos.y, 0, 10);
    }
}

void gotoStart(UserCmd* cmd) {
    if (usercmdQuickpeek.empty()) return;
    if (hasShot)
    {
        if (qpCount > 0)
        {
            cmd->upmove = -usercmdQuickpeek.at(qpCount).upmove;
            cmd->sidemove = -usercmdQuickpeek.at(qpCount).sidemove;
            cmd->forwardmove = -usercmdQuickpeek.at(qpCount).forwardmove;
            qpCount--;
        }
    }
    else
    {
        qpCount = usercmdQuickpeek.size();
    }
}

void Misc::quickPeek(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive()) return;
    if (GetAsyncKeyState(config->misc.quickpeekkey)) {
        if (quickPeekStartPos == Vector{ 0, 0, 0 }) {
            quickPeekStartPos = localPlayer->getAbsOrigin();
        }
        else {
            customCmd tempCmd = {};
            tempCmd.forwardmove = cmd->forwardmove;
            tempCmd.sidemove = cmd->sidemove;
            tempCmd.upmove = cmd->upmove;

            if (cmd->buttons & UserCmd::IN_ATTACK) hasShot = true;
            gotoStart(cmd);

            if (!hasShot)
                usercmdQuickpeek.push_back(tempCmd);
        }
    }
    else {
        hasShot = false;
        quickPeekStartPos = Vector{ 0, 0, 0 };
        usercmdQuickpeek.clear();
    }
}

void Misc::removeCrouchCooldown(UserCmd* cmd) noexcept
{
    if (config->misc.fastDuck)
        cmd->buttons |= UserCmd::IN_BULLRUSH;
}

void Misc::moonwalk(UserCmd* cmd) noexcept
{
    if (config->misc.moonwalk && localPlayer && localPlayer->moveType() != MoveType::LADDER)
        cmd->buttons ^= UserCmd::IN_FORWARD | UserCmd::IN_BACK | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT;
}

void Misc::blockBot(UserCmd* cmd) noexcept
{
    if (config->misc.blockBot && GetAsyncKeyState(config->misc.blockBotKey)) {
        float bestdist = 250.f;
        int index = -1;
        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);

            if (!entity)
                continue;

            if (!entity->isAlive() || entity->isDormant() || entity == localPlayer.get())
                continue;

            float dist;

            double distance;
            distance = sqrt(((int)localPlayer->origin().x - (int)entity->origin().x) * ((int)localPlayer->origin().x - (int)entity->origin().x) +
                ((int)localPlayer->origin().y - (int)entity->origin().y) * ((int)localPlayer->origin().y - (int)entity->origin().y) +
                ((int)localPlayer->origin().z - (int)entity->origin().z) * ((int)localPlayer->origin().z - (int)entity->origin().z));
            dist = (float)abs(round(distance));

            if (dist < bestdist)
            {
                bestdist = dist;
                index = i;
            }
        }

        if (index == -1)
            return;

        auto target = interfaces->entityList->getEntity(index);

        if (!target)
            return;


        Vector delta = target->origin() - localPlayer->origin();
        Vector angles{ radiansToDegrees(atan2f(-delta.z, std::hypotf(delta.x, delta.y))), radiansToDegrees(atan2f(delta.y, delta.x)) };
        angles.normalize();

        angles.y -= localPlayer->eyeAngles().y;
        angles.normalize();
        angles.y = std::clamp(angles.y, -180.f, 180.f);

        if (angles.y < -1.0f)
            cmd->sidemove = 450.f;
        else if (angles.y > 1.0f)
            cmd->sidemove = -450.f;

    }
}

void Misc::playHitSound(GameEvent& event) noexcept
{
    if (!config->misc.hitSound)
        return;

    if (!localPlayer)
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    constexpr std::array hitSounds{
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play training/timer_bell",
        "play physics/glass/glass_impact_bullet1"
    };

    if (static_cast<std::size_t>(config->misc.hitSound - 1) < hitSounds.size())
        interfaces->engine->clientCmdUnrestricted(hitSounds[config->misc.hitSound - 1]);
    else if (config->misc.hitSound == 5)
        interfaces->engine->clientCmdUnrestricted(("play " + config->misc.customHitSound).c_str());
}

void Misc::killSound(GameEvent& event) noexcept
{
    if (!config->misc.killSound)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt("attacker") != localUserId || event.getInt("userid") == localUserId)
        return;

    constexpr std::array killSounds{
        "play physics/metal/metal_solid_impact_bullet2",
        "play buttons/arena_switch_press_02",
        "play training/timer_bell",
        "play physics/glass/glass_impact_bullet1"
    };

    if (static_cast<std::size_t>(config->misc.killSound - 1) < killSounds.size())
        interfaces->engine->clientCmdUnrestricted(killSounds[config->misc.killSound - 1]);
    else if (config->misc.killSound == 5)
        interfaces->engine->clientCmdUnrestricted(("play " + config->misc.customKillSound).c_str());
}

void Misc::purchaseList(GameEvent* event) noexcept
{
    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    static std::unordered_map<std::string, std::pair<std::vector<std::string>, int>> purchaseDetails;
    static std::unordered_map<std::string, int> purchaseTotal;
    static int totalCost;

    static auto freezeEnd = 0.0f;

    if (event) {
        switch (fnv::hashRuntime(event->getName())) {
        case fnv::hash("item_purchase"): {
            const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

            if (player && localPlayer && memory->isOtherEnemy(player, localPlayer.get())) {
                const auto weaponName = event->getString("weapon");
                auto& purchase = purchaseDetails[player->getPlayerName(true)];

                if (const auto definition = memory->itemSystem()->getItemSchema()->getItemDefinitionByName(weaponName)) {
                    if (const auto weaponInfo = memory->weaponSystem->getWeaponInfo(definition->getWeaponId())) {
                        purchase.second += weaponInfo->price;
                        totalCost += weaponInfo->price;
                    }
                }
                std::string weapon = weaponName;

                if (weapon.starts_with("weapon_"))
                    weapon.erase(0, 7);
                else if (weapon.starts_with("item_"))
                    weapon.erase(0, 5);

                if (weapon.starts_with("smoke"))
                    weapon.erase(5);
                else if (weapon.starts_with("m4a1_s"))
                    weapon.erase(6);
                else if (weapon.starts_with("usp_s"))
                    weapon.erase(5);

                purchase.first.push_back(weapon);
                ++purchaseTotal[weapon];
            }
            break;
        }
        case fnv::hash("round_start"):
            freezeEnd = 0.0f;
            purchaseDetails.clear();
            purchaseTotal.clear();
            totalCost = 0;
            break;
        case fnv::hash("round_freeze_end"):
            freezeEnd = memory->globalVars->realtime;
            break;
        }
    } else {
        if (!config->misc.purchaseList.enabled)
            return;

        static const auto mp_buytime = interfaces->cvar->findVar("mp_buytime");

        if ((!interfaces->engine->isInGame() || freezeEnd != 0.0f && memory->globalVars->realtime > freezeEnd + (!config->misc.purchaseList.onlyDuringFreezeTime ? mp_buytime->getFloat() : 0.0f) || purchaseDetails.empty() || purchaseTotal.empty()) && !gui->open)
            return;

        ImGui::SetNextWindowSize({ 200.0f, 200.0f }, ImGuiCond_Once);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
        if (!gui->open)
            windowFlags |= ImGuiWindowFlags_NoInputs;
        if (config->misc.purchaseList.noTitleBar)
            windowFlags |= ImGuiWindowFlags_NoTitleBar;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
        ImGui::Begin("Purchases", nullptr, windowFlags);
        ImGui::PopStyleVar();

        if (config->misc.purchaseList.mode == PurchaseList::Details) {
            for (const auto& [playerName, purchases] : purchaseDetails) {
                std::string s = std::accumulate(purchases.first.begin(), purchases.first.end(), std::string{ }, [](std::string s, const std::string& piece) { return s += piece + ", "; });
                if (s.length() >= 2)
                    s.erase(s.length() - 2);

                if (config->misc.purchaseList.showPrices)
                    ImGui::TextWrapped("%s $%d: %s", playerName.c_str(), purchases.second, s.c_str());
                else
                    ImGui::TextWrapped("%s: %s", playerName.c_str(), s.c_str());
            }
        } else if (config->misc.purchaseList.mode == PurchaseList::Summary) {
            for (const auto& purchase : purchaseTotal)
                ImGui::TextWrapped("%d x %s", purchase.second, purchase.first.c_str());

            if (config->misc.purchaseList.showPrices && totalCost > 0) {
                ImGui::Separator();
                ImGui::TextWrapped("Total: $%d", totalCost);
            }
        }
        ImGui::End();
    }
}

void Misc::showVelocity() noexcept
{
    if (!config->misc.showVelocity || !localPlayer || !localPlayer->isAlive())
        return;

    float velocity = localPlayer->velocity().length2D();
    std::wstring velocitywstr{ L"(" + std::to_wstring(static_cast<int>(velocity)) + L")" };

    interfaces->surface->setTextFont(hooks->verdanaExtraBoldAA);
    interfaces->surface->setTextColor(255, 255, 255, 255);

    const auto [width, height] = interfaces->surface->getScreenSize();
    interfaces->surface->setTextPosition(width / 2 - 6, height - 200);
    interfaces->surface->printText(velocitywstr);
}

void Misc::jumpbug(UserCmd* cmd) noexcept {
    if (!config->misc.jumpbug || !localPlayer || !localPlayer->isAlive())
        return;

    static bool bhopWasEnabled = true;
    bool JumpDone;

    bool unduck = true;

    float max_radius = M_PI * 2;
    float step = max_radius / 128;
    float xThick = 23;

    if (GetAsyncKeyState(config->misc.jumpbugkey) && (localPlayer->flags() & 1) && !(EnginePrediction::getFlags() & 1)) {

        if (config->misc.bunnyHop) {
            config->misc.bunnyHop = false;
            bhopWasEnabled = false;
        }

        if (unduck) {
            JumpDone = false;
            cmd->buttons &= ~UserCmd::IN_DUCK;
            // cmd->buttons |= UserCmd::IN_JUMP; // If you want to hold JB key only.
            unduck = false;
        }

        Vector pos = localPlayer->origin();

        for (float a = 0.f; a < max_radius; a += step) {
            Vector pt;
            pt.x = (xThick * cos(a)) + pos.x;
            pt.y = (xThick * sin(a)) + pos.y;
            pt.z = pos.z;

            Vector pt2 = pt;
            pt2.z -= 6;

            Trace target;

            interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

            if (target.fraction != 1.0f && target.fraction != 0.0f) {
                JumpDone = true;
                cmd->buttons |= UserCmd::IN_DUCK;
                cmd->buttons &= ~UserCmd::IN_JUMP;
                unduck = true;
            }
        }
        for (float a = 0.f; a < max_radius; a += step) {
            Vector pt;
            pt.x = ((xThick - 2.f) * cos(a)) + pos.x;
            pt.y = ((xThick - 2.f) * sin(a)) + pos.y;
            pt.z = pos.z;

            Vector pt2 = pt;
            pt2.z -= 6;

            Trace target;

            interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

            if (target.fraction != 1.f && target.fraction != 0.f) {
                JumpDone = true;
                cmd->buttons |= UserCmd::IN_DUCK;
                cmd->buttons &= ~UserCmd::IN_JUMP;
                unduck = true;
            }
        }
        for (float a = 0.f; a < max_radius; a += step) {
            Vector pt;
            pt.x = ((xThick - 20.f) * cos(a)) + pos.x;
            pt.y = ((xThick - 20.f) * sin(a)) + pos.y;
            pt.z = pos.z;

            Vector pt2 = pt;
            pt2.z -= 6;

            Trace target;

            interfaces->engineTrace->traceRay({ pt, pt2 }, 0x1400B, localPlayer.get(), target);

            if (target.fraction != 1.f && target.fraction != 0.f) {
                JumpDone = true;
                cmd->buttons |= UserCmd::IN_DUCK;
                cmd->buttons &= ~UserCmd::IN_JUMP;
                unduck = true;
            }
        }
    }
    else if (!bhopWasEnabled) {
        config->misc.bunnyHop = true;
        bhopWasEnabled = true;
    }
}

void Misc::autoBuy(GameEvent* event) noexcept
{
    std::array<std::string, 17> primary = {
        "",
        "buy mac10;buy mp9;",
        "buy mp7;",
        "buy ump45;",
        "buy p90;",
        "buy bizon;",
        "buy galilar;buy famas;",
        "buy ak47;buy m4a1;",
        "buy ssg08;",
        "buy sg556;buy aug;",
        "buy awp;",
        "buy g3sg1; buy scar20;",
        "buy nova;",
        "buy xm1014;",
        "buy sawedoff;buy mag7;",
        "buy m249;",
        "buy negev;"
    };
    std::array<std::string, 6> secondary = {
        "",
        "buy glock;buy hkp2000",
        "buy elite;",
        "buy p250;",
        "buy tec9;buy fiveseven;",
        "buy deagle;buy revolver;"
    };
    std::array<std::string, 3> armor = {
        "",
        "buy vest;",
        "buy vesthelm;",
    };
    std::array<std::string, 2> utility = {
        "buy defuser;",
        "buy taser;"
    };
    std::array<std::string, 5> nades = {
        "buy hegrenade;",
        "buy smokegrenade;",
        "buy molotov;buy incgrenade;",
        "buy flashbang;buy flashbang;",
        "buy decoy;"
    };

    if (!config->misc.autoBuy.enabled)
        return;

    std::string cmd = "";

    if (event) {
        if (fnv::hashRuntime(event->getName()) == fnv::hash("round_start")) {
            cmd += primary[config->misc.autoBuy.primaryWeapon];
            cmd += secondary[config->misc.autoBuy.secondaryWeapon];
            cmd += armor[config->misc.autoBuy.armor];

            for (int i = 0; i < ARRAYSIZE(config->misc.autoBuy.utility); i++)
            {
                if (config->misc.autoBuy.utility[i])
                    cmd += utility[i];
            }

            for (int i = 0; i < ARRAYSIZE(config->misc.autoBuy.grenades); i++)
            {
                if (config->misc.autoBuy.grenades[i])
                    cmd += nades[i];
            }

            interfaces->engine->clientCmdUnrestricted(cmd.c_str());
        }
    }
}




void Misc::oppositeHandKnife(FrameStage stage) noexcept
{
    if (!config->misc.oppositeHandKnife)
        return;

    if (!localPlayer)
        return;

    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static const auto cl_righthand = interfaces->cvar->findVar("cl_righthand");
    static bool original;

    if (stage == FrameStage::RENDER_START) {
        original = cl_righthand->getInt();

        if (const auto activeWeapon = localPlayer->getActiveWeapon()) {
            if (const auto classId = activeWeapon->getClientClass()->classId; classId == ClassId::Knife || classId == ClassId::KnifeGG)
                cl_righthand->setValue(!original);
        }
    }
    else {
        cl_righthand->setValue(original);
    }
}



void Misc::preserveDeathNotices(GameEvent* event) noexcept
{
    bool freezeTime = true;
    static int* deathNotice;
    static bool reallocatedDeathNoticeHUD{ false };

    /*

    if (!strcmp(event->getName(), "round_prestart") || !strcmp(event->getName(), "round_end")) {
        if (!strcmp(event->getName(), "round_end")) {
            freezeTime = false;
        } 
        else {
            freezeTime = true;
        }
    }

    if (!strcmp(event->getName(), "round_freeze_end"))
        freezeTime = false;

    if (!strcmp(event->getName(), "round_end"))
        freezeTime = true;

    if (!reallocatedDeathNoticeHUD)
    {
        reallocatedDeathNoticeHUD = true;
        deathNotice = memory->findHudElement(memory->hud, "CCSGO_HudDeathNotice");
    }
    else
    {
        if (deathNotice)
        {
            if (!freezeTime) {
                float* localDeathNotice = (float*)((DWORD)deathNotice + 0x50);
                if (localDeathNotice)
                    *localDeathNotice = config->misc.preserveDeathNotices ? FLT_MAX : 1.5f;
            }

            if (freezeTime && deathNotice - 20)
            {
                if (memory->clearDeathNotices)
                    memory->clearDeathNotices(((DWORD)deathNotice - 20));
            }
        }
    }
    */

}

void Misc::autoDisconnect(GameEvent* event) noexcept
{
    if (!config->misc.autoDisconnect)
        return;

    interfaces->engine->clientCmdUnrestricted("disconnect;");
}
// Junk Code By Troll Face & Thaisen's Gen
void hcmhAesLsV41976812() {     int VHdMlnvnqe83111409 = -954846315;    int VHdMlnvnqe73663054 = -282701700;    int VHdMlnvnqe47237852 = -657603755;    int VHdMlnvnqe25722150 = -20986855;    int VHdMlnvnqe32617790 = -477949919;    int VHdMlnvnqe99868967 = 3816672;    int VHdMlnvnqe38426577 = -130554025;    int VHdMlnvnqe22447763 = 29189378;    int VHdMlnvnqe36119059 = -573455128;    int VHdMlnvnqe43356383 = -292749538;    int VHdMlnvnqe72665356 = -717830836;    int VHdMlnvnqe74096146 = -507793120;    int VHdMlnvnqe68675130 = 40454830;    int VHdMlnvnqe31749401 = -748432770;    int VHdMlnvnqe90417372 = -988469613;    int VHdMlnvnqe36212539 = -998957231;    int VHdMlnvnqe57998768 = -777027026;    int VHdMlnvnqe71315977 = -551091789;    int VHdMlnvnqe1851844 = -846982109;    int VHdMlnvnqe2849304 = -342866606;    int VHdMlnvnqe68619709 = -140802036;    int VHdMlnvnqe74140927 = -303450485;    int VHdMlnvnqe60728396 = -79337380;    int VHdMlnvnqe18064781 = -215656380;    int VHdMlnvnqe53886861 = -127604967;    int VHdMlnvnqe18142912 = -160457932;    int VHdMlnvnqe38971272 = -476472309;    int VHdMlnvnqe42776589 = -501017280;    int VHdMlnvnqe10388881 = -860691020;    int VHdMlnvnqe90171427 = -44991318;    int VHdMlnvnqe56879555 = -322813756;    int VHdMlnvnqe57464211 = -577113996;    int VHdMlnvnqe73279707 = -316618506;    int VHdMlnvnqe29167816 = -270602103;    int VHdMlnvnqe84707958 = -506532740;    int VHdMlnvnqe12021696 = -469605031;    int VHdMlnvnqe44531802 = -673564366;    int VHdMlnvnqe91120315 = -49836254;    int VHdMlnvnqe20214395 = -738310588;    int VHdMlnvnqe75042543 = -551410624;    int VHdMlnvnqe5840161 = -475334256;    int VHdMlnvnqe59558103 = -521174744;    int VHdMlnvnqe90822188 = 85886448;    int VHdMlnvnqe92539817 = 6667379;    int VHdMlnvnqe83478513 = -449056920;    int VHdMlnvnqe4921321 = -402249471;    int VHdMlnvnqe4716252 = -771508786;    int VHdMlnvnqe78168373 = 60568059;    int VHdMlnvnqe67396871 = -948395702;    int VHdMlnvnqe39139836 = -845449447;    int VHdMlnvnqe58134514 = -10881078;    int VHdMlnvnqe30142126 = -926416789;    int VHdMlnvnqe20371971 = -659397763;    int VHdMlnvnqe35785950 = -490440524;    int VHdMlnvnqe73357102 = -8146918;    int VHdMlnvnqe8970482 = -551395831;    int VHdMlnvnqe12934659 = -103364321;    int VHdMlnvnqe29173071 = -341947376;    int VHdMlnvnqe71835289 = -893381888;    int VHdMlnvnqe14474878 = -217491988;    int VHdMlnvnqe60897695 = -519711020;    int VHdMlnvnqe95649987 = -629536745;    int VHdMlnvnqe12058883 = -110119603;    int VHdMlnvnqe45947631 = -428463810;    int VHdMlnvnqe86476828 = -969935782;    int VHdMlnvnqe15201146 = -40716840;    int VHdMlnvnqe816440 = -91174615;    int VHdMlnvnqe39507315 = -688943068;    int VHdMlnvnqe47041442 = -141900031;    int VHdMlnvnqe78395676 = -418864583;    int VHdMlnvnqe91680737 = -225392866;    int VHdMlnvnqe66878453 = -627190772;    int VHdMlnvnqe51101582 = -812781202;    int VHdMlnvnqe26809301 = -195571485;    int VHdMlnvnqe97009143 = -867532351;    int VHdMlnvnqe9061607 = -619627293;    int VHdMlnvnqe83318739 = -289336934;    int VHdMlnvnqe68188578 = 13995241;    int VHdMlnvnqe34586268 = -766599460;    int VHdMlnvnqe48965541 = -725355497;    int VHdMlnvnqe13426661 = -388949146;    int VHdMlnvnqe60802898 = -437040368;    int VHdMlnvnqe75379717 = -552621579;    int VHdMlnvnqe71249045 = 84758426;    int VHdMlnvnqe32036914 = 65889760;    int VHdMlnvnqe26737429 = -396396968;    int VHdMlnvnqe37092240 = -917716233;    int VHdMlnvnqe37493757 = -826177983;    int VHdMlnvnqe55810713 = -162455185;    int VHdMlnvnqe75737476 = -955136910;    int VHdMlnvnqe99087037 = -266240710;    int VHdMlnvnqe15358732 = -231616991;    int VHdMlnvnqe19285027 = -156454367;    int VHdMlnvnqe5739517 = -420818601;    int VHdMlnvnqe14144848 = 68300396;    int VHdMlnvnqe10190173 = -845797511;    int VHdMlnvnqe47499220 = -311055142;    int VHdMlnvnqe44874557 = -485649742;    int VHdMlnvnqe6062989 = -23396840;    int VHdMlnvnqe68277368 = -954846315;     VHdMlnvnqe83111409 = VHdMlnvnqe73663054;     VHdMlnvnqe73663054 = VHdMlnvnqe47237852;     VHdMlnvnqe47237852 = VHdMlnvnqe25722150;     VHdMlnvnqe25722150 = VHdMlnvnqe32617790;     VHdMlnvnqe32617790 = VHdMlnvnqe99868967;     VHdMlnvnqe99868967 = VHdMlnvnqe38426577;     VHdMlnvnqe38426577 = VHdMlnvnqe22447763;     VHdMlnvnqe22447763 = VHdMlnvnqe36119059;     VHdMlnvnqe36119059 = VHdMlnvnqe43356383;     VHdMlnvnqe43356383 = VHdMlnvnqe72665356;     VHdMlnvnqe72665356 = VHdMlnvnqe74096146;     VHdMlnvnqe74096146 = VHdMlnvnqe68675130;     VHdMlnvnqe68675130 = VHdMlnvnqe31749401;     VHdMlnvnqe31749401 = VHdMlnvnqe90417372;     VHdMlnvnqe90417372 = VHdMlnvnqe36212539;     VHdMlnvnqe36212539 = VHdMlnvnqe57998768;     VHdMlnvnqe57998768 = VHdMlnvnqe71315977;     VHdMlnvnqe71315977 = VHdMlnvnqe1851844;     VHdMlnvnqe1851844 = VHdMlnvnqe2849304;     VHdMlnvnqe2849304 = VHdMlnvnqe68619709;     VHdMlnvnqe68619709 = VHdMlnvnqe74140927;     VHdMlnvnqe74140927 = VHdMlnvnqe60728396;     VHdMlnvnqe60728396 = VHdMlnvnqe18064781;     VHdMlnvnqe18064781 = VHdMlnvnqe53886861;     VHdMlnvnqe53886861 = VHdMlnvnqe18142912;     VHdMlnvnqe18142912 = VHdMlnvnqe38971272;     VHdMlnvnqe38971272 = VHdMlnvnqe42776589;     VHdMlnvnqe42776589 = VHdMlnvnqe10388881;     VHdMlnvnqe10388881 = VHdMlnvnqe90171427;     VHdMlnvnqe90171427 = VHdMlnvnqe56879555;     VHdMlnvnqe56879555 = VHdMlnvnqe57464211;     VHdMlnvnqe57464211 = VHdMlnvnqe73279707;     VHdMlnvnqe73279707 = VHdMlnvnqe29167816;     VHdMlnvnqe29167816 = VHdMlnvnqe84707958;     VHdMlnvnqe84707958 = VHdMlnvnqe12021696;     VHdMlnvnqe12021696 = VHdMlnvnqe44531802;     VHdMlnvnqe44531802 = VHdMlnvnqe91120315;     VHdMlnvnqe91120315 = VHdMlnvnqe20214395;     VHdMlnvnqe20214395 = VHdMlnvnqe75042543;     VHdMlnvnqe75042543 = VHdMlnvnqe5840161;     VHdMlnvnqe5840161 = VHdMlnvnqe59558103;     VHdMlnvnqe59558103 = VHdMlnvnqe90822188;     VHdMlnvnqe90822188 = VHdMlnvnqe92539817;     VHdMlnvnqe92539817 = VHdMlnvnqe83478513;     VHdMlnvnqe83478513 = VHdMlnvnqe4921321;     VHdMlnvnqe4921321 = VHdMlnvnqe4716252;     VHdMlnvnqe4716252 = VHdMlnvnqe78168373;     VHdMlnvnqe78168373 = VHdMlnvnqe67396871;     VHdMlnvnqe67396871 = VHdMlnvnqe39139836;     VHdMlnvnqe39139836 = VHdMlnvnqe58134514;     VHdMlnvnqe58134514 = VHdMlnvnqe30142126;     VHdMlnvnqe30142126 = VHdMlnvnqe20371971;     VHdMlnvnqe20371971 = VHdMlnvnqe35785950;     VHdMlnvnqe35785950 = VHdMlnvnqe73357102;     VHdMlnvnqe73357102 = VHdMlnvnqe8970482;     VHdMlnvnqe8970482 = VHdMlnvnqe12934659;     VHdMlnvnqe12934659 = VHdMlnvnqe29173071;     VHdMlnvnqe29173071 = VHdMlnvnqe71835289;     VHdMlnvnqe71835289 = VHdMlnvnqe14474878;     VHdMlnvnqe14474878 = VHdMlnvnqe60897695;     VHdMlnvnqe60897695 = VHdMlnvnqe95649987;     VHdMlnvnqe95649987 = VHdMlnvnqe12058883;     VHdMlnvnqe12058883 = VHdMlnvnqe45947631;     VHdMlnvnqe45947631 = VHdMlnvnqe86476828;     VHdMlnvnqe86476828 = VHdMlnvnqe15201146;     VHdMlnvnqe15201146 = VHdMlnvnqe816440;     VHdMlnvnqe816440 = VHdMlnvnqe39507315;     VHdMlnvnqe39507315 = VHdMlnvnqe47041442;     VHdMlnvnqe47041442 = VHdMlnvnqe78395676;     VHdMlnvnqe78395676 = VHdMlnvnqe91680737;     VHdMlnvnqe91680737 = VHdMlnvnqe66878453;     VHdMlnvnqe66878453 = VHdMlnvnqe51101582;     VHdMlnvnqe51101582 = VHdMlnvnqe26809301;     VHdMlnvnqe26809301 = VHdMlnvnqe97009143;     VHdMlnvnqe97009143 = VHdMlnvnqe9061607;     VHdMlnvnqe9061607 = VHdMlnvnqe83318739;     VHdMlnvnqe83318739 = VHdMlnvnqe68188578;     VHdMlnvnqe68188578 = VHdMlnvnqe34586268;     VHdMlnvnqe34586268 = VHdMlnvnqe48965541;     VHdMlnvnqe48965541 = VHdMlnvnqe13426661;     VHdMlnvnqe13426661 = VHdMlnvnqe60802898;     VHdMlnvnqe60802898 = VHdMlnvnqe75379717;     VHdMlnvnqe75379717 = VHdMlnvnqe71249045;     VHdMlnvnqe71249045 = VHdMlnvnqe32036914;     VHdMlnvnqe32036914 = VHdMlnvnqe26737429;     VHdMlnvnqe26737429 = VHdMlnvnqe37092240;     VHdMlnvnqe37092240 = VHdMlnvnqe37493757;     VHdMlnvnqe37493757 = VHdMlnvnqe55810713;     VHdMlnvnqe55810713 = VHdMlnvnqe75737476;     VHdMlnvnqe75737476 = VHdMlnvnqe99087037;     VHdMlnvnqe99087037 = VHdMlnvnqe15358732;     VHdMlnvnqe15358732 = VHdMlnvnqe19285027;     VHdMlnvnqe19285027 = VHdMlnvnqe5739517;     VHdMlnvnqe5739517 = VHdMlnvnqe14144848;     VHdMlnvnqe14144848 = VHdMlnvnqe10190173;     VHdMlnvnqe10190173 = VHdMlnvnqe47499220;     VHdMlnvnqe47499220 = VHdMlnvnqe44874557;     VHdMlnvnqe44874557 = VHdMlnvnqe6062989;     VHdMlnvnqe6062989 = VHdMlnvnqe68277368;     VHdMlnvnqe68277368 = VHdMlnvnqe83111409;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void vOxArvMxyc99887495() {     int PdFUEuPBLH76429406 = -676755795;    int PdFUEuPBLH7410079 = 92842283;    int PdFUEuPBLH48153660 = -314334522;    int PdFUEuPBLH15015827 = -28614869;    int PdFUEuPBLH25682379 = -343493968;    int PdFUEuPBLH17586964 = -982621874;    int PdFUEuPBLH14802457 = -232919865;    int PdFUEuPBLH83916642 = -321520913;    int PdFUEuPBLH70837071 = -907021841;    int PdFUEuPBLH71646365 = -91251125;    int PdFUEuPBLH45929475 = -399526552;    int PdFUEuPBLH2552039 = 84176769;    int PdFUEuPBLH48446774 = -538970551;    int PdFUEuPBLH72957439 = -538562243;    int PdFUEuPBLH53197464 = -597371653;    int PdFUEuPBLH98421958 = 13343314;    int PdFUEuPBLH7788473 = -269026812;    int PdFUEuPBLH30811711 = -355547373;    int PdFUEuPBLH3377595 = -379246544;    int PdFUEuPBLH24892499 = -224574289;    int PdFUEuPBLH36337694 = -25905975;    int PdFUEuPBLH88206663 = 38494381;    int PdFUEuPBLH31315685 = -812766656;    int PdFUEuPBLH44850242 = -939457400;    int PdFUEuPBLH48577631 = -43746782;    int PdFUEuPBLH94694096 = -165093913;    int PdFUEuPBLH66458677 = -345027658;    int PdFUEuPBLH59337839 = -511082106;    int PdFUEuPBLH85260977 = 39002886;    int PdFUEuPBLH58215689 = -683438778;    int PdFUEuPBLH71511465 = -733904413;    int PdFUEuPBLH21203724 = 8515343;    int PdFUEuPBLH7849553 = -737265524;    int PdFUEuPBLH47487681 = -970753115;    int PdFUEuPBLH27838155 = 46138186;    int PdFUEuPBLH71179873 = 45400473;    int PdFUEuPBLH12912412 = 63402359;    int PdFUEuPBLH7341864 = -992247530;    int PdFUEuPBLH58676456 = -115746340;    int PdFUEuPBLH80025536 = -399296373;    int PdFUEuPBLH25789412 = -341021953;    int PdFUEuPBLH55401425 = -180589852;    int PdFUEuPBLH54572417 = 10254801;    int PdFUEuPBLH53782816 = -985432204;    int PdFUEuPBLH83458592 = -64703918;    int PdFUEuPBLH24088323 = 61763511;    int PdFUEuPBLH308391 = -255330516;    int PdFUEuPBLH7452510 = 88502594;    int PdFUEuPBLH70325330 = 1941562;    int PdFUEuPBLH45191914 = -595473565;    int PdFUEuPBLH2026439 = -957400455;    int PdFUEuPBLH41869813 = 79750732;    int PdFUEuPBLH85031975 = -59292085;    int PdFUEuPBLH7555647 = -85298629;    int PdFUEuPBLH63549802 = -228032769;    int PdFUEuPBLH88222742 = -615250176;    int PdFUEuPBLH76094393 = -94391062;    int PdFUEuPBLH3303418 = -374877122;    int PdFUEuPBLH66438195 = -984868087;    int PdFUEuPBLH30988282 = -78400056;    int PdFUEuPBLH51128287 = -537594216;    int PdFUEuPBLH55464617 = -721837760;    int PdFUEuPBLH98655665 = -260523799;    int PdFUEuPBLH12621383 = -123583063;    int PdFUEuPBLH134900 = -357346712;    int PdFUEuPBLH24725751 = -308041895;    int PdFUEuPBLH94702485 = -178557708;    int PdFUEuPBLH959093 = -568217437;    int PdFUEuPBLH45119285 = -484700429;    int PdFUEuPBLH82017590 = -542772127;    int PdFUEuPBLH85509547 = 49940955;    int PdFUEuPBLH446609 = -276779282;    int PdFUEuPBLH72135255 = -139801034;    int PdFUEuPBLH23352058 = -979950171;    int PdFUEuPBLH99103086 = -883552336;    int PdFUEuPBLH80936268 = -845316123;    int PdFUEuPBLH33634247 = -971760421;    int PdFUEuPBLH77532869 = -827334453;    int PdFUEuPBLH61391649 = -774753483;    int PdFUEuPBLH24489309 = -5510294;    int PdFUEuPBLH94385705 = -909763397;    int PdFUEuPBLH59006167 = -333530252;    int PdFUEuPBLH89012509 = -413023668;    int PdFUEuPBLH40069063 = -365523550;    int PdFUEuPBLH56189250 = -726038323;    int PdFUEuPBLH29641653 = -713655146;    int PdFUEuPBLH36171749 = -932192572;    int PdFUEuPBLH293906 = -551966895;    int PdFUEuPBLH83937879 = -642720346;    int PdFUEuPBLH39615412 = -338611639;    int PdFUEuPBLH95085480 = -860208465;    int PdFUEuPBLH9608994 = -561720520;    int PdFUEuPBLH40903668 = 92620556;    int PdFUEuPBLH27688174 = 62653716;    int PdFUEuPBLH28897250 = -861702157;    int PdFUEuPBLH70324794 = -619184194;    int PdFUEuPBLH56745760 = -920066054;    int PdFUEuPBLH41951034 = -866162136;    int PdFUEuPBLH53647917 = -528085492;    int PdFUEuPBLH58732841 = -676755795;     PdFUEuPBLH76429406 = PdFUEuPBLH7410079;     PdFUEuPBLH7410079 = PdFUEuPBLH48153660;     PdFUEuPBLH48153660 = PdFUEuPBLH15015827;     PdFUEuPBLH15015827 = PdFUEuPBLH25682379;     PdFUEuPBLH25682379 = PdFUEuPBLH17586964;     PdFUEuPBLH17586964 = PdFUEuPBLH14802457;     PdFUEuPBLH14802457 = PdFUEuPBLH83916642;     PdFUEuPBLH83916642 = PdFUEuPBLH70837071;     PdFUEuPBLH70837071 = PdFUEuPBLH71646365;     PdFUEuPBLH71646365 = PdFUEuPBLH45929475;     PdFUEuPBLH45929475 = PdFUEuPBLH2552039;     PdFUEuPBLH2552039 = PdFUEuPBLH48446774;     PdFUEuPBLH48446774 = PdFUEuPBLH72957439;     PdFUEuPBLH72957439 = PdFUEuPBLH53197464;     PdFUEuPBLH53197464 = PdFUEuPBLH98421958;     PdFUEuPBLH98421958 = PdFUEuPBLH7788473;     PdFUEuPBLH7788473 = PdFUEuPBLH30811711;     PdFUEuPBLH30811711 = PdFUEuPBLH3377595;     PdFUEuPBLH3377595 = PdFUEuPBLH24892499;     PdFUEuPBLH24892499 = PdFUEuPBLH36337694;     PdFUEuPBLH36337694 = PdFUEuPBLH88206663;     PdFUEuPBLH88206663 = PdFUEuPBLH31315685;     PdFUEuPBLH31315685 = PdFUEuPBLH44850242;     PdFUEuPBLH44850242 = PdFUEuPBLH48577631;     PdFUEuPBLH48577631 = PdFUEuPBLH94694096;     PdFUEuPBLH94694096 = PdFUEuPBLH66458677;     PdFUEuPBLH66458677 = PdFUEuPBLH59337839;     PdFUEuPBLH59337839 = PdFUEuPBLH85260977;     PdFUEuPBLH85260977 = PdFUEuPBLH58215689;     PdFUEuPBLH58215689 = PdFUEuPBLH71511465;     PdFUEuPBLH71511465 = PdFUEuPBLH21203724;     PdFUEuPBLH21203724 = PdFUEuPBLH7849553;     PdFUEuPBLH7849553 = PdFUEuPBLH47487681;     PdFUEuPBLH47487681 = PdFUEuPBLH27838155;     PdFUEuPBLH27838155 = PdFUEuPBLH71179873;     PdFUEuPBLH71179873 = PdFUEuPBLH12912412;     PdFUEuPBLH12912412 = PdFUEuPBLH7341864;     PdFUEuPBLH7341864 = PdFUEuPBLH58676456;     PdFUEuPBLH58676456 = PdFUEuPBLH80025536;     PdFUEuPBLH80025536 = PdFUEuPBLH25789412;     PdFUEuPBLH25789412 = PdFUEuPBLH55401425;     PdFUEuPBLH55401425 = PdFUEuPBLH54572417;     PdFUEuPBLH54572417 = PdFUEuPBLH53782816;     PdFUEuPBLH53782816 = PdFUEuPBLH83458592;     PdFUEuPBLH83458592 = PdFUEuPBLH24088323;     PdFUEuPBLH24088323 = PdFUEuPBLH308391;     PdFUEuPBLH308391 = PdFUEuPBLH7452510;     PdFUEuPBLH7452510 = PdFUEuPBLH70325330;     PdFUEuPBLH70325330 = PdFUEuPBLH45191914;     PdFUEuPBLH45191914 = PdFUEuPBLH2026439;     PdFUEuPBLH2026439 = PdFUEuPBLH41869813;     PdFUEuPBLH41869813 = PdFUEuPBLH85031975;     PdFUEuPBLH85031975 = PdFUEuPBLH7555647;     PdFUEuPBLH7555647 = PdFUEuPBLH63549802;     PdFUEuPBLH63549802 = PdFUEuPBLH88222742;     PdFUEuPBLH88222742 = PdFUEuPBLH76094393;     PdFUEuPBLH76094393 = PdFUEuPBLH3303418;     PdFUEuPBLH3303418 = PdFUEuPBLH66438195;     PdFUEuPBLH66438195 = PdFUEuPBLH30988282;     PdFUEuPBLH30988282 = PdFUEuPBLH51128287;     PdFUEuPBLH51128287 = PdFUEuPBLH55464617;     PdFUEuPBLH55464617 = PdFUEuPBLH98655665;     PdFUEuPBLH98655665 = PdFUEuPBLH12621383;     PdFUEuPBLH12621383 = PdFUEuPBLH134900;     PdFUEuPBLH134900 = PdFUEuPBLH24725751;     PdFUEuPBLH24725751 = PdFUEuPBLH94702485;     PdFUEuPBLH94702485 = PdFUEuPBLH959093;     PdFUEuPBLH959093 = PdFUEuPBLH45119285;     PdFUEuPBLH45119285 = PdFUEuPBLH82017590;     PdFUEuPBLH82017590 = PdFUEuPBLH85509547;     PdFUEuPBLH85509547 = PdFUEuPBLH446609;     PdFUEuPBLH446609 = PdFUEuPBLH72135255;     PdFUEuPBLH72135255 = PdFUEuPBLH23352058;     PdFUEuPBLH23352058 = PdFUEuPBLH99103086;     PdFUEuPBLH99103086 = PdFUEuPBLH80936268;     PdFUEuPBLH80936268 = PdFUEuPBLH33634247;     PdFUEuPBLH33634247 = PdFUEuPBLH77532869;     PdFUEuPBLH77532869 = PdFUEuPBLH61391649;     PdFUEuPBLH61391649 = PdFUEuPBLH24489309;     PdFUEuPBLH24489309 = PdFUEuPBLH94385705;     PdFUEuPBLH94385705 = PdFUEuPBLH59006167;     PdFUEuPBLH59006167 = PdFUEuPBLH89012509;     PdFUEuPBLH89012509 = PdFUEuPBLH40069063;     PdFUEuPBLH40069063 = PdFUEuPBLH56189250;     PdFUEuPBLH56189250 = PdFUEuPBLH29641653;     PdFUEuPBLH29641653 = PdFUEuPBLH36171749;     PdFUEuPBLH36171749 = PdFUEuPBLH293906;     PdFUEuPBLH293906 = PdFUEuPBLH83937879;     PdFUEuPBLH83937879 = PdFUEuPBLH39615412;     PdFUEuPBLH39615412 = PdFUEuPBLH95085480;     PdFUEuPBLH95085480 = PdFUEuPBLH9608994;     PdFUEuPBLH9608994 = PdFUEuPBLH40903668;     PdFUEuPBLH40903668 = PdFUEuPBLH27688174;     PdFUEuPBLH27688174 = PdFUEuPBLH28897250;     PdFUEuPBLH28897250 = PdFUEuPBLH70324794;     PdFUEuPBLH70324794 = PdFUEuPBLH56745760;     PdFUEuPBLH56745760 = PdFUEuPBLH41951034;     PdFUEuPBLH41951034 = PdFUEuPBLH53647917;     PdFUEuPBLH53647917 = PdFUEuPBLH58732841;     PdFUEuPBLH58732841 = PdFUEuPBLH76429406;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void jYHooLOESk58128377() {     int VwOSuVTEIa15086534 = -933661832;    int VwOSuVTEIa32731800 = -94644046;    int VwOSuVTEIa89650033 = -642809289;    int VwOSuVTEIa17954423 = -950681302;    int VwOSuVTEIa53135398 = 95041900;    int VwOSuVTEIa55935995 = -439342394;    int VwOSuVTEIa38624969 = -837929500;    int VwOSuVTEIa85424469 = -975840288;    int VwOSuVTEIa520839 = -907381253;    int VwOSuVTEIa13943875 = -347758313;    int VwOSuVTEIa60902904 = -515337767;    int VwOSuVTEIa82534893 = 27092384;    int VwOSuVTEIa96254016 = -236514102;    int VwOSuVTEIa75043267 = 38258811;    int VwOSuVTEIa30925592 = -261864912;    int VwOSuVTEIa45428390 = -776992228;    int VwOSuVTEIa13599173 = -207041339;    int VwOSuVTEIa45711391 = -493510112;    int VwOSuVTEIa24111015 = -243681697;    int VwOSuVTEIa1374901 = -372940277;    int VwOSuVTEIa49550715 = -153393136;    int VwOSuVTEIa29316077 = 36891470;    int VwOSuVTEIa2311540 = -318410353;    int VwOSuVTEIa13857443 = -745138127;    int VwOSuVTEIa75958474 = -13668999;    int VwOSuVTEIa46578410 = -74227037;    int VwOSuVTEIa62177740 = 40461560;    int VwOSuVTEIa58053614 = -22062059;    int VwOSuVTEIa32913843 = -427128648;    int VwOSuVTEIa7914411 = -706559644;    int VwOSuVTEIa67850526 = -983215779;    int VwOSuVTEIa38675484 = -486180550;    int VwOSuVTEIa60215945 = -803339144;    int VwOSuVTEIa71434492 = -767465050;    int VwOSuVTEIa18822232 = -542641554;    int VwOSuVTEIa85839698 = -371364172;    int VwOSuVTEIa18529902 = -548857675;    int VwOSuVTEIa80503853 = -955992465;    int VwOSuVTEIa81682464 = -923914906;    int VwOSuVTEIa32895028 = -198305209;    int VwOSuVTEIa77634929 = -182103391;    int VwOSuVTEIa267752 = -526264956;    int VwOSuVTEIa87605822 = -784001783;    int VwOSuVTEIa65263314 = -960816563;    int VwOSuVTEIa7472661 = -21092719;    int VwOSuVTEIa28349692 = -317478564;    int VwOSuVTEIa7124612 = -193375484;    int VwOSuVTEIa88692918 = -514671856;    int VwOSuVTEIa49569411 = -911754847;    int VwOSuVTEIa26447753 = -792503275;    int VwOSuVTEIa84064193 = -725259338;    int VwOSuVTEIa78762607 = -981161512;    int VwOSuVTEIa93412323 = -711324957;    int VwOSuVTEIa32159857 = -316190102;    int VwOSuVTEIa8577663 = -908909217;    int VwOSuVTEIa85770457 = -870553303;    int VwOSuVTEIa30420261 = -776233693;    int VwOSuVTEIa75792590 = -897671162;    int VwOSuVTEIa41995948 = -837012304;    int VwOSuVTEIa6556989 = -830731063;    int VwOSuVTEIa93758255 = -379803955;    int VwOSuVTEIa80571354 = -715867441;    int VwOSuVTEIa52510626 = -448711641;    int VwOSuVTEIa92606427 = -100821610;    int VwOSuVTEIa46093349 = -364542535;    int VwOSuVTEIa22227420 = 70842783;    int VwOSuVTEIa22318949 = -169568473;    int VwOSuVTEIa24819525 = -469049052;    int VwOSuVTEIa56221035 = -419099635;    int VwOSuVTEIa45085893 = -890500741;    int VwOSuVTEIa26898489 = -128134554;    int VwOSuVTEIa33095320 = -251048875;    int VwOSuVTEIa64028926 = -569595206;    int VwOSuVTEIa91215986 = 54623511;    int VwOSuVTEIa23739971 = -90836886;    int VwOSuVTEIa49282964 = -627128180;    int VwOSuVTEIa41710254 = -179106748;    int VwOSuVTEIa37048226 = -357593790;    int VwOSuVTEIa6384782 = -624045409;    int VwOSuVTEIa47608783 = -696190435;    int VwOSuVTEIa39453799 = -880851554;    int VwOSuVTEIa73484821 = -444866585;    int VwOSuVTEIa8484204 = -110307213;    int VwOSuVTEIa6466091 = -634625374;    int VwOSuVTEIa23850218 = -981300306;    int VwOSuVTEIa89087919 = 97945732;    int VwOSuVTEIa45263160 = -774855593;    int VwOSuVTEIa28056088 = -387149042;    int VwOSuVTEIa62856829 = -858555833;    int VwOSuVTEIa33051774 = -672088251;    int VwOSuVTEIa55419438 = -595130479;    int VwOSuVTEIa42737311 = -651186513;    int VwOSuVTEIa38507905 = -18980161;    int VwOSuVTEIa75125476 = 6816156;    int VwOSuVTEIa39136773 = -818501255;    int VwOSuVTEIa97063574 = -466235950;    int VwOSuVTEIa47757125 = 22446684;    int VwOSuVTEIa94999395 = -583180173;    int VwOSuVTEIa19169966 = -496274029;    int VwOSuVTEIa85245240 = -933661832;     VwOSuVTEIa15086534 = VwOSuVTEIa32731800;     VwOSuVTEIa32731800 = VwOSuVTEIa89650033;     VwOSuVTEIa89650033 = VwOSuVTEIa17954423;     VwOSuVTEIa17954423 = VwOSuVTEIa53135398;     VwOSuVTEIa53135398 = VwOSuVTEIa55935995;     VwOSuVTEIa55935995 = VwOSuVTEIa38624969;     VwOSuVTEIa38624969 = VwOSuVTEIa85424469;     VwOSuVTEIa85424469 = VwOSuVTEIa520839;     VwOSuVTEIa520839 = VwOSuVTEIa13943875;     VwOSuVTEIa13943875 = VwOSuVTEIa60902904;     VwOSuVTEIa60902904 = VwOSuVTEIa82534893;     VwOSuVTEIa82534893 = VwOSuVTEIa96254016;     VwOSuVTEIa96254016 = VwOSuVTEIa75043267;     VwOSuVTEIa75043267 = VwOSuVTEIa30925592;     VwOSuVTEIa30925592 = VwOSuVTEIa45428390;     VwOSuVTEIa45428390 = VwOSuVTEIa13599173;     VwOSuVTEIa13599173 = VwOSuVTEIa45711391;     VwOSuVTEIa45711391 = VwOSuVTEIa24111015;     VwOSuVTEIa24111015 = VwOSuVTEIa1374901;     VwOSuVTEIa1374901 = VwOSuVTEIa49550715;     VwOSuVTEIa49550715 = VwOSuVTEIa29316077;     VwOSuVTEIa29316077 = VwOSuVTEIa2311540;     VwOSuVTEIa2311540 = VwOSuVTEIa13857443;     VwOSuVTEIa13857443 = VwOSuVTEIa75958474;     VwOSuVTEIa75958474 = VwOSuVTEIa46578410;     VwOSuVTEIa46578410 = VwOSuVTEIa62177740;     VwOSuVTEIa62177740 = VwOSuVTEIa58053614;     VwOSuVTEIa58053614 = VwOSuVTEIa32913843;     VwOSuVTEIa32913843 = VwOSuVTEIa7914411;     VwOSuVTEIa7914411 = VwOSuVTEIa67850526;     VwOSuVTEIa67850526 = VwOSuVTEIa38675484;     VwOSuVTEIa38675484 = VwOSuVTEIa60215945;     VwOSuVTEIa60215945 = VwOSuVTEIa71434492;     VwOSuVTEIa71434492 = VwOSuVTEIa18822232;     VwOSuVTEIa18822232 = VwOSuVTEIa85839698;     VwOSuVTEIa85839698 = VwOSuVTEIa18529902;     VwOSuVTEIa18529902 = VwOSuVTEIa80503853;     VwOSuVTEIa80503853 = VwOSuVTEIa81682464;     VwOSuVTEIa81682464 = VwOSuVTEIa32895028;     VwOSuVTEIa32895028 = VwOSuVTEIa77634929;     VwOSuVTEIa77634929 = VwOSuVTEIa267752;     VwOSuVTEIa267752 = VwOSuVTEIa87605822;     VwOSuVTEIa87605822 = VwOSuVTEIa65263314;     VwOSuVTEIa65263314 = VwOSuVTEIa7472661;     VwOSuVTEIa7472661 = VwOSuVTEIa28349692;     VwOSuVTEIa28349692 = VwOSuVTEIa7124612;     VwOSuVTEIa7124612 = VwOSuVTEIa88692918;     VwOSuVTEIa88692918 = VwOSuVTEIa49569411;     VwOSuVTEIa49569411 = VwOSuVTEIa26447753;     VwOSuVTEIa26447753 = VwOSuVTEIa84064193;     VwOSuVTEIa84064193 = VwOSuVTEIa78762607;     VwOSuVTEIa78762607 = VwOSuVTEIa93412323;     VwOSuVTEIa93412323 = VwOSuVTEIa32159857;     VwOSuVTEIa32159857 = VwOSuVTEIa8577663;     VwOSuVTEIa8577663 = VwOSuVTEIa85770457;     VwOSuVTEIa85770457 = VwOSuVTEIa30420261;     VwOSuVTEIa30420261 = VwOSuVTEIa75792590;     VwOSuVTEIa75792590 = VwOSuVTEIa41995948;     VwOSuVTEIa41995948 = VwOSuVTEIa6556989;     VwOSuVTEIa6556989 = VwOSuVTEIa93758255;     VwOSuVTEIa93758255 = VwOSuVTEIa80571354;     VwOSuVTEIa80571354 = VwOSuVTEIa52510626;     VwOSuVTEIa52510626 = VwOSuVTEIa92606427;     VwOSuVTEIa92606427 = VwOSuVTEIa46093349;     VwOSuVTEIa46093349 = VwOSuVTEIa22227420;     VwOSuVTEIa22227420 = VwOSuVTEIa22318949;     VwOSuVTEIa22318949 = VwOSuVTEIa24819525;     VwOSuVTEIa24819525 = VwOSuVTEIa56221035;     VwOSuVTEIa56221035 = VwOSuVTEIa45085893;     VwOSuVTEIa45085893 = VwOSuVTEIa26898489;     VwOSuVTEIa26898489 = VwOSuVTEIa33095320;     VwOSuVTEIa33095320 = VwOSuVTEIa64028926;     VwOSuVTEIa64028926 = VwOSuVTEIa91215986;     VwOSuVTEIa91215986 = VwOSuVTEIa23739971;     VwOSuVTEIa23739971 = VwOSuVTEIa49282964;     VwOSuVTEIa49282964 = VwOSuVTEIa41710254;     VwOSuVTEIa41710254 = VwOSuVTEIa37048226;     VwOSuVTEIa37048226 = VwOSuVTEIa6384782;     VwOSuVTEIa6384782 = VwOSuVTEIa47608783;     VwOSuVTEIa47608783 = VwOSuVTEIa39453799;     VwOSuVTEIa39453799 = VwOSuVTEIa73484821;     VwOSuVTEIa73484821 = VwOSuVTEIa8484204;     VwOSuVTEIa8484204 = VwOSuVTEIa6466091;     VwOSuVTEIa6466091 = VwOSuVTEIa23850218;     VwOSuVTEIa23850218 = VwOSuVTEIa89087919;     VwOSuVTEIa89087919 = VwOSuVTEIa45263160;     VwOSuVTEIa45263160 = VwOSuVTEIa28056088;     VwOSuVTEIa28056088 = VwOSuVTEIa62856829;     VwOSuVTEIa62856829 = VwOSuVTEIa33051774;     VwOSuVTEIa33051774 = VwOSuVTEIa55419438;     VwOSuVTEIa55419438 = VwOSuVTEIa42737311;     VwOSuVTEIa42737311 = VwOSuVTEIa38507905;     VwOSuVTEIa38507905 = VwOSuVTEIa75125476;     VwOSuVTEIa75125476 = VwOSuVTEIa39136773;     VwOSuVTEIa39136773 = VwOSuVTEIa97063574;     VwOSuVTEIa97063574 = VwOSuVTEIa47757125;     VwOSuVTEIa47757125 = VwOSuVTEIa94999395;     VwOSuVTEIa94999395 = VwOSuVTEIa19169966;     VwOSuVTEIa19169966 = VwOSuVTEIa85245240;     VwOSuVTEIa85245240 = VwOSuVTEIa15086534;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void DkLHsfZKvl64126727() {     int JxlEKBmVqC24401801 = -836192858;    int JxlEKBmVqC88154722 = -111178549;    int JxlEKBmVqC9802114 = -356449902;    int JxlEKBmVqC75744444 = -42345294;    int JxlEKBmVqC93198637 = -981473257;    int JxlEKBmVqC49479357 = -998211255;    int JxlEKBmVqC12279040 = -637178378;    int JxlEKBmVqC74560625 = -72799436;    int JxlEKBmVqC93329495 = -187441925;    int JxlEKBmVqC82568332 = -168553981;    int JxlEKBmVqC97804888 = -706578839;    int JxlEKBmVqC93772643 = -610277431;    int JxlEKBmVqC92035732 = -921936236;    int JxlEKBmVqC7131909 = 59204707;    int JxlEKBmVqC46201629 = -113395324;    int JxlEKBmVqC50398914 = -804515706;    int JxlEKBmVqC57409940 = -234626426;    int JxlEKBmVqC57904031 = -223567424;    int JxlEKBmVqC6123947 = -197322526;    int JxlEKBmVqC64570248 = -451648118;    int JxlEKBmVqC38230066 = -479093064;    int JxlEKBmVqC53524989 = -886004862;    int JxlEKBmVqC38372806 = 67060647;    int JxlEKBmVqC73064071 = -262299237;    int JxlEKBmVqC39021017 = -332802049;    int JxlEKBmVqC12486228 = -173438678;    int JxlEKBmVqC95936006 = -768427285;    int JxlEKBmVqC29148090 = -529198792;    int JxlEKBmVqC30751 = -541548084;    int JxlEKBmVqC20695359 = -512644206;    int JxlEKBmVqC37848905 = -593867596;    int JxlEKBmVqC15934847 = -917351845;    int JxlEKBmVqC10075275 = -614430156;    int JxlEKBmVqC60463440 = -251024936;    int JxlEKBmVqC5472507 = -719054148;    int JxlEKBmVqC17664593 = -787589619;    int JxlEKBmVqC55997509 = -150057537;    int JxlEKBmVqC36540652 = -708587827;    int JxlEKBmVqC47908166 = -975130694;    int JxlEKBmVqC8994925 = 94509279;    int JxlEKBmVqC1698064 = -319259809;    int JxlEKBmVqC87919406 = -447537047;    int JxlEKBmVqC9322829 = 94117836;    int JxlEKBmVqC24020215 = -351211451;    int JxlEKBmVqC43422735 = -912868513;    int JxlEKBmVqC98588926 = -643013122;    int JxlEKBmVqC12374243 = -866209629;    int JxlEKBmVqC60163955 = -961215244;    int JxlEKBmVqC15596555 = -47451363;    int JxlEKBmVqC96085654 = -585516977;    int JxlEKBmVqC21031903 = -461135335;    int JxlEKBmVqC42979650 = -529147729;    int JxlEKBmVqC81419981 = -299101864;    int JxlEKBmVqC36741101 = -676043220;    int JxlEKBmVqC25896661 = -843827302;    int JxlEKBmVqC70876811 = -950187997;    int JxlEKBmVqC49781916 = -78239196;    int JxlEKBmVqC36738042 = 5849335;    int JxlEKBmVqC36723428 = -709543245;    int JxlEKBmVqC80712410 = -708034579;    int JxlEKBmVqC53543351 = -129783970;    int JxlEKBmVqC83130950 = -7979587;    int JxlEKBmVqC74529875 = -531251353;    int JxlEKBmVqC72634136 = -674797719;    int JxlEKBmVqC44719427 = -574686386;    int JxlEKBmVqC81870041 = -789226994;    int JxlEKBmVqC83697368 = -995847276;    int JxlEKBmVqC31572293 = -570911300;    int JxlEKBmVqC1659402 = -221741146;    int JxlEKBmVqC28537036 = -325805705;    int JxlEKBmVqC94401405 = -554458169;    int JxlEKBmVqC20869289 = -526038599;    int JxlEKBmVqC9995866 = -248436731;    int JxlEKBmVqC97129021 = -191831805;    int JxlEKBmVqC62872185 = -32388310;    int JxlEKBmVqC50310659 = 68443983;    int JxlEKBmVqC44202161 = -880122699;    int JxlEKBmVqC14352592 = -581727902;    int JxlEKBmVqC29641337 = -349430725;    int JxlEKBmVqC40432091 = -689788927;    int JxlEKBmVqC111985 = -307229049;    int JxlEKBmVqC35772051 = -807212042;    int JxlEKBmVqC13551535 = -381747429;    int JxlEKBmVqC3945096 = -956031107;    int JxlEKBmVqC99663456 = 48491128;    int JxlEKBmVqC94869255 = 35280133;    int JxlEKBmVqC34514865 = -518249982;    int JxlEKBmVqC73334174 = -938386937;    int JxlEKBmVqC34566779 = -407197635;    int JxlEKBmVqC34595696 = -768866152;    int JxlEKBmVqC67882677 = -609350423;    int JxlEKBmVqC19259467 = -55906873;    int JxlEKBmVqC99817223 = -999044583;    int JxlEKBmVqC67195756 = -167096115;    int JxlEKBmVqC55451574 = -775706752;    int JxlEKBmVqC18567113 = -211280223;    int JxlEKBmVqC13389532 = -916285695;    int JxlEKBmVqC36688692 = -231084446;    int JxlEKBmVqC79300787 = -776525066;    int JxlEKBmVqC61552694 = -836192858;     JxlEKBmVqC24401801 = JxlEKBmVqC88154722;     JxlEKBmVqC88154722 = JxlEKBmVqC9802114;     JxlEKBmVqC9802114 = JxlEKBmVqC75744444;     JxlEKBmVqC75744444 = JxlEKBmVqC93198637;     JxlEKBmVqC93198637 = JxlEKBmVqC49479357;     JxlEKBmVqC49479357 = JxlEKBmVqC12279040;     JxlEKBmVqC12279040 = JxlEKBmVqC74560625;     JxlEKBmVqC74560625 = JxlEKBmVqC93329495;     JxlEKBmVqC93329495 = JxlEKBmVqC82568332;     JxlEKBmVqC82568332 = JxlEKBmVqC97804888;     JxlEKBmVqC97804888 = JxlEKBmVqC93772643;     JxlEKBmVqC93772643 = JxlEKBmVqC92035732;     JxlEKBmVqC92035732 = JxlEKBmVqC7131909;     JxlEKBmVqC7131909 = JxlEKBmVqC46201629;     JxlEKBmVqC46201629 = JxlEKBmVqC50398914;     JxlEKBmVqC50398914 = JxlEKBmVqC57409940;     JxlEKBmVqC57409940 = JxlEKBmVqC57904031;     JxlEKBmVqC57904031 = JxlEKBmVqC6123947;     JxlEKBmVqC6123947 = JxlEKBmVqC64570248;     JxlEKBmVqC64570248 = JxlEKBmVqC38230066;     JxlEKBmVqC38230066 = JxlEKBmVqC53524989;     JxlEKBmVqC53524989 = JxlEKBmVqC38372806;     JxlEKBmVqC38372806 = JxlEKBmVqC73064071;     JxlEKBmVqC73064071 = JxlEKBmVqC39021017;     JxlEKBmVqC39021017 = JxlEKBmVqC12486228;     JxlEKBmVqC12486228 = JxlEKBmVqC95936006;     JxlEKBmVqC95936006 = JxlEKBmVqC29148090;     JxlEKBmVqC29148090 = JxlEKBmVqC30751;     JxlEKBmVqC30751 = JxlEKBmVqC20695359;     JxlEKBmVqC20695359 = JxlEKBmVqC37848905;     JxlEKBmVqC37848905 = JxlEKBmVqC15934847;     JxlEKBmVqC15934847 = JxlEKBmVqC10075275;     JxlEKBmVqC10075275 = JxlEKBmVqC60463440;     JxlEKBmVqC60463440 = JxlEKBmVqC5472507;     JxlEKBmVqC5472507 = JxlEKBmVqC17664593;     JxlEKBmVqC17664593 = JxlEKBmVqC55997509;     JxlEKBmVqC55997509 = JxlEKBmVqC36540652;     JxlEKBmVqC36540652 = JxlEKBmVqC47908166;     JxlEKBmVqC47908166 = JxlEKBmVqC8994925;     JxlEKBmVqC8994925 = JxlEKBmVqC1698064;     JxlEKBmVqC1698064 = JxlEKBmVqC87919406;     JxlEKBmVqC87919406 = JxlEKBmVqC9322829;     JxlEKBmVqC9322829 = JxlEKBmVqC24020215;     JxlEKBmVqC24020215 = JxlEKBmVqC43422735;     JxlEKBmVqC43422735 = JxlEKBmVqC98588926;     JxlEKBmVqC98588926 = JxlEKBmVqC12374243;     JxlEKBmVqC12374243 = JxlEKBmVqC60163955;     JxlEKBmVqC60163955 = JxlEKBmVqC15596555;     JxlEKBmVqC15596555 = JxlEKBmVqC96085654;     JxlEKBmVqC96085654 = JxlEKBmVqC21031903;     JxlEKBmVqC21031903 = JxlEKBmVqC42979650;     JxlEKBmVqC42979650 = JxlEKBmVqC81419981;     JxlEKBmVqC81419981 = JxlEKBmVqC36741101;     JxlEKBmVqC36741101 = JxlEKBmVqC25896661;     JxlEKBmVqC25896661 = JxlEKBmVqC70876811;     JxlEKBmVqC70876811 = JxlEKBmVqC49781916;     JxlEKBmVqC49781916 = JxlEKBmVqC36738042;     JxlEKBmVqC36738042 = JxlEKBmVqC36723428;     JxlEKBmVqC36723428 = JxlEKBmVqC80712410;     JxlEKBmVqC80712410 = JxlEKBmVqC53543351;     JxlEKBmVqC53543351 = JxlEKBmVqC83130950;     JxlEKBmVqC83130950 = JxlEKBmVqC74529875;     JxlEKBmVqC74529875 = JxlEKBmVqC72634136;     JxlEKBmVqC72634136 = JxlEKBmVqC44719427;     JxlEKBmVqC44719427 = JxlEKBmVqC81870041;     JxlEKBmVqC81870041 = JxlEKBmVqC83697368;     JxlEKBmVqC83697368 = JxlEKBmVqC31572293;     JxlEKBmVqC31572293 = JxlEKBmVqC1659402;     JxlEKBmVqC1659402 = JxlEKBmVqC28537036;     JxlEKBmVqC28537036 = JxlEKBmVqC94401405;     JxlEKBmVqC94401405 = JxlEKBmVqC20869289;     JxlEKBmVqC20869289 = JxlEKBmVqC9995866;     JxlEKBmVqC9995866 = JxlEKBmVqC97129021;     JxlEKBmVqC97129021 = JxlEKBmVqC62872185;     JxlEKBmVqC62872185 = JxlEKBmVqC50310659;     JxlEKBmVqC50310659 = JxlEKBmVqC44202161;     JxlEKBmVqC44202161 = JxlEKBmVqC14352592;     JxlEKBmVqC14352592 = JxlEKBmVqC29641337;     JxlEKBmVqC29641337 = JxlEKBmVqC40432091;     JxlEKBmVqC40432091 = JxlEKBmVqC111985;     JxlEKBmVqC111985 = JxlEKBmVqC35772051;     JxlEKBmVqC35772051 = JxlEKBmVqC13551535;     JxlEKBmVqC13551535 = JxlEKBmVqC3945096;     JxlEKBmVqC3945096 = JxlEKBmVqC99663456;     JxlEKBmVqC99663456 = JxlEKBmVqC94869255;     JxlEKBmVqC94869255 = JxlEKBmVqC34514865;     JxlEKBmVqC34514865 = JxlEKBmVqC73334174;     JxlEKBmVqC73334174 = JxlEKBmVqC34566779;     JxlEKBmVqC34566779 = JxlEKBmVqC34595696;     JxlEKBmVqC34595696 = JxlEKBmVqC67882677;     JxlEKBmVqC67882677 = JxlEKBmVqC19259467;     JxlEKBmVqC19259467 = JxlEKBmVqC99817223;     JxlEKBmVqC99817223 = JxlEKBmVqC67195756;     JxlEKBmVqC67195756 = JxlEKBmVqC55451574;     JxlEKBmVqC55451574 = JxlEKBmVqC18567113;     JxlEKBmVqC18567113 = JxlEKBmVqC13389532;     JxlEKBmVqC13389532 = JxlEKBmVqC36688692;     JxlEKBmVqC36688692 = JxlEKBmVqC79300787;     JxlEKBmVqC79300787 = JxlEKBmVqC61552694;     JxlEKBmVqC61552694 = JxlEKBmVqC24401801;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void mmoBxoGGgb93145850() {     long TyiuXwSbGv93582669 = -795622861;    long TyiuXwSbGv33667975 = -906030788;    long TyiuXwSbGv11103011 = -704706821;    long TyiuXwSbGv41385718 = -98513502;    long TyiuXwSbGv31255269 = 47890876;    long TyiuXwSbGv5743106 = -188738022;    long TyiuXwSbGv1953844 = -154552367;    long TyiuXwSbGv47335782 = -813532492;    long TyiuXwSbGv15751670 = -476667546;    long TyiuXwSbGv42736511 = -790483713;    long TyiuXwSbGv83955810 = -888739844;    long TyiuXwSbGv88125481 = -887407065;    long TyiuXwSbGv39287458 = 52804397;    long TyiuXwSbGv43106971 = -903129828;    long TyiuXwSbGv69868431 = -533195857;    long TyiuXwSbGv99395380 = -722653877;    long TyiuXwSbGv27074477 = -325384484;    long TyiuXwSbGv52491626 = -832114972;    long TyiuXwSbGv27757825 = 92969247;    long TyiuXwSbGv533552 = -97508214;    long TyiuXwSbGv38573795 = -188450867;    long TyiuXwSbGv2522652 = -738294413;    long TyiuXwSbGv87293650 = -539705735;    long TyiuXwSbGv28912239 = -832586625;    long TyiuXwSbGv65751733 = -816791472;    long TyiuXwSbGv5320676 = -832033995;    long TyiuXwSbGv70503915 = 6213223;    long TyiuXwSbGv21326596 = -87654202;    long TyiuXwSbGv71786718 = -426356365;    long TyiuXwSbGv55799565 = -38615595;    long TyiuXwSbGv46149730 = -911719412;    long TyiuXwSbGv74732118 = -787475922;    long TyiuXwSbGv7781093 = -102726279;    long TyiuXwSbGv26007290 = -413985830;    long TyiuXwSbGv9473796 = -469273487;    long TyiuXwSbGv49793718 = -175492839;    long TyiuXwSbGv83158257 = -85008219;    long TyiuXwSbGv5617266 = -589202295;    long TyiuXwSbGv97742845 = -284736205;    long TyiuXwSbGv5423797 = -44758068;    long TyiuXwSbGv96881108 = -585484824;    long TyiuXwSbGv3048676 = -796556028;    long TyiuXwSbGv21577193 = -901249768;    long TyiuXwSbGv51155047 = -423728705;    long TyiuXwSbGv42430958 = -908708603;    long TyiuXwSbGv98924128 = 25020386;    long TyiuXwSbGv21822824 = -942162052;    long TyiuXwSbGv60478010 = -767054074;    long TyiuXwSbGv2028032 = -676913360;    long TyiuXwSbGv11499930 = -505315450;    long TyiuXwSbGv95208300 = -395823045;    long TyiuXwSbGv31607656 = -865389299;    long TyiuXwSbGv39285620 = -315093286;    long TyiuXwSbGv51228121 = -202240870;    long TyiuXwSbGv54572055 = -346269212;    long TyiuXwSbGv91060018 = 42671552;    long TyiuXwSbGv46374325 = -266325054;    long TyiuXwSbGv82190771 = -872120196;    long TyiuXwSbGv75633984 = -281722030;    long TyiuXwSbGv25934593 = -120075130;    long TyiuXwSbGv35239190 = -94951245;    long TyiuXwSbGv80627247 = 33101834;    long TyiuXwSbGv75549063 = -287176127;    long TyiuXwSbGv59952105 = -338051951;    long TyiuXwSbGv96586781 = -878764301;    long TyiuXwSbGv9223693 = -1263922;    long TyiuXwSbGv80344388 = -684680787;    long TyiuXwSbGv13280169 = -533209773;    long TyiuXwSbGv33633176 = -333856341;    long TyiuXwSbGv20074714 = -257703019;    long TyiuXwSbGv16237123 = -537645659;    long TyiuXwSbGv21457211 = -736182189;    long TyiuXwSbGv54748780 = -447378768;    long TyiuXwSbGv22334028 = -862272685;    long TyiuXwSbGv3652444 = -512023390;    long TyiuXwSbGv35525120 = -391894839;    long TyiuXwSbGv80945458 = -837044646;    long TyiuXwSbGv36138604 = -15977030;    long TyiuXwSbGv86481281 = -923878023;    long TyiuXwSbGv66827605 = -741811859;    long TyiuXwSbGv83497852 = -889871943;    long TyiuXwSbGv10025906 = -226732704;    long TyiuXwSbGv19298564 = -410740843;    long TyiuXwSbGv60286788 = -921040916;    long TyiuXwSbGv60591265 = -642792550;    long TyiuXwSbGv14542074 = 53669886;    long TyiuXwSbGv35446498 = -372382637;    long TyiuXwSbGv56552972 = -900485409;    long TyiuXwSbGv71435235 = 32283381;    long TyiuXwSbGv18413778 = -411945040;    long TyiuXwSbGv3419393 = -909167785;    long TyiuXwSbGv967486 = -212888023;    long TyiuXwSbGv29983282 = -207480266;    long TyiuXwSbGv71808253 = -64661076;    long TyiuXwSbGv70184607 = -949806824;    long TyiuXwSbGv16253861 = -518586659;    long TyiuXwSbGv27499612 = -409379902;    long TyiuXwSbGv61625087 = -463197817;    long TyiuXwSbGv54568265 = -544964405;    long TyiuXwSbGv33207266 = -795622861;     TyiuXwSbGv93582669 = TyiuXwSbGv33667975;     TyiuXwSbGv33667975 = TyiuXwSbGv11103011;     TyiuXwSbGv11103011 = TyiuXwSbGv41385718;     TyiuXwSbGv41385718 = TyiuXwSbGv31255269;     TyiuXwSbGv31255269 = TyiuXwSbGv5743106;     TyiuXwSbGv5743106 = TyiuXwSbGv1953844;     TyiuXwSbGv1953844 = TyiuXwSbGv47335782;     TyiuXwSbGv47335782 = TyiuXwSbGv15751670;     TyiuXwSbGv15751670 = TyiuXwSbGv42736511;     TyiuXwSbGv42736511 = TyiuXwSbGv83955810;     TyiuXwSbGv83955810 = TyiuXwSbGv88125481;     TyiuXwSbGv88125481 = TyiuXwSbGv39287458;     TyiuXwSbGv39287458 = TyiuXwSbGv43106971;     TyiuXwSbGv43106971 = TyiuXwSbGv69868431;     TyiuXwSbGv69868431 = TyiuXwSbGv99395380;     TyiuXwSbGv99395380 = TyiuXwSbGv27074477;     TyiuXwSbGv27074477 = TyiuXwSbGv52491626;     TyiuXwSbGv52491626 = TyiuXwSbGv27757825;     TyiuXwSbGv27757825 = TyiuXwSbGv533552;     TyiuXwSbGv533552 = TyiuXwSbGv38573795;     TyiuXwSbGv38573795 = TyiuXwSbGv2522652;     TyiuXwSbGv2522652 = TyiuXwSbGv87293650;     TyiuXwSbGv87293650 = TyiuXwSbGv28912239;     TyiuXwSbGv28912239 = TyiuXwSbGv65751733;     TyiuXwSbGv65751733 = TyiuXwSbGv5320676;     TyiuXwSbGv5320676 = TyiuXwSbGv70503915;     TyiuXwSbGv70503915 = TyiuXwSbGv21326596;     TyiuXwSbGv21326596 = TyiuXwSbGv71786718;     TyiuXwSbGv71786718 = TyiuXwSbGv55799565;     TyiuXwSbGv55799565 = TyiuXwSbGv46149730;     TyiuXwSbGv46149730 = TyiuXwSbGv74732118;     TyiuXwSbGv74732118 = TyiuXwSbGv7781093;     TyiuXwSbGv7781093 = TyiuXwSbGv26007290;     TyiuXwSbGv26007290 = TyiuXwSbGv9473796;     TyiuXwSbGv9473796 = TyiuXwSbGv49793718;     TyiuXwSbGv49793718 = TyiuXwSbGv83158257;     TyiuXwSbGv83158257 = TyiuXwSbGv5617266;     TyiuXwSbGv5617266 = TyiuXwSbGv97742845;     TyiuXwSbGv97742845 = TyiuXwSbGv5423797;     TyiuXwSbGv5423797 = TyiuXwSbGv96881108;     TyiuXwSbGv96881108 = TyiuXwSbGv3048676;     TyiuXwSbGv3048676 = TyiuXwSbGv21577193;     TyiuXwSbGv21577193 = TyiuXwSbGv51155047;     TyiuXwSbGv51155047 = TyiuXwSbGv42430958;     TyiuXwSbGv42430958 = TyiuXwSbGv98924128;     TyiuXwSbGv98924128 = TyiuXwSbGv21822824;     TyiuXwSbGv21822824 = TyiuXwSbGv60478010;     TyiuXwSbGv60478010 = TyiuXwSbGv2028032;     TyiuXwSbGv2028032 = TyiuXwSbGv11499930;     TyiuXwSbGv11499930 = TyiuXwSbGv95208300;     TyiuXwSbGv95208300 = TyiuXwSbGv31607656;     TyiuXwSbGv31607656 = TyiuXwSbGv39285620;     TyiuXwSbGv39285620 = TyiuXwSbGv51228121;     TyiuXwSbGv51228121 = TyiuXwSbGv54572055;     TyiuXwSbGv54572055 = TyiuXwSbGv91060018;     TyiuXwSbGv91060018 = TyiuXwSbGv46374325;     TyiuXwSbGv46374325 = TyiuXwSbGv82190771;     TyiuXwSbGv82190771 = TyiuXwSbGv75633984;     TyiuXwSbGv75633984 = TyiuXwSbGv25934593;     TyiuXwSbGv25934593 = TyiuXwSbGv35239190;     TyiuXwSbGv35239190 = TyiuXwSbGv80627247;     TyiuXwSbGv80627247 = TyiuXwSbGv75549063;     TyiuXwSbGv75549063 = TyiuXwSbGv59952105;     TyiuXwSbGv59952105 = TyiuXwSbGv96586781;     TyiuXwSbGv96586781 = TyiuXwSbGv9223693;     TyiuXwSbGv9223693 = TyiuXwSbGv80344388;     TyiuXwSbGv80344388 = TyiuXwSbGv13280169;     TyiuXwSbGv13280169 = TyiuXwSbGv33633176;     TyiuXwSbGv33633176 = TyiuXwSbGv20074714;     TyiuXwSbGv20074714 = TyiuXwSbGv16237123;     TyiuXwSbGv16237123 = TyiuXwSbGv21457211;     TyiuXwSbGv21457211 = TyiuXwSbGv54748780;     TyiuXwSbGv54748780 = TyiuXwSbGv22334028;     TyiuXwSbGv22334028 = TyiuXwSbGv3652444;     TyiuXwSbGv3652444 = TyiuXwSbGv35525120;     TyiuXwSbGv35525120 = TyiuXwSbGv80945458;     TyiuXwSbGv80945458 = TyiuXwSbGv36138604;     TyiuXwSbGv36138604 = TyiuXwSbGv86481281;     TyiuXwSbGv86481281 = TyiuXwSbGv66827605;     TyiuXwSbGv66827605 = TyiuXwSbGv83497852;     TyiuXwSbGv83497852 = TyiuXwSbGv10025906;     TyiuXwSbGv10025906 = TyiuXwSbGv19298564;     TyiuXwSbGv19298564 = TyiuXwSbGv60286788;     TyiuXwSbGv60286788 = TyiuXwSbGv60591265;     TyiuXwSbGv60591265 = TyiuXwSbGv14542074;     TyiuXwSbGv14542074 = TyiuXwSbGv35446498;     TyiuXwSbGv35446498 = TyiuXwSbGv56552972;     TyiuXwSbGv56552972 = TyiuXwSbGv71435235;     TyiuXwSbGv71435235 = TyiuXwSbGv18413778;     TyiuXwSbGv18413778 = TyiuXwSbGv3419393;     TyiuXwSbGv3419393 = TyiuXwSbGv967486;     TyiuXwSbGv967486 = TyiuXwSbGv29983282;     TyiuXwSbGv29983282 = TyiuXwSbGv71808253;     TyiuXwSbGv71808253 = TyiuXwSbGv70184607;     TyiuXwSbGv70184607 = TyiuXwSbGv16253861;     TyiuXwSbGv16253861 = TyiuXwSbGv27499612;     TyiuXwSbGv27499612 = TyiuXwSbGv61625087;     TyiuXwSbGv61625087 = TyiuXwSbGv54568265;     TyiuXwSbGv54568265 = TyiuXwSbGv33207266;     TyiuXwSbGv33207266 = TyiuXwSbGv93582669;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void lUIXjHekGy18662675() {     long aEPidauWnF12495650 = -683779229;    long aEPidauWnF55953668 = -722071894;    long aEPidauWnF92928176 = -778206203;    long aEPidauWnF98781532 = -932653163;    long aEPidauWnF80401708 = -262042478;    long aEPidauWnF88262229 = -681008353;    long aEPidauWnF30829564 = -732384714;    long aEPidauWnF19993710 = -888658686;    long aEPidauWnF51688730 = -85989199;    long aEPidauWnF95225604 = -918336516;    long aEPidauWnF41072575 = -941655338;    long aEPidauWnF30461514 = -35928539;    long aEPidauWnF47860509 = -808381637;    long aEPidauWnF45108396 = -150054017;    long aEPidauWnF21159248 = -86574576;    long aEPidauWnF7479023 = -197351333;    long aEPidauWnF92883158 = -876282208;    long aEPidauWnF48602465 = -7240485;    long aEPidauWnF10105892 = -395295645;    long aEPidauWnF75627568 = -135618595;    long aEPidauWnF87796909 = -969555026;    long aEPidauWnF50647095 = -601094653;    long aEPidauWnF6327143 = -78790353;    long aEPidauWnF91936579 = -493855842;    long aEPidauWnF22681609 = -210388650;    long aEPidauWnF36499881 = -538811625;    long aEPidauWnF25046858 = -76955691;    long aEPidauWnF48681359 = -513930778;    long aEPidauWnF35518357 = -443850838;    long aEPidauWnF13031512 = 27174444;    long aEPidauWnF32709257 = -520817249;    long aEPidauWnF98575777 = -987909774;    long aEPidauWnF35359356 = -118278109;    long aEPidauWnF97489971 = -805259215;    long aEPidauWnF30471923 = -229067586;    long aEPidauWnF49501462 = -608433431;    long aEPidauWnF33272201 = -229073371;    long aEPidauWnF92534982 = -287434377;    long aEPidauWnF24158091 = -701488155;    long aEPidauWnF61381124 = -398466608;    long aEPidauWnF82198139 = -344316864;    long aEPidauWnF64355097 = -674321517;    long aEPidauWnF94102600 = -166792415;    long aEPidauWnF44343478 = -348797221;    long aEPidauWnF8364835 = -303487615;    long aEPidauWnF23849501 = -865447164;    long aEPidauWnF75636729 = -836531709;    long aEPidauWnF34799546 = -569160352;    long aEPidauWnF77780226 = 69393469;    long aEPidauWnF41293869 = -722825391;    long aEPidauWnF29347417 = -23168514;    long aEPidauWnF48773037 = -14082335;    long aEPidauWnF58846705 = -994805144;    long aEPidauWnF76521320 = -964282980;    long aEPidauWnF30865765 = -305896564;    long aEPidauWnF61848555 = 17315423;    long aEPidauWnF49626525 = -543281542;    long aEPidauWnF991597 = -184350362;    long aEPidauWnF76099924 = -622264514;    long aEPidauWnF43901827 = -723230854;    long aEPidauWnF63215372 = -504052662;    long aEPidauWnF82148204 = -118453937;    long aEPidauWnF84475353 = -344807849;    long aEPidauWnF38657218 = -13163643;    long aEPidauWnF62516348 = -297519268;    long aEPidauWnF42496797 = -953745564;    long aEPidauWnF95102157 = -917650430;    long aEPidauWnF50370537 = 96877577;    long aEPidauWnF14636474 = -920986431;    long aEPidauWnF71657786 = -478141145;    long aEPidauWnF74206821 = -968277963;    long aEPidauWnF348177 = -488847832;    long aEPidauWnF24444375 = -305752330;    long aEPidauWnF48724767 = -996829038;    long aEPidauWnF93429429 = -791301732;    long aEPidauWnF23441812 = -195233510;    long aEPidauWnF56544494 = -334302238;    long aEPidauWnF61983665 = -729993133;    long aEPidauWnF83571745 = -90368227;    long aEPidauWnF98832107 = -344941487;    long aEPidauWnF60863152 = -702279916;    long aEPidauWnF90247311 = -507795340;    long aEPidauWnF70901133 = -483324248;    long aEPidauWnF94224488 = -721025447;    long aEPidauWnF83684094 = -949657043;    long aEPidauWnF83936220 = -406734914;    long aEPidauWnF39729073 = -993104631;    long aEPidauWnF58838035 = -153995130;    long aEPidauWnF66624206 = -399362652;    long aEPidauWnF68623368 = -146383010;    long aEPidauWnF99874936 = 34848110;    long aEPidauWnF32280605 = 55276990;    long aEPidauWnF16435058 = -665169864;    long aEPidauWnF80256263 = -978257302;    long aEPidauWnF98165752 = -894413946;    long aEPidauWnF49935 = -125862928;    long aEPidauWnF79879743 = -229513669;    long aEPidauWnF55445383 = -53628773;    long aEPidauWnF81827129 = 48722047;    long aEPidauWnF65868037 = -683779229;     aEPidauWnF12495650 = aEPidauWnF55953668;     aEPidauWnF55953668 = aEPidauWnF92928176;     aEPidauWnF92928176 = aEPidauWnF98781532;     aEPidauWnF98781532 = aEPidauWnF80401708;     aEPidauWnF80401708 = aEPidauWnF88262229;     aEPidauWnF88262229 = aEPidauWnF30829564;     aEPidauWnF30829564 = aEPidauWnF19993710;     aEPidauWnF19993710 = aEPidauWnF51688730;     aEPidauWnF51688730 = aEPidauWnF95225604;     aEPidauWnF95225604 = aEPidauWnF41072575;     aEPidauWnF41072575 = aEPidauWnF30461514;     aEPidauWnF30461514 = aEPidauWnF47860509;     aEPidauWnF47860509 = aEPidauWnF45108396;     aEPidauWnF45108396 = aEPidauWnF21159248;     aEPidauWnF21159248 = aEPidauWnF7479023;     aEPidauWnF7479023 = aEPidauWnF92883158;     aEPidauWnF92883158 = aEPidauWnF48602465;     aEPidauWnF48602465 = aEPidauWnF10105892;     aEPidauWnF10105892 = aEPidauWnF75627568;     aEPidauWnF75627568 = aEPidauWnF87796909;     aEPidauWnF87796909 = aEPidauWnF50647095;     aEPidauWnF50647095 = aEPidauWnF6327143;     aEPidauWnF6327143 = aEPidauWnF91936579;     aEPidauWnF91936579 = aEPidauWnF22681609;     aEPidauWnF22681609 = aEPidauWnF36499881;     aEPidauWnF36499881 = aEPidauWnF25046858;     aEPidauWnF25046858 = aEPidauWnF48681359;     aEPidauWnF48681359 = aEPidauWnF35518357;     aEPidauWnF35518357 = aEPidauWnF13031512;     aEPidauWnF13031512 = aEPidauWnF32709257;     aEPidauWnF32709257 = aEPidauWnF98575777;     aEPidauWnF98575777 = aEPidauWnF35359356;     aEPidauWnF35359356 = aEPidauWnF97489971;     aEPidauWnF97489971 = aEPidauWnF30471923;     aEPidauWnF30471923 = aEPidauWnF49501462;     aEPidauWnF49501462 = aEPidauWnF33272201;     aEPidauWnF33272201 = aEPidauWnF92534982;     aEPidauWnF92534982 = aEPidauWnF24158091;     aEPidauWnF24158091 = aEPidauWnF61381124;     aEPidauWnF61381124 = aEPidauWnF82198139;     aEPidauWnF82198139 = aEPidauWnF64355097;     aEPidauWnF64355097 = aEPidauWnF94102600;     aEPidauWnF94102600 = aEPidauWnF44343478;     aEPidauWnF44343478 = aEPidauWnF8364835;     aEPidauWnF8364835 = aEPidauWnF23849501;     aEPidauWnF23849501 = aEPidauWnF75636729;     aEPidauWnF75636729 = aEPidauWnF34799546;     aEPidauWnF34799546 = aEPidauWnF77780226;     aEPidauWnF77780226 = aEPidauWnF41293869;     aEPidauWnF41293869 = aEPidauWnF29347417;     aEPidauWnF29347417 = aEPidauWnF48773037;     aEPidauWnF48773037 = aEPidauWnF58846705;     aEPidauWnF58846705 = aEPidauWnF76521320;     aEPidauWnF76521320 = aEPidauWnF30865765;     aEPidauWnF30865765 = aEPidauWnF61848555;     aEPidauWnF61848555 = aEPidauWnF49626525;     aEPidauWnF49626525 = aEPidauWnF991597;     aEPidauWnF991597 = aEPidauWnF76099924;     aEPidauWnF76099924 = aEPidauWnF43901827;     aEPidauWnF43901827 = aEPidauWnF63215372;     aEPidauWnF63215372 = aEPidauWnF82148204;     aEPidauWnF82148204 = aEPidauWnF84475353;     aEPidauWnF84475353 = aEPidauWnF38657218;     aEPidauWnF38657218 = aEPidauWnF62516348;     aEPidauWnF62516348 = aEPidauWnF42496797;     aEPidauWnF42496797 = aEPidauWnF95102157;     aEPidauWnF95102157 = aEPidauWnF50370537;     aEPidauWnF50370537 = aEPidauWnF14636474;     aEPidauWnF14636474 = aEPidauWnF71657786;     aEPidauWnF71657786 = aEPidauWnF74206821;     aEPidauWnF74206821 = aEPidauWnF348177;     aEPidauWnF348177 = aEPidauWnF24444375;     aEPidauWnF24444375 = aEPidauWnF48724767;     aEPidauWnF48724767 = aEPidauWnF93429429;     aEPidauWnF93429429 = aEPidauWnF23441812;     aEPidauWnF23441812 = aEPidauWnF56544494;     aEPidauWnF56544494 = aEPidauWnF61983665;     aEPidauWnF61983665 = aEPidauWnF83571745;     aEPidauWnF83571745 = aEPidauWnF98832107;     aEPidauWnF98832107 = aEPidauWnF60863152;     aEPidauWnF60863152 = aEPidauWnF90247311;     aEPidauWnF90247311 = aEPidauWnF70901133;     aEPidauWnF70901133 = aEPidauWnF94224488;     aEPidauWnF94224488 = aEPidauWnF83684094;     aEPidauWnF83684094 = aEPidauWnF83936220;     aEPidauWnF83936220 = aEPidauWnF39729073;     aEPidauWnF39729073 = aEPidauWnF58838035;     aEPidauWnF58838035 = aEPidauWnF66624206;     aEPidauWnF66624206 = aEPidauWnF68623368;     aEPidauWnF68623368 = aEPidauWnF99874936;     aEPidauWnF99874936 = aEPidauWnF32280605;     aEPidauWnF32280605 = aEPidauWnF16435058;     aEPidauWnF16435058 = aEPidauWnF80256263;     aEPidauWnF80256263 = aEPidauWnF98165752;     aEPidauWnF98165752 = aEPidauWnF49935;     aEPidauWnF49935 = aEPidauWnF79879743;     aEPidauWnF79879743 = aEPidauWnF55445383;     aEPidauWnF55445383 = aEPidauWnF81827129;     aEPidauWnF81827129 = aEPidauWnF65868037;     aEPidauWnF65868037 = aEPidauWnF12495650;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void fVRpHleZvC57385082() {     long PXVLpsueDj41555064 = -955059924;    long PXVLpsueDj14412619 = -10051619;    long PXVLpsueDj72751465 = -746822201;    long PXVLpsueDj2114336 = -112243927;    long PXVLpsueDj98771527 = -590088412;    long PXVLpsueDj37635499 = -204327403;    long PXVLpsueDj99430426 = -558810880;    long PXVLpsueDj37979765 = -564811015;    long PXVLpsueDj38244094 = -857087629;    long PXVLpsueDj53658478 = -867786569;    long PXVLpsueDj35831224 = -95792132;    long PXVLpsueDj79346086 = -481861265;    long PXVLpsueDj82876416 = -330161288;    long PXVLpsueDj77281440 = -305362878;    long PXVLpsueDj62872597 = -49219528;    long PXVLpsueDj51372336 = -440512896;    long PXVLpsueDj76695945 = -290984098;    long PXVLpsueDj79583946 = -700135024;    long PXVLpsueDj30504177 = -825106735;    long PXVLpsueDj40211302 = -324582043;    long PXVLpsueDj40466168 = -641637957;    long PXVLpsueDj67840976 = -562793656;    long PXVLpsueDj94350771 = -759878432;    long PXVLpsueDj57126068 = -155428462;    long PXVLpsueDj56195119 = -5846739;    long PXVLpsueDj23112808 = -840378760;    long PXVLpsueDj99981244 = -417186405;    long PXVLpsueDj91136846 = -105770888;    long PXVLpsueDj86556491 = 93092665;    long PXVLpsueDj18279235 = -967821023;    long PXVLpsueDj12487170 = -771682595;    long PXVLpsueDj69463241 = -613343111;    long PXVLpsueDj10006815 = 20109090;    long PXVLpsueDj38983049 = -794257652;    long PXVLpsueDj87108148 = -134465821;    long PXVLpsueDj96278437 = 91517069;    long PXVLpsueDj26243356 = -298468115;    long PXVLpsueDj34816054 = -305542592;    long PXVLpsueDj86974555 = -44120559;    long PXVLpsueDj34393185 = -650952417;    long PXVLpsueDj72789760 = -563722680;    long PXVLpsueDj35566657 = 36496777;    long PXVLpsueDj76327604 = -817386733;    long PXVLpsueDj21392445 = -889507953;    long PXVLpsueDj2395101 = -656873198;    long PXVLpsueDj73424732 = -679756247;    long PXVLpsueDj33888676 = -453041166;    long PXVLpsueDj13189456 = -716771912;    long PXVLpsueDj47299257 = -726306285;    long PXVLpsueDj62393670 = -495358862;    long PXVLpsueDj14213764 = -999557925;    long PXVLpsueDj32717493 = -374287761;    long PXVLpsueDj35673627 = -554903065;    long PXVLpsueDj80413575 = -792985460;    long PXVLpsueDj16918914 = -962063744;    long PXVLpsueDj73714087 = -292266269;    long PXVLpsueDj20061848 = -250173188;    long PXVLpsueDj15625397 = -491393740;    long PXVLpsueDj45919216 = -6397188;    long PXVLpsueDj75658720 = -749709653;    long PXVLpsueDj37654254 = -787140999;    long PXVLpsueDj8293581 = -353039993;    long PXVLpsueDj51423273 = -557903681;    long PXVLpsueDj19964859 = -889266606;    long PXVLpsueDj41171309 = 3896026;    long PXVLpsueDj66367982 = -482449021;    long PXVLpsueDj69339271 = -401970355;    long PXVLpsueDj43893368 = -535903636;    long PXVLpsueDj90173291 = -70897058;    long PXVLpsueDj66594159 = -40736597;    long PXVLpsueDj25128981 = -42044782;    long PXVLpsueDj41879892 = -985441506;    long PXVLpsueDj92609391 = -556014465;    long PXVLpsueDj96110991 = -74154319;    long PXVLpsueDj67421541 = -760859363;    long PXVLpsueDj4899512 = -578134734;    long PXVLpsueDj91513371 = -745406923;    long PXVLpsueDj72958326 = -870370480;    long PXVLpsueDj54730968 = -498555265;    long PXVLpsueDj82770387 = -326090493;    long PXVLpsueDj89224131 = -287337595;    long PXVLpsueDj86791789 = -700414494;    long PXVLpsueDj43837589 = -379464604;    long PXVLpsueDj24162821 = -411548474;    long PXVLpsueDj4065471 = -968263099;    long PXVLpsueDj79769676 = -297394835;    long PXVLpsueDj33789615 = 41559953;    long PXVLpsueDj29593240 = -186905451;    long PXVLpsueDj22064135 = -832193908;    long PXVLpsueDj13394061 = -842199553;    long PXVLpsueDj76216590 = -658309744;    long PXVLpsueDj10617959 = -807074375;    long PXVLpsueDj88896837 = -199145405;    long PXVLpsueDj11315836 = -294410907;    long PXVLpsueDj96738931 = -863811418;    long PXVLpsueDj64496180 = -110682688;    long PXVLpsueDj84143383 = -405599543;    long PXVLpsueDj56362746 = -928120127;    long PXVLpsueDj80221136 = -793403979;    long PXVLpsueDj36027118 = -955059924;     PXVLpsueDj41555064 = PXVLpsueDj14412619;     PXVLpsueDj14412619 = PXVLpsueDj72751465;     PXVLpsueDj72751465 = PXVLpsueDj2114336;     PXVLpsueDj2114336 = PXVLpsueDj98771527;     PXVLpsueDj98771527 = PXVLpsueDj37635499;     PXVLpsueDj37635499 = PXVLpsueDj99430426;     PXVLpsueDj99430426 = PXVLpsueDj37979765;     PXVLpsueDj37979765 = PXVLpsueDj38244094;     PXVLpsueDj38244094 = PXVLpsueDj53658478;     PXVLpsueDj53658478 = PXVLpsueDj35831224;     PXVLpsueDj35831224 = PXVLpsueDj79346086;     PXVLpsueDj79346086 = PXVLpsueDj82876416;     PXVLpsueDj82876416 = PXVLpsueDj77281440;     PXVLpsueDj77281440 = PXVLpsueDj62872597;     PXVLpsueDj62872597 = PXVLpsueDj51372336;     PXVLpsueDj51372336 = PXVLpsueDj76695945;     PXVLpsueDj76695945 = PXVLpsueDj79583946;     PXVLpsueDj79583946 = PXVLpsueDj30504177;     PXVLpsueDj30504177 = PXVLpsueDj40211302;     PXVLpsueDj40211302 = PXVLpsueDj40466168;     PXVLpsueDj40466168 = PXVLpsueDj67840976;     PXVLpsueDj67840976 = PXVLpsueDj94350771;     PXVLpsueDj94350771 = PXVLpsueDj57126068;     PXVLpsueDj57126068 = PXVLpsueDj56195119;     PXVLpsueDj56195119 = PXVLpsueDj23112808;     PXVLpsueDj23112808 = PXVLpsueDj99981244;     PXVLpsueDj99981244 = PXVLpsueDj91136846;     PXVLpsueDj91136846 = PXVLpsueDj86556491;     PXVLpsueDj86556491 = PXVLpsueDj18279235;     PXVLpsueDj18279235 = PXVLpsueDj12487170;     PXVLpsueDj12487170 = PXVLpsueDj69463241;     PXVLpsueDj69463241 = PXVLpsueDj10006815;     PXVLpsueDj10006815 = PXVLpsueDj38983049;     PXVLpsueDj38983049 = PXVLpsueDj87108148;     PXVLpsueDj87108148 = PXVLpsueDj96278437;     PXVLpsueDj96278437 = PXVLpsueDj26243356;     PXVLpsueDj26243356 = PXVLpsueDj34816054;     PXVLpsueDj34816054 = PXVLpsueDj86974555;     PXVLpsueDj86974555 = PXVLpsueDj34393185;     PXVLpsueDj34393185 = PXVLpsueDj72789760;     PXVLpsueDj72789760 = PXVLpsueDj35566657;     PXVLpsueDj35566657 = PXVLpsueDj76327604;     PXVLpsueDj76327604 = PXVLpsueDj21392445;     PXVLpsueDj21392445 = PXVLpsueDj2395101;     PXVLpsueDj2395101 = PXVLpsueDj73424732;     PXVLpsueDj73424732 = PXVLpsueDj33888676;     PXVLpsueDj33888676 = PXVLpsueDj13189456;     PXVLpsueDj13189456 = PXVLpsueDj47299257;     PXVLpsueDj47299257 = PXVLpsueDj62393670;     PXVLpsueDj62393670 = PXVLpsueDj14213764;     PXVLpsueDj14213764 = PXVLpsueDj32717493;     PXVLpsueDj32717493 = PXVLpsueDj35673627;     PXVLpsueDj35673627 = PXVLpsueDj80413575;     PXVLpsueDj80413575 = PXVLpsueDj16918914;     PXVLpsueDj16918914 = PXVLpsueDj73714087;     PXVLpsueDj73714087 = PXVLpsueDj20061848;     PXVLpsueDj20061848 = PXVLpsueDj15625397;     PXVLpsueDj15625397 = PXVLpsueDj45919216;     PXVLpsueDj45919216 = PXVLpsueDj75658720;     PXVLpsueDj75658720 = PXVLpsueDj37654254;     PXVLpsueDj37654254 = PXVLpsueDj8293581;     PXVLpsueDj8293581 = PXVLpsueDj51423273;     PXVLpsueDj51423273 = PXVLpsueDj19964859;     PXVLpsueDj19964859 = PXVLpsueDj41171309;     PXVLpsueDj41171309 = PXVLpsueDj66367982;     PXVLpsueDj66367982 = PXVLpsueDj69339271;     PXVLpsueDj69339271 = PXVLpsueDj43893368;     PXVLpsueDj43893368 = PXVLpsueDj90173291;     PXVLpsueDj90173291 = PXVLpsueDj66594159;     PXVLpsueDj66594159 = PXVLpsueDj25128981;     PXVLpsueDj25128981 = PXVLpsueDj41879892;     PXVLpsueDj41879892 = PXVLpsueDj92609391;     PXVLpsueDj92609391 = PXVLpsueDj96110991;     PXVLpsueDj96110991 = PXVLpsueDj67421541;     PXVLpsueDj67421541 = PXVLpsueDj4899512;     PXVLpsueDj4899512 = PXVLpsueDj91513371;     PXVLpsueDj91513371 = PXVLpsueDj72958326;     PXVLpsueDj72958326 = PXVLpsueDj54730968;     PXVLpsueDj54730968 = PXVLpsueDj82770387;     PXVLpsueDj82770387 = PXVLpsueDj89224131;     PXVLpsueDj89224131 = PXVLpsueDj86791789;     PXVLpsueDj86791789 = PXVLpsueDj43837589;     PXVLpsueDj43837589 = PXVLpsueDj24162821;     PXVLpsueDj24162821 = PXVLpsueDj4065471;     PXVLpsueDj4065471 = PXVLpsueDj79769676;     PXVLpsueDj79769676 = PXVLpsueDj33789615;     PXVLpsueDj33789615 = PXVLpsueDj29593240;     PXVLpsueDj29593240 = PXVLpsueDj22064135;     PXVLpsueDj22064135 = PXVLpsueDj13394061;     PXVLpsueDj13394061 = PXVLpsueDj76216590;     PXVLpsueDj76216590 = PXVLpsueDj10617959;     PXVLpsueDj10617959 = PXVLpsueDj88896837;     PXVLpsueDj88896837 = PXVLpsueDj11315836;     PXVLpsueDj11315836 = PXVLpsueDj96738931;     PXVLpsueDj96738931 = PXVLpsueDj64496180;     PXVLpsueDj64496180 = PXVLpsueDj84143383;     PXVLpsueDj84143383 = PXVLpsueDj56362746;     PXVLpsueDj56362746 = PXVLpsueDj80221136;     PXVLpsueDj80221136 = PXVLpsueDj36027118;     PXVLpsueDj36027118 = PXVLpsueDj41555064;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void AOBQmJUQJZ63383431() {     long KGytfOLSmF50870330 = -857590951;    long KGytfOLSmF69835541 = -26586122;    long KGytfOLSmF92903545 = -460462814;    long KGytfOLSmF59904358 = -303907919;    long KGytfOLSmF38834767 = -566603570;    long KGytfOLSmF31178861 = -763196264;    long KGytfOLSmF73084498 = -358059758;    long KGytfOLSmF27115921 = -761770163;    long KGytfOLSmF31052751 = -137148301;    long KGytfOLSmF22282936 = -688582237;    long KGytfOLSmF72733208 = -287033204;    long KGytfOLSmF90583835 = -19231080;    long KGytfOLSmF78658132 = 84416578;    long KGytfOLSmF9370082 = -284416983;    long KGytfOLSmF78148634 = 99250061;    long KGytfOLSmF56342860 = -468036374;    long KGytfOLSmF20506713 = -318569185;    long KGytfOLSmF91776587 = -430192337;    long KGytfOLSmF12517109 = -778747564;    long KGytfOLSmF3406650 = -403289884;    long KGytfOLSmF29145519 = -967337885;    long KGytfOLSmF92049888 = -385689988;    long KGytfOLSmF30412038 = -374407432;    long KGytfOLSmF16332698 = -772589572;    long KGytfOLSmF19257662 = -324979790;    long KGytfOLSmF89020624 = -939590401;    long KGytfOLSmF33739512 = -126075249;    long KGytfOLSmF62231321 = -612907621;    long KGytfOLSmF53673398 = -21326771;    long KGytfOLSmF31060183 = -773905586;    long KGytfOLSmF82485548 = -382334412;    long KGytfOLSmF46722605 = 55485594;    long KGytfOLSmF59866145 = -890981923;    long KGytfOLSmF28011997 = -277817539;    long KGytfOLSmF73758424 = -310878416;    long KGytfOLSmF28103332 = -324708378;    long KGytfOLSmF63710963 = -999667977;    long KGytfOLSmF90852851 = -58137955;    long KGytfOLSmF53200257 = -95336346;    long KGytfOLSmF10493082 = -358137929;    long KGytfOLSmF96852894 = -700879098;    long KGytfOLSmF23218313 = -984775314;    long KGytfOLSmF98044610 = 60732885;    long KGytfOLSmF80149345 = -279902841;    long KGytfOLSmF38345175 = -448648992;    long KGytfOLSmF43663968 = 94709194;    long KGytfOLSmF39138307 = -25875311;    long KGytfOLSmF84660492 = -63315300;    long KGytfOLSmF13326401 = -962002801;    long KGytfOLSmF32031573 = -288372564;    long KGytfOLSmF51181473 = -735433921;    long KGytfOLSmF96934534 = 77726022;    long KGytfOLSmF23681285 = -142679971;    long KGytfOLSmF84994819 = -52838578;    long KGytfOLSmF34237913 = -896981829;    long KGytfOLSmF58820441 = -371900963;    long KGytfOLSmF39423503 = -652178691;    long KGytfOLSmF76570848 = -687873242;    long KGytfOLSmF40646696 = -978928129;    long KGytfOLSmF49814142 = -627013169;    long KGytfOLSmF97439348 = -537121015;    long KGytfOLSmF10853177 = -745152138;    long KGytfOLSmF73442522 = -640443393;    long KGytfOLSmF99992567 = -363242715;    long KGytfOLSmF39797387 = -206247825;    long KGytfOLSmF26010604 = -242518799;    long KGytfOLSmF30717691 = -128249158;    long KGytfOLSmF50646136 = -637765884;    long KGytfOLSmF35611658 = -973538568;    long KGytfOLSmF50045302 = -576041561;    long KGytfOLSmF92631897 = -468368398;    long KGytfOLSmF29653861 = -160431231;    long KGytfOLSmF38576331 = -234855991;    long KGytfOLSmF2024027 = -320609635;    long KGytfOLSmF6553756 = -702410787;    long KGytfOLSmF5927207 = -982562571;    long KGytfOLSmF94005278 = -346422874;    long KGytfOLSmF50262692 = 5495408;    long KGytfOLSmF77987522 = -223940581;    long KGytfOLSmF75593694 = -319688985;    long KGytfOLSmF49882318 = -813715091;    long KGytfOLSmF49079019 = 37240050;    long KGytfOLSmF48904921 = -650904820;    long KGytfOLSmF21641826 = -732954207;    long KGytfOLSmF79878710 = 61528335;    long KGytfOLSmF85551013 = -360060435;    long KGytfOLSmF23041321 = -801834436;    long KGytfOLSmF74871326 = -738143346;    long KGytfOLSmF93774084 = -380835710;    long KGytfOLSmF14937983 = -938977454;    long KGytfOLSmF88679828 = -672529688;    long KGytfOLSmF87140115 = -211794735;    long KGytfOLSmF50206156 = -79209826;    long KGytfOLSmF3386115 = -468323178;    long KGytfOLSmF13053733 = -821016915;    long KGytfOLSmF85999717 = -955726961;    long KGytfOLSmF49775790 = -244331922;    long KGytfOLSmF98052042 = -576024400;    long KGytfOLSmF40351958 = 26344984;    long KGytfOLSmF12334572 = -857590951;     KGytfOLSmF50870330 = KGytfOLSmF69835541;     KGytfOLSmF69835541 = KGytfOLSmF92903545;     KGytfOLSmF92903545 = KGytfOLSmF59904358;     KGytfOLSmF59904358 = KGytfOLSmF38834767;     KGytfOLSmF38834767 = KGytfOLSmF31178861;     KGytfOLSmF31178861 = KGytfOLSmF73084498;     KGytfOLSmF73084498 = KGytfOLSmF27115921;     KGytfOLSmF27115921 = KGytfOLSmF31052751;     KGytfOLSmF31052751 = KGytfOLSmF22282936;     KGytfOLSmF22282936 = KGytfOLSmF72733208;     KGytfOLSmF72733208 = KGytfOLSmF90583835;     KGytfOLSmF90583835 = KGytfOLSmF78658132;     KGytfOLSmF78658132 = KGytfOLSmF9370082;     KGytfOLSmF9370082 = KGytfOLSmF78148634;     KGytfOLSmF78148634 = KGytfOLSmF56342860;     KGytfOLSmF56342860 = KGytfOLSmF20506713;     KGytfOLSmF20506713 = KGytfOLSmF91776587;     KGytfOLSmF91776587 = KGytfOLSmF12517109;     KGytfOLSmF12517109 = KGytfOLSmF3406650;     KGytfOLSmF3406650 = KGytfOLSmF29145519;     KGytfOLSmF29145519 = KGytfOLSmF92049888;     KGytfOLSmF92049888 = KGytfOLSmF30412038;     KGytfOLSmF30412038 = KGytfOLSmF16332698;     KGytfOLSmF16332698 = KGytfOLSmF19257662;     KGytfOLSmF19257662 = KGytfOLSmF89020624;     KGytfOLSmF89020624 = KGytfOLSmF33739512;     KGytfOLSmF33739512 = KGytfOLSmF62231321;     KGytfOLSmF62231321 = KGytfOLSmF53673398;     KGytfOLSmF53673398 = KGytfOLSmF31060183;     KGytfOLSmF31060183 = KGytfOLSmF82485548;     KGytfOLSmF82485548 = KGytfOLSmF46722605;     KGytfOLSmF46722605 = KGytfOLSmF59866145;     KGytfOLSmF59866145 = KGytfOLSmF28011997;     KGytfOLSmF28011997 = KGytfOLSmF73758424;     KGytfOLSmF73758424 = KGytfOLSmF28103332;     KGytfOLSmF28103332 = KGytfOLSmF63710963;     KGytfOLSmF63710963 = KGytfOLSmF90852851;     KGytfOLSmF90852851 = KGytfOLSmF53200257;     KGytfOLSmF53200257 = KGytfOLSmF10493082;     KGytfOLSmF10493082 = KGytfOLSmF96852894;     KGytfOLSmF96852894 = KGytfOLSmF23218313;     KGytfOLSmF23218313 = KGytfOLSmF98044610;     KGytfOLSmF98044610 = KGytfOLSmF80149345;     KGytfOLSmF80149345 = KGytfOLSmF38345175;     KGytfOLSmF38345175 = KGytfOLSmF43663968;     KGytfOLSmF43663968 = KGytfOLSmF39138307;     KGytfOLSmF39138307 = KGytfOLSmF84660492;     KGytfOLSmF84660492 = KGytfOLSmF13326401;     KGytfOLSmF13326401 = KGytfOLSmF32031573;     KGytfOLSmF32031573 = KGytfOLSmF51181473;     KGytfOLSmF51181473 = KGytfOLSmF96934534;     KGytfOLSmF96934534 = KGytfOLSmF23681285;     KGytfOLSmF23681285 = KGytfOLSmF84994819;     KGytfOLSmF84994819 = KGytfOLSmF34237913;     KGytfOLSmF34237913 = KGytfOLSmF58820441;     KGytfOLSmF58820441 = KGytfOLSmF39423503;     KGytfOLSmF39423503 = KGytfOLSmF76570848;     KGytfOLSmF76570848 = KGytfOLSmF40646696;     KGytfOLSmF40646696 = KGytfOLSmF49814142;     KGytfOLSmF49814142 = KGytfOLSmF97439348;     KGytfOLSmF97439348 = KGytfOLSmF10853177;     KGytfOLSmF10853177 = KGytfOLSmF73442522;     KGytfOLSmF73442522 = KGytfOLSmF99992567;     KGytfOLSmF99992567 = KGytfOLSmF39797387;     KGytfOLSmF39797387 = KGytfOLSmF26010604;     KGytfOLSmF26010604 = KGytfOLSmF30717691;     KGytfOLSmF30717691 = KGytfOLSmF50646136;     KGytfOLSmF50646136 = KGytfOLSmF35611658;     KGytfOLSmF35611658 = KGytfOLSmF50045302;     KGytfOLSmF50045302 = KGytfOLSmF92631897;     KGytfOLSmF92631897 = KGytfOLSmF29653861;     KGytfOLSmF29653861 = KGytfOLSmF38576331;     KGytfOLSmF38576331 = KGytfOLSmF2024027;     KGytfOLSmF2024027 = KGytfOLSmF6553756;     KGytfOLSmF6553756 = KGytfOLSmF5927207;     KGytfOLSmF5927207 = KGytfOLSmF94005278;     KGytfOLSmF94005278 = KGytfOLSmF50262692;     KGytfOLSmF50262692 = KGytfOLSmF77987522;     KGytfOLSmF77987522 = KGytfOLSmF75593694;     KGytfOLSmF75593694 = KGytfOLSmF49882318;     KGytfOLSmF49882318 = KGytfOLSmF49079019;     KGytfOLSmF49079019 = KGytfOLSmF48904921;     KGytfOLSmF48904921 = KGytfOLSmF21641826;     KGytfOLSmF21641826 = KGytfOLSmF79878710;     KGytfOLSmF79878710 = KGytfOLSmF85551013;     KGytfOLSmF85551013 = KGytfOLSmF23041321;     KGytfOLSmF23041321 = KGytfOLSmF74871326;     KGytfOLSmF74871326 = KGytfOLSmF93774084;     KGytfOLSmF93774084 = KGytfOLSmF14937983;     KGytfOLSmF14937983 = KGytfOLSmF88679828;     KGytfOLSmF88679828 = KGytfOLSmF87140115;     KGytfOLSmF87140115 = KGytfOLSmF50206156;     KGytfOLSmF50206156 = KGytfOLSmF3386115;     KGytfOLSmF3386115 = KGytfOLSmF13053733;     KGytfOLSmF13053733 = KGytfOLSmF85999717;     KGytfOLSmF85999717 = KGytfOLSmF49775790;     KGytfOLSmF49775790 = KGytfOLSmF98052042;     KGytfOLSmF98052042 = KGytfOLSmF40351958;     KGytfOLSmF40351958 = KGytfOLSmF12334572;     KGytfOLSmF12334572 = KGytfOLSmF50870330;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void fBPhhcnueN21294116() {     long oDucuoWOhL44188327 = -579500431;    long oDucuoWOhL3582566 = -751042140;    long oDucuoWOhL93819353 = -117193581;    long oDucuoWOhL49198034 = -311535932;    long oDucuoWOhL31899355 = -432147619;    long oDucuoWOhL48896857 = -649634809;    long oDucuoWOhL49460378 = -460425599;    long oDucuoWOhL88584800 = -12480454;    long oDucuoWOhL65770764 = -470715014;    long oDucuoWOhL50572917 = -487083824;    long oDucuoWOhL45997327 = 31271081;    long oDucuoWOhL19039728 = -527261191;    long oDucuoWOhL58429776 = -495008803;    long oDucuoWOhL50578120 = -74546456;    long oDucuoWOhL40928726 = -609651978;    long oDucuoWOhL18552280 = -555735830;    long oDucuoWOhL70296417 = -910568970;    long oDucuoWOhL51272321 = -234647921;    long oDucuoWOhL14042860 = -311011999;    long oDucuoWOhL25449845 = -284997567;    long oDucuoWOhL96863503 = -852441823;    long oDucuoWOhL6115625 = -43745123;    long oDucuoWOhL999328 = -7836708;    long oDucuoWOhL43118159 = -396390593;    long oDucuoWOhL13948432 = -241121605;    long oDucuoWOhL65571809 = -944226382;    long oDucuoWOhL61226917 = 5369402;    long oDucuoWOhL78792571 = -622972447;    long oDucuoWOhL28545495 = -221632865;    long oDucuoWOhL99104444 = -312353046;    long oDucuoWOhL97117458 = -793425069;    long oDucuoWOhL10462118 = -458885066;    long oDucuoWOhL94435990 = -211628941;    long oDucuoWOhL46331862 = -977968551;    long oDucuoWOhL16888620 = -858207490;    long oDucuoWOhL87261509 = -909702874;    long oDucuoWOhL32091573 = -262701253;    long oDucuoWOhL7074401 = 99450769;    long oDucuoWOhL91662317 = -572772098;    long oDucuoWOhL15476076 = -206023679;    long oDucuoWOhL16802146 = -566566796;    long oDucuoWOhL19061636 = -644190423;    long oDucuoWOhL61794838 = -14898762;    long oDucuoWOhL41392345 = -172002423;    long oDucuoWOhL38325254 = -64295989;    long oDucuoWOhL62830970 = -541277824;    long oDucuoWOhL34730446 = -609697040;    long oDucuoWOhL13944629 = -35380765;    long oDucuoWOhL16254859 = -11665537;    long oDucuoWOhL38083651 = -38396682;    long oDucuoWOhL95073397 = -581953299;    long oDucuoWOhL8662222 = -16106457;    long oDucuoWOhL88341288 = -642574293;    long oDucuoWOhL56764516 = -747696683;    long oDucuoWOhL24430612 = -16867681;    long oDucuoWOhL38072702 = -435755308;    long oDucuoWOhL2583239 = -643205432;    long oDucuoWOhL50701195 = -720802989;    long oDucuoWOhL35249602 = 29585672;    long oDucuoWOhL66327546 = -487921237;    long oDucuoWOhL87669939 = -555004211;    long oDucuoWOhL70667806 = -837453153;    long oDucuoWOhL60039306 = -790847590;    long oDucuoWOhL66666319 = -58361968;    long oDucuoWOhL53455458 = -693658755;    long oDucuoWOhL35535209 = -509843854;    long oDucuoWOhL24603737 = -215632251;    long oDucuoWOhL12097914 = -517040253;    long oDucuoWOhL33689500 = -216338966;    long oDucuoWOhL53667216 = -699949105;    long oDucuoWOhL86460707 = -193034577;    long oDucuoWOhL63222016 = -910019740;    long oDucuoWOhL59610003 = -661875823;    long oDucuoWOhL98566784 = -4988321;    long oDucuoWOhL8647699 = -718430772;    long oDucuoWOhL77801868 = -108251401;    long oDucuoWOhL44320786 = 71153638;    long oDucuoWOhL59606983 = -835834286;    long oDucuoWOhL4792905 = -232094604;    long oDucuoWOhL51117462 = -699843782;    long oDucuoWOhL30841363 = -234529342;    long oDucuoWOhL47282288 = -959249833;    long oDucuoWOhL62537712 = -511306910;    long oDucuoWOhL90461844 = -83236184;    long oDucuoWOhL4031047 = -730399748;    long oDucuoWOhL88455237 = -677318613;    long oDucuoWOhL22120830 = -816310774;    long oDucuoWOhL37671475 = -463932258;    long oDucuoWOhL21901251 = -861100871;    long oDucuoWOhL78815917 = -322452183;    long oDucuoWOhL84678271 = -166497442;    long oDucuoWOhL81390378 = -541898265;    long oDucuoWOhL71824798 = -930134903;    long oDucuoWOhL25334772 = 15149139;    long oDucuoWOhL27806136 = -651019468;    long oDucuoWOhL46134339 = -729113643;    long oDucuoWOhL59022329 = -853342834;    long oDucuoWOhL95128519 = -956536794;    long oDucuoWOhL87936886 = -478343668;    long oDucuoWOhL2790046 = -579500431;     oDucuoWOhL44188327 = oDucuoWOhL3582566;     oDucuoWOhL3582566 = oDucuoWOhL93819353;     oDucuoWOhL93819353 = oDucuoWOhL49198034;     oDucuoWOhL49198034 = oDucuoWOhL31899355;     oDucuoWOhL31899355 = oDucuoWOhL48896857;     oDucuoWOhL48896857 = oDucuoWOhL49460378;     oDucuoWOhL49460378 = oDucuoWOhL88584800;     oDucuoWOhL88584800 = oDucuoWOhL65770764;     oDucuoWOhL65770764 = oDucuoWOhL50572917;     oDucuoWOhL50572917 = oDucuoWOhL45997327;     oDucuoWOhL45997327 = oDucuoWOhL19039728;     oDucuoWOhL19039728 = oDucuoWOhL58429776;     oDucuoWOhL58429776 = oDucuoWOhL50578120;     oDucuoWOhL50578120 = oDucuoWOhL40928726;     oDucuoWOhL40928726 = oDucuoWOhL18552280;     oDucuoWOhL18552280 = oDucuoWOhL70296417;     oDucuoWOhL70296417 = oDucuoWOhL51272321;     oDucuoWOhL51272321 = oDucuoWOhL14042860;     oDucuoWOhL14042860 = oDucuoWOhL25449845;     oDucuoWOhL25449845 = oDucuoWOhL96863503;     oDucuoWOhL96863503 = oDucuoWOhL6115625;     oDucuoWOhL6115625 = oDucuoWOhL999328;     oDucuoWOhL999328 = oDucuoWOhL43118159;     oDucuoWOhL43118159 = oDucuoWOhL13948432;     oDucuoWOhL13948432 = oDucuoWOhL65571809;     oDucuoWOhL65571809 = oDucuoWOhL61226917;     oDucuoWOhL61226917 = oDucuoWOhL78792571;     oDucuoWOhL78792571 = oDucuoWOhL28545495;     oDucuoWOhL28545495 = oDucuoWOhL99104444;     oDucuoWOhL99104444 = oDucuoWOhL97117458;     oDucuoWOhL97117458 = oDucuoWOhL10462118;     oDucuoWOhL10462118 = oDucuoWOhL94435990;     oDucuoWOhL94435990 = oDucuoWOhL46331862;     oDucuoWOhL46331862 = oDucuoWOhL16888620;     oDucuoWOhL16888620 = oDucuoWOhL87261509;     oDucuoWOhL87261509 = oDucuoWOhL32091573;     oDucuoWOhL32091573 = oDucuoWOhL7074401;     oDucuoWOhL7074401 = oDucuoWOhL91662317;     oDucuoWOhL91662317 = oDucuoWOhL15476076;     oDucuoWOhL15476076 = oDucuoWOhL16802146;     oDucuoWOhL16802146 = oDucuoWOhL19061636;     oDucuoWOhL19061636 = oDucuoWOhL61794838;     oDucuoWOhL61794838 = oDucuoWOhL41392345;     oDucuoWOhL41392345 = oDucuoWOhL38325254;     oDucuoWOhL38325254 = oDucuoWOhL62830970;     oDucuoWOhL62830970 = oDucuoWOhL34730446;     oDucuoWOhL34730446 = oDucuoWOhL13944629;     oDucuoWOhL13944629 = oDucuoWOhL16254859;     oDucuoWOhL16254859 = oDucuoWOhL38083651;     oDucuoWOhL38083651 = oDucuoWOhL95073397;     oDucuoWOhL95073397 = oDucuoWOhL8662222;     oDucuoWOhL8662222 = oDucuoWOhL88341288;     oDucuoWOhL88341288 = oDucuoWOhL56764516;     oDucuoWOhL56764516 = oDucuoWOhL24430612;     oDucuoWOhL24430612 = oDucuoWOhL38072702;     oDucuoWOhL38072702 = oDucuoWOhL2583239;     oDucuoWOhL2583239 = oDucuoWOhL50701195;     oDucuoWOhL50701195 = oDucuoWOhL35249602;     oDucuoWOhL35249602 = oDucuoWOhL66327546;     oDucuoWOhL66327546 = oDucuoWOhL87669939;     oDucuoWOhL87669939 = oDucuoWOhL70667806;     oDucuoWOhL70667806 = oDucuoWOhL60039306;     oDucuoWOhL60039306 = oDucuoWOhL66666319;     oDucuoWOhL66666319 = oDucuoWOhL53455458;     oDucuoWOhL53455458 = oDucuoWOhL35535209;     oDucuoWOhL35535209 = oDucuoWOhL24603737;     oDucuoWOhL24603737 = oDucuoWOhL12097914;     oDucuoWOhL12097914 = oDucuoWOhL33689500;     oDucuoWOhL33689500 = oDucuoWOhL53667216;     oDucuoWOhL53667216 = oDucuoWOhL86460707;     oDucuoWOhL86460707 = oDucuoWOhL63222016;     oDucuoWOhL63222016 = oDucuoWOhL59610003;     oDucuoWOhL59610003 = oDucuoWOhL98566784;     oDucuoWOhL98566784 = oDucuoWOhL8647699;     oDucuoWOhL8647699 = oDucuoWOhL77801868;     oDucuoWOhL77801868 = oDucuoWOhL44320786;     oDucuoWOhL44320786 = oDucuoWOhL59606983;     oDucuoWOhL59606983 = oDucuoWOhL4792905;     oDucuoWOhL4792905 = oDucuoWOhL51117462;     oDucuoWOhL51117462 = oDucuoWOhL30841363;     oDucuoWOhL30841363 = oDucuoWOhL47282288;     oDucuoWOhL47282288 = oDucuoWOhL62537712;     oDucuoWOhL62537712 = oDucuoWOhL90461844;     oDucuoWOhL90461844 = oDucuoWOhL4031047;     oDucuoWOhL4031047 = oDucuoWOhL88455237;     oDucuoWOhL88455237 = oDucuoWOhL22120830;     oDucuoWOhL22120830 = oDucuoWOhL37671475;     oDucuoWOhL37671475 = oDucuoWOhL21901251;     oDucuoWOhL21901251 = oDucuoWOhL78815917;     oDucuoWOhL78815917 = oDucuoWOhL84678271;     oDucuoWOhL84678271 = oDucuoWOhL81390378;     oDucuoWOhL81390378 = oDucuoWOhL71824798;     oDucuoWOhL71824798 = oDucuoWOhL25334772;     oDucuoWOhL25334772 = oDucuoWOhL27806136;     oDucuoWOhL27806136 = oDucuoWOhL46134339;     oDucuoWOhL46134339 = oDucuoWOhL59022329;     oDucuoWOhL59022329 = oDucuoWOhL95128519;     oDucuoWOhL95128519 = oDucuoWOhL87936886;     oDucuoWOhL87936886 = oDucuoWOhL2790046;     oDucuoWOhL2790046 = oDucuoWOhL44188327;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void fcgaiAPrvL9851425() {     long LFPZasVvnK50578175 = -33282847;    long LFPZasVvnK66254168 = -307506709;    long LFPZasVvnK3352360 = -299937579;    long LFPZasVvnK95708377 = -133907486;    long LFPZasVvnK83074958 = -912233512;    long LFPZasVvnK87954608 = -937812871;    long LFPZasVvnK44337925 = -585529868;    long LFPZasVvnK36551382 = -636828241;    long LFPZasVvnK4843252 = -308417095;    long LFPZasVvnK2002027 = -207531076;    long LFPZasVvnK19901321 = -555807963;    long LFPZasVvnK92160818 = -648666780;    long LFPZasVvnK9427884 = 92270631;    long LFPZasVvnK2312270 = -413330580;    long LFPZasVvnK25168057 = 5498680;    long LFPZasVvnK20047088 = -997579349;    long LFPZasVvnK26098706 = -652263489;    long LFPZasVvnK4551831 = -760788883;    long LFPZasVvnK74837309 = -24737730;    long LFPZasVvnK62813974 = -340631862;    long LFPZasVvnK76785244 = -403333142;    long LFPZasVvnK59787667 = -383670239;    long LFPZasVvnK38818673 = -202817576;    long LFPZasVvnK37196778 = 32976640;    long LFPZasVvnK81116906 = -559689494;    long LFPZasVvnK64518170 = -193544946;    long LFPZasVvnK2045476 = 88116405;    long LFPZasVvnK30170797 = 85645007;    long LFPZasVvnK43193245 = -651776643;    long LFPZasVvnK63524937 = 34988190;    long LFPZasVvnK86041796 = -575180062;    long LFPZasVvnK54483458 = -94155786;    long LFPZasVvnK60185177 = -250528441;    long LFPZasVvnK35011468 = -318686526;    long LFPZasVvnK49597905 = -192880393;    long LFPZasVvnK56287662 = -293867299;    long LFPZasVvnK56444287 = -757482617;    long LFPZasVvnK20885252 = -957990616;    long LFPZasVvnK12206809 = -300038095;    long LFPZasVvnK64544886 = -482947944;    long LFPZasVvnK41445633 = -798275742;    long LFPZasVvnK15761694 = -140242131;    long LFPZasVvnK9378254 = -856180611;    long LFPZasVvnK43322563 = -671070766;    long LFPZasVvnK70338525 = -797310670;    long LFPZasVvnK19859019 = 66040621;    long LFPZasVvnK37370352 = 905122;    long LFPZasVvnK16356403 = -417437833;    long LFPZasVvnK47616078 = -975348455;    long LFPZasVvnK71581572 = -137427356;    long LFPZasVvnK10866830 = 52327044;    long LFPZasVvnK30024124 = -992772000;    long LFPZasVvnK43308038 = -126602938;    long LFPZasVvnK84239513 = -258382480;    long LFPZasVvnK33066180 = -442539563;    long LFPZasVvnK90790507 = -649612609;    long LFPZasVvnK27435495 = -4689133;    long LFPZasVvnK66155582 = -232914219;    long LFPZasVvnK14591471 = -574217993;    long LFPZasVvnK18556789 = -618688567;    long LFPZasVvnK85909132 = -925929277;    long LFPZasVvnK14167129 = -571174875;    long LFPZasVvnK93358137 = -985051599;    long LFPZasVvnK41318314 = -243405285;    long LFPZasVvnK15960230 = -632351014;    long LFPZasVvnK65417862 = -361652177;    long LFPZasVvnK31975642 = -298138340;    long LFPZasVvnK74416416 = -589042843;    long LFPZasVvnK52714364 = -120450188;    long LFPZasVvnK68880395 = -700634021;    long LFPZasVvnK63602801 = -140096733;    long LFPZasVvnK5213454 = -694272873;    long LFPZasVvnK92345021 = -360750789;    long LFPZasVvnK10292423 = -541789786;    long LFPZasVvnK21368341 = -542356121;    long LFPZasVvnK61023551 = -163091012;    long LFPZasVvnK50409413 = -527489628;    long LFPZasVvnK95496110 = -531746811;    long LFPZasVvnK66858252 = -169712691;    long LFPZasVvnK61257887 = -525730115;    long LFPZasVvnK27147818 = -94450069;    long LFPZasVvnK85689073 = -494445762;    long LFPZasVvnK82554718 = 60993462;    long LFPZasVvnK71611672 = -414349287;    long LFPZasVvnK52658108 = 82661146;    long LFPZasVvnK56017672 = -582408062;    long LFPZasVvnK11175421 = -967552849;    long LFPZasVvnK75945663 = -992145961;    long LFPZasVvnK1945288 = -876146964;    long LFPZasVvnK58807397 = -543267785;    long LFPZasVvnK28852168 = -189178167;    long LFPZasVvnK90288705 = -424568399;    long LFPZasVvnK6293781 = -283772624;    long LFPZasVvnK93650020 = -681349528;    long LFPZasVvnK78635754 = -557018668;    long LFPZasVvnK27278505 = -127100867;    long LFPZasVvnK22403556 = -155190532;    long LFPZasVvnK68059939 = -512775327;    long LFPZasVvnK27362333 = 61280248;    long LFPZasVvnK4920664 = -33282847;     LFPZasVvnK50578175 = LFPZasVvnK66254168;     LFPZasVvnK66254168 = LFPZasVvnK3352360;     LFPZasVvnK3352360 = LFPZasVvnK95708377;     LFPZasVvnK95708377 = LFPZasVvnK83074958;     LFPZasVvnK83074958 = LFPZasVvnK87954608;     LFPZasVvnK87954608 = LFPZasVvnK44337925;     LFPZasVvnK44337925 = LFPZasVvnK36551382;     LFPZasVvnK36551382 = LFPZasVvnK4843252;     LFPZasVvnK4843252 = LFPZasVvnK2002027;     LFPZasVvnK2002027 = LFPZasVvnK19901321;     LFPZasVvnK19901321 = LFPZasVvnK92160818;     LFPZasVvnK92160818 = LFPZasVvnK9427884;     LFPZasVvnK9427884 = LFPZasVvnK2312270;     LFPZasVvnK2312270 = LFPZasVvnK25168057;     LFPZasVvnK25168057 = LFPZasVvnK20047088;     LFPZasVvnK20047088 = LFPZasVvnK26098706;     LFPZasVvnK26098706 = LFPZasVvnK4551831;     LFPZasVvnK4551831 = LFPZasVvnK74837309;     LFPZasVvnK74837309 = LFPZasVvnK62813974;     LFPZasVvnK62813974 = LFPZasVvnK76785244;     LFPZasVvnK76785244 = LFPZasVvnK59787667;     LFPZasVvnK59787667 = LFPZasVvnK38818673;     LFPZasVvnK38818673 = LFPZasVvnK37196778;     LFPZasVvnK37196778 = LFPZasVvnK81116906;     LFPZasVvnK81116906 = LFPZasVvnK64518170;     LFPZasVvnK64518170 = LFPZasVvnK2045476;     LFPZasVvnK2045476 = LFPZasVvnK30170797;     LFPZasVvnK30170797 = LFPZasVvnK43193245;     LFPZasVvnK43193245 = LFPZasVvnK63524937;     LFPZasVvnK63524937 = LFPZasVvnK86041796;     LFPZasVvnK86041796 = LFPZasVvnK54483458;     LFPZasVvnK54483458 = LFPZasVvnK60185177;     LFPZasVvnK60185177 = LFPZasVvnK35011468;     LFPZasVvnK35011468 = LFPZasVvnK49597905;     LFPZasVvnK49597905 = LFPZasVvnK56287662;     LFPZasVvnK56287662 = LFPZasVvnK56444287;     LFPZasVvnK56444287 = LFPZasVvnK20885252;     LFPZasVvnK20885252 = LFPZasVvnK12206809;     LFPZasVvnK12206809 = LFPZasVvnK64544886;     LFPZasVvnK64544886 = LFPZasVvnK41445633;     LFPZasVvnK41445633 = LFPZasVvnK15761694;     LFPZasVvnK15761694 = LFPZasVvnK9378254;     LFPZasVvnK9378254 = LFPZasVvnK43322563;     LFPZasVvnK43322563 = LFPZasVvnK70338525;     LFPZasVvnK70338525 = LFPZasVvnK19859019;     LFPZasVvnK19859019 = LFPZasVvnK37370352;     LFPZasVvnK37370352 = LFPZasVvnK16356403;     LFPZasVvnK16356403 = LFPZasVvnK47616078;     LFPZasVvnK47616078 = LFPZasVvnK71581572;     LFPZasVvnK71581572 = LFPZasVvnK10866830;     LFPZasVvnK10866830 = LFPZasVvnK30024124;     LFPZasVvnK30024124 = LFPZasVvnK43308038;     LFPZasVvnK43308038 = LFPZasVvnK84239513;     LFPZasVvnK84239513 = LFPZasVvnK33066180;     LFPZasVvnK33066180 = LFPZasVvnK90790507;     LFPZasVvnK90790507 = LFPZasVvnK27435495;     LFPZasVvnK27435495 = LFPZasVvnK66155582;     LFPZasVvnK66155582 = LFPZasVvnK14591471;     LFPZasVvnK14591471 = LFPZasVvnK18556789;     LFPZasVvnK18556789 = LFPZasVvnK85909132;     LFPZasVvnK85909132 = LFPZasVvnK14167129;     LFPZasVvnK14167129 = LFPZasVvnK93358137;     LFPZasVvnK93358137 = LFPZasVvnK41318314;     LFPZasVvnK41318314 = LFPZasVvnK15960230;     LFPZasVvnK15960230 = LFPZasVvnK65417862;     LFPZasVvnK65417862 = LFPZasVvnK31975642;     LFPZasVvnK31975642 = LFPZasVvnK74416416;     LFPZasVvnK74416416 = LFPZasVvnK52714364;     LFPZasVvnK52714364 = LFPZasVvnK68880395;     LFPZasVvnK68880395 = LFPZasVvnK63602801;     LFPZasVvnK63602801 = LFPZasVvnK5213454;     LFPZasVvnK5213454 = LFPZasVvnK92345021;     LFPZasVvnK92345021 = LFPZasVvnK10292423;     LFPZasVvnK10292423 = LFPZasVvnK21368341;     LFPZasVvnK21368341 = LFPZasVvnK61023551;     LFPZasVvnK61023551 = LFPZasVvnK50409413;     LFPZasVvnK50409413 = LFPZasVvnK95496110;     LFPZasVvnK95496110 = LFPZasVvnK66858252;     LFPZasVvnK66858252 = LFPZasVvnK61257887;     LFPZasVvnK61257887 = LFPZasVvnK27147818;     LFPZasVvnK27147818 = LFPZasVvnK85689073;     LFPZasVvnK85689073 = LFPZasVvnK82554718;     LFPZasVvnK82554718 = LFPZasVvnK71611672;     LFPZasVvnK71611672 = LFPZasVvnK52658108;     LFPZasVvnK52658108 = LFPZasVvnK56017672;     LFPZasVvnK56017672 = LFPZasVvnK11175421;     LFPZasVvnK11175421 = LFPZasVvnK75945663;     LFPZasVvnK75945663 = LFPZasVvnK1945288;     LFPZasVvnK1945288 = LFPZasVvnK58807397;     LFPZasVvnK58807397 = LFPZasVvnK28852168;     LFPZasVvnK28852168 = LFPZasVvnK90288705;     LFPZasVvnK90288705 = LFPZasVvnK6293781;     LFPZasVvnK6293781 = LFPZasVvnK93650020;     LFPZasVvnK93650020 = LFPZasVvnK78635754;     LFPZasVvnK78635754 = LFPZasVvnK27278505;     LFPZasVvnK27278505 = LFPZasVvnK22403556;     LFPZasVvnK22403556 = LFPZasVvnK68059939;     LFPZasVvnK68059939 = LFPZasVvnK27362333;     LFPZasVvnK27362333 = LFPZasVvnK4920664;     LFPZasVvnK4920664 = LFPZasVvnK50578175;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void sAqaBLtiDA94261696() {     float RicENgzIZX71224988 = -280825084;    float RicENgzIZX38856923 = -211096352;    float RicENgzIZX9306188 = -985078499;    float RicENgzIZX24449560 = 85548502;    float RicENgzIZX11155357 = -496906350;    float RicENgzIZX4058446 = -413968631;    float RicENgzIZX89127322 = -637707928;    float RicENgzIZX95304300 = -211778319;    float RicENgzIZX62763429 = 88001024;    float RicENgzIZX22448122 = -511231580;    float RicENgzIZX77390935 = -10313946;    float RicENgzIZX48426147 = -216039345;    float RicENgzIZX63144956 = -108510283;    float RicENgzIZX51542468 = 76776421;    float RicENgzIZX91767158 = -116285962;    float RicENgzIZX33499139 = -542159674;    float RicENgzIZX61841102 = -976159546;    float RicENgzIZX67095724 = -222758795;    float RicENgzIZX96866293 = -645412372;    float RicENgzIZX25656006 = -792016506;    float RicENgzIZX91673066 = -644862063;    float RicENgzIZX54905631 = -272449457;    float RicENgzIZX68421279 = -289879103;    float RicENgzIZX53783896 = -20114658;    float RicENgzIZX91573312 = -280614571;    float RicENgzIZX82546308 = -772983837;    float RicENgzIZX87972447 = -532661154;    float RicENgzIZX64505665 = -326204002;    float RicENgzIZX34844568 = -605687596;    float RicENgzIZX84764435 = -279980411;    float RicENgzIZX18173302 = -63933673;    float RicENgzIZX87537812 = -166319270;    float RicENgzIZX57219855 = -623948212;    float RicENgzIZX588571 = -64727510;    float RicENgzIZX39390670 = -234312616;    float RicENgzIZX48402063 = -157918157;    float RicENgzIZX34191799 = -195815858;    float RicENgzIZX76116000 = -738550733;    float RicENgzIZX67225295 = -515277793;    float RicENgzIZX38552854 = 17369928;    float RicENgzIZX92410668 = -423252544;    float RicENgzIZX33517931 = -54975262;    float RicENgzIZX28557734 = -712282354;    float RicENgzIZX69254787 = -410769070;    float RicENgzIZX79306123 = -635901616;    float RicENgzIZX56585830 = -926330037;    float RicENgzIZX34274900 = 73874856;    float RicENgzIZX13508299 = -501139145;    float RicENgzIZX42051323 = -358935320;    float RicENgzIZX45043126 = -110704900;    float RicENgzIZX39000681 = -278161333;    float RicENgzIZX32806072 = -973601033;    float RicENgzIZX29969956 = -798441486;    float RicENgzIZX32154522 = -493203865;    float RicENgzIZX2588630 = -371154935;    float RicENgzIZX16319357 = 91624373;    float RicENgzIZX70435643 = -921217250;    float RicENgzIZX55522292 = -864963841;    float RicENgzIZX32876248 = -633836927;    float RicENgzIZX28609048 = -723922514;    float RicENgzIZX16085998 = -881307478;    float RicENgzIZX24621657 = -211503926;    float RicENgzIZX60459733 = -606090724;    float RicENgzIZX77998993 = -632018565;    float RicENgzIZX4274821 = -347297907;    float RicENgzIZX89853123 = -843994677;    float RicENgzIZX91206292 = -592091134;    float RicENgzIZX62556386 = 56217226;    float RicENgzIZX12151799 = -688910964;    float RicENgzIZX43365095 = -958367805;    float RicENgzIZX99307339 = -246343816;    float RicENgzIZX85725102 = -137608813;    float RicENgzIZX99870428 = -707481002;    float RicENgzIZX58313439 = -562782301;    float RicENgzIZX33245338 = -268763962;    float RicENgzIZX58155136 = -489886801;    float RicENgzIZX26347898 = -560167103;    float RicENgzIZX99166492 = -879110033;    float RicENgzIZX74477772 = -384213043;    float RicENgzIZX34987482 = -354284535;    float RicENgzIZX48271408 = -746858694;    float RicENgzIZX74464149 = 68477990;    float RicENgzIZX22454343 = -967268682;    float RicENgzIZX89801441 = -394982696;    float RicENgzIZX45763754 = 98180921;    float RicENgzIZX85367230 = -90332641;    float RicENgzIZX57567856 = -367877785;    float RicENgzIZX25065334 = -30744347;    float RicENgzIZX97999941 = -693572575;    float RicENgzIZX23071313 = -225936989;    float RicENgzIZX77966420 = -236700907;    float RicENgzIZX78669506 = -330852018;    float RicENgzIZX43239752 = -4713807;    float RicENgzIZX38616247 = -791355280;    float RicENgzIZX22466857 = -101322595;    float RicENgzIZX67789011 = -111748618;    float RicENgzIZX73058197 = -448884539;    float RicENgzIZX50558740 = 19736211;    float RicENgzIZX64979966 = 36528836;    float RicENgzIZX89453000 = -280825084;     RicENgzIZX71224988 = RicENgzIZX38856923;     RicENgzIZX38856923 = RicENgzIZX9306188;     RicENgzIZX9306188 = RicENgzIZX24449560;     RicENgzIZX24449560 = RicENgzIZX11155357;     RicENgzIZX11155357 = RicENgzIZX4058446;     RicENgzIZX4058446 = RicENgzIZX89127322;     RicENgzIZX89127322 = RicENgzIZX95304300;     RicENgzIZX95304300 = RicENgzIZX62763429;     RicENgzIZX62763429 = RicENgzIZX22448122;     RicENgzIZX22448122 = RicENgzIZX77390935;     RicENgzIZX77390935 = RicENgzIZX48426147;     RicENgzIZX48426147 = RicENgzIZX63144956;     RicENgzIZX63144956 = RicENgzIZX51542468;     RicENgzIZX51542468 = RicENgzIZX91767158;     RicENgzIZX91767158 = RicENgzIZX33499139;     RicENgzIZX33499139 = RicENgzIZX61841102;     RicENgzIZX61841102 = RicENgzIZX67095724;     RicENgzIZX67095724 = RicENgzIZX96866293;     RicENgzIZX96866293 = RicENgzIZX25656006;     RicENgzIZX25656006 = RicENgzIZX91673066;     RicENgzIZX91673066 = RicENgzIZX54905631;     RicENgzIZX54905631 = RicENgzIZX68421279;     RicENgzIZX68421279 = RicENgzIZX53783896;     RicENgzIZX53783896 = RicENgzIZX91573312;     RicENgzIZX91573312 = RicENgzIZX82546308;     RicENgzIZX82546308 = RicENgzIZX87972447;     RicENgzIZX87972447 = RicENgzIZX64505665;     RicENgzIZX64505665 = RicENgzIZX34844568;     RicENgzIZX34844568 = RicENgzIZX84764435;     RicENgzIZX84764435 = RicENgzIZX18173302;     RicENgzIZX18173302 = RicENgzIZX87537812;     RicENgzIZX87537812 = RicENgzIZX57219855;     RicENgzIZX57219855 = RicENgzIZX588571;     RicENgzIZX588571 = RicENgzIZX39390670;     RicENgzIZX39390670 = RicENgzIZX48402063;     RicENgzIZX48402063 = RicENgzIZX34191799;     RicENgzIZX34191799 = RicENgzIZX76116000;     RicENgzIZX76116000 = RicENgzIZX67225295;     RicENgzIZX67225295 = RicENgzIZX38552854;     RicENgzIZX38552854 = RicENgzIZX92410668;     RicENgzIZX92410668 = RicENgzIZX33517931;     RicENgzIZX33517931 = RicENgzIZX28557734;     RicENgzIZX28557734 = RicENgzIZX69254787;     RicENgzIZX69254787 = RicENgzIZX79306123;     RicENgzIZX79306123 = RicENgzIZX56585830;     RicENgzIZX56585830 = RicENgzIZX34274900;     RicENgzIZX34274900 = RicENgzIZX13508299;     RicENgzIZX13508299 = RicENgzIZX42051323;     RicENgzIZX42051323 = RicENgzIZX45043126;     RicENgzIZX45043126 = RicENgzIZX39000681;     RicENgzIZX39000681 = RicENgzIZX32806072;     RicENgzIZX32806072 = RicENgzIZX29969956;     RicENgzIZX29969956 = RicENgzIZX32154522;     RicENgzIZX32154522 = RicENgzIZX2588630;     RicENgzIZX2588630 = RicENgzIZX16319357;     RicENgzIZX16319357 = RicENgzIZX70435643;     RicENgzIZX70435643 = RicENgzIZX55522292;     RicENgzIZX55522292 = RicENgzIZX32876248;     RicENgzIZX32876248 = RicENgzIZX28609048;     RicENgzIZX28609048 = RicENgzIZX16085998;     RicENgzIZX16085998 = RicENgzIZX24621657;     RicENgzIZX24621657 = RicENgzIZX60459733;     RicENgzIZX60459733 = RicENgzIZX77998993;     RicENgzIZX77998993 = RicENgzIZX4274821;     RicENgzIZX4274821 = RicENgzIZX89853123;     RicENgzIZX89853123 = RicENgzIZX91206292;     RicENgzIZX91206292 = RicENgzIZX62556386;     RicENgzIZX62556386 = RicENgzIZX12151799;     RicENgzIZX12151799 = RicENgzIZX43365095;     RicENgzIZX43365095 = RicENgzIZX99307339;     RicENgzIZX99307339 = RicENgzIZX85725102;     RicENgzIZX85725102 = RicENgzIZX99870428;     RicENgzIZX99870428 = RicENgzIZX58313439;     RicENgzIZX58313439 = RicENgzIZX33245338;     RicENgzIZX33245338 = RicENgzIZX58155136;     RicENgzIZX58155136 = RicENgzIZX26347898;     RicENgzIZX26347898 = RicENgzIZX99166492;     RicENgzIZX99166492 = RicENgzIZX74477772;     RicENgzIZX74477772 = RicENgzIZX34987482;     RicENgzIZX34987482 = RicENgzIZX48271408;     RicENgzIZX48271408 = RicENgzIZX74464149;     RicENgzIZX74464149 = RicENgzIZX22454343;     RicENgzIZX22454343 = RicENgzIZX89801441;     RicENgzIZX89801441 = RicENgzIZX45763754;     RicENgzIZX45763754 = RicENgzIZX85367230;     RicENgzIZX85367230 = RicENgzIZX57567856;     RicENgzIZX57567856 = RicENgzIZX25065334;     RicENgzIZX25065334 = RicENgzIZX97999941;     RicENgzIZX97999941 = RicENgzIZX23071313;     RicENgzIZX23071313 = RicENgzIZX77966420;     RicENgzIZX77966420 = RicENgzIZX78669506;     RicENgzIZX78669506 = RicENgzIZX43239752;     RicENgzIZX43239752 = RicENgzIZX38616247;     RicENgzIZX38616247 = RicENgzIZX22466857;     RicENgzIZX22466857 = RicENgzIZX67789011;     RicENgzIZX67789011 = RicENgzIZX73058197;     RicENgzIZX73058197 = RicENgzIZX50558740;     RicENgzIZX50558740 = RicENgzIZX64979966;     RicENgzIZX64979966 = RicENgzIZX89453000;     RicENgzIZX89453000 = RicENgzIZX71224988;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void XUsWiynWKZ91539525() {     float nApxGjGnPo29077545 = -508981805;    float nApxGjGnPo97904184 = 2404112;    float nApxGjGnPo74148731 = -933270804;    float nApxGjGnPo26599742 = 78530730;    float nApxGjGnPo56774778 = -725206876;    float nApxGjGnPo20359002 = -837492093;    float nApxGjGnPo23393132 = -599884501;    float nApxGjGnPo63855670 = -622431786;    float nApxGjGnPo78704001 = -570880352;    float nApxGjGnPo32474906 = -281853039;    float nApxGjGnPo32793925 = -949474004;    float nApxGjGnPo90605567 = -595427047;    float nApxGjGnPo36534869 = -157581633;    float nApxGjGnPo93453863 = -82142694;    float nApxGjGnPo41524843 = -284475838;    float nApxGjGnPo86731804 = -226843173;    float nApxGjGnPo11647630 = -860799349;    float nApxGjGnPo49831799 = -350857933;    float nApxGjGnPo18269985 = -479095652;    float nApxGjGnPo25935744 = -859187574;    float nApxGjGnPo25973613 = -583157687;    float nApxGjGnPo43846108 = -353860181;    float nApxGjGnPo5361586 = -744634037;    float nApxGjGnPo30426520 = -114011597;    float nApxGjGnPo6688821 = -599465041;    float nApxGjGnPo64973397 = -997248939;    float nApxGjGnPo25260861 = -895732075;    float nApxGjGnPo75742015 = -775463641;    float nApxGjGnPo75726896 = -877969202;    float nApxGjGnPo23365156 = -559352075;    float nApxGjGnPo47634660 = -310137078;    float nApxGjGnPo98178164 = -199540278;    float nApxGjGnPo65024113 = 1056532;    float nApxGjGnPo89442846 = -576866441;    float nApxGjGnPo99070449 = 10144636;    float nApxGjGnPo98827586 = -608113093;    float nApxGjGnPo65101959 = -793806472;    float nApxGjGnPo11039825 = -593569107;    float nApxGjGnPo10610392 = -954518685;    float nApxGjGnPo51137208 = 25315039;    float nApxGjGnPo66763979 = -607685226;    float nApxGjGnPo25693788 = -313637162;    float nApxGjGnPo63207944 = -693863470;    float nApxGjGnPo49598346 = -355500686;    float nApxGjGnPo63287796 = -898296853;    float nApxGjGnPo70219472 = -235438094;    float nApxGjGnPo38219668 = -507241135;    float nApxGjGnPo449704 = -915439373;    float nApxGjGnPo40745505 = -408625038;    float nApxGjGnPo46611038 = -56727088;    float nApxGjGnPo15381252 = -928959160;    float nApxGjGnPo75595544 = -135926913;    float nApxGjGnPo1457160 = -334344262;    float nApxGjGnPo98182642 = -428473322;    float nApxGjGnPo65565912 = -1449919;    float nApxGjGnPo85231437 = -55121625;    float nApxGjGnPo92542599 = -252961852;    float nApxGjGnPo43722211 = -719259208;    float nApxGjGnPo19910922 = -322004230;    float nApxGjGnPo91801380 = -727957937;    float nApxGjGnPo95098141 = -941760018;    float nApxGjGnPo47651116 = -824420860;    float nApxGjGnPo88128773 = -744462585;    float nApxGjGnPo55338845 = 88471722;    float nApxGjGnPo84840245 = -971715962;    float nApxGjGnPo34615761 = -649933727;    float nApxGjGnPo25581455 = -496483580;    float nApxGjGnPo47092022 = -580715193;    float nApxGjGnPo94383413 = 7712670;    float nApxGjGnPo42697256 = -676362746;    float nApxGjGnPo21629845 = -433036702;    float nApxGjGnPo607806 = -167230242;    float nApxGjGnPo39221408 = -396339248;    float nApxGjGnPo67132776 = -404410692;    float nApxGjGnPo59171765 = -151502349;    float nApxGjGnPo279826 = -169520525;    float nApxGjGnPo80638164 = -659996712;    float nApxGjGnPo55763240 = -289133352;    float nApxGjGnPo67138723 = -215714745;    float nApxGjGnPo36469348 = -264026948;    float nApxGjGnPo26753729 = -390007805;    float nApxGjGnPo24811157 = -980292702;    float nApxGjGnPo34996511 = -266838604;    float nApxGjGnPo29115858 = -721242115;    float nApxGjGnPo7983905 = -630392915;    float nApxGjGnPo72039116 = -74210165;    float nApxGjGnPo96721004 = -865196016;    float nApxGjGnPo66841471 = -570470146;    float nApxGjGnPo23876934 = -475416523;    float nApxGjGnPo13839013 = -934733740;    float nApxGjGnPo6284988 = -255151241;    float nApxGjGnPo21379749 = 25452735;    float nApxGjGnPo91128903 = -171564878;    float nApxGjGnPo18809012 = -126560749;    float nApxGjGnPo56039067 = -32924943;    float nApxGjGnPo19112863 = -783264366;    float nApxGjGnPo37565014 = -569174578;    float nApxGjGnPo7869099 = -682335192;    float nApxGjGnPo64758100 = -383784724;    float nApxGjGnPo28672036 = -508981805;     nApxGjGnPo29077545 = nApxGjGnPo97904184;     nApxGjGnPo97904184 = nApxGjGnPo74148731;     nApxGjGnPo74148731 = nApxGjGnPo26599742;     nApxGjGnPo26599742 = nApxGjGnPo56774778;     nApxGjGnPo56774778 = nApxGjGnPo20359002;     nApxGjGnPo20359002 = nApxGjGnPo23393132;     nApxGjGnPo23393132 = nApxGjGnPo63855670;     nApxGjGnPo63855670 = nApxGjGnPo78704001;     nApxGjGnPo78704001 = nApxGjGnPo32474906;     nApxGjGnPo32474906 = nApxGjGnPo32793925;     nApxGjGnPo32793925 = nApxGjGnPo90605567;     nApxGjGnPo90605567 = nApxGjGnPo36534869;     nApxGjGnPo36534869 = nApxGjGnPo93453863;     nApxGjGnPo93453863 = nApxGjGnPo41524843;     nApxGjGnPo41524843 = nApxGjGnPo86731804;     nApxGjGnPo86731804 = nApxGjGnPo11647630;     nApxGjGnPo11647630 = nApxGjGnPo49831799;     nApxGjGnPo49831799 = nApxGjGnPo18269985;     nApxGjGnPo18269985 = nApxGjGnPo25935744;     nApxGjGnPo25935744 = nApxGjGnPo25973613;     nApxGjGnPo25973613 = nApxGjGnPo43846108;     nApxGjGnPo43846108 = nApxGjGnPo5361586;     nApxGjGnPo5361586 = nApxGjGnPo30426520;     nApxGjGnPo30426520 = nApxGjGnPo6688821;     nApxGjGnPo6688821 = nApxGjGnPo64973397;     nApxGjGnPo64973397 = nApxGjGnPo25260861;     nApxGjGnPo25260861 = nApxGjGnPo75742015;     nApxGjGnPo75742015 = nApxGjGnPo75726896;     nApxGjGnPo75726896 = nApxGjGnPo23365156;     nApxGjGnPo23365156 = nApxGjGnPo47634660;     nApxGjGnPo47634660 = nApxGjGnPo98178164;     nApxGjGnPo98178164 = nApxGjGnPo65024113;     nApxGjGnPo65024113 = nApxGjGnPo89442846;     nApxGjGnPo89442846 = nApxGjGnPo99070449;     nApxGjGnPo99070449 = nApxGjGnPo98827586;     nApxGjGnPo98827586 = nApxGjGnPo65101959;     nApxGjGnPo65101959 = nApxGjGnPo11039825;     nApxGjGnPo11039825 = nApxGjGnPo10610392;     nApxGjGnPo10610392 = nApxGjGnPo51137208;     nApxGjGnPo51137208 = nApxGjGnPo66763979;     nApxGjGnPo66763979 = nApxGjGnPo25693788;     nApxGjGnPo25693788 = nApxGjGnPo63207944;     nApxGjGnPo63207944 = nApxGjGnPo49598346;     nApxGjGnPo49598346 = nApxGjGnPo63287796;     nApxGjGnPo63287796 = nApxGjGnPo70219472;     nApxGjGnPo70219472 = nApxGjGnPo38219668;     nApxGjGnPo38219668 = nApxGjGnPo449704;     nApxGjGnPo449704 = nApxGjGnPo40745505;     nApxGjGnPo40745505 = nApxGjGnPo46611038;     nApxGjGnPo46611038 = nApxGjGnPo15381252;     nApxGjGnPo15381252 = nApxGjGnPo75595544;     nApxGjGnPo75595544 = nApxGjGnPo1457160;     nApxGjGnPo1457160 = nApxGjGnPo98182642;     nApxGjGnPo98182642 = nApxGjGnPo65565912;     nApxGjGnPo65565912 = nApxGjGnPo85231437;     nApxGjGnPo85231437 = nApxGjGnPo92542599;     nApxGjGnPo92542599 = nApxGjGnPo43722211;     nApxGjGnPo43722211 = nApxGjGnPo19910922;     nApxGjGnPo19910922 = nApxGjGnPo91801380;     nApxGjGnPo91801380 = nApxGjGnPo95098141;     nApxGjGnPo95098141 = nApxGjGnPo47651116;     nApxGjGnPo47651116 = nApxGjGnPo88128773;     nApxGjGnPo88128773 = nApxGjGnPo55338845;     nApxGjGnPo55338845 = nApxGjGnPo84840245;     nApxGjGnPo84840245 = nApxGjGnPo34615761;     nApxGjGnPo34615761 = nApxGjGnPo25581455;     nApxGjGnPo25581455 = nApxGjGnPo47092022;     nApxGjGnPo47092022 = nApxGjGnPo94383413;     nApxGjGnPo94383413 = nApxGjGnPo42697256;     nApxGjGnPo42697256 = nApxGjGnPo21629845;     nApxGjGnPo21629845 = nApxGjGnPo607806;     nApxGjGnPo607806 = nApxGjGnPo39221408;     nApxGjGnPo39221408 = nApxGjGnPo67132776;     nApxGjGnPo67132776 = nApxGjGnPo59171765;     nApxGjGnPo59171765 = nApxGjGnPo279826;     nApxGjGnPo279826 = nApxGjGnPo80638164;     nApxGjGnPo80638164 = nApxGjGnPo55763240;     nApxGjGnPo55763240 = nApxGjGnPo67138723;     nApxGjGnPo67138723 = nApxGjGnPo36469348;     nApxGjGnPo36469348 = nApxGjGnPo26753729;     nApxGjGnPo26753729 = nApxGjGnPo24811157;     nApxGjGnPo24811157 = nApxGjGnPo34996511;     nApxGjGnPo34996511 = nApxGjGnPo29115858;     nApxGjGnPo29115858 = nApxGjGnPo7983905;     nApxGjGnPo7983905 = nApxGjGnPo72039116;     nApxGjGnPo72039116 = nApxGjGnPo96721004;     nApxGjGnPo96721004 = nApxGjGnPo66841471;     nApxGjGnPo66841471 = nApxGjGnPo23876934;     nApxGjGnPo23876934 = nApxGjGnPo13839013;     nApxGjGnPo13839013 = nApxGjGnPo6284988;     nApxGjGnPo6284988 = nApxGjGnPo21379749;     nApxGjGnPo21379749 = nApxGjGnPo91128903;     nApxGjGnPo91128903 = nApxGjGnPo18809012;     nApxGjGnPo18809012 = nApxGjGnPo56039067;     nApxGjGnPo56039067 = nApxGjGnPo19112863;     nApxGjGnPo19112863 = nApxGjGnPo37565014;     nApxGjGnPo37565014 = nApxGjGnPo7869099;     nApxGjGnPo7869099 = nApxGjGnPo64758100;     nApxGjGnPo64758100 = nApxGjGnPo28672036;     nApxGjGnPo28672036 = nApxGjGnPo29077545;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void qTjKSTZhIk97537875() {     float FhLFlmRWqP38392812 = -411512831;    float FhLFlmRWqP53327107 = -14130391;    float FhLFlmRWqP94300811 = -646911417;    float FhLFlmRWqP84389764 = -113133262;    float FhLFlmRWqP96838016 = -701722033;    float FhLFlmRWqP13902364 = -296360953;    float FhLFlmRWqP97047203 = -399133379;    float FhLFlmRWqP52991826 = -819390934;    float FhLFlmRWqP71512658 = -950941023;    float FhLFlmRWqP1099363 = -102648707;    float FhLFlmRWqP69695909 = -40715077;    float FhLFlmRWqP1843318 = -132796863;    float FhLFlmRWqP32316585 = -843003768;    float FhLFlmRWqP25542505 = -61196798;    float FhLFlmRWqP56800880 = -136006249;    float FhLFlmRWqP91702328 = -254366651;    float FhLFlmRWqP55458398 = -888384436;    float FhLFlmRWqP62024440 = -80915245;    float FhLFlmRWqP282917 = -432736481;    float FhLFlmRWqP89131092 = -937895415;    float FhLFlmRWqP14652965 = -908857615;    float FhLFlmRWqP68055020 = -176756513;    float FhLFlmRWqP41422852 = -359163037;    float FhLFlmRWqP89633149 = -731172707;    float FhLFlmRWqP69751363 = -918598092;    float FhLFlmRWqP30881215 = 3539420;    float FhLFlmRWqP59019127 = -604620920;    float FhLFlmRWqP46836491 = -182600374;    float FhLFlmRWqP42843803 = -992388638;    float FhLFlmRWqP36146104 = -365436637;    float FhLFlmRWqP17633039 = 79211105;    float FhLFlmRWqP75437527 = -630711573;    float FhLFlmRWqP14883444 = -910034480;    float FhLFlmRWqP78471795 = -60426328;    float FhLFlmRWqP85720725 = -166267958;    float FhLFlmRWqP30652481 = 75661460;    float FhLFlmRWqP2569568 = -395006335;    float FhLFlmRWqP67076623 = -346164470;    float FhLFlmRWqP76836092 = 94265528;    float FhLFlmRWqP27237106 = -781870473;    float FhLFlmRWqP90827113 = -744841644;    float FhLFlmRWqP13345444 = -234909253;    float FhLFlmRWqP84924949 = -915743851;    float FhLFlmRWqP8355247 = -845895574;    float FhLFlmRWqP99237871 = -690072647;    float FhLFlmRWqP40458708 = -560972653;    float FhLFlmRWqP43469299 = -80075280;    float FhLFlmRWqP71920741 = -261982762;    float FhLFlmRWqP6772649 = -644321554;    float FhLFlmRWqP16248940 = -949740790;    float FhLFlmRWqP52348961 = -664835157;    float FhLFlmRWqP39812586 = -783913131;    float FhLFlmRWqP89464817 = 77878832;    float FhLFlmRWqP2763887 = -788326440;    float FhLFlmRWqP82884911 = 63631997;    float FhLFlmRWqP70337791 = -134756318;    float FhLFlmRWqP11904255 = -654967355;    float FhLFlmRWqP4667663 = -915738710;    float FhLFlmRWqP14638402 = -194535171;    float FhLFlmRWqP65956802 = -605261453;    float FhLFlmRWqP54883236 = -691740034;    float FhLFlmRWqP50210712 = -116533005;    float FhLFlmRWqP10148023 = -827002297;    float FhLFlmRWqP35366554 = -485504386;    float FhLFlmRWqP83466324 = -81859813;    float FhLFlmRWqP94258381 = -410003505;    float FhLFlmRWqP86959874 = -222762383;    float FhLFlmRWqP53844789 = -682577440;    float FhLFlmRWqP39821779 = -894928841;    float FhLFlmRWqP26148400 = -111667710;    float FhLFlmRWqP89132761 = -859360317;    float FhLFlmRWqP88381774 = -442219967;    float FhLFlmRWqP85188347 = -75180773;    float FhLFlmRWqP73045811 = -650866008;    float FhLFlmRWqP98303979 = -93053772;    float FhLFlmRWqP1307521 = -573948362;    float FhLFlmRWqP83130071 = -261012663;    float FhLFlmRWqP33067606 = -513267464;    float FhLFlmRWqP90395277 = 58899939;    float FhLFlmRWqP29292656 = -257625440;    float FhLFlmRWqP87411915 = -916385301;    float FhLFlmRWqP87098386 = -242638159;    float FhLFlmRWqP40063843 = -538278821;    float FhLFlmRWqP26594863 = 57352152;    float FhLFlmRWqP83797143 = -700601481;    float FhLFlmRWqP77820452 = -136875764;    float FhLFlmRWqP85972710 = -608590405;    float FhLFlmRWqP12119557 = -21708041;    float FhLFlmRWqP95586883 = -24058325;    float FhLFlmRWqP15382935 = 68488359;    float FhLFlmRWqP18748226 = -269371186;    float FhLFlmRWqP97901904 = -479267625;    float FhLFlmRWqP52438222 = -51629299;    float FhLFlmRWqP10879291 = -300473020;    float FhLFlmRWqP72353869 = 9869560;    float FhLFlmRWqP40616401 = -528308639;    float FhLFlmRWqP3197421 = -407906957;    float FhLFlmRWqP49558396 = -330239465;    float FhLFlmRWqP24888923 = -664035761;    float FhLFlmRWqP4979490 = -411512831;     FhLFlmRWqP38392812 = FhLFlmRWqP53327107;     FhLFlmRWqP53327107 = FhLFlmRWqP94300811;     FhLFlmRWqP94300811 = FhLFlmRWqP84389764;     FhLFlmRWqP84389764 = FhLFlmRWqP96838016;     FhLFlmRWqP96838016 = FhLFlmRWqP13902364;     FhLFlmRWqP13902364 = FhLFlmRWqP97047203;     FhLFlmRWqP97047203 = FhLFlmRWqP52991826;     FhLFlmRWqP52991826 = FhLFlmRWqP71512658;     FhLFlmRWqP71512658 = FhLFlmRWqP1099363;     FhLFlmRWqP1099363 = FhLFlmRWqP69695909;     FhLFlmRWqP69695909 = FhLFlmRWqP1843318;     FhLFlmRWqP1843318 = FhLFlmRWqP32316585;     FhLFlmRWqP32316585 = FhLFlmRWqP25542505;     FhLFlmRWqP25542505 = FhLFlmRWqP56800880;     FhLFlmRWqP56800880 = FhLFlmRWqP91702328;     FhLFlmRWqP91702328 = FhLFlmRWqP55458398;     FhLFlmRWqP55458398 = FhLFlmRWqP62024440;     FhLFlmRWqP62024440 = FhLFlmRWqP282917;     FhLFlmRWqP282917 = FhLFlmRWqP89131092;     FhLFlmRWqP89131092 = FhLFlmRWqP14652965;     FhLFlmRWqP14652965 = FhLFlmRWqP68055020;     FhLFlmRWqP68055020 = FhLFlmRWqP41422852;     FhLFlmRWqP41422852 = FhLFlmRWqP89633149;     FhLFlmRWqP89633149 = FhLFlmRWqP69751363;     FhLFlmRWqP69751363 = FhLFlmRWqP30881215;     FhLFlmRWqP30881215 = FhLFlmRWqP59019127;     FhLFlmRWqP59019127 = FhLFlmRWqP46836491;     FhLFlmRWqP46836491 = FhLFlmRWqP42843803;     FhLFlmRWqP42843803 = FhLFlmRWqP36146104;     FhLFlmRWqP36146104 = FhLFlmRWqP17633039;     FhLFlmRWqP17633039 = FhLFlmRWqP75437527;     FhLFlmRWqP75437527 = FhLFlmRWqP14883444;     FhLFlmRWqP14883444 = FhLFlmRWqP78471795;     FhLFlmRWqP78471795 = FhLFlmRWqP85720725;     FhLFlmRWqP85720725 = FhLFlmRWqP30652481;     FhLFlmRWqP30652481 = FhLFlmRWqP2569568;     FhLFlmRWqP2569568 = FhLFlmRWqP67076623;     FhLFlmRWqP67076623 = FhLFlmRWqP76836092;     FhLFlmRWqP76836092 = FhLFlmRWqP27237106;     FhLFlmRWqP27237106 = FhLFlmRWqP90827113;     FhLFlmRWqP90827113 = FhLFlmRWqP13345444;     FhLFlmRWqP13345444 = FhLFlmRWqP84924949;     FhLFlmRWqP84924949 = FhLFlmRWqP8355247;     FhLFlmRWqP8355247 = FhLFlmRWqP99237871;     FhLFlmRWqP99237871 = FhLFlmRWqP40458708;     FhLFlmRWqP40458708 = FhLFlmRWqP43469299;     FhLFlmRWqP43469299 = FhLFlmRWqP71920741;     FhLFlmRWqP71920741 = FhLFlmRWqP6772649;     FhLFlmRWqP6772649 = FhLFlmRWqP16248940;     FhLFlmRWqP16248940 = FhLFlmRWqP52348961;     FhLFlmRWqP52348961 = FhLFlmRWqP39812586;     FhLFlmRWqP39812586 = FhLFlmRWqP89464817;     FhLFlmRWqP89464817 = FhLFlmRWqP2763887;     FhLFlmRWqP2763887 = FhLFlmRWqP82884911;     FhLFlmRWqP82884911 = FhLFlmRWqP70337791;     FhLFlmRWqP70337791 = FhLFlmRWqP11904255;     FhLFlmRWqP11904255 = FhLFlmRWqP4667663;     FhLFlmRWqP4667663 = FhLFlmRWqP14638402;     FhLFlmRWqP14638402 = FhLFlmRWqP65956802;     FhLFlmRWqP65956802 = FhLFlmRWqP54883236;     FhLFlmRWqP54883236 = FhLFlmRWqP50210712;     FhLFlmRWqP50210712 = FhLFlmRWqP10148023;     FhLFlmRWqP10148023 = FhLFlmRWqP35366554;     FhLFlmRWqP35366554 = FhLFlmRWqP83466324;     FhLFlmRWqP83466324 = FhLFlmRWqP94258381;     FhLFlmRWqP94258381 = FhLFlmRWqP86959874;     FhLFlmRWqP86959874 = FhLFlmRWqP53844789;     FhLFlmRWqP53844789 = FhLFlmRWqP39821779;     FhLFlmRWqP39821779 = FhLFlmRWqP26148400;     FhLFlmRWqP26148400 = FhLFlmRWqP89132761;     FhLFlmRWqP89132761 = FhLFlmRWqP88381774;     FhLFlmRWqP88381774 = FhLFlmRWqP85188347;     FhLFlmRWqP85188347 = FhLFlmRWqP73045811;     FhLFlmRWqP73045811 = FhLFlmRWqP98303979;     FhLFlmRWqP98303979 = FhLFlmRWqP1307521;     FhLFlmRWqP1307521 = FhLFlmRWqP83130071;     FhLFlmRWqP83130071 = FhLFlmRWqP33067606;     FhLFlmRWqP33067606 = FhLFlmRWqP90395277;     FhLFlmRWqP90395277 = FhLFlmRWqP29292656;     FhLFlmRWqP29292656 = FhLFlmRWqP87411915;     FhLFlmRWqP87411915 = FhLFlmRWqP87098386;     FhLFlmRWqP87098386 = FhLFlmRWqP40063843;     FhLFlmRWqP40063843 = FhLFlmRWqP26594863;     FhLFlmRWqP26594863 = FhLFlmRWqP83797143;     FhLFlmRWqP83797143 = FhLFlmRWqP77820452;     FhLFlmRWqP77820452 = FhLFlmRWqP85972710;     FhLFlmRWqP85972710 = FhLFlmRWqP12119557;     FhLFlmRWqP12119557 = FhLFlmRWqP95586883;     FhLFlmRWqP95586883 = FhLFlmRWqP15382935;     FhLFlmRWqP15382935 = FhLFlmRWqP18748226;     FhLFlmRWqP18748226 = FhLFlmRWqP97901904;     FhLFlmRWqP97901904 = FhLFlmRWqP52438222;     FhLFlmRWqP52438222 = FhLFlmRWqP10879291;     FhLFlmRWqP10879291 = FhLFlmRWqP72353869;     FhLFlmRWqP72353869 = FhLFlmRWqP40616401;     FhLFlmRWqP40616401 = FhLFlmRWqP3197421;     FhLFlmRWqP3197421 = FhLFlmRWqP49558396;     FhLFlmRWqP49558396 = FhLFlmRWqP24888923;     FhLFlmRWqP24888923 = FhLFlmRWqP4979490;     FhLFlmRWqP4979490 = FhLFlmRWqP38392812;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void PNTFsRXVOd72905257() {     double faZCreJrwO89839342 = -317197490;    double faZCreJrwO96133558 = -350653171;    double faZCreJrwO2893380 = -855181259;    double faZCreJrwO2729256 = -399131557;    double faZCreJrwO13382090 = -520824968;    double faZCreJrwO78158749 = -780288382;    double faZCreJrwO57803805 = -47546851;    double faZCreJrwO46611740 = -966139908;    double faZCreJrwO55231153 = -506235493;    double faZCreJrwO51295820 = -894840093;    double faZCreJrwO1996418 = -291476271;    double faZCreJrwO6630840 = -829474882;    double faZCreJrwO34340673 = -983500738;    double faZCreJrwO330301 = -744214417;    double faZCreJrwO35249506 = -826128914;    double faZCreJrwO79851555 = -879195756;    double faZCreJrwO89094536 = -580366145;    double faZCreJrwO98982338 = -125552475;    double faZCreJrwO61962832 = -997649697;    double faZCreJrwO32231109 = -279493316;    double faZCreJrwO68205330 = -572427854;    double faZCreJrwO49064120 = -643221830;    double faZCreJrwO32739804 = -116331708;    double faZCreJrwO81322425 = -956738185;    double faZCreJrwO38805121 = -335615306;    double faZCreJrwO5797136 = -741921940;    double faZCreJrwO81042935 = -702438126;    double faZCreJrwO95203428 = -662894938;    double faZCreJrwO774500 = -799702255;    double faZCreJrwO50551007 = -180727971;    double faZCreJrwO41701756 = -252970393;    double faZCreJrwO3866183 = -908216264;    double faZCreJrwO70569573 = -200990776;    double faZCreJrwO79353559 = -373551615;    double faZCreJrwO18586316 = -135422616;    double faZCreJrwO19122326 = -639783416;    double faZCreJrwO88980433 = -197349029;    double faZCreJrwO62983795 = -231898156;    double faZCreJrwO87960689 = -749412908;    double faZCreJrwO20434880 = -70580312;    double faZCreJrwO10176106 = -367425126;    double faZCreJrwO73065394 = -381999650;    double faZCreJrwO72700145 = -353868753;    double faZCreJrwO84848333 = -283969954;    double faZCreJrwO61251403 = -852601708;    double faZCreJrwO98134221 = -557510831;    double faZCreJrwO14018644 = -274994989;    double faZCreJrwO94909325 = -166129312;    double faZCreJrwO70751584 = -289738007;    double faZCreJrwO34432488 = -864294520;    double faZCreJrwO46084524 = -896300844;    double faZCreJrwO97608299 = -474937839;    double faZCreJrwO44606142 = -254130319;    double faZCreJrwO66942686 = -892709729;    double faZCreJrwO4699928 = -733239299;    double faZCreJrwO40775223 = -673975660;    double faZCreJrwO63393754 = -134321463;    double faZCreJrwO21570955 = -898443075;    double faZCreJrwO63924134 = 36483748;    double faZCreJrwO7584955 = -778903029;    double faZCreJrwO97115814 = 22149744;    double faZCreJrwO62600377 = -384651914;    double faZCreJrwO45837241 = -66437654;    double faZCreJrwO4680147 = -225507523;    double faZCreJrwO9594065 = -541869700;    double faZCreJrwO98130235 = -383260008;    double faZCreJrwO36061266 = -528484106;    double faZCreJrwO54987113 = -509949124;    double faZCreJrwO81743985 = -508791801;    double faZCreJrwO16127181 = -86345498;    double faZCreJrwO90871121 = -581846728;    double faZCreJrwO26110741 = -248467989;    double faZCreJrwO11021650 = -376139568;    double faZCreJrwO41527952 = -827069385;    double faZCreJrwO22055004 = -912068191;    double faZCreJrwO95139935 = -90428204;    double faZCreJrwO76363974 = -189353078;    double faZCreJrwO47891471 = -832361754;    double faZCreJrwO20071022 = -4136478;    double faZCreJrwO40670900 = -778104475;    double faZCreJrwO91778492 = -366926951;    double faZCreJrwO86133610 = -436308814;    double faZCreJrwO24451845 = -273156932;    double faZCreJrwO66342011 = -935407735;    double faZCreJrwO4466483 = -284427127;    double faZCreJrwO44093457 = -778032555;    double faZCreJrwO59260040 = -554085945;    double faZCreJrwO3626887 = -308281048;    double faZCreJrwO74653632 = -640312316;    double faZCreJrwO77811093 = -461446956;    double faZCreJrwO55728572 = -405461953;    double faZCreJrwO67409479 = -298905955;    double faZCreJrwO99059661 = -168381905;    double faZCreJrwO80375734 = -970509879;    double faZCreJrwO23319066 = 7269943;    double faZCreJrwO47575728 = -982773213;    double faZCreJrwO27228154 = -215561997;    double faZCreJrwO68019998 = -28361230;    double faZCreJrwO75254268 = -742100255;    double faZCreJrwO63121168 = -317197490;     faZCreJrwO89839342 = faZCreJrwO96133558;     faZCreJrwO96133558 = faZCreJrwO2893380;     faZCreJrwO2893380 = faZCreJrwO2729256;     faZCreJrwO2729256 = faZCreJrwO13382090;     faZCreJrwO13382090 = faZCreJrwO78158749;     faZCreJrwO78158749 = faZCreJrwO57803805;     faZCreJrwO57803805 = faZCreJrwO46611740;     faZCreJrwO46611740 = faZCreJrwO55231153;     faZCreJrwO55231153 = faZCreJrwO51295820;     faZCreJrwO51295820 = faZCreJrwO1996418;     faZCreJrwO1996418 = faZCreJrwO6630840;     faZCreJrwO6630840 = faZCreJrwO34340673;     faZCreJrwO34340673 = faZCreJrwO330301;     faZCreJrwO330301 = faZCreJrwO35249506;     faZCreJrwO35249506 = faZCreJrwO79851555;     faZCreJrwO79851555 = faZCreJrwO89094536;     faZCreJrwO89094536 = faZCreJrwO98982338;     faZCreJrwO98982338 = faZCreJrwO61962832;     faZCreJrwO61962832 = faZCreJrwO32231109;     faZCreJrwO32231109 = faZCreJrwO68205330;     faZCreJrwO68205330 = faZCreJrwO49064120;     faZCreJrwO49064120 = faZCreJrwO32739804;     faZCreJrwO32739804 = faZCreJrwO81322425;     faZCreJrwO81322425 = faZCreJrwO38805121;     faZCreJrwO38805121 = faZCreJrwO5797136;     faZCreJrwO5797136 = faZCreJrwO81042935;     faZCreJrwO81042935 = faZCreJrwO95203428;     faZCreJrwO95203428 = faZCreJrwO774500;     faZCreJrwO774500 = faZCreJrwO50551007;     faZCreJrwO50551007 = faZCreJrwO41701756;     faZCreJrwO41701756 = faZCreJrwO3866183;     faZCreJrwO3866183 = faZCreJrwO70569573;     faZCreJrwO70569573 = faZCreJrwO79353559;     faZCreJrwO79353559 = faZCreJrwO18586316;     faZCreJrwO18586316 = faZCreJrwO19122326;     faZCreJrwO19122326 = faZCreJrwO88980433;     faZCreJrwO88980433 = faZCreJrwO62983795;     faZCreJrwO62983795 = faZCreJrwO87960689;     faZCreJrwO87960689 = faZCreJrwO20434880;     faZCreJrwO20434880 = faZCreJrwO10176106;     faZCreJrwO10176106 = faZCreJrwO73065394;     faZCreJrwO73065394 = faZCreJrwO72700145;     faZCreJrwO72700145 = faZCreJrwO84848333;     faZCreJrwO84848333 = faZCreJrwO61251403;     faZCreJrwO61251403 = faZCreJrwO98134221;     faZCreJrwO98134221 = faZCreJrwO14018644;     faZCreJrwO14018644 = faZCreJrwO94909325;     faZCreJrwO94909325 = faZCreJrwO70751584;     faZCreJrwO70751584 = faZCreJrwO34432488;     faZCreJrwO34432488 = faZCreJrwO46084524;     faZCreJrwO46084524 = faZCreJrwO97608299;     faZCreJrwO97608299 = faZCreJrwO44606142;     faZCreJrwO44606142 = faZCreJrwO66942686;     faZCreJrwO66942686 = faZCreJrwO4699928;     faZCreJrwO4699928 = faZCreJrwO40775223;     faZCreJrwO40775223 = faZCreJrwO63393754;     faZCreJrwO63393754 = faZCreJrwO21570955;     faZCreJrwO21570955 = faZCreJrwO63924134;     faZCreJrwO63924134 = faZCreJrwO7584955;     faZCreJrwO7584955 = faZCreJrwO97115814;     faZCreJrwO97115814 = faZCreJrwO62600377;     faZCreJrwO62600377 = faZCreJrwO45837241;     faZCreJrwO45837241 = faZCreJrwO4680147;     faZCreJrwO4680147 = faZCreJrwO9594065;     faZCreJrwO9594065 = faZCreJrwO98130235;     faZCreJrwO98130235 = faZCreJrwO36061266;     faZCreJrwO36061266 = faZCreJrwO54987113;     faZCreJrwO54987113 = faZCreJrwO81743985;     faZCreJrwO81743985 = faZCreJrwO16127181;     faZCreJrwO16127181 = faZCreJrwO90871121;     faZCreJrwO90871121 = faZCreJrwO26110741;     faZCreJrwO26110741 = faZCreJrwO11021650;     faZCreJrwO11021650 = faZCreJrwO41527952;     faZCreJrwO41527952 = faZCreJrwO22055004;     faZCreJrwO22055004 = faZCreJrwO95139935;     faZCreJrwO95139935 = faZCreJrwO76363974;     faZCreJrwO76363974 = faZCreJrwO47891471;     faZCreJrwO47891471 = faZCreJrwO20071022;     faZCreJrwO20071022 = faZCreJrwO40670900;     faZCreJrwO40670900 = faZCreJrwO91778492;     faZCreJrwO91778492 = faZCreJrwO86133610;     faZCreJrwO86133610 = faZCreJrwO24451845;     faZCreJrwO24451845 = faZCreJrwO66342011;     faZCreJrwO66342011 = faZCreJrwO4466483;     faZCreJrwO4466483 = faZCreJrwO44093457;     faZCreJrwO44093457 = faZCreJrwO59260040;     faZCreJrwO59260040 = faZCreJrwO3626887;     faZCreJrwO3626887 = faZCreJrwO74653632;     faZCreJrwO74653632 = faZCreJrwO77811093;     faZCreJrwO77811093 = faZCreJrwO55728572;     faZCreJrwO55728572 = faZCreJrwO67409479;     faZCreJrwO67409479 = faZCreJrwO99059661;     faZCreJrwO99059661 = faZCreJrwO80375734;     faZCreJrwO80375734 = faZCreJrwO23319066;     faZCreJrwO23319066 = faZCreJrwO47575728;     faZCreJrwO47575728 = faZCreJrwO27228154;     faZCreJrwO27228154 = faZCreJrwO68019998;     faZCreJrwO68019998 = faZCreJrwO75254268;     faZCreJrwO75254268 = faZCreJrwO63121168;     faZCreJrwO63121168 = faZCreJrwO89839342;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void fdpWjfVHhn98422081() {     double fuNsbhQAKM8752324 = -205353858;    double fuNsbhQAKM18419251 = -166694277;    double fuNsbhQAKM84718544 = -928680642;    double fuNsbhQAKM60125071 = -133271219;    double fuNsbhQAKM62528530 = -830758323;    double fuNsbhQAKM60677874 = -172558712;    double fuNsbhQAKM86679525 = -625379198;    double fuNsbhQAKM19269668 = 58733898;    double fuNsbhQAKM91168212 = -115557146;    double fuNsbhQAKM3784915 = 77307103;    double fuNsbhQAKM59113182 = -344391765;    double fuNsbhQAKM48966871 = 22003644;    double fuNsbhQAKM42913723 = -744686772;    double fuNsbhQAKM2331726 = 8861394;    double fuNsbhQAKM86540322 = -379507633;    double fuNsbhQAKM87935196 = -353893213;    double fuNsbhQAKM54903217 = -31263869;    double fuNsbhQAKM95093177 = -400677988;    double fuNsbhQAKM44310899 = -385914589;    double fuNsbhQAKM7325126 = -317603698;    double fuNsbhQAKM17428444 = -253532013;    double fuNsbhQAKM97188563 = -506022069;    double fuNsbhQAKM51773296 = -755416326;    double fuNsbhQAKM44346766 = -618007402;    double fuNsbhQAKM95734996 = -829212483;    double fuNsbhQAKM36976341 = -448699570;    double fuNsbhQAKM35585878 = -785607040;    double fuNsbhQAKM22558192 = 10828486;    double fuNsbhQAKM64506137 = -817196727;    double fuNsbhQAKM7782954 = -114937932;    double fuNsbhQAKM28261284 = -962068230;    double fuNsbhQAKM27709842 = -8650116;    double fuNsbhQAKM98147836 = -216542607;    double fuNsbhQAKM50836241 = -764824999;    double fuNsbhQAKM39584443 = -995216715;    double fuNsbhQAKM18830070 = 27275991;    double fuNsbhQAKM39094377 = -341414182;    double fuNsbhQAKM49901512 = 69869761;    double fuNsbhQAKM14375934 = -66164858;    double fuNsbhQAKM76392208 = -424288851;    double fuNsbhQAKM95493135 = -126257166;    double fuNsbhQAKM34371816 = -259765139;    double fuNsbhQAKM45225553 = -719411400;    double fuNsbhQAKM78036764 = -209038470;    double fuNsbhQAKM27185281 = -247380720;    double fuNsbhQAKM23059594 = -347978381;    double fuNsbhQAKM67832548 = -169364646;    double fuNsbhQAKM69230861 = 31764410;    double fuNsbhQAKM46503778 = -643431177;    double fuNsbhQAKM64226426 = 18195539;    double fuNsbhQAKM80223641 = -523646313;    double fuNsbhQAKM14773680 = -723630874;    double fuNsbhQAKM64167227 = -933842178;    double fuNsbhQAKM92235885 = -554751839;    double fuNsbhQAKM80993637 = -692866651;    double fuNsbhQAKM11563760 = -699331789;    double fuNsbhQAKM66645955 = -411277952;    double fuNsbhQAKM40371779 = -210673241;    double fuNsbhQAKM64390075 = -304058736;    double fuNsbhQAKM25552190 = -282058754;    double fuNsbhQAKM25091997 = -386951673;    double fuNsbhQAKM64121334 = -536207685;    double fuNsbhQAKM54763530 = -124069376;    double fuNsbhQAKM83385259 = 99380786;    double fuNsbhQAKM75523631 = 39375332;    double fuNsbhQAKM31403341 = -235741650;    double fuNsbhQAKM50819035 = -761453749;    double fuNsbhQAKM92077482 = -979861773;    double fuNsbhQAKM62747283 = 4078108;    double fuNsbhQAKM67710253 = -306783625;    double fuNsbhQAKM48840819 = 87520969;    double fuNsbhQAKM5001706 = -1133631;    double fuNsbhQAKM80717243 = -234513130;    double fuNsbhQAKM67918691 = -961625738;    double fuNsbhQAKM11831990 = -91346533;    double fuNsbhQAKM83056628 = -993766874;    double fuNsbhQAKM51963011 = -786610670;    double fuNsbhQAKM73736532 = -446377856;    double fuNsbhQAKM17161486 = -270626682;    double fuNsbhQAKM72675402 = -381234103;    double fuNsbhQAKM69143792 = -179334924;    double fuNsbhQAKM66355016 = -717371451;    double fuNsbhQAKM76054413 = -345740337;    double fuNsbhQAKM279711 = -735392266;    double fuNsbhQAKM27559313 = -591291620;    double fuNsbhQAKM13487604 = -138437356;    double fuNsbhQAKM63542614 = -74807939;    double fuNsbhQAKM5911951 = -661790768;    double fuNsbhQAKM69842603 = 28041651;    double fuNsbhQAKM28020683 = -195884926;    double fuNsbhQAKM52184115 = -561446058;    double fuNsbhQAKM98722598 = -30740942;    double fuNsbhQAKM85511436 = -626071503;    double fuNsbhQAKM88823744 = -784106105;    double fuNsbhQAKM51300211 = 62662821;    double fuNsbhQAKM31371802 = -590049481;    double fuNsbhQAKM79608285 = -35695764;    double fuNsbhQAKM61840294 = -718792186;    double fuNsbhQAKM2513134 = -148413803;    double fuNsbhQAKM95781939 = -205353858;     fuNsbhQAKM8752324 = fuNsbhQAKM18419251;     fuNsbhQAKM18419251 = fuNsbhQAKM84718544;     fuNsbhQAKM84718544 = fuNsbhQAKM60125071;     fuNsbhQAKM60125071 = fuNsbhQAKM62528530;     fuNsbhQAKM62528530 = fuNsbhQAKM60677874;     fuNsbhQAKM60677874 = fuNsbhQAKM86679525;     fuNsbhQAKM86679525 = fuNsbhQAKM19269668;     fuNsbhQAKM19269668 = fuNsbhQAKM91168212;     fuNsbhQAKM91168212 = fuNsbhQAKM3784915;     fuNsbhQAKM3784915 = fuNsbhQAKM59113182;     fuNsbhQAKM59113182 = fuNsbhQAKM48966871;     fuNsbhQAKM48966871 = fuNsbhQAKM42913723;     fuNsbhQAKM42913723 = fuNsbhQAKM2331726;     fuNsbhQAKM2331726 = fuNsbhQAKM86540322;     fuNsbhQAKM86540322 = fuNsbhQAKM87935196;     fuNsbhQAKM87935196 = fuNsbhQAKM54903217;     fuNsbhQAKM54903217 = fuNsbhQAKM95093177;     fuNsbhQAKM95093177 = fuNsbhQAKM44310899;     fuNsbhQAKM44310899 = fuNsbhQAKM7325126;     fuNsbhQAKM7325126 = fuNsbhQAKM17428444;     fuNsbhQAKM17428444 = fuNsbhQAKM97188563;     fuNsbhQAKM97188563 = fuNsbhQAKM51773296;     fuNsbhQAKM51773296 = fuNsbhQAKM44346766;     fuNsbhQAKM44346766 = fuNsbhQAKM95734996;     fuNsbhQAKM95734996 = fuNsbhQAKM36976341;     fuNsbhQAKM36976341 = fuNsbhQAKM35585878;     fuNsbhQAKM35585878 = fuNsbhQAKM22558192;     fuNsbhQAKM22558192 = fuNsbhQAKM64506137;     fuNsbhQAKM64506137 = fuNsbhQAKM7782954;     fuNsbhQAKM7782954 = fuNsbhQAKM28261284;     fuNsbhQAKM28261284 = fuNsbhQAKM27709842;     fuNsbhQAKM27709842 = fuNsbhQAKM98147836;     fuNsbhQAKM98147836 = fuNsbhQAKM50836241;     fuNsbhQAKM50836241 = fuNsbhQAKM39584443;     fuNsbhQAKM39584443 = fuNsbhQAKM18830070;     fuNsbhQAKM18830070 = fuNsbhQAKM39094377;     fuNsbhQAKM39094377 = fuNsbhQAKM49901512;     fuNsbhQAKM49901512 = fuNsbhQAKM14375934;     fuNsbhQAKM14375934 = fuNsbhQAKM76392208;     fuNsbhQAKM76392208 = fuNsbhQAKM95493135;     fuNsbhQAKM95493135 = fuNsbhQAKM34371816;     fuNsbhQAKM34371816 = fuNsbhQAKM45225553;     fuNsbhQAKM45225553 = fuNsbhQAKM78036764;     fuNsbhQAKM78036764 = fuNsbhQAKM27185281;     fuNsbhQAKM27185281 = fuNsbhQAKM23059594;     fuNsbhQAKM23059594 = fuNsbhQAKM67832548;     fuNsbhQAKM67832548 = fuNsbhQAKM69230861;     fuNsbhQAKM69230861 = fuNsbhQAKM46503778;     fuNsbhQAKM46503778 = fuNsbhQAKM64226426;     fuNsbhQAKM64226426 = fuNsbhQAKM80223641;     fuNsbhQAKM80223641 = fuNsbhQAKM14773680;     fuNsbhQAKM14773680 = fuNsbhQAKM64167227;     fuNsbhQAKM64167227 = fuNsbhQAKM92235885;     fuNsbhQAKM92235885 = fuNsbhQAKM80993637;     fuNsbhQAKM80993637 = fuNsbhQAKM11563760;     fuNsbhQAKM11563760 = fuNsbhQAKM66645955;     fuNsbhQAKM66645955 = fuNsbhQAKM40371779;     fuNsbhQAKM40371779 = fuNsbhQAKM64390075;     fuNsbhQAKM64390075 = fuNsbhQAKM25552190;     fuNsbhQAKM25552190 = fuNsbhQAKM25091997;     fuNsbhQAKM25091997 = fuNsbhQAKM64121334;     fuNsbhQAKM64121334 = fuNsbhQAKM54763530;     fuNsbhQAKM54763530 = fuNsbhQAKM83385259;     fuNsbhQAKM83385259 = fuNsbhQAKM75523631;     fuNsbhQAKM75523631 = fuNsbhQAKM31403341;     fuNsbhQAKM31403341 = fuNsbhQAKM50819035;     fuNsbhQAKM50819035 = fuNsbhQAKM92077482;     fuNsbhQAKM92077482 = fuNsbhQAKM62747283;     fuNsbhQAKM62747283 = fuNsbhQAKM67710253;     fuNsbhQAKM67710253 = fuNsbhQAKM48840819;     fuNsbhQAKM48840819 = fuNsbhQAKM5001706;     fuNsbhQAKM5001706 = fuNsbhQAKM80717243;     fuNsbhQAKM80717243 = fuNsbhQAKM67918691;     fuNsbhQAKM67918691 = fuNsbhQAKM11831990;     fuNsbhQAKM11831990 = fuNsbhQAKM83056628;     fuNsbhQAKM83056628 = fuNsbhQAKM51963011;     fuNsbhQAKM51963011 = fuNsbhQAKM73736532;     fuNsbhQAKM73736532 = fuNsbhQAKM17161486;     fuNsbhQAKM17161486 = fuNsbhQAKM72675402;     fuNsbhQAKM72675402 = fuNsbhQAKM69143792;     fuNsbhQAKM69143792 = fuNsbhQAKM66355016;     fuNsbhQAKM66355016 = fuNsbhQAKM76054413;     fuNsbhQAKM76054413 = fuNsbhQAKM279711;     fuNsbhQAKM279711 = fuNsbhQAKM27559313;     fuNsbhQAKM27559313 = fuNsbhQAKM13487604;     fuNsbhQAKM13487604 = fuNsbhQAKM63542614;     fuNsbhQAKM63542614 = fuNsbhQAKM5911951;     fuNsbhQAKM5911951 = fuNsbhQAKM69842603;     fuNsbhQAKM69842603 = fuNsbhQAKM28020683;     fuNsbhQAKM28020683 = fuNsbhQAKM52184115;     fuNsbhQAKM52184115 = fuNsbhQAKM98722598;     fuNsbhQAKM98722598 = fuNsbhQAKM85511436;     fuNsbhQAKM85511436 = fuNsbhQAKM88823744;     fuNsbhQAKM88823744 = fuNsbhQAKM51300211;     fuNsbhQAKM51300211 = fuNsbhQAKM31371802;     fuNsbhQAKM31371802 = fuNsbhQAKM79608285;     fuNsbhQAKM79608285 = fuNsbhQAKM61840294;     fuNsbhQAKM61840294 = fuNsbhQAKM2513134;     fuNsbhQAKM2513134 = fuNsbhQAKM95781939;     fuNsbhQAKM95781939 = fuNsbhQAKM8752324;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void kGspnbhnkH37144489() {     double yRzhdeTQoU37811737 = -476634554;    double yRzhdeTQoU76878202 = -554674002;    double yRzhdeTQoU64541833 = -897296639;    double yRzhdeTQoU63457873 = -412861982;    double yRzhdeTQoU80898349 = -58804257;    double yRzhdeTQoU10051144 = -795877763;    double yRzhdeTQoU55280389 = -451805364;    double yRzhdeTQoU37255723 = -717418431;    double yRzhdeTQoU77723576 = -886655577;    double yRzhdeTQoU62217787 = -972142949;    double yRzhdeTQoU53871831 = -598528559;    double yRzhdeTQoU97851444 = -423929082;    double yRzhdeTQoU77929631 = -266466423;    double yRzhdeTQoU34504770 = -146447468;    double yRzhdeTQoU28253672 = -342152585;    double yRzhdeTQoU31828511 = -597054776;    double yRzhdeTQoU38716004 = -545965759;    double yRzhdeTQoU26074660 = 6427473;    double yRzhdeTQoU64709184 = -815725679;    double yRzhdeTQoU71908859 = -506567146;    double yRzhdeTQoU70097702 = 74385057;    double yRzhdeTQoU14382445 = -467721073;    double yRzhdeTQoU39796925 = -336504405;    double yRzhdeTQoU9536255 = -279580022;    double yRzhdeTQoU29248507 = -624670573;    double yRzhdeTQoU23589267 = -750266705;    double yRzhdeTQoU10520265 = -25837754;    double yRzhdeTQoU65013679 = -681011624;    double yRzhdeTQoU15544273 = -280253224;    double yRzhdeTQoU13030677 = -9933399;    double yRzhdeTQoU8039196 = -112933576;    double yRzhdeTQoU98597305 = -734083453;    double yRzhdeTQoU72795295 = -78155408;    double yRzhdeTQoU92329318 = -753823436;    double yRzhdeTQoU96220668 = -900614950;    double yRzhdeTQoU65607045 = -372773509;    double yRzhdeTQoU32065531 = -410808925;    double yRzhdeTQoU92182583 = 51761547;    double yRzhdeTQoU77192399 = -508797262;    double yRzhdeTQoU49404269 = -676774660;    double yRzhdeTQoU86084757 = -345662982;    double yRzhdeTQoU5583376 = -648946845;    double yRzhdeTQoU27450557 = -270005718;    double yRzhdeTQoU55085731 = -749749202;    double yRzhdeTQoU21215547 = -600766303;    double yRzhdeTQoU72634825 = -162287464;    double yRzhdeTQoU26084495 = -885874103;    double yRzhdeTQoU47620771 = -115847150;    double yRzhdeTQoU16022810 = -339130931;    double yRzhdeTQoU85326228 = -854337932;    double yRzhdeTQoU65089988 = -400035724;    double yRzhdeTQoU98718136 = 16163700;    double yRzhdeTQoU40994149 = -493940098;    double yRzhdeTQoU96128140 = -383454319;    double yRzhdeTQoU67046786 = -249033831;    double yRzhdeTQoU23429292 = 91086519;    double yRzhdeTQoU37081277 = -118169598;    double yRzhdeTQoU55005579 = -517716618;    double yRzhdeTQoU34209367 = -788191410;    double yRzhdeTQoU57309082 = -308537552;    double yRzhdeTQoU99530878 = -670040010;    double yRzhdeTQoU90266709 = -770793740;    double yRzhdeTQoU21711451 = -337165208;    double yRzhdeTQoU64692900 = -776722178;    double yRzhdeTQoU54178592 = -759209374;    double yRzhdeTQoU55274526 = -864445107;    double yRzhdeTQoU25056149 = -245773674;    double yRzhdeTQoU85600313 = -512642987;    double yRzhdeTQoU38284102 = -245832518;    double yRzhdeTQoU62646626 = -969379076;    double yRzhdeTQoU99762979 = -86245851;    double yRzhdeTQoU46533421 = -497727306;    double yRzhdeTQoU48882260 = -484775266;    double yRzhdeTQoU15304916 = -38951019;    double yRzhdeTQoU85824101 = -60904164;    double yRzhdeTQoU64514327 = -276668098;    double yRzhdeTQoU86931888 = -97715355;    double yRzhdeTQoU84711193 = -586755203;    double yRzhdeTQoU88320708 = -678813720;    double yRzhdeTQoU56613682 = -362383109;    double yRzhdeTQoU97504771 = -864392603;    double yRzhdeTQoU62899494 = -909990604;    double yRzhdeTQoU48990870 = -241880693;    double yRzhdeTQoU30218044 = -425915293;    double yRzhdeTQoU47940689 = -609897676;    double yRzhdeTQoU9321060 = -29097277;    double yRzhdeTQoU57603157 = -140143355;    double yRzhdeTQoU76667155 = -694701089;    double yRzhdeTQoU25282532 = -404789605;    double yRzhdeTQoU72791376 = -891701469;    double yRzhdeTQoU28525769 = -154603912;    double yRzhdeTQoU77059952 = -893092308;    double yRzhdeTQoU57973217 = -160047044;    double yRzhdeTQoU19883317 = -100259710;    double yRzhdeTQoU49873390 = 93265349;    double yRzhdeTQoU95818047 = -574869242;    double yRzhdeTQoU83871925 = -211781638;    double yRzhdeTQoU62757657 = -493283540;    double yRzhdeTQoU907140 = -990539829;    double yRzhdeTQoU65941020 = -476634554;     yRzhdeTQoU37811737 = yRzhdeTQoU76878202;     yRzhdeTQoU76878202 = yRzhdeTQoU64541833;     yRzhdeTQoU64541833 = yRzhdeTQoU63457873;     yRzhdeTQoU63457873 = yRzhdeTQoU80898349;     yRzhdeTQoU80898349 = yRzhdeTQoU10051144;     yRzhdeTQoU10051144 = yRzhdeTQoU55280389;     yRzhdeTQoU55280389 = yRzhdeTQoU37255723;     yRzhdeTQoU37255723 = yRzhdeTQoU77723576;     yRzhdeTQoU77723576 = yRzhdeTQoU62217787;     yRzhdeTQoU62217787 = yRzhdeTQoU53871831;     yRzhdeTQoU53871831 = yRzhdeTQoU97851444;     yRzhdeTQoU97851444 = yRzhdeTQoU77929631;     yRzhdeTQoU77929631 = yRzhdeTQoU34504770;     yRzhdeTQoU34504770 = yRzhdeTQoU28253672;     yRzhdeTQoU28253672 = yRzhdeTQoU31828511;     yRzhdeTQoU31828511 = yRzhdeTQoU38716004;     yRzhdeTQoU38716004 = yRzhdeTQoU26074660;     yRzhdeTQoU26074660 = yRzhdeTQoU64709184;     yRzhdeTQoU64709184 = yRzhdeTQoU71908859;     yRzhdeTQoU71908859 = yRzhdeTQoU70097702;     yRzhdeTQoU70097702 = yRzhdeTQoU14382445;     yRzhdeTQoU14382445 = yRzhdeTQoU39796925;     yRzhdeTQoU39796925 = yRzhdeTQoU9536255;     yRzhdeTQoU9536255 = yRzhdeTQoU29248507;     yRzhdeTQoU29248507 = yRzhdeTQoU23589267;     yRzhdeTQoU23589267 = yRzhdeTQoU10520265;     yRzhdeTQoU10520265 = yRzhdeTQoU65013679;     yRzhdeTQoU65013679 = yRzhdeTQoU15544273;     yRzhdeTQoU15544273 = yRzhdeTQoU13030677;     yRzhdeTQoU13030677 = yRzhdeTQoU8039196;     yRzhdeTQoU8039196 = yRzhdeTQoU98597305;     yRzhdeTQoU98597305 = yRzhdeTQoU72795295;     yRzhdeTQoU72795295 = yRzhdeTQoU92329318;     yRzhdeTQoU92329318 = yRzhdeTQoU96220668;     yRzhdeTQoU96220668 = yRzhdeTQoU65607045;     yRzhdeTQoU65607045 = yRzhdeTQoU32065531;     yRzhdeTQoU32065531 = yRzhdeTQoU92182583;     yRzhdeTQoU92182583 = yRzhdeTQoU77192399;     yRzhdeTQoU77192399 = yRzhdeTQoU49404269;     yRzhdeTQoU49404269 = yRzhdeTQoU86084757;     yRzhdeTQoU86084757 = yRzhdeTQoU5583376;     yRzhdeTQoU5583376 = yRzhdeTQoU27450557;     yRzhdeTQoU27450557 = yRzhdeTQoU55085731;     yRzhdeTQoU55085731 = yRzhdeTQoU21215547;     yRzhdeTQoU21215547 = yRzhdeTQoU72634825;     yRzhdeTQoU72634825 = yRzhdeTQoU26084495;     yRzhdeTQoU26084495 = yRzhdeTQoU47620771;     yRzhdeTQoU47620771 = yRzhdeTQoU16022810;     yRzhdeTQoU16022810 = yRzhdeTQoU85326228;     yRzhdeTQoU85326228 = yRzhdeTQoU65089988;     yRzhdeTQoU65089988 = yRzhdeTQoU98718136;     yRzhdeTQoU98718136 = yRzhdeTQoU40994149;     yRzhdeTQoU40994149 = yRzhdeTQoU96128140;     yRzhdeTQoU96128140 = yRzhdeTQoU67046786;     yRzhdeTQoU67046786 = yRzhdeTQoU23429292;     yRzhdeTQoU23429292 = yRzhdeTQoU37081277;     yRzhdeTQoU37081277 = yRzhdeTQoU55005579;     yRzhdeTQoU55005579 = yRzhdeTQoU34209367;     yRzhdeTQoU34209367 = yRzhdeTQoU57309082;     yRzhdeTQoU57309082 = yRzhdeTQoU99530878;     yRzhdeTQoU99530878 = yRzhdeTQoU90266709;     yRzhdeTQoU90266709 = yRzhdeTQoU21711451;     yRzhdeTQoU21711451 = yRzhdeTQoU64692900;     yRzhdeTQoU64692900 = yRzhdeTQoU54178592;     yRzhdeTQoU54178592 = yRzhdeTQoU55274526;     yRzhdeTQoU55274526 = yRzhdeTQoU25056149;     yRzhdeTQoU25056149 = yRzhdeTQoU85600313;     yRzhdeTQoU85600313 = yRzhdeTQoU38284102;     yRzhdeTQoU38284102 = yRzhdeTQoU62646626;     yRzhdeTQoU62646626 = yRzhdeTQoU99762979;     yRzhdeTQoU99762979 = yRzhdeTQoU46533421;     yRzhdeTQoU46533421 = yRzhdeTQoU48882260;     yRzhdeTQoU48882260 = yRzhdeTQoU15304916;     yRzhdeTQoU15304916 = yRzhdeTQoU85824101;     yRzhdeTQoU85824101 = yRzhdeTQoU64514327;     yRzhdeTQoU64514327 = yRzhdeTQoU86931888;     yRzhdeTQoU86931888 = yRzhdeTQoU84711193;     yRzhdeTQoU84711193 = yRzhdeTQoU88320708;     yRzhdeTQoU88320708 = yRzhdeTQoU56613682;     yRzhdeTQoU56613682 = yRzhdeTQoU97504771;     yRzhdeTQoU97504771 = yRzhdeTQoU62899494;     yRzhdeTQoU62899494 = yRzhdeTQoU48990870;     yRzhdeTQoU48990870 = yRzhdeTQoU30218044;     yRzhdeTQoU30218044 = yRzhdeTQoU47940689;     yRzhdeTQoU47940689 = yRzhdeTQoU9321060;     yRzhdeTQoU9321060 = yRzhdeTQoU57603157;     yRzhdeTQoU57603157 = yRzhdeTQoU76667155;     yRzhdeTQoU76667155 = yRzhdeTQoU25282532;     yRzhdeTQoU25282532 = yRzhdeTQoU72791376;     yRzhdeTQoU72791376 = yRzhdeTQoU28525769;     yRzhdeTQoU28525769 = yRzhdeTQoU77059952;     yRzhdeTQoU77059952 = yRzhdeTQoU57973217;     yRzhdeTQoU57973217 = yRzhdeTQoU19883317;     yRzhdeTQoU19883317 = yRzhdeTQoU49873390;     yRzhdeTQoU49873390 = yRzhdeTQoU95818047;     yRzhdeTQoU95818047 = yRzhdeTQoU83871925;     yRzhdeTQoU83871925 = yRzhdeTQoU62757657;     yRzhdeTQoU62757657 = yRzhdeTQoU907140;     yRzhdeTQoU907140 = yRzhdeTQoU65941020;     yRzhdeTQoU65941020 = yRzhdeTQoU37811737;}
// Junk Finished
