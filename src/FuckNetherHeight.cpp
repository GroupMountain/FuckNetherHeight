#include "Global.h"
#include "TCHelper.h"

namespace dimension_utils {
void sendEmptyChunk(const NetworkIdentifier& netId, int chunkX, int chunkZ, bool forceUpdate) {
    std::array<uchar, 4096> biome{};
    LevelChunkPacket        levelChunkPacket;
    BinaryStream            binaryStream{levelChunkPacket.mSerializedChunk, false};
    VarIntDataOutput        varIntDataOutput(&binaryStream);

    varIntDataOutput.writeBytes(&biome, 4096); // write void biome
    for (int i = 1; i < 8; i++) {
        varIntDataOutput.writeByte((127 << 1) | 1);
    }
    varIntDataOutput.mStream->writeUnsignedChar(0); // write border blocks

    levelChunkPacket.mPos.x          = chunkX;
    levelChunkPacket.mPos.z          = chunkZ;
    levelChunkPacket.mDimensionType  = 0;
    levelChunkPacket.mCacheEnabled   = false;
    levelChunkPacket.mSubChunksCount = 0;

    ll::service::getLevel()->getPacketSender()->sendToClient(netId, levelChunkPacket, SubClientId::PrimaryClient);

    if (forceUpdate) {
        NetworkBlockPosition pos{chunkX << 4, 80, chunkZ << 4};
        UpdateBlockPacket    blockPacket;
        blockPacket.mPos         = pos;
        blockPacket.mLayer       = UpdateBlockPacket::BlockLayer::Standard;
        blockPacket.mUpdateFlags = BlockUpdateFlag::Neighbors;
        ll::service::getLevel()->getPacketSender()->sendToClient(netId, blockPacket, SubClientId::PrimaryClient);
    }
}

void sendEmptyChunks(const NetworkIdentifier& netId, const Vec3& position, int radius, bool forceUpdate) {
    int chunkX = (int)(position.x) >> 4;
    int chunkZ = (int)(position.z) >> 4;
    for (int x = -radius; x <= radius; x++) {
        for (int z = -radius; z <= radius; z++) {
            sendEmptyChunk(netId, chunkX + x, chunkZ + z, forceUpdate);
        }
    }
}

void fakeChangeDimension(
    const NetworkIdentifier& netId,
    ActorRuntimeID           runtimeId,
    DimensionType            fakeDimId,
    const Vec3&              pos
) {
    PlayerFogPacket fogPacket{{}};
    ll::service::getLevel()->getPacketSender()->sendToClient(netId, fogPacket, SubClientId::PrimaryClient);
    ChangeDimensionPacket changeDimensionPacket{fakeDimId, pos, true};
    ll::service::getLevel()->getPacketSender()->sendToClient(netId, changeDimensionPacket, SubClientId::PrimaryClient);
    PlayerActionPacket playerActionPacket{PlayerActionType::ChangeDimensionAck, runtimeId};
    ll::service::getLevel()->getPacketSender()->sendToClient(netId, playerActionPacket, SubClientId::PrimaryClient);
    sendEmptyChunks(netId, pos, 3, true);
}
} // namespace dimension_utils

void PatchNetherHeight() {
    uintptr_t regionStart =
        (uintptr_t)ll::memory::resolveSymbol("??0NetherDimension@@QEAA@AEAVILevel@@AEAVScheduler@@@Z");
    std::vector<uint16_t> pattern = TCHelper::splitHex("41 B9 00 00 80 00");
    uintptr_t             address = ModUtils::SigScan(pattern, regionStart, 0x300);
    ModUtils::Replace(address, TCHelper::splitHex("41 B9 00 00 80 00"), TCHelper::splitHex8("41 B9 00 00 00 01"));
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    SubChunkRequestHandle,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVSubChunkRequestPacket@@@Z",
    void,
    NetworkIdentifier const& id,
    SubChunkRequestPacket&   pkt
) {
    auto pl            = this->getServerPlayer(id, pkt.mClientSubId);
    pkt.mDimensionType = pl->getDimensionId();
    return origin(id, pkt);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    SubChunkPacketWrite,
    ll::memory::HookPriority::Normal,
    SubChunkPacket,
    "?write@SubChunkPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    ChangeDimensionPacketWrite,
    ll::memory::HookPriority::Normal,
    ChangeDimensionPacket,
    "?write@ChangeDimensionPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionId == 1) {
        this->mDimensionId = 2;
    }
    return origin(bs);
}


LL_AUTO_TYPE_INSTANCE_HOOK(
    LevelRequestPlayerChangeDimensionHook,
    HookPriority::Normal,
    Level,
    "?requestPlayerChangeDimension@Level@@UEAAXAEAVPlayer@@$$QEAVChangeDimensionRequest@@@Z",
    void,
    class Player&                  player,
    class ChangeDimensionRequest&& changeRequest
) {
    auto inId = player.getDimensionId();
    if ((inId == 1 && changeRequest.mToDimensionId == 2) || (inId == 2 && changeRequest.mToDimensionId == 1)) {
        dimension_utils::fakeChangeDimension(
            player.getNetworkIdentifier(),
            player.getRuntimeID(),
            0,
            player.getPosition()
        );
    }
    return origin(player, std::move(changeRequest));
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    StartGamePacketWrite,
    ll::memory::HookPriority::Normal,
    StartGamePacket,
    "?write@StartGamePacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    auto settings = this->mSettings.getSpawnSettings();
    if (settings.dimension == 1) {
        settings.dimension = 2;
        this->mSettings.setSpawnSettings(settings);
    }
    return origin(bs);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    ClientGenerationHook,
    ll::memory::HookPriority::Normal,
    PropertiesSettings,
    &PropertiesSettings::isClientSideGenEnabled,
    bool
) {
    return false;
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    AddVolumeEntityPacketHook,
    ll::memory::HookPriority::Normal,
    AddVolumeEntityPacket,
    "?write@AddVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    ClientboundMapItemDataPacketHook,
    ll::memory::HookPriority::Normal,
    ClientboundMapItemDataPacket,
    "?write@ClientboundMapItemDataPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimension == 1) {
        this->mDimension = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    RemoveVolumeEntityPacketHook,
    ll::memory::HookPriority::Normal,
    RemoveVolumeEntityPacket,
    "?write@RemoveVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    SetSpawnPositionPacketHook,
    ll::memory::HookPriority::Normal,
    SetSpawnPositionPacket,
    "?write@SetSpawnPositionPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    SpawnParticleEffectPacketHook,
    ll::memory::HookPriority::Normal,
    SpawnParticleEffectPacket,
    "?write@SpawnParticleEffectPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mVanillaDimensionId == 1) {
        this->mVanillaDimensionId = 2;
    }
    return origin(bs);
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    LevelChunkPacketHook,
    ll::memory::HookPriority::Normal,
    LevelChunkPacket,
    "?write@LevelChunkPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& bs
) {
    if (this->mDimensionType == 1) {
        this->mDimensionType = 2;
    }
    return origin(bs);
}