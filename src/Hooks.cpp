#include "Hooks.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/network//LoopbackPacketSender.h"
#include "mc/network/LoopbackPacketSender.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/NetworkBlockPosition.h"
#include "mc/network/NetworkIdentifierWithSubId.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/packet/AddVolumeEntityPacket.h"
#include "mc/network/packet/ChangeDimensionPacket.h"
#include "mc/network/packet/DebugDrawerPacket.h"
#include "mc/network/packet/InteractPacket.h"
#include "mc/network/packet/InventoryTransactionPacket.h"
#include "mc/network/packet/LevelChunkPacket.h"
#include "mc/network/packet/PlayerActionPacket.h"
#include "mc/network/packet/PlayerActionType.h"
#include "mc/network/packet/PlayerAuthInputPacket.h"
#include "mc/network/packet/RemoveVolumeEntityPacket.h"
#include "mc/network/packet/ShapeDataPayload.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/network/packet/StartGamePacket.h"
#include "mc/network/packet/SubChunkPacket.h"
#include "mc/network/packet/SubChunkRequestPacket.h"
#include "mc/network/packet/UpdateBlockPacket.h"
#include "mc/server/PropertiesSettings.h" // IWYU pragma: keep
#include "mc/server/ServerInstance.h"
#include "mc/server/ServerPlayer.h"
#include "mc/util/MolangVariable.h"
#include "mc/util/VarIntDataOutput.h"
#include "mc/world/level/ChangeDimensionRequest.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/LoadingScreenIdManager.h"
#include "mc/world/level/SpawnSettings.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionArguments.h"
#include "mc/world/level/dimension/VanillaDimensions.h"


