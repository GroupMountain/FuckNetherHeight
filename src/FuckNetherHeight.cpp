#include "Global.h"

namespace dimension_utils {
void sendEmptyChunk(Player& player, int chunkX, int chunkZ, bool forceUpdate) {
    std::array<uchar, 4096> biome{};
    LevelChunkPacket        levelChunkPacket;
    BinaryStream            binaryStream{levelChunkPacket.mSerializedChunk, false};
    VarIntDataOutput        varIntDataOutput(binaryStream);

    varIntDataOutput.writeBytes(&biome, 4096); // write void biome
    for (int i = 1; i < 8; i++) {
        varIntDataOutput.writeByte((127u << 1) | 1);
    }
    varIntDataOutput.mStream.writeUnsignedChar(0); // write border blocks
    levelChunkPacket.mPos->x         = chunkX;
    levelChunkPacket.mPos->z         = chunkZ;
    levelChunkPacket.mDimensionId    = 0;
    levelChunkPacket.mCacheEnabled   = false;
    levelChunkPacket.mSubChunksCount = 0;

    levelChunkPacket.sendTo(player);

    if (forceUpdate) {
        UpdateBlockPacket(
            {chunkX << 4, 80, chunkZ << 4},
            (uint)SubChunk::BlockLayer::Standard,
            BlockTypeRegistry::getDefaultBlockState("minecraft:air").getRuntimeId(),
            (uchar)1
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

LoadingScreenIdManager* manager = nullptr;

void fakeChangeDimension(Player& player) {
    PlayerFogPacket(std::vector<std::string>{}).sendTo(player);
    GMLIB_BinaryStream binaryStream;
    binaryStream.writePacketHeader(MinecraftPacketIds::ChangeDimension);
    binaryStream.writeVarInt(VanillaDimensions::toSerializedInt(VanillaDimensions::Overworld()));
    binaryStream.writeVec3(player.getPosition());
    binaryStream.writeBool(true);
    binaryStream.writeBool(true);
    binaryStream.writeUnsignedInt(manager->getNextLoadingScreenId().mValue.value());
    binaryStream.sendTo(player);
    PlayerActionPacket(PlayerActionType::ChangeDimensionAck, player.getRuntimeID()).sendTo(player);
    sendEmptyChunks(player, 3, false); // todo(killcerr): fix forceUpdate
}
} // namespace dimension_utils

LL_TYPE_INSTANCE_HOOK(
    PlayerDimensionTransfererCtorHook,
    HookPriority::Normal,
    PlayerDimensionTransferer,
    &PlayerDimensionTransferer::$ctor,
    void*,
    ::std::unique_ptr<::IPlayerDimensionTransferProxy>   playerDimensionTransferProxy,
    bool                                                 isClientSide,
    ::Bedrock::NotNullNonOwnerPtr<::PortalForcer>        portalForcer,
    ::std::unique_ptr<::ISharedSpawnGetter>              sharedSpawnGetter,
    ::Bedrock::NonOwnerPointer<::LevelStorage>           levelStorage,
    ::Bedrock::NonOwnerPointer<::LoadingScreenIdManager> loadingScreenIdManager
) {
    dimension_utils::manager = loadingScreenIdManager;
    return origin(
        std::move(playerDimensionTransferProxy),
        isClientSide,
        portalForcer,
        std::move(sharedSpawnGetter),
        levelStorage,
        loadingScreenIdManager
    );
}


#define DIM_ID_MODIRY(name, ...)                                                                                       \
    if (name == VanillaDimensions::Nether()) {                                                                         \
        name = (decltype(name))VanillaDimensions::TheEnd();                                                            \
        __VA_ARGS__                                                                                                    \
    }
using namespace ll::literals;
LL_TYPE_INSTANCE_HOOK(
    ModifyNetherHeightHook,
    HookPriority::Normal,
    Dimension,
    &Dimension::$ctor,
    void*,
    ILevel&              level,
    DimensionType        dimId,
    DimensionHeightRange heightRange,
    Scheduler&           callbackContext,
    std::string          name
) {
    if (dimId == VanillaDimensions::Nether()) {
        heightRange.mMax = 256;
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
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&     id,
    SubChunkRequestPacket const& pkt
) {
    SubChunkRequestPacket fakepkt = pkt;    //QwQ
    fakepkt.mDimensionType =
        ll::service::getServerNetworkHandler()->_getServerPlayer(id, pkt.mClientSubId)->getDimensionId();
    return origin(id, fakepkt);
}

LL_TYPE_INSTANCE_HOOK(
    SubChunkPacketWrite,
    HookPriority::Normal,
    SubChunkPacket,
    &SubChunkPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    SubchunkPacketCtorHook,
    HookPriority::Normal,
    SubChunkPacket,
    &SubChunkPacket::$ctor,
    void*,
    ::DimensionType const& dimension,
    ::SubChunkPos const&   centerPos,
    bool
) {
    return origin(dimension, centerPos, false);
}

LL_TYPE_INSTANCE_HOOK(
    ChangeDimensionPacketWrite,
    HookPriority::Normal,
    ChangeDimensionPacket,
    &ChangeDimensionPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(*this->mDimensionId);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    LevelRequestPlayerChangeDimensionHook,
    HookPriority::Normal,
    Level,
    &Level::$requestPlayerChangeDimension,
    void,
    Player&                  player,
    ChangeDimensionRequest&& changeRequest
) {
    auto fromId = player.getDimensionId();
    if ((fromId.id == 1 && changeRequest.mToDimensionId->id == 2)
        || (fromId.id == 2 && changeRequest.mToDimensionId->id == 1)) {
        dimension_utils::fakeChangeDimension(player);
    }
    return origin(player, std::move(changeRequest));
}

LL_TYPE_INSTANCE_HOOK(
    StartGamePacketWrite,
    HookPriority::Normal,
    StartGamePacket,
    &StartGamePacket::$write,
    void,
    BinaryStream& stream
) {
    auto settings = this->mSettings->getSpawnSettings();
    DIM_ID_MODIRY(*settings.dimension, this->mSettings->setSpawnSettings(settings););
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    AddVolumeEntityPacketHook,
    HookPriority::Normal,
    AddVolumeEntityPacket,
    &AddVolumeEntityPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    ClientboundMapItemDataPacketHook,
    HookPriority::Normal,
    ClientboundMapItemDataPacket,
    &ClientboundMapItemDataPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(DimensionType{this->mDimension});
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    RemoveVolumeEntityPacketHook,
    HookPriority::Normal,
    RemoveVolumeEntityPacket,
    &RemoveVolumeEntityPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    SetSpawnPositionPacketHook,
    HookPriority::Normal,
    SetSpawnPositionPacket,
    &SetSpawnPositionPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    SpawnParticleEffectPacketHook,
    HookPriority::Normal,
    SpawnParticleEffectPacket,
    &SpawnParticleEffectPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(DimensionType{this->mVanillaDimensionId});
    return origin(stream);
}
LL_TYPE_INSTANCE_HOOK(CacheHook, HookPriority::Normal, ClientBlobCache::Server::ActiveTransfersManager, &ClientBlobCache::Server::ActiveTransfersManager::isCacheEnabledFor, bool, ::NetworkIdentifier const&) {
    return false;
}
LL_TYPE_INSTANCE_HOOK(
    LevelChunkPacketHook,
    HookPriority::Normal,
    LevelChunkPacket,
    &LevelChunkPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(*this->mDimensionId);
    this->mCacheEnabled = false;
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
        LevelChunkPacketHook,
        PlayerDimensionTransfererCtorHook,
        CacheHook,
        SubchunkPacketCtorHook>
        hook;
};

static std::unique_ptr<Impl> impl;

void enableMod() {
    if (!impl) impl = std::make_unique<Impl>();
}

void disableMod() {
    if (impl) impl.reset();
}