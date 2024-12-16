#include "Global.h"

namespace dimension_utils {
void sendEmptyChunk(Player& player, int chunkX, int chunkZ, bool forceUpdate) {
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

    levelChunkPacket.sendTo(player);

    if (forceUpdate) {
        UpdateBlockPacket(
            {chunkX << 4, 80, chunkZ << 4},
            (uint)UpdateBlockPacket::BlockLayer::Standard,
            0,
            (uchar)BlockUpdateFlag::Neighbors
        )
            .sendTo(player);
    }
}

void sendEmptyChunks(Player& player, int radius, bool forceUpdate) {
    int chunkX = (int)(player.getPosition().x) >> 4;
    int chunkZ = (int)(player.getPosition().z) >> 4;
    for (int x = -radius; x <= radius; x++) {
        for (int z = -radius; z <= radius; z++) {
            sendEmptyChunk(player, chunkX + x, chunkZ + z, forceUpdate);
        }
    }
}

void fakeChangeDimension(Player& player) {
    PlayerFogPacket(std::vector<std::string>{}).sendTo(player);
    ChangeDimensionPacket(VanillaDimensions::Overworld, player.getPosition(), true).sendTo(player);
    PlayerActionPacket(PlayerActionType::ChangeDimensionAck, player.getRuntimeID()).sendTo(player); 
    sendEmptyChunks(player, 3, true);
}
} // namespace dimension_utils

#define DIM_ID_MODIRY(name, ...)                                                                                       \
    if (name == VanillaDimensions::Nether) {                                                                           \
        name = VanillaDimensions::TheEnd;                                                                              \
        __VA_ARGS__                                                                                                    \
    }

LL_TYPE_INSTANCE_HOOK(
    ModifyNetherHeightHook,
    HookPriority::Normal,
    Dimension,
    "??0Dimension@@QEAA@AEAVILevel@@V?$AutomaticID@VDimension@@H@@VDimensionHeightRange@@AEAVScheduler@@V?$basic_"
    "string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
    Dimension*,
    ILevel&              level,
    DimensionType        dimId,
    DimensionHeightRange heightRange,
    Scheduler&           callbackContext,
    std::string          name
) {
    if (dimId == VanillaDimensions::Nether) {
        heightRange.max = 256;
    }
    return origin(level, dimId, heightRange, callbackContext, name);
}

LL_TYPE_INSTANCE_HOOK(
    ClientGenerationHook,
    HookPriority::Normal,
    PropertiesSettings,
    &PropertiesSettings::isClientSideGenEnabled,
    bool
) {
    return false;
}

LL_TYPE_INSTANCE_HOOK(
    SubChunkRequestHandle,
    HookPriority::Normal,
    ServerNetworkHandler,
    "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVSubChunkRequestPacket@@@Z",
    void,
    NetworkIdentifier const& id,
    SubChunkRequestPacket&   pkt
) {
    pkt.mDimensionType = this->getServerPlayer(id, pkt.mClientSubId)->getDimensionId();
    return origin(id, pkt);
}

LL_TYPE_INSTANCE_HOOK(
    SubChunkPacketWrite,
    HookPriority::Normal,
    SubChunkPacket,
    "?write@SubChunkPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    ChangeDimensionPacketWrite,
    HookPriority::Normal,
    ChangeDimensionPacket,
    "?write@ChangeDimensionPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mDimensionId);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    LevelRequestPlayerChangeDimensionHook,
    HookPriority::Normal,
    Level,
    "?requestPlayerChangeDimension@Level@@UEAAXAEAVPlayer@@$$QEAVChangeDimensionRequest@@@Z",
    void,
    Player&                  player,
    ChangeDimensionRequest&& changeRequest
) {
    auto fromId = player.getDimensionId();
    if ((fromId == 1 && changeRequest.mToDimensionId == 2) || (fromId == 2 && changeRequest.mToDimensionId == 1)) {
        dimension_utils::fakeChangeDimension(player);
    }
    return origin(player, std::move(changeRequest));
}

LL_TYPE_INSTANCE_HOOK(
    StartGamePacketWrite,
    HookPriority::Normal,
    StartGamePacket,
    "?write@StartGamePacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    auto settings = this->mSettings.getSpawnSettings();
    DIM_ID_MODIRY(settings.dimension, this->mSettings.setSpawnSettings(settings););
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    AddVolumeEntityPacketHook,
    HookPriority::Normal,
    AddVolumeEntityPacket,
    "?write@AddVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    ClientboundMapItemDataPacketHook,
    HookPriority::Normal,
    ClientboundMapItemDataPacket,
    "?write@ClientboundMapItemDataPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mDimension);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    RemoveVolumeEntityPacketHook,
    HookPriority::Normal,
    RemoveVolumeEntityPacket,
    "?write@RemoveVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    SetSpawnPositionPacketHook,
    HookPriority::Normal,
    SetSpawnPositionPacket,
    "?write@SetSpawnPositionPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    SpawnParticleEffectPacketHook,
    HookPriority::Normal,
    SpawnParticleEffectPacket,
    "?write@SpawnParticleEffectPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mVanillaDimensionId);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    LevelChunkPacketHook,
    HookPriority::Normal,
    LevelChunkPacket,
    "?write@LevelChunkPacket@@UEBAXAEAVBinaryStream@@@Z",
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(this->mDimensionType);
    return origin(stream);
}

struct Impl {
    ll::memory::HookRegistrar<
        ModifyNetherHeightHook,
        ClientGenerationHook,
        SubChunkRequestHandle,
        SubChunkPacketWrite,
        ChangeDimensionPacketWrite,
        LevelRequestPlayerChangeDimensionHook,
        StartGamePacketWrite,
        AddVolumeEntityPacketHook,
        ClientboundMapItemDataPacketHook,
        RemoveVolumeEntityPacketHook,
        SetSpawnPositionPacketHook,
        SpawnParticleEffectPacketHook,
        LevelChunkPacketHook>
        hook;
};

static std::unique_ptr<Impl> impl;

void enableMod() {
    if (!impl) impl = std::make_unique<Impl>();
}

void disableMod() {
    if (impl) impl.reset();
}