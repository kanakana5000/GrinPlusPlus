#include "Pool.h"
#include "TransactionValidator.h"
#include "TransactionAggregator.h"

#include <VectorUtil.h>
#include <algorithm>

// Query the tx pool for all known txs based on kernel short_ids from the provided compact_block.
// Note: does not validate that we return the full set of required txs. The caller will need to validate that themselves.
std::vector<Transaction> Pool::GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);

	std::vector<Transaction> transactionsFound;
	for (const TxPoolEntry& txPoolEntry : m_transactions)
	{
		for (const TransactionKernel& kernel : txPoolEntry.GetTransaction().GetBody().GetKernels())
		{
			const ShortId shortId = ShortId::Create(kernel.GetHash(), hash, nonce);
			if (missingShortIds.find(shortId) != missingShortIds.cend())
			{
				transactionsFound.push_back(txPoolEntry.GetTransaction());

				if (transactionsFound.size() == missingShortIds.size())
				{
					return transactionsFound;
				}
			}
		}
	}

	return transactionsFound;
}

bool Pool::AddTransaction(const Transaction& transaction, const EDandelionStatus status)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	if (TransactionValidator().ValidateTransaction(transaction))
	{
		m_transactions.emplace_back(TxPoolEntry(transaction, status, std::time_t()));
		return true;
	}

	return false;
}

std::vector<Transaction> Pool::FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);

	std::set<Transaction> transactionSet;
	for (const TxPoolEntry& txPoolEntry : m_transactions)
	{
		for (const TransactionKernel& kernel : txPoolEntry.GetTransaction().GetBody().GetKernels())
		{
			if (kernels.count(kernel) > 0)
			{
				transactionSet.insert(txPoolEntry.GetTransaction());
			}
		}
	}

	std::vector<Transaction> transactions;
	std::copy(transactionSet.begin(), transactionSet.end(), std::back_inserter(transactions));

	return transactions;
}

std::vector<Transaction> Pool::FindTransactionsByStatus(const EDandelionStatus status) const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);

	std::vector<Transaction> transactions;
	for (const TxPoolEntry& txPoolEntry : m_transactions)
	{
		if (txPoolEntry.GetStatus() == status)
		{
			transactions.push_back(txPoolEntry.GetTransaction());
		}
	}

	return transactions;
}

std::vector<Transaction> Pool::GetExpiredTransactions(const uint16_t embargoSeconds) const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);

	const std::time_t cutoff = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::seconds(embargoSeconds));

	std::vector<Transaction> transactions;
	for (const TxPoolEntry& txPoolEntry : m_transactions)
	{
		if (txPoolEntry.GetTimestamp() < cutoff)
		{
			transactions.push_back(txPoolEntry.GetTransaction());
		}
	}

	return transactions;
}

void Pool::RemoveTransactions(const std::vector<Transaction>& transactions)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	auto iter = m_transactions.begin();
	while (iter != m_transactions.end())
	{
		bool remove = false;
		for (const Transaction& transaction : transactions)
		{
			if (transaction == iter->GetTransaction())
			{
				remove = true;
				break;
			}
		}

		if (remove)
		{
			m_transactions.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

// Quick reconciliation step - we can evict any txs in the pool where
// inputs or kernels intersect with the block.
void Pool::ReconcileBlock(const FullBlock& block)
{
	std::lock_guard<std::shared_mutex> lockGuard(m_transactionsMutex);

	// Filter txs in the pool based on the latest block.
	// Reject any txs where we see a matching tx kernel in the block.
	// Also reject any txs where we see a conflicting tx,
	// where an input is spent in a different tx.
	auto iter = m_transactions.begin();
	while (iter != m_transactions.end())
	{
		if (ShouldEvict_Locked(iter->GetTransaction(), block))
		{
			m_transactions.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

bool Pool::ShouldEvict_Locked(const Transaction& transaction, const FullBlock& block) const
{
	const std::vector<TransactionInput>& blockInputs = block.GetTransactionBody().GetInputs();
	for (const TransactionInput& input : transaction.GetBody().GetInputs())
	{
		if (std::find(blockInputs.begin(), blockInputs.end(), input) != blockInputs.end())
		{
			return true;
		}
	}

	const std::vector<TransactionKernel>& blockKernels = block.GetTransactionBody().GetKernels();
	for (const TransactionKernel& kernel : transaction.GetBody().GetKernels())
	{
		if (std::find(blockKernels.begin(), blockKernels.end(), kernel) != blockKernels.end())
		{
			return true;
		}
	}

	return false;
}

std::unique_ptr<Transaction> Pool::Aggregate() const
{
	std::shared_lock<std::shared_mutex> lockGuard(m_transactionsMutex);
	if (m_transactions.empty())
	{
		return std::unique_ptr<Transaction>(nullptr);
	}

	std::vector<Transaction> transactions;
	for (const TxPoolEntry& entry : m_transactions)
	{
		transactions.push_back(entry.GetTransaction());
	}

	std::unique_ptr<Transaction> pAggregateTransaction = TransactionAggregator::Aggregate(transactions);
	if (pAggregateTransaction != nullptr)
	{
		// TODO: tx.validate(self.verifier_cache.clone()) ? ;
	}

	return pAggregateTransaction;
}