#include "BlockProcessor.h"
#include "BlockHeaderProcessor.h"
#include "../Validators/BlockValidator.h"

#include <Consensus/BlockTime.h>
#include <Infrastructure/Logger.h>
#include <HeaderMMR.h>
#include <HexUtil.h>
#include <StringUtil.h>
#include <algorithm>

BlockProcessor::BlockProcessor(const Config& config, ChainState& chainState)
	: m_config(config), m_chainState(chainState)
{

}

EBlockChainStatus BlockProcessor::ProcessBlock(const FullBlock& block)
{	
	const uint64_t candidateHeight = m_chainState.GetHeight(EChainType::CANDIDATE);
	const uint64_t horizonHeight = std::max(candidateHeight, (uint64_t)Consensus::CUT_THROUGH_HORIZON) - Consensus::CUT_THROUGH_HORIZON;

	const BlockHeader& header = block.GetBlockHeader();
	uint64_t height = header.GetHeight();
	if (height <= horizonHeight)
	{
		LoggerAPI::LogError("BlockProcessor::ProcessBlock - Can't process blocks beyond horizon.");
		return EBlockChainStatus::INVALID;
	}

	// Make sure header is processed and valid before processing block.
	const EBlockChainStatus headerStatus = BlockHeaderProcessor(m_config, m_chainState).ProcessSingleHeader(header);
	if (headerStatus == EBlockChainStatus::SUCCESS
		|| headerStatus == EBlockChainStatus::ALREADY_EXISTS
		|| headerStatus == EBlockChainStatus::ORPHANED)
	{
		const EBlockChainStatus returnStatus = ProcessBlockInternal(block);
		if (returnStatus == EBlockChainStatus::SUCCESS)
		{
			LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s successfully processed. Checking for orphans now.", header.FormatHash().c_str()));
		}

		// 4. Check for subsequent orphans
		EBlockChainStatus status = returnStatus;
		while (status == EBlockChainStatus::SUCCESS)
		{
			std::unique_ptr<BlockHeader> pOrphanHeader = m_chainState.GetBlockHeaderByHeight(++height, EChainType::CANDIDATE);
			if (pOrphanHeader == nullptr)
			{
				break;
			}

			std::unique_ptr<FullBlock> pOrphanBlock = m_chainState.GetOrphanBlock(pOrphanHeader->GetHash());
			if (pOrphanBlock == nullptr)
			{
				break;
			}

			LoggerAPI::LogDebug(StringUtil::Format("BlockProcessor::ProcessBlock - Orphan block %s found. Processing now.", pOrphanHeader->FormatHash().c_str()));
			status = ProcessBlockInternal(*pOrphanBlock);
		}

		return returnStatus;
	}

	return headerStatus;
}

EBlockChainStatus BlockProcessor::ProcessBlockInternal(const FullBlock& block)
{
	LockedChainState lockedState = m_chainState.GetLocked();
	Chain& confirmedChain = lockedState.m_chainStore.GetConfirmedChain();
	const BlockHeader& header = block.GetBlockHeader();

	// 1. Check if already part of confirmed chain
	BlockIndex* pConfirmedIndex = confirmedChain.GetByHeight(header.GetHeight());
	if (pConfirmedIndex != nullptr && pConfirmedIndex->GetHash() == header.GetHash())
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s already part of confirmed chain.", header.FormatHash().c_str()));
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// 2. Orphan if block should be processed as an orphan
	if (ShouldOrphan(block, lockedState))
	{
		return ProcessOrphanBlock(block, lockedState);
	}

	// 3. Fully process block
	return ProcessNextBlock(block, lockedState);
}

EBlockChainStatus BlockProcessor::ProcessNextBlock(const FullBlock& block, LockedChainState& lockedState)
{
	lockedState.m_orphanPool.RemoveOrphan(block.GetHash());

	Chain& confirmedChain = lockedState.m_chainStore.GetConfirmedChain();
	confirmedChain.Rewind(block.GetBlockHeader().GetHeight() - 1);

	std::unique_ptr<BlockHeader> pPreviousHeader = lockedState.m_blockStore.GetBlockHeaderByHash(block.GetBlockHeader().GetPreviousBlockHash());
	if (pPreviousHeader == nullptr)
	{
		return EBlockChainStatus::STORE_ERROR;
	}

	ITxHashSet* pTxHashSet = lockedState.m_txHashSetManager.GetTxHashSet();
	pTxHashSet->Rewind(*pPreviousHeader);

	pTxHashSet->ApplyBlock(block);
	if (!BlockValidator(lockedState.m_transactionPool, pTxHashSet).IsBlockValid(block, pPreviousHeader->GetTotalKernelOffset()))
	{
		pTxHashSet->Discard();
		return EBlockChainStatus::INVALID;
	}

	lockedState.m_blockStore.AddBlock(block);

	//pTxHashSet->Commit();

	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();
	confirmedChain.AddBlock(candidateChain.GetByHeight(block.GetBlockHeader().GetHeight()));

	return EBlockChainStatus::SUCCESS;
}

EBlockChainStatus BlockProcessor::ProcessOrphanBlock(const FullBlock& block, LockedChainState& lockedState)
{
	// Check if already processed as an orphan
	if (lockedState.m_orphanPool.IsOrphan(block.GetHash()))
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Block %s already processed as an orphan.", block.GetBlockHeader().FormatHash().c_str()));
		return EBlockChainStatus::ALREADY_EXISTS;
	}

	// TODO: Finish implementing. Should be able to do some validation.

	lockedState.m_orphanPool.AddOrphanBlock(block);

	return EBlockChainStatus::ORPHANED;
}

bool BlockProcessor::ShouldOrphan(const FullBlock& block, LockedChainState& lockedState)
{
	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();

	// Orphan if previous block not a part of candidate chain.
	const BlockHeader& header = block.GetBlockHeader();
	BlockIndex* pPreviousCandidateIndex = candidateChain.GetByHeight(header.GetHeight() - 1);
	if (pPreviousCandidateIndex == nullptr || pPreviousCandidateIndex->GetHash() != header.GetPreviousBlockHash())
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Previous block header missing. Treating %s as orphan.", header.FormatHash().c_str()));

		return true;
	}

	// Orphan if block not a part of candidate chain.
	BlockIndex* pCandidateIndex = candidateChain.GetByHeight(header.GetHeight());
	if (nullptr == pCandidateIndex || pCandidateIndex->GetHash() != header.GetHash())
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Candidate block mismatch. Treating %s as orphan.", header.FormatHash().c_str()));

		return true;
	}

	Chain& confirmedChain = lockedState.m_chainStore.GetConfirmedChain();

	// Orphan if previous block not a part of confirmed chain.
	BlockIndex* pPreviousConfirmedIndex = confirmedChain.GetByHeight(header.GetHeight() - 1);
	if (pPreviousConfirmedIndex == nullptr || pPreviousConfirmedIndex->GetHash() != header.GetPreviousBlockHash())
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Previous confirmed block missing. Treating %s as orphan.", header.FormatHash().c_str()));

		return true;
	}

	// Orphan if different block a part of confirmed chain.
	BlockIndex* pConfirmedIndex = confirmedChain.GetByHeight(header.GetHeight());
	if (nullptr != pConfirmedIndex && pConfirmedIndex->GetHash() != header.GetHash())
	{
		LoggerAPI::LogInfo(StringUtil::Format("BlockProcessor::ProcessBlock - Confirmed block mismatch. Treating %s as orphan.", header.FormatHash().c_str()));

		return true;
	}

	return false;
}