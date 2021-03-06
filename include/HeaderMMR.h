#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <Hash.h>
#include <vector>

#ifdef MW_PMMR
#define PMMR_API __declspec(dllexport)
#else
#define PMMR_API __declspec(dllimport)
#endif

// Forward Declarations
class Config;
class BlockHeader;

class IHeaderMMR
{
public:
	virtual void AddHeader(const BlockHeader& header) = 0;
	virtual Hash Root(const uint64_t nextHeight) const = 0;

	virtual bool Rewind(const uint64_t nextHeight) = 0;
	virtual bool Rollback() = 0;
	virtual bool Commit() = 0;
};

namespace HeaderMMRAPI
{
	PMMR_API IHeaderMMR* OpenHeaderMMR(const Config& config);
	PMMR_API void CloseHeaderMMR(IHeaderMMR* pHeaderMMR);
}