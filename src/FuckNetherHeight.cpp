#include "Global.h"
#include "mc/common/ActorRuntimeID.h" // IWYU pragma: keep
#include "mc/deps/core/utility/AutomaticID.h"
#include "mc/network/packet/AddVolumeEntityPacket.h"
#include "mc/network/packet/ChangeDimensionPacket.h"
#include "mc/network/packet/ClientboundMapItemDataPacket.h"
#include "mc/network/packet/LevelChunkPacket.h"
#include "mc/network/packet/PlayerActionPacket.h"
#include "mc/network/packet/PlayerActionType.h"
#include "mc/network/packet/PlayerFogPacket.h"
#include "mc/network/packet/RemoveVolumeEntityPacket.h"
#include "mc/network/packet/SetSpawnPositionPacket.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/network/packet/StartGamePacket.h"
#include "mc/network/packet/SubChunkPacket.h"
#include "mc/network/packet/SubChunkRequestPacket.h"
#include "mc/network/packet/UpdateBlockPacket.h"
#include "mc/server/PropertiesSettings.h"
#include "mc/util/NewType.h" // IWYU pragma: keep
#include "mc/util/VarIntDataOutput.h"
#include "mc/world/level/ChangeDimensionRequest.h"
#include "mc/world/level/IPlayerDimensionTransferProxy.h"
#include "mc/world/level/ISharedSpawnGetter.h"
#include "mc/world/level/LevelSettings.h"
#include "mc/world/level/LoadingScreenIdManager.h"
#include "mc/world/level/PlayerDimensionTransferer.h"
#include "mc/world/level/SpawnSettings.h"
#include "mc/world/level/block/registry/BlockTypeRegistry.h"
#include "mc/world/level/chunk/SubChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionHeightRange.h"
#include "mc/world/level/dimension/VanillaDimensions.h"


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
    sendEmptyChunks(player, 3, true);
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
    "??0Dimension@@QEAA@AEAVILevel@@V?$AutomaticID@VDimension@@H@@VDimensionHeightRange@@AEAVScheduler@@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"_sym,
    Dimension*,
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
    "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVSubChunkRequestPacket@@@Z"_sym,
    void,
    NetworkIdentifier const& id,
    SubChunkRequestPacket&   pkt
) {
    pkt.mDimensionType =
        ll::service::getServerNetworkHandler()->_getServerPlayer(id, pkt.mClientSubId)->getDimensionId();
    return origin(id, pkt);
}

LL_TYPE_INSTANCE_HOOK(
    SubChunkPacketWrite,
    HookPriority::Normal,
    SubChunkPacket,
    "?write@SubChunkPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    ChangeDimensionPacketWrite,
    HookPriority::Normal,
    ChangeDimensionPacket,
    "?write@ChangeDimensionPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(*this->mDimensionId);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    LevelRequestPlayerChangeDimensionHook,
    HookPriority::Normal,
    Level,
    "?requestPlayerChangeDimension@Level@@UEAAXAEAVPlayer@@$$QEAVChangeDimensionRequest@@@Z"_sym,
    void,
    Player&                  player,
    ChangeDimensionRequest&& changeRequest
) {
    auto fromId = player.getDimensionId();
    if ((fromId == 1 && *changeRequest.mToDimensionId == 2) || (fromId == 2 && *changeRequest.mToDimensionId == 1)) {
        dimension_utils::fakeChangeDimension(player);
    }
    return origin(player, std::move(changeRequest));
}

LL_TYPE_INSTANCE_HOOK(
    StartGamePacketWrite,
    HookPriority::Normal,
    StartGamePacket,
    "?write@StartGamePacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
    void,
    BinaryStream const& stream
) {
    auto settings = this->mSettings->getSpawnSettings();
    DIM_ID_MODIRY(*settings.dimension, this->mSettings->setSpawnSettings(settings););
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    AddVolumeEntityPacketHook,
    HookPriority::Normal,
    AddVolumeEntityPacket,
    "?write@AddVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    ClientboundMapItemDataPacketHook,
    HookPriority::Normal,
    ClientboundMapItemDataPacket,
    "?write@ClientboundMapItemDataPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
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
    "?write@RemoveVolumeEntityPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    SetSpawnPositionPacketHook,
    HookPriority::Normal,
    SetSpawnPositionPacket,
    "?write@SetSpawnPositionPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(*this->mDimensionType);
    return origin(stream);
}

LL_TYPE_INSTANCE_HOOK(
    SpawnParticleEffectPacketHook,
    HookPriority::Normal,
    SpawnParticleEffectPacket,
    "?write@SpawnParticleEffectPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
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
    "?write@LevelChunkPacket@@UEBAXAEAVBinaryStream@@@Z"_sym,
    void,
    BinaryStream const& stream
) {
    DIM_ID_MODIRY(*this->mDimensionId);
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
        PlayerDimensionTransfererCtorHook>
        hook;
};

static std::unique_ptr<Impl> impl;

void enableMod() {
    if (!impl) impl = std::make_unique<Impl>();
}

void disableMod() {
    if (impl) impl.reset();
}