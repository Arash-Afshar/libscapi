#include "../../include/interactive_mid_protocols/CommitmentSchemePedersenHash.hpp"

void CmtPedersenHashDecommitmentMessage::initFromString(const string & s) {
	auto vec = explode(s, ':');
	assert(vec.size() == 2);
	r = make_shared<BigIntegerRandomValue>(biginteger(vec[0]));
	vector<byte> tmp(vec[1].begin(), vec[1].end());
	x = make_shared<vector<byte>>(tmp);
}

string CmtPedersenHashDecommitmentMessage::toString() {
	string output = (string) r->getR();
	const byte * xBytes = &((*x)[0]);
	output += ":";
	output += string(reinterpret_cast<char const*>(xBytes), x->size());
	return output;
};

/**
* This constructor receives as arguments an instance of a Dlog Group and an instance
* of a Cryptographic Hash such that they keep the condition that the size in bytes
* of the resulting hash is less than the size in bytes of the order of the DlogGroup.
* Otherwise, it throws IllegalArgumentException.
* An established channel has to be provided by the user of the class.
* @param channel an established channel obtained via the Communication Layer
* @param dlog
* @param hash
* @param random
* @throws IllegalArgumentException if the size in bytes of the resulting hash is bigger than the size in bytes of the order of the DlogGroup
* @throws SecurityLevelException if the Dlog Group is not DDH
* @throws InvalidDlogGroupException if the parameters of the group do not conform the type the group is supposed to be
* @throws CheatAttemptException if the commetter suspects that the receiver is trying to cheat.
* @throws IOException if there was a problem during the communication
* @throws ClassNotFoundException if there was a problem with the serialization mechanism.
*/
CmtPedersenHashCommitter::CmtPedersenHashCommitter(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<CryptographicHash> hash) : CmtPedersenCommitterCore(channel, dlog) {

	if (hash->getHashedMsgSize() > bytesCount(dlog->getOrder())) {
		throw invalid_argument("The size in bytes of the resulting hash is bigger than the size in bytes of the order of the DlogGroup.");
	}
	this->hash = hash;
}

/*
* Runs COMMIT_ElGamal to commit to value H(x).
* @return the created commitment.
*/
shared_ptr<CmtCCommitmentMsg> CmtPedersenHashCommitter::generateCommitmentMsg(shared_ptr<CmtCommitValue> input, long id) {
	auto in = dynamic_pointer_cast<CmtByteArrayCommitValue>(input);
	//Check that the input x is in the end a byte[]
	if (in == NULL)
		throw invalid_argument("The input must be of type CmtByteArrayCommitValue");
	//Hash the input x with the hash function
	auto x = *in->getXVector();
	
	
	//calculate H(x) = Hash(x)
	vector<byte> hashValArray;
	hash->update(x, 0, x.size());
	hash->hashFinal(hashValArray, 0);
	
	biginteger newInput = abs(decodeBigInteger(hashValArray.data(), hashValArray.size()));
	//After the input has been manipulated with the Hash call the super's commit function. 
	auto biCom = make_shared<CmtBigIntegerCommitValue>(make_shared<biginteger>(newInput));
	auto output = CmtPedersenCommitterCore::generateCommitmentMsg(biCom, id);
	auto values = commitmentMap[id];
	commitmentMap[id] = make_shared<CmtPedersenCommitmentPhaseValues>(values->getR(), input, static_pointer_cast<GroupElement>(values->getComputedCommitment()));
	return output;
}

shared_ptr<CmtCDecommitmentMessage> CmtPedersenHashCommitter::generateDecommitmentMsg(long id)  {

	//Fetch the commitment according to the requested ID
	auto tmp = dynamic_pointer_cast<CmtByteArrayCommitValue>(commitmentMap[id]->getX());
	auto x = tmp->getXVector();
	
	//Get the relevant random value used in the commitment phase
	auto r = dynamic_pointer_cast<BigIntegerRandomValue>(commitmentMap[id]->getR());

	return make_shared<CmtPedersenHashDecommitmentMessage>(r, x);
}

/**
* This function samples random commit value and returns it.
* @return the sampled commit value
*/
shared_ptr<CmtCommitValue> CmtPedersenHashCommitter::sampleRandomCommitValue()  {
	vector<byte> val;
	gen_random_bytes_vector(val, 32, random);
	return make_shared<CmtByteArrayCommitValue>(make_shared<vector<byte>>(val));
}

