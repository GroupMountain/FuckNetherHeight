#include <LoggerAPI.h>
#include "version.h"
#include <HookAPI.h>
#include <ModUtils/ModUtils.h>
#include <llapi/utils/TypeConversionHelper.hpp>
#include <mc/SubChunkPacket.hpp>
#include <mc/SubChunkRequestPacket.hpp>
#include <mc/ChangeDimensionPacket.hpp>
#include <mc/ServerPlayer.hpp>
#include <mc/Player.hpp>
#include <mc/UserEntityIdentifierComponent.hpp>
#include <mc/Level.hpp>
#include <EventAPI.h>

extern Logger logger;

//修改下界高度为256
void PatchNetherHeight() {
    uintptr_t regionStart = (uintptr_t)dlsym_real("??0NetherDimension@@QEAA@AEAVILevel@@AEAVScheduler@@@Z");
    std::vector<uint16_t> pattern = TCHelper::splitHex("41 B9 00 00 80 00");
    uintptr_t address = ModUtils::SigScan(pattern, regionStart, 114514);
    ModUtils::Replace(address, TCHelper::splitHex("41 B9 00 00 80 00"), TCHelper::splitHex8("41 B9 00 00 00 01"));
}

void PluginInit()
{
    PatchNetherHeight();
    logger.info("Loaded Version: {}, Author: Tsubasa6848", PLUGIN_FILE_VERSION_STRING);
}

// 欺骗服务端，请求真实维度
TInstanceHook(void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVSubChunkRequestPacket@@@Z", ServerNetworkHandler, NetworkIdentifier* a1, SubChunkRequestPacket* a2) {
    auto pl = a2->getPlayerFromPacket(this, a1);
    a2->mDimensionType = pl->getDimensionId();
	return original(this, a1, a2);
}

// 欺骗客户端，发送假维度
TInstanceHook(void, "?write@SubChunkPacket@@UEBAXAEAVBinaryStream@@@Z", SubChunkPacket, void* a1) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
	return original(this, a1);
}

// 切换维度，地狱发送末地
TInstanceHook(void, "?write@ChangeDimensionPacket@@UEBAXAEAVBinaryStream@@@Z", ChangeDimensionPacket, void* a1) {
    if (this->mToDimensionId == 1) {
        this->mToDimensionId = 2;
    }
	return original(this, a1);
}

// 关闭客户端区块预生成
TClasslessInstanceHook(bool, "?isClientSideGenEnabled@PropertiesSettings@@QEBA_NXZ") {
	return false;
}



/*
TClasslessInstanceHook(void, "?sendToClient@LoopbackPacketSender@@UEAAXPEBVUserEntityIdentifierComponent@@AEBVPacket@@@Z", UserEntityIdentifierComponent* a1, Packet* a2) {
    if (a2->getId() == MinecraftPacketIds::ChangeDimension) {
        auto pkt = (ChangeDimensionPacket*)a2;
        auto pl = Global<Level>->getPlayer(a1->mUUID);
        if (pkt->mToDimensionId == 1) {
            pkt->mToDimensionId = 2;
        }
        return original(this, a1, pkt);
    } 
	return original(this, a1, a2);
}
*/