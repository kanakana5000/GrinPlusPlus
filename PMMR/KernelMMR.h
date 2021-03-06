#pragma once

#include "Common/MMR.h"
#include "Common/LeafSet.h"
#include "Common/HashFile.h"
#include "Common/DataFile.h"

#include <Core/TransactionKernel.h>
#include <Hash.h>
#include <Config/Config.h>
#include <stdint.h>

#define KERNEL_SIZE 114

class KernelMMR : public MMR
{
public:
	static KernelMMR* Load(const Config& config);

	std::unique_ptr<TransactionKernel> GetKernelAt(const uint64_t mmrIndex) const;

	virtual Hash Root(const uint64_t lastMMRIndex) const override final;
	virtual uint64_t GetSize() const override final { return m_hashFile.GetSize(); }
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final { return std::make_unique<Hash>(m_hashFile.GetHashAt(mmrIndex)); }

	virtual bool Rewind(const uint64_t lastMMRIndex) override final;
	virtual bool Flush() override final;
	//virtual bool Discard() override final;

	bool ApplyKernel(const TransactionKernel& kernel);

private:
	KernelMMR(const Config& config, HashFile&& hashFile, LeafSet&& leafSet, DataFile<KERNEL_SIZE>&& dataFile);

	Hash KernelMMR::HashWithIndex(const TransactionKernel& kernel, const uint64_t index) const;

	const Config& m_config;
	HashFile m_hashFile;
	LeafSet m_leafSet;
	DataFile<KERNEL_SIZE> m_dataFile;
};