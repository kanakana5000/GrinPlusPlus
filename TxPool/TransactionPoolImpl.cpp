#include "TransactionPoolImpl.h"
#include "TransactionValidator.h"
#include "TransactionBodyValidator.h"
#include "TransactionAggregator.h"
#include "ValidTransactionFinder.h"

#include <Database/BlockDb.h>
#include <Crypto/RandomNumberGenerator.h>

TransactionPool::TransactionPool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB)
	: m_config(config), m_txHashSetManager(txHashSetManager), m_blockDB(blockDB)
{

}

// Query the tx pool for all known txs based on kernel short_ids from the provided compact_block.
// Note: does not validate that we return the full set of required txs. The caller will need to validate that themselves.
std::vector<Transaction> TransactionPool::GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	return m_memPool.GetTransactionsByShortId(hash, nonce, missingShortIds);
}

bool TransactionPool::AddTransaction(const Transaction& transaction, const EPoolType poolType, const BlockHeader& lastConfirmedBlock)
{
	if (poolType == EPoolType::MEMPOOL)
	{
		// TODO: Load BlockSums
		const bool added = m_memPool.AddTransaction(transaction, EDandelionStatus::FLUFFED);
		if (added)
		{
			m_stemPool.RemoveTransactions(std::vector<Transaction>({ transaction }));
			return true;
		}
	}
	else if (poolType == EPoolType::STEMPOOL)
	{
		const uint8_t random = (uint8_t)RandomNumberGenerator::GeneratePseudoRandomNumber(0, 100);
		if (random <= m_config.GetDandelionConfig().GetStemProbability())
		{
			return m_stemPool.AddTransaction(transaction, EDandelionStatus::TO_STEM);
		}
		else
		{
			return m_stemPool.AddTransaction(transaction, EDandelionStatus::TO_FLUFF);
		}
	}

	return false;
}

std::vector<Transaction> TransactionPool::FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const
{
	return m_memPool.FindTransactionsByKernel(kernels);
}

void TransactionPool::ReconcileBlock(const FullBlock& block)
{
	// TODO: Finish implementing
	// First reconcile the txpool.
	m_memPool.ReconcileBlock(block);
	//self.txpool.reconcile(None, &block.header) ? ;

	// Now reconcile our stempool, accounting for the updated txpool txs.
	m_stemPool.ReconcileBlock(block);
	//let txpool_tx = self.txpool.aggregate_transaction() ? ;
	//self.stempool.reconcile(txpool_tx, &block.header) ? ;
}

std::unique_ptr<Transaction> TransactionPool::GetTransactionToStem(const BlockHeader& lastConfirmedBlock)
{
	const std::vector<Transaction> transactionsToStem = m_stemPool.FindTransactionsByStatus(EDandelionStatus::TO_STEM);
	if (transactionsToStem.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const std::unique_ptr<Transaction> pMemPoolAggTx = m_memPool.Aggregate();

	std::vector<Transaction> validTransactionsToStem = ValidTransactionFinder(m_txHashSetManager, m_blockDB).FindValidTransactions(transactionsToStem, pMemPoolAggTx, lastConfirmedBlock);
	if (validTransactionsToStem.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	// TODO: tx_pool.stempool.transition_to_state(&stem_txs, PoolEntryState::Stemmed);

	std::unique_ptr<Transaction> pTransactionToStem = TransactionAggregator::Aggregate(validTransactionsToStem);
	if (pTransactionToStem != nullptr)
	{
		// TODO: agg_tx.validate(verifier_cache.clone()) ? ;
	}

	return pTransactionToStem;
}

std::unique_ptr<Transaction> TransactionPool::GetTransactionToFluff(const BlockHeader& lastConfirmedBlock)
{
	const std::vector<Transaction> transactionsToFluff = m_stemPool.FindTransactionsByStatus(EDandelionStatus::TO_FLUFF);
	if (transactionsToFluff.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	const std::unique_ptr<Transaction> pMemPoolAggTx = m_memPool.Aggregate();

	std::vector<Transaction> validTransactionsToFluff = ValidTransactionFinder(m_txHashSetManager, m_blockDB).FindValidTransactions(transactionsToFluff, pMemPoolAggTx, lastConfirmedBlock);
	if (validTransactionsToFluff.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	// TODO: tx_pool.stempool.transition_to_state(&stem_txs, PoolEntryState::Fluffed);

	std::unique_ptr<Transaction> pTransactionToFluff = TransactionAggregator::Aggregate(validTransactionsToFluff);
	if (pTransactionToFluff != nullptr)
	{
		// TODO: agg_tx.validate(verifier_cache.clone()) ? ;
	}

	return pTransactionToFluff;
}

std::vector<Transaction> TransactionPool::GetExpiredTransactions() const
{
	const uint16_t embargoSeconds = m_config.GetDandelionConfig().GetEmbargoSeconds() + (uint16_t)RandomNumberGenerator::GeneratePseudoRandomNumber(0, 30);
	return m_stemPool.GetExpiredTransactions(embargoSeconds);
}

bool TransactionPool::ValidateTransaction(const Transaction& transaction) const
{
	return TransactionValidator().ValidateTransaction(transaction);
}

bool TransactionPool::ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward) const
{
	return TransactionBodyValidator().ValidateTransactionBody(transactionBody, withReward);
}

namespace TxPoolAPI
{
	TX_POOL_API ITransactionPool* CreateTransactionPool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB)
	{
		return new TransactionPool(config, txHashSetManager, blockDB);
	}

	TX_POOL_API void DestroyTransactionPool(ITransactionPool* pTransactionPool)
	{
		delete ((TransactionPool*)pTransactionPool);
	}
}