namespace {
void patchPacket(MinecraftPacketIds id, Packet& packet) {
    switch (id) {
    case MinecraftPacketIds::RemoveVolumeEntityPacket: {
        auto& pk = (RemoveVolumeEntityPacket&)packet;
        if (pk.mDimensionType == VanillaDimensions::Nether()) {
            pk.mDimensionType = VanillaDimensions::TheEnd();
        }
    }
    case MinecraftPacketIds::AddVolumeEntityPacket: {
        auto& pk = (AddVolumeEntityPacket&)packet;
        if (pk.mDimensionType == VanillaDimensions::Nether()) {
            pk.mDimensionType = VanillaDimensions::TheEnd();
        }
    }
    default:
        return;
    }
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    LoopbackPacketSenderHook0,
    HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClient,
    void,
    ::NetworkIdentifier const& id,
    ::Packet const&            packet,
    ::SubClientId              recipientSubId
) {
    patchPacket(packet.getId(), const_cast<Packet&>(packet));
    auto player = ll::service::getServerNetworkHandler()->_getServerPlayer(id, recipientSubId);
    if (player == nullptr) return origin(id, packet, recipientSubId);
    if (player && player->getDimensionId() == VanillaDimensions::Nether()
        && packet.getId() == MinecraftPacketIds::FullChunkData) {
        auto& pk = (LevelChunkPacket&)packet;
        if (pk.mDimensionId == VanillaDimensions::Nether()) {
            pk.mDimensionId = VanillaDimensions::TheEnd();
        }
        if (pk.mClientRequestSubChunkLimit <= 8) {
            pk.mClientRequestSubChunkLimit = 11;
        }
    }
    origin(id, packet, recipientSubId);
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    LoopbackPacketSenderHook1,
    HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClient,
    void,
    ::UserEntityIdentifierComponent const* userIdentifier,
    ::Packet const&                        packet
) {
    patchPacket(packet.getId(), const_cast<Packet&>(packet));
    auto player = ll::service::getServerNetworkHandler()->_getServerPlayer(
        userIdentifier->mNetworkId,
        userIdentifier->mClientSubId
    );
    if (player == nullptr) return origin(userIdentifier, packet);
    if (packet.getId() == MinecraftPacketIds::ChangeDimension) {
        auto& pk = (ChangeDimensionPacket&)packet;
        if (pk.mDimensionId == VanillaDimensions::Nether()) {
            pk.mDimensionId = VanillaDimensions::TheEnd();
        }
    }
    origin(userIdentifier, packet);
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    LoopbackPacketSenderHook2,
    HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClients,
    void,
    ::std::vector<::NetworkIdentifierWithSubId> const& ids,
    ::Packet const&                                    packet
) {
    patchPacket(packet.getId(), const_cast<Packet&>(packet));
    origin(ids, packet);
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    DimensionCtorHook,
    HookPriority::Normal,
    Dimension,
    &Dimension::$ctor,
    void*,
    ::DimensionArguments&& args
) {
    if (args.mDimId == VanillaDimensions::Nether()) args.mHeightRange->mMax = 256;
    return origin(std::move(args));
}
void sendEmptyChunk(const NetworkIdentifier& netId, int chunkX, int chunkZ, bool forceUpdate) {
    std::array<uchar, 4096> biome{};
    LevelChunkPacket        levelChunkPacket;
    BinaryStream            binaryStream{levelChunkPacket.mSerializedChunk, false};
    VarIntDataOutput        varIntDataOutput(binaryStream);

    varIntDataOutput.writeBytes(&biome, 4096); // write void biome
    for (int i = 1; i <= 8; i++) {
        varIntDataOutput.writeByte(255ui8);
    }
    varIntDataOutput.mStream.writeByte(0, "Byte", 0); // write border blocks

    levelChunkPacket.mPos->x         = chunkX;
    levelChunkPacket.mPos->z         = chunkZ;
    levelChunkPacket.mDimensionId    = VanillaDimensions::Overworld();
    levelChunkPacket.mCacheEnabled   = false;
    levelChunkPacket.mSubChunksCount = 0;

    ll::service::getLevel()->getPacketSender()->sendToClient(netId, levelChunkPacket, SubClientId::PrimaryClient);

    if (forceUpdate) {
        NetworkBlockPosition pos{
            BlockPos{chunkX << 4, 80, chunkZ << 4}
        };
        UpdateBlockPacket blockPacket;
        blockPacket.mPos         = pos;
        blockPacket.mLayer       = 0;
        blockPacket.mUpdateFlags = 1;
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
    const Vec3&              pos,
    std::optional<uint>      screedId
) {
    ChangeDimensionPacket changeDimensionPacket;
    changeDimensionPacket.mDimensionId     = fakeDimId;
    changeDimensionPacket.mPos             = pos;
    changeDimensionPacket.mRespawn         = true;
    changeDimensionPacket.mLoadingScreenId = {screedId};
    ll::service::getLevel()->getPacketSender()->sendToClient(netId, changeDimensionPacket, SubClientId::PrimaryClient);
    PlayerActionPacket playerActionPacket;
    playerActionPacket.mAction    = PlayerActionType::ChangeDimensionAck;
    playerActionPacket.mRuntimeId = runtimeId;
    ll::service::getLevel()->getPacketSender()->sendToClient(netId, playerActionPacket, SubClientId::PrimaryClient);
    sendEmptyChunks(netId, pos, 3, true);
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    BuildSubChunkPacketDataHook,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::_buildSubChunkPacketData,
    void,
    const ::NetworkIdentifier&     source,
    const ::ServerPlayer*          player,
    const ::SubChunkRequestPacket& packet,
    ::SubChunkPacket&              responsePacket,
    uint                           requestCount,
    bool
) {
    const_cast<SubChunkRequestPacket&>(packet).mDimensionType = player->getDimensionId();
    origin(source, player, packet, responsePacket, requestCount, false);
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    SpawnParticleEffectPacketCtorHook,
    HookPriority::Normal,
    SpawnParticleEffectPacket,
    &SpawnParticleEffectPacket::$ctor,
    void*,
    ::SpawnParticleEffectPacketPayload payload
) {
    if (payload.mVanillaDimensionId == VanillaDimensions::Nether()) {
        payload.mVanillaDimensionId = static_cast<uchar>(VanillaDimensions::TheEnd());
    }
    return origin(std::move(payload));
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    RequestPlayerChangeDimensionHook,
    HookPriority::Normal,
    Level,
    &Level::$requestPlayerChangeDimension,
    void,
    ::Player&                  player,
    ::ChangeDimensionRequest&& changeRequest
) {
    if (changeRequest.mToDimensionId == VanillaDimensions::Nether()
        || (changeRequest.mFromDimensionId == VanillaDimensions::Nether()
            && changeRequest.mToDimensionId == VanillaDimensions::TheEnd())) {
        auto loadingScreenIdManager = ll::memory::dAccess<LoadingScreenIdManager*>(&this->mLoadingScreenIdManager, 8);
        auto screenId               = loadingScreenIdManager->mLastLoadingScreenId + 1;
        ++loadingScreenIdManager->mLastLoadingScreenId;
        fakeChangeDimension(
            player.getNetworkIdentifier(),
            player.getRuntimeID(),
            VanillaDimensions::Overworld(),
            player.getPosition(),
            screenId
        );
    }
    return origin(player, std::move(changeRequest));
}
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    LevelInitializeHook,
    HookPriority::Normal,
    Level,
    &Level::$initialize,
    bool,
    std::string const&   levelName,
    LevelSettings const& levelSettings,
    Experiments const&   experiments,
    std::string const*   levelId,
    std::optional<std::reference_wrapper<
        std::unordered_map<std::string, std::unique_ptr<::BiomeJsonDocumentGlue::ResolvedBiomeData>>>>
        biomeIdToResolvedData
) {
    mClientSideChunkGenEnabled = false;
    return origin(levelName, levelSettings, experiments, levelId, biomeIdToResolvedData);
}
#ifdef LL_PLAT_S
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    PropertiesSettingsCtorHook,
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
#endif
LL_TYPE_INSTANCE_HOOK /*NOLINT*/ (
    StartGamePacketCtorHook,
    HookPriority::Normal,
    StartGamePacket,
    &StartGamePacket::$ctor,
    void*,
    ::LevelSettings const&          settings,
    ::ActorUniqueID                 entityId,
    ::ActorRuntimeID                runtimeId,
    ::GameType                      entityGameType,
    bool                            enableItemStackNetManager,
    ::Vec3 const&                   pos,
    ::Vec2 const&                   rot,
    ::std::string const&            levelId,
    ::std::string const&            levelName,
    ::ContentIdentity const&        premiumTemplateContentIdentity,
    ::std::string const&            multiplayerCorrelationId,
    ::BlockDefinitionGroup const&   blockDefinitionGroup,
    bool                            isTrial,
    ::CompoundTag                   playerPropertyData,
    ::PlayerMovementSettings const& movementSettings,
    bool                            enableTickDeathSystems,
    ::std::string const&            serverVersion,
    ::mce::UUID const&              worldTemplateId,
    uint64                          levelCurrentTime,
    int                             enchantmentSeed,
    uint64                          blockTypeRegistryChecksum
) {
    if (settings.mSpawnSettings->dimension == VanillaDimensions::Nether()) {
        const_cast<ll::TypedStorage<sizeof(DimensionType), alignof(DimensionType), ::DimensionType>&>(
            settings.mSpawnSettings->dimension
        ) = VanillaDimensions::TheEnd();
    }
    return origin(
        settings,
        entityId,
        runtimeId,
        entityGameType,
        enableItemStackNetManager,
        pos,
        rot,
        levelId,
        levelName,
        premiumTemplateContentIdentity,
        multiplayerCorrelationId,
        blockDefinitionGroup,
        isTrial,
        playerPropertyData,
        movementSettings,
        enableTickDeathSystems,
        serverVersion,
        worldTemplateId,
        levelCurrentTime,
        enchantmentSeed,
        blockTypeRegistryChecksum
    );
}
} // namespace

