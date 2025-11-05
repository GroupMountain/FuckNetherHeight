#include "Global.h"
#include "ll/api/memory/Hook.h"
#include "mc/deps/core/utility/AutomaticID.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/legacy/ActorRuntimeID.h" // IWYU pragma: keep
#include "mc/network/NetworkBlockPosition.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/packet/AddVolumeEntityPacket.h"
#include "mc/network/packet/ChangeDimensionPacket.h"
#include "mc/network/packet/ClientCacheStatusPacket.h"
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
#include "mc/server/ServerPlayer.h"
#include "mc/util/LoadingScreenId.h" // IWYU pragma: keep
#include "mc/util/NewType.h"         // IWYU pragma: keep
#include "mc/util/VarIntDataOutput.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/ChangeDimensionRequest.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/IPlayerDimensionTransferProxy.h"
#include "mc/world/level/ISharedSpawnGetter.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/LevelSettings.h"
#include "mc/world/level/LoadingScreenIdManager.h"
#include "mc/world/level/PlayerDimensionTransferer.h"
#include "mc/world/level/SpawnSettings.h"
#include "mc/world/level/block/registry/BlockTypeRegistry.h" // IWYU pragma: keep
#include "mc/world/level/chunk/SubChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionArguments.h"
#include "mc/world/level/dimension/DimensionHeightRange.h" // IWYU pragma: keep
#include "mc/world/level/dimension/VanillaDimensions.h"

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
    DimensionArguments&& args
) {
    if (args.mDimId == VanillaDimensions::Nether()) args.mHeightRange->mMax = 256;
    return origin(std::move(args));
}

LL_TYPE_INSTANCE_HOOK(
    ClientGenerationHook,
    HookPriority::Normal,
    PropertiesSettings,
    &PropertiesSettings::$ctor,
    void*,
    std::string const& filename
) {
    auto res                                                                 = origin(filename);
    reinterpret_cast<PropertiesSettings*>(res)->mClientSideGenerationEnabled = false;
    return res;
}

LL_TYPE_INSTANCE_HOOK(
    SubChunkRequestHandle,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const&     source,
    ::SubChunkRequestPacket const& packet
) {
    const_cast<SubChunkRequestPacket&>(packet).mDimensionType =
        thisFor<NetEventCallback>()->_getServerPlayer(source, packet.mSenderSubId)->getDimensionId();
    return origin(source, packet);
}

LL_TYPE_INSTANCE_HOOK(
    BuildSubChunkPacketData,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::_buildSubChunkPacketData,
    void,
    ::NetworkIdentifier const&     source,
    ::ServerPlayer const*          player,
    ::SubChunkRequestPacket const& packet,
    ::SubChunkPacket&              responsePacket,
    uint                           requestCount,
    bool                           clientCacheEnabled
) {
    origin(source, player, packet, responsePacket, requestCount, clientCacheEnabled);
    DIM_ID_MODIRY(responsePacket.mDimensionType);
}

LL_TYPE_INSTANCE_HOOK(
    ClientCacheStatusHook,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const&       source,
    ::ClientCacheStatusPacket const& packet
) {
    const_cast<ClientCacheStatusPacket&>(packet).mEnabled = false;
    return origin(source, packet);
}

#define PACKET_WRITE_HOOK(PACKET)                                                                                      \
    void PACKET##Modify(PACKET* packet);                                                                               \
    LL_TYPE_INSTANCE_HOOK(PACKET##Write0, HookPriority::Normal, PACKET, &PACKET::$write, void, BinaryStream& stream) { \
        PACKET##Modify(this);                                                                                          \
        return origin(stream);                                                                                         \
    }                                                                                                                  \
    LL_TYPE_INSTANCE_HOOK(                                                                                             \
        PACKET##Write1,                                                                                                \
        HookPriority::Normal,                                                                                          \
        PACKET,                                                                                                        \
        &PACKET::$writeWithSerializationMode,                                                                          \
        void,                                                                                                          \
        BinaryStream&                    stream,                                                                       \
        cereal::ReflectionCtx const&     reflectionCtx,                                                                \
        std::optional<SerializationMode> overrideMode                                                                  \
    ) {                                                                                                                \
        PACKET##Modify(this);                                                                                          \
        return origin(stream, reflectionCtx, overrideMode);                                                            \
    }                                                                                                                  \
    void PACKET##Modify(PACKET* packet)

