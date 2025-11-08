#include "pch.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wreinterpret-base-class"
#pragma clang diagnostic ignored "-Winconsistent-dllimport"
#endif
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
    ClientCacheBlobStatusHandle,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const&,
    ::ClientCacheBlobStatusPacket const&
) {
    return;
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
    bool
) {
    origin(source, player, packet, responsePacket, requestCount, false);
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
    ClientboundMapItemDataPacketWrite,
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
#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4273)
LL_TYPE_INSTANCE_HOOK(
    LevelRequestPlayerChangeDimensionHook,
    HookPriority::Normal,
    Level,
    &Level::$requestPlayerChangeDimension,
    void,
    Player&                  player,
    ChangeDimensionRequest&& changeRequest
) {
    // auto from    = *changeRequest.mFromDimensionId;
    // auto to      = *changeRequest.mToDimensionId;
    // auto fromPos = *changeRequest.mFromLocation;
    // auto toPos   = *changeRequest.mToLocation;
    // if (from == VanillaDimensions::Nether()) {
    //     ChangeDimensionPacket packet;
    //     packet.mDimensionId             = VanillaDimensions::Nether();
    //     packet.mPos                     = fromPos;
    //     packet.mRespawn                 = true;
    //     packet.mLoadingScreenId->mValue = ++mLoadingScreenIdManager->mValue->mUnk7db596.as<std::uint32_t>();
    //     player.sendNetworkPacket(packet);
    //     PlayerFogPacket fog;
    //     fog.mFogStack = {};
    //     player.sendNetworkPacket(fog);
    // }
    // origin(player, std::move(changeRequest));
    // if (to == VanillaDimensions::Nether()) {
    //     ChangeDimensionPacket packet;
    //     packet.mDimensionId             = VanillaDimensions::TheEnd();
    //     packet.mPos                     = toPos;
    //     packet.mRespawn                 = true;
    //     packet.mLoadingScreenId->mValue = ++mLoadingScreenIdManager->mValue->mUnk7db596.as<std::uint32_t>();
    //     player.sendNetworkPacket(packet);
    // }
    if ((changeRequest.mFromDimensionId == VanillaDimensions::Nether()
         && changeRequest.mToDimensionId == VanillaDimensions::TheEnd())
        || (changeRequest.mFromDimensionId == VanillaDimensions::TheEnd()
            && changeRequest.mToDimensionId == VanillaDimensions::Nether())) {
        ChangeDimensionRequest request0{
            changeRequest.mState,
            changeRequest.mFromDimensionId,
            VanillaDimensions::Overworld(),
            changeRequest.mFromLocation,
            changeRequest.mToLocation,
            changeRequest.mUsePortal,
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
    } else {
        origin(player, std::move(changeRequest));
    }
}
ChangeDimensionRequest::~ChangeDimensionRequest() = default;
#pragma warning(pop)
PACKET_WRITE_HOOK(DebugDrawerPacket) {
    for (auto& shape : *packet->mPayload->mShapes) {
        DIM_ID_MODIRY(shape.mDimensionId);
    }
}
PACKET_WRITE_HOOK(ChangeDimensionPacket) { DIM_ID_MODIRY(packet->mDimensionId); }


struct Impl {
    ll::memory::HookRegistrar<
        ModifyNetherHeightHook,
        ClientGenerationHook,
        SubChunkRequestHandle,
        BuildSubChunkPacketData,
        LevelRequestPlayerChangeDimensionHook,
        StartGamePacketWrite,
        AddVolumeEntityPacketWrite0,
        AddVolumeEntityPacketWrite1,
        ClientboundMapItemDataPacketWrite,
        RemoveVolumeEntityPacketWrite0,
        RemoveVolumeEntityPacketWrite1,
        SetSpawnPositionPacketWrite0,
        SetSpawnPositionPacketWrite1,
        SpawnParticleEffectPacketWrite0,
        SpawnParticleEffectPacketWrite1,
        LevelChunkPacketHook,
        ClientCacheStatusHook,
        ClientCacheBlobStatusHandle,
        DebugDrawerPacketWrite0,
        DebugDrawerPacketWrite1,
        ChangeDimensionPacketWrite0,
        ChangeDimensionPacketWrite1>
        hook;
};

static std::unique_ptr<Impl> impl;

void enableMod() {
    if (!impl) impl = std::make_unique<Impl>();
}

void disableMod() {
    if (impl) impl.reset();
}