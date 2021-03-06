#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <TxPool/DandelionStatus.h>
#include <TxPool/PoolType.h>
#include <Core/Transaction.h>
#include <Core/ShortId.h>
#include <Core/FullBlock.h>
#include <Core/BlockHeader.h>
#include <Core/BlockSums.h>
#include <Config/Config.h>
#include <PMMR/TxHashSetManager.h>
#include <Hash.h>
#include <vector>
#include <set>

// Forward Declarations
class IBlockDB;

#ifdef MW_TX_POOL
#define TX_POOL_API EXPORT
#else
#define TX_POOL_API IMPORT
#endif

class ITransactionPool
{
public:
	virtual std::vector<Transaction> GetTransactionsByShortId(const Hash& hash, const uint64_t nonce, const std::set<ShortId>& missingShortIds) const = 0;
	virtual bool AddTransaction(const Transaction& transaction, const EPoolType poolType, const BlockHeader& lastConfirmedBlock) = 0;
	virtual std::vector<Transaction> FindTransactionsByKernel(const std::set<TransactionKernel>& kernels) const = 0;
	//virtual std::vector<Transaction> FindTransactionsByStatus(const EDandelionStatus status, const EPoolType poolType) const = 0;
	virtual void ReconcileBlock(const FullBlock& block) = 0;

	// Dandelion
	virtual std::unique_ptr<Transaction> GetTransactionToStem(const BlockHeader& lastConfirmedHeader) = 0;
	virtual std::unique_ptr<Transaction> GetTransactionToFluff(const BlockHeader& lastConfirmedHeader) = 0;
	virtual std::vector<Transaction> GetExpiredTransactions() const = 0;

	virtual bool ValidateTransaction(const Transaction& transaction) const = 0;
	virtual bool ValidateTransactionBody(const TransactionBody& transactionBody, const bool withReward) const = 0;

	// TODO: Prepare Mineable Transactions
};

namespace TxPoolAPI
{
	//
	// Creates a new instance of the Transaction Pool.
	//
	TX_POOL_API ITransactionPool* CreateTransactionPool(const Config& config, const TxHashSetManager& txHashSetManager, const IBlockDB& blockDB);

	//
	// Destroys the instance of the Transaction Pool
	//
	TX_POOL_API void DestroyTransactionPool(ITransactionPool* pTransactionPool);
}