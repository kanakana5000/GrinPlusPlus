#include <Core/Transaction.h>

#include <Crypto.h>
#include <Serialization/Serializer.h>

Transaction::Transaction(BlindingFactor&& offset, TransactionBody&& transactionBody)
	: m_offset(std::move(offset)), m_transactionBody(std::move(transactionBody))
{

}

void Transaction::Serialize(Serializer& serializer) const
{
	// Serialize BlindingFactor/Offset
	const CBigInteger<32>& offsetBytes = m_offset.GetBlindingFactorBytes();
	serializer.AppendBigInteger(offsetBytes);

	// Serialize Transaction Body
	m_transactionBody.Serialize(serializer);
}

Transaction Transaction::Deserialize(ByteBuffer& byteBuffer)
{
	// Read BlindingFactor/Offset (32 bytes)
	BlindingFactor offset = BlindingFactor::Deserialize(byteBuffer);

	// Read Transaction Body (variable size)
	TransactionBody transactionBody = TransactionBody::Deserialize(byteBuffer);

	return Transaction(std::move(offset), std::move(transactionBody));
}

const Hash& Transaction::GetHash() const
{
	if (m_hash == Hash())
	{
		Serializer serializer;
		Serialize(serializer);

		m_hash = Crypto::Blake2b(serializer.GetBytes());
	}

	return m_hash;
}