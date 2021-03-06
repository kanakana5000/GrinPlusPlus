#include "BlockAPI.h"
#include "RestUtil.h"
#include "JSONFactory.h"

#include <StringUtil.h>
#include <Infrastructure/Logger.h>

//
// Handles requests to retrieve a single block by hash, height, or output commitment.
//
// APIs:
// GET /v1/blocks/<hash>
// GET /v1/blocks/<height>
// GET /v1/blocks/<output commit>
//
// Return results as "compact blocks" by passing "?compact" query
// GET /v1/blocks/<hash>?compact
int BlockAPI::GetBlock_Handler(struct mg_connection* conn, void* pBlockChainServer)
{
	const std::string requestedBlock = RestUtil::GetURIParam(conn, "/v1/blocks/");
	const std::string queryString = RestUtil::GetQueryString(conn);

	if (queryString == "compact")
	{
		const std::string response = "NOT IMPLEMENTED";
		return RestUtil::BuildBadRequestResponse(conn, response);

		// TODO: Implement
		//std::unique_ptr<CompactBlock> pCompactBlock = GetCompactBlock(requestedBlock, (IBlockChainServer*)pBlockChainServer);

		//if (nullptr != pCompactBlock)
		//{
		//	const std::string response = BuildCompactBlockJSON(*pCompactBlock);
		//	return RestUtil::BuildSuccessResponse(conn, response);
		//}
	}
	else
	{
		std::unique_ptr<FullBlock> pFullBlock = GetBlock(requestedBlock, (IBlockChainServer*)pBlockChainServer);

		if (nullptr != pFullBlock)
		{
			const Json::Value blockJSON = JSONFactory::BuildBlockJSON(*pFullBlock);
			return RestUtil::BuildSuccessResponse(conn, blockJSON.toStyledString());
		}
	}

	const std::string response = "BLOCK NOT FOUND";
	return RestUtil::BuildBadRequestResponse(conn, response);
}

std::unique_ptr<FullBlock> BlockAPI::GetBlock(const std::string& requestedBlock, IBlockChainServer* pBlockChainServer)
{
	if (requestedBlock.length() == 64 && HexUtil::IsValidHex(requestedBlock))
	{
		try
		{
			const Hash hash = Hash::FromHex(requestedBlock);
			std::unique_ptr<FullBlock> pBlock = pBlockChainServer->GetBlockByHash(hash);
			if (pBlock != nullptr)
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - Found block with hash %s.", requestedBlock.c_str()));
				return pBlock;
			}

			pBlock = pBlockChainServer->GetBlockByCommitment(hash);
			if (pBlock != nullptr)
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - Found block with output commitment %s.", requestedBlock.c_str()));
				return pBlock;
			}

			LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - No block found with hash or commitment %s.", requestedBlock.c_str()));
		}
		catch (const std::exception&)
		{
			LoggerAPI::LogError(StringUtil::Format("BlockAPI::GetBlock - Failed converting %s to a Hash.", requestedBlock.c_str()));
		}
	}
	else
	{
		try
		{
			std::string::size_type sz = 0;
			const uint64_t height = std::stoull(requestedBlock, &sz, 0);

			std::unique_ptr<FullBlock> pBlock = pBlockChainServer->GetBlockByHeight(height);
			if (pBlock != nullptr)
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - Found block at height %s.", requestedBlock.c_str()));
				return pBlock;
			}
			else
			{
				LoggerAPI::LogInfo(StringUtil::Format("BlockAPI::GetBlock - No block found at height %s.", requestedBlock.c_str()));
			}
		}
		catch (const std::invalid_argument&)
		{
			LoggerAPI::LogError(StringUtil::Format("BlockAPI::GetBlock - Failed converting %s to height.", requestedBlock.c_str()));
		}
	}

	return std::unique_ptr<FullBlock>(nullptr);
}