struct FuckNetherHeightHooks::Impl {
    ll::memory::HookRegistrar<
        LoopbackPacketSenderHook0,
        LoopbackPacketSenderHook1,
        LoopbackPacketSenderHook2,
        DimensionCtorHook,
        BuildSubChunkPacketDataHook,
        SpawnParticleEffectPacketCtorHook,
        RequestPlayerChangeDimensionHook,
        StartGamePacketCtorHook,
        LevelInitializeHook
#ifdef LL_PLAT_S
        ,
        PropertiesSettingsCtorHook
#endif
        >
        hooks;
};
FuckNetherHeightHooks::FuckNetherHeightHooks() : pImpl(std::make_unique<Impl>()) {}
FuckNetherHeightHooks::~FuckNetherHeightHooks() = default;
FuckNetherHeightHooks& FuckNetherHeightHooks::getInstance() {
    static FuckNetherHeightHooks hooks;
    return hooks;
}

MolangVariableMap::MolangVariableMap(MolangVariableMap const& rhs) {
    mMapFromVariableIndexToVariableArrayOffset = rhs.mMapFromVariableIndexToVariableArrayOffset;
    mVariables                                 = {};
    for (auto& ptr : *rhs.mVariables) {
        mVariables->push_back(std::make_unique<MolangVariable>(*ptr));
    }
    mHasPublicVariables = rhs.mHasPublicVariables;
}