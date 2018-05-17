#pragma once

#include <libscapi/include/interactive_mid_protocols/CommitmentScheme.hpp>
#include "../../include/common/CommonMaliciousYao.hpp"

/**
* This class used to hold some data of the receiver in the difference protocol. 
* It is written to a file in the end of the offline phase and read again in the beginning of the online phase. 
*
* This bundle holds the wCommitment which is the sigma array generated by the receiver, the decommitment of this w
* and the commitment pairs of a committed value.
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University 
*
*/
class DifferenceCommitmentReceiverBundle 
{
private:
	shared_ptr<vector<byte>> w;											//The sigma array generated by the receiver.
	shared_ptr<CmtCDecommitmentMessage> decomW;							//The decommitment of the above w.
	vector<vector<byte>> cParts;			//The commitment pairs of a committed value. 

public:
	DifferenceCommitmentReceiverBundle(){}

	/**
	* A constructor that sets the parameters.
	* @param w The sigma array generated by the receiver.
	* @param decomW The decommitment of the above w.
	* @param cParts The commitment pairs of a committed value.
	*/
	DifferenceCommitmentReceiverBundle(shared_ptr<vector<byte>> w, shared_ptr<CmtCDecommitmentMessage> decomW,	vector<vector<byte>> & cParts)
	{
		this->w = w;
		this->decomW = decomW;
		this->cParts = cParts;
	}

	/*
	* Get function for each class member.
	*/

	shared_ptr<vector<byte>> getW() { return this->w; }

	vector<vector<byte>> getC() { return this->cParts; }

	shared_ptr<CmtCDecommitmentMessage> getDecommitmentToW() { return this->decomW; }

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & w;
		ar & decomW;
		ar & cParts;
	}
};