PACKET_WRITE_HOOK(ChangeDimensionPacket) { DIM_ID_MODIRY(*packet->mDimensionId); }
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
PACKET_WRITE_HOOK(AddVolumeEntityPacket) { DIM_ID_MODIRY(*packet->mDimensionType); }
LL_TYPE_INSTANCE_HOOK(
    ClientboundMapItemDataPacketHook,
    HookPriority::Normal,
    ClientboundMapItemDataPacket,
    &ClientboundMapItemDataPacket::$write,
    void,
    BinaryStream& stream
) {
    DIM_ID_MODIRY(this->mDimension);
    return origin(stream);
}
PACKET_WRITE_HOOK(RemoveVolumeEntityPacket) { DIM_ID_MODIRY(*packet->mDimensionType); }
PACKET_WRITE_HOOK(SetSpawnPositionPacket) { DIM_ID_MODIRY(*packet->mDimensionType); }
PACKET_WRITE_HOOK(SpawnParticleEffectPacket) { DIM_ID_MODIRY(packet->mVanillaDimensionId); }

LL_TYPE_INSTANCE_HOOK(
    LevelChunkPacketHook,
    HookPriority::Normal,
    LevelChunkPacket,
    &LevelChunkPacket::$write,
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
    if (player.getDimensionId() != VanillaDimensions::Nether()) {
        return origin(player, std::move(changeRequest));
    }
    ChangeDimensionRequest request0{
        changeRequest.mState,
        changeRequest.mFromDimensionId,
        VanillaDimensions::Overworld(),
        changeRequest.mFromLocation,
        changeRequest.mToLocation,
        false,
        changeRequest.mRespawn,
        changeRequest.mAgentTag ? changeRequest.mAgentTag->clone() : nullptr,
    };
    origin(player, std::move(request0));
    ChangeDimensionRequest request1{
        changeRequest.mState,
        VanillaDimensions::Overworld(),
        changeRequest.mToDimensionId,
        changeRequest.mFromLocation,
        changeRequest.mToLocation,
        changeRequest.mUsePortal,
        changeRequest.mRespawn,
        changeRequest.mAgentTag ? changeRequest.mAgentTag->clone() : nullptr,
    };
    origin(player, std::move(request1));
}
ChangeDimensionRequest::~ChangeDimensionRequest() = default;


struct Impl {
    ll::memory::HookRegistrar<
        ModifyNetherHeightHook,
        ClientGenerationHook,
        SubChunkRequestHandle,
        BuildSubChunkPacketData,
        ChangeDimensionPacketWrite0,
        ChangeDimensionPacketWrite1,
        LevelRequestPlayerChangeDimensionHook,
        StartGamePacketWrite,
        AddVolumeEntityPacketWrite0,
        AddVolumeEntityPacketWrite1,
        ClientboundMapItemDataPacketHook,
        RemoveVolumeEntityPacketWrite0,
        RemoveVolumeEntityPacketWrite1,
        SetSpawnPositionPacketWrite0,
        SetSpawnPositionPacketWrite1,
        SpawnParticleEffectPacketWrite0,
        SpawnParticleEffectPacketWrite1,
        LevelChunkPacketHook,
        ClientCacheStatusHook>
        hook;
};

static std::unique_ptr<Impl> impl;

void enableMod() {
    if (!impl) impl = std::make_unique<Impl>();
}

void disableMod() {
    if (impl) impl.reset();
}