/**
* This function converts the given commit value to a byte array.
* @param value
* @return the generated bytes.
*/
vector<byte> CmtPedersenHashCommitter::generateBytesFromCommitValue(CmtCommitValue* value)  {
	auto in = dynamic_cast<CmtByteArrayCommitValue*>(value);
	if (in == NULL)
		throw invalid_argument("The given value must be of type CmtByteArrayCommitValue");
	return *in->getXVector();
}

/**
* This constructor receives as arguments an instance of a Dlog Group and an instance
* of a Cryptographic Hash such that they keep the condition that the size in bytes
* of the resulting hash is less than the size in bytes of the order of the DlogGroup.
* Otherwise, it throws IllegalArgumentException.
* An established channel has to be provided by the user of the class.
* @param channel an established channel obtained via the Communication Layer
* @param dlog
* @param hash
* @param random
* @throws IllegalArgumentException if the size in bytes of the resulting hash is bigger than the size in bytes of the order of the DlogGroup
* @throws SecurityLevelException if the Dlog Group is not DDH
* @throws InvalidDlogGroupException if the parameters of the group do not conform the type the group is supposed to be
* @throws IOException if there was a problem during the communication
*/
CmtPedersenHashReceiver::CmtPedersenHashReceiver(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<CryptographicHash> hash) : CmtPedersenReceiverCore(channel, dlog) {

	if (hash->getHashedMsgSize() > bytesCount(dlog->getOrder())) {
		throw invalid_argument("The size in bytes of the resulting hash is bigger than the size in bytes of the order of the DlogGroup.");
	}
	this->hash = hash;
}

shared_ptr<CmtCommitValue> CmtPedersenHashReceiver::receiveDecommitment(long id)  {
	vector<byte> raw_msg;
	channel->readWithSizeIntoVector(raw_msg);
	shared_ptr<CmtPedersenHashDecommitmentMessage> msg = make_shared<CmtPedersenHashDecommitmentMessage>();
	msg->initFromByteVector(raw_msg);
	auto receivedCommitment = commitmentMap[id];
	auto cmtCommitMsg = static_pointer_cast<CmtCCommitmentMsg>(receivedCommitment);
	return verifyDecommitment(cmtCommitMsg.get(), msg.get());
} 

/**
* Verifies that the commitment was to H(x).
*/
shared_ptr<CmtCommitValue> CmtPedersenHashReceiver::verifyDecommitment(CmtCCommitmentMsg* commitmentMsg, CmtCDecommitmentMessage* decommitmentMsg)  {
	auto decom = dynamic_cast<CmtPedersenHashDecommitmentMessage*>(decommitmentMsg);
	if (decom == NULL) {
		throw invalid_argument("The given decommiment message should be an instance of CmtPedersenDecommitmentMessage");
	}
	//Hash the input x with the hash function
	vector<byte> x = decom->getXValue();
	
	//calculate H(x) = Hash(x)
	vector<byte> hashValArray;
	hash->update(x, 0, x.size());
	hash->hashFinal(hashValArray, 0);
	
	biginteger xBi = abs(decodeBigInteger(hashValArray.data(), hashValArray.size()));
	auto r = static_pointer_cast<BigIntegerRandomValue>(decom->getR());
	auto innerDecom = make_shared<CmtPedersenDecommitmentMessage>(make_shared<biginteger>(xBi), r);
	auto val = CmtPedersenReceiverCore::verifyDecommitment(commitmentMsg, innerDecom.get());
	//If the inner Pedersen core algorithm returned null it means that it rejected the decommitment, so Pedersen Hash also rejects the answer and returns null
	if (val == NULL)
		return NULL;
	//The decommitment was accpeted by Pedersen core. Now, Pedersen Hash has to return the original value before the hashing.
	return make_shared<CmtByteArrayCommitValue>(make_shared<vector<byte>>(x));
}

/**
* This function converts the given commit value to a byte array.
* @param value
* @return the generated bytes.
*/
vector<byte> CmtPedersenHashReceiver::generateBytesFromCommitValue(CmtCommitValue* value) {
	auto val = dynamic_cast<CmtByteArrayCommitValue*>(value);
	if (val == NULL)
		throw invalid_argument("The given value must be of type CmtByteArrayCommitValue");
	return *val->getXVector();
}