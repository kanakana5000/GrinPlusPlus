#include "JSONFactory.h"

#include <HexUtil.h>
#include <TimeUtil.h>

Json::Value JSONFactory::BuildBlockJSON(const FullBlock& block)
{
	Json::Value blockNode;
	blockNode["header"] = BuildHeaderJSON(block.GetBlockHeader());
	blockNode["Body"] = BuildTransactionBodyJSON(block.GetTransactionBody());
	return blockNode;
}

Json::Value JSONFactory::BuildHeaderJSON(const BlockHeader& header)
{
	Json::Value headerNode;
	headerNode["Height"] = header.GetHeight();
	headerNode["Hash"] = HexUtil::ConvertToHex(header.GetHash().GetData(), false, false);
	headerNode["Version"] = header.GetVersion();

	headerNode["TimestampRaw"] = header.GetTimestamp();
	headerNode["TimestampLocal"] = TimeUtil::FormatLocal(header.GetTimestamp());
	headerNode["TimestampUTC"] = TimeUtil::FormatUTC(header.GetTimestamp());

	headerNode["PreviousHash"] = HexUtil::ConvertToHex(header.GetPreviousBlockHash().GetData(), false, false);
	headerNode["PreviousMMRRoot"] = HexUtil::ConvertToHex(header.GetPreviousRoot().GetData(), false, false);

	headerNode["KernelMMRRoot"] = HexUtil::ConvertToHex(header.GetKernelRoot().GetData(), false, false);
	headerNode["KernelMMRSize"] = header.GetKernelMMRSize();
	headerNode["TotalKernelOffset"] = HexUtil::ConvertToHex(header.GetTotalKernelOffset().GetBlindingFactorBytes().GetData(), false, false);

	headerNode["OutputMMRRoot"] = HexUtil::ConvertToHex(header.GetOutputRoot().GetData(), false, false);
	headerNode["OutputMMRSIze"] = header.GetOutputMMRSize();
	headerNode["RangeProofMMRRoot"] = HexUtil::ConvertToHex(header.GetRangeProofRoot().GetData(), false, false);

	headerNode["ScalingDifficulty"] = header.GetScalingDifficulty();
	headerNode["TotalDifficulty"] = header.GetTotalDifficulty();
	headerNode["Nonce"] = header.GetNonce();

	return headerNode;
}

Json::Value JSONFactory::BuildTransactionBodyJSON(const TransactionBody& body)
{
	Json::Value transactionBodyNode;

	// Transaction Inputs
	Json::Value inputsNode;
	for (const TransactionInput& input : body.GetInputs())
	{
		inputsNode.append(BuildTransactionInputJSON(input));
	}
	transactionBodyNode["inputs"] = inputsNode;

	// Transaction Outputs
	Json::Value outputsNode;
	for (const TransactionOutput& output : body.GetOutputs())
	{
		outputsNode.append(BuildTransactionOutputJSON(output));
	}
	transactionBodyNode["outputs"] = outputsNode;

	// Transaction Kernels
	Json::Value kernelsNode;
	for (const TransactionKernel& kernel : body.GetKernels())
	{
		kernelsNode.append(BuildTransactionKernelJSON(kernel));
	}
	transactionBodyNode["kernels"] = kernelsNode;

	return transactionBodyNode;
}

Json::Value JSONFactory::BuildTransactionInputJSON(const TransactionInput& input)
{
	Json::Value inputNode;

	// TODO: Implement

	return inputNode;
}

Json::Value JSONFactory::BuildTransactionOutputJSON(const TransactionOutput& output)
{
	Json::Value outputNode;

	// TODO: Implement

	return outputNode;
}

Json::Value JSONFactory::BuildTransactionKernelJSON(const TransactionKernel& kernel)
{
	Json::Value kernelNode;

	// TODO: Implement

	return kernelNode;
}