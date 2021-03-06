#include "ConfigReader.h"
#include "ConfigProps.h"

#include <Config/Environment.h>
#include <Config/Genesis.h>
#include <BitUtil.h>
#include <filesystem>

Config ConfigReader::ReadConfig(const Json::Value& root) const
{
	// Read Mode
	const EClientMode clientMode = ReadClientMode(root);

	// Read Environment
	const Environment environment = ReadEnvironment(root);

	// Read Directories
	const std::string dataPath = ReadDataPath(root);

	// Read P2P Config
	const P2PConfig p2pConfig = ReadP2P(root);

	// Read Dandelion Config
	const DandelionConfig dandelionConfig = ReadDandelion(root);

	// TODO: Mempool, mining, wallet, and logger settings

	return Config(clientMode, environment, dataPath, dandelionConfig, p2pConfig);
}

EClientMode ConfigReader::ReadClientMode(const Json::Value& root) const
{
	if (root.isMember(ConfigProps::CLIENT_MODE))
	{
		const std::string mode = root.get(ConfigProps::CLIENT_MODE, "FAST_SYNC").asString();
		if (mode == "FAST_SYNC")
		{
			return EClientMode::FAST_SYNC;
		}
		else if (mode == "LIGHT_CLIENT")
		{
			return EClientMode::LIGHT_CLIENT;
		}
		else if (mode == "FULL_HISTORY")
		{
			return EClientMode::FULL_HISTORY;
		}
	}

	return EClientMode::FAST_SYNC;
}

Environment ConfigReader::ReadEnvironment(const Json::Value& root) const
{
	const uint16_t FLOONET_PORT = 13414;
	const std::vector<unsigned char> FLOONET_MAGIC = { 83, 59 };

	const uint32_t floonetPrivateVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x04, 0xA4);
	const uint32_t floonetPublicVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x08, 0xDF);
	if (root.isMember(ConfigProps::ENVIRONMENT))
	{
		const std::string environment = root.get(ConfigProps::ENVIRONMENT, "FLOONET").asString();
		if (environment == "MAINNET")
		{
			const uint16_t MAINNET_PORT = 3414;
			const std::vector<unsigned char> MAINNET_MAGIC = { 97, 61 };
			return Environment(EEnvironmentType::MAINNET, Genesis::MAINNET_GENESIS, MAINNET_MAGIC, MAINNET_PORT, floonetPrivateVersion, floonetPublicVersion); // MAINNET: Figure out private/public Version
		}
		else if (environment == "FLOONET")
		{
			return Environment(EEnvironmentType::FLOONET, Genesis::FLOONET_GENESIS, FLOONET_MAGIC, FLOONET_PORT, floonetPrivateVersion, floonetPublicVersion);
		}
	}


	return Environment(EEnvironmentType::FLOONET, Genesis::FLOONET_GENESIS, FLOONET_MAGIC, FLOONET_PORT, floonetPrivateVersion, floonetPublicVersion);
}

std::string ConfigReader::ReadDataPath(const Json::Value& root) const
{
	const std::string defaultPath = std::filesystem::current_path().string() + "/DATA/";
	if (root.isMember(ConfigProps::DATA_PATH))
	{
		return root.get(ConfigProps::DATA_PATH, defaultPath).asString();
	}

	return defaultPath;
}

P2PConfig ConfigReader::ReadP2P(const Json::Value& root) const
{
	int maxPeers = 30;
	int minPeers = 15;

	if (root.isMember(ConfigProps::P2P::P2P))
	{
		const Json::Value& p2pRoot = root[ConfigProps::P2P::P2P];

		if (p2pRoot.isMember(ConfigProps::P2P::MAX_PEERS))
		{
			maxPeers = p2pRoot.get(ConfigProps::P2P::MAX_PEERS, 30).asInt();
		}

		if (p2pRoot.isMember(ConfigProps::P2P::MIN_PEERS))
		{
			minPeers = p2pRoot.get(ConfigProps::P2P::MIN_PEERS, 15).asInt();
		}
	}

	return P2PConfig(maxPeers, minPeers);
}

DandelionConfig ConfigReader::ReadDandelion(const Json::Value& root) const
{
	uint16_t relaySeconds = 600;
	uint16_t embargoSeconds = 180;
	uint8_t patienceSeconds = 10;
	uint8_t stemProbability = 90;

	if (root.isMember(ConfigProps::Dandelion::DANDELION))
	{
		const Json::Value& dandelionRoot = root[ConfigProps::Dandelion::DANDELION];

		if (dandelionRoot.isMember(ConfigProps::Dandelion::RELAY_SECS))
		{
			relaySeconds = (uint16_t)dandelionRoot.get(ConfigProps::Dandelion::RELAY_SECS, 600).asUInt();
		}

		if (dandelionRoot.isMember(ConfigProps::Dandelion::EMBARGO_SECS))
		{
			embargoSeconds = (uint16_t)dandelionRoot.get(ConfigProps::Dandelion::EMBARGO_SECS, 180).asUInt();
		}

		if (dandelionRoot.isMember(ConfigProps::Dandelion::PATIENCE_SECS))
		{
			patienceSeconds = (uint8_t)dandelionRoot.get(ConfigProps::Dandelion::PATIENCE_SECS, 10).asUInt();
		}

		if (dandelionRoot.isMember(ConfigProps::Dandelion::STEM_PROBABILITY))
		{
			stemProbability = (uint8_t)dandelionRoot.get(ConfigProps::Dandelion::STEM_PROBABILITY, 90).asUInt();
		}
	}

	return DandelionConfig(relaySeconds, embargoSeconds, patienceSeconds, stemProbability);
}