//
// Created by liork on 17/09/17.
//

/**
* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
* Copyright (c) 2016 LIBSCAPI (http://crypto.biu.ac.il/SCAPI)
* This file is part of the SCAPI project.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* We request that any publication and/or code referring to and/or based on SCAPI contain an appropriate citation to SCAPI, including a reference to
* http://crypto.biu.ac.il/SCAPI.
*
* Libscapi uses several open source libraries. Please see these projects for any further licensing issues.
* For more information , See https://github.com/cryptobiu/libscapi/blob/master/LICENSE.MD
*
* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*
*/

#include "../../include/infra/Measurement.hpp"
#include "../../include/cryptoInfra/Protocol.hpp"


using namespace std;


Measurement::Measurement(Protocol &protocol)
{
    init(protocol);
}

Measurement::Measurement(Protocol &protocol, vector<string> names)
{
    init(protocol);
    init(names);
}

Measurement::Measurement(string protocolName, int internalIterationsNumber, int partyId, int partiesNumber,
                         string partiesFile)
{
    init(protocolName, internalIterationsNumber, partyId, partiesNumber, partiesFile);
}

Measurement::Measurement(string protocolName, int internalIterationsNumber, int partyId, int partiesNumber,
                         string partiesFile, vector<string> names)
{
    init(protocolName, internalIterationsNumber, partyId, partiesNumber, partiesFile);
    init(names);
}

void Measurement::setTaskNames(vector<string> & names)
{
    init(names);
}

void Measurement::init(Protocol &protocol)
{
    m_arguments = protocol.getArguments();
    CmdParser parser = protocol.getParser();
    m_protocolName = parser.getValueByKey(m_arguments, "protocolName");
    m_numberOfIterations = stoi(parser.getValueByKey(m_arguments,"internalIterationsNumber"));
    string partyId = parser.getValueByKey(m_arguments, "partyID");
    if(partyId.compare("NotFound") != 0)
    {
        m_partyId =  stoi(partyId);
    }

    string partiesFile = parser.getValueByKey(m_arguments, "partiesFile");
    setCommInterface(partiesFile);
    m_numOfParties = atoi(parser.getValueByKey(m_arguments, "numParties").c_str());
}

void Measurement::init(string protocolName, int internalIterationsNumber, int partyId, int partiesNumber,
                       string partiesFile)
{
    m_protocolName = protocolName;
    m_numberOfIterations = internalIterationsNumber;
    m_partyId = partyId;
    m_numOfParties = partiesNumber;
    setCommInterface(partiesFile);
}

void Measurement::init(vector <string> names)
{
    m_cpuStartTimes = new vector<vector<long>>(names.size(), vector<long>(m_numberOfIterations));
    m_commSentStartTimes = new vector<vector<unsigned long int>>(names.size(),
            vector<unsigned long int>(m_numberOfIterations, 0));
    m_commReceivedStartTimes = new vector<vector<unsigned long int>>(names.size(),
            vector<unsigned long int>(m_numberOfIterations, 0));
    m_memoryUsage = new vector<vector<long>>(names.size(), vector<long>(m_numberOfIterations, 0));
    m_cpuEndTimes = new vector<vector<long>>(names.size(), vector<long>(m_numberOfIterations, 0));
    m_commSentEndTimes = new vector<vector<unsigned long int>>(names.size(),
            vector<unsigned long int>(m_numberOfIterations, 0));
    m_commReceivedEndTimes = new vector<vector<unsigned long int>>(names.size(),
            vector<unsigned long int>(m_numberOfIterations, 0));
    m_names = move(names);
}



int Measurement::getTaskIdx(string name)
{
    auto it = std::find(m_names.begin(), m_names.end(), name);
    auto idx = distance(m_names.begin(), it);
    return idx;
}

void Measurement::setCommInterface(string partiesFile)
{
    static const string ipPattern = "party_0_ip";
    //define addresses prefixes
    static const string localPrefix = "127.0";
    static const string awsPrefix = "172.0";
    static const string serversPrefix = "10.0";

    std::string ip;
    FILE * pf = fopen(partiesFile.c_str(), "r");
    if(NULL != pf)
    {
        char buffer[64];
        if(NULL != fgets(buffer, 64, pf))
        {
            char * sc = strstr(buffer, ":");
            if(NULL != sc)
            {
                ip.assign(buffer, sc);
            }
            else if(0 == strncmp(buffer, ipPattern.c_str(), ipPattern.size()))
            {
                //old format
                ip = buffer;
                std::string::size_type x = ip.find('=');
                if(std::string::npos != x)
                {
                    ip.erase(0, x + 1);
                }
            }
            if(ip.empty())
                cerr << "Unrecognized format" << endl;
        }
        fclose(pf);
    }
    else
        cerr << "Unable to open file, exit" << endl;

    if(ip.find(localPrefix) != string::npos)
        m_interface = "lo";
    else if (ip.find(serversPrefix) != string::npos)
        m_interface = "eth0";
    else
        m_interface = "ens3";
}

void Measurement::writeValue(unsigned long int value)
{
    (*m_commSentEndTimes)[0][0] += value;
    (*m_commReceivedEndTimes)[0][0] += value;
}

void Measurement::startSubTask(string taskName, int currentIterationNum)
{
    //calculate cpu start time
    auto now = system_clock::now();

    //Cast the time point to ms, then get its duration, then get the duration's count.
    auto ms = time_point_cast<milliseconds>(now).time_since_epoch().count();

    int taskIdx = getTaskIdx(taskName);

    (*m_cpuStartTimes)[taskIdx][currentIterationNum] = ms;
    tuple<unsigned long int, unsigned long int> startData = commData(m_interface.c_str());
    (*m_commSentStartTimes)[taskIdx][currentIterationNum] = get<0>(startData);
    (*m_commReceivedStartTimes)[taskIdx][currentIterationNum] = get<1>(startData);
}

void Measurement::endSubTask(string taskName, int currentIterationNum)
{
    int taskIdx = getTaskIdx(taskName);
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);
    (*m_memoryUsage)[taskIdx][currentIterationNum] = r_usage.ru_maxrss;

    auto now = system_clock::now();
    //Cast the time point to ms, then get its duration, then get the duration's count.
    auto ms = time_point_cast<milliseconds>(now).time_since_epoch().count();
    (*m_cpuEndTimes)[taskIdx][currentIterationNum] = ms - (*m_cpuStartTimes)[taskIdx][currentIterationNum];

//    tuple<unsigned long int, unsigned long int> endData = commData(m_interface.c_str());
//    (*m_commSentEndTimes)[taskIdx][currentIterationNum] = get<0>(endData) -
//            (*m_commSentStartTimes)[taskIdx][currentIterationNum];
//    (*m_commReceivedEndTimes)[taskIdx][currentIterationNum] = get<1>(endData) -
//            (*m_commReceivedStartTimes)[taskIdx][currentIterationNum];
}

tuple<unsigned long int, unsigned long int> Measurement::commData(const char * nic_)
{
	unsigned long rbytes = 0, tbytes = 0;
	std::string nic = nic_;
	FILE * pf = fopen("/proc/net/dev", "r");
	if(NULL != pf)
	{
		char sz[2048];
		while(NULL != fgets(sz, 2048, pf))
		{
			std::string line = sz;
            while(isspace(line[0])) line.erase(0, 1);
			std::string::size_type i = line.find(nic);
			if(std::string::npos != i)
			{
				line = line.substr(nic.size() + 1); //+1 for colon ':'
				for(int j = 0; j < 9; j++)
				{
					while(isspace(line[0])) line.erase(0, 1);
					i = line.find_first_of(" \f\n\r\t\v");
					if(std::string::npos == i) break;

					if(j == 0) //rbytes
					{
						rbytes = strtol(line.substr(0, i).c_str(), NULL, 10);
					}
					else if(j == 8)//tbytes
					{
						tbytes = strtol(line.substr(0, i).c_str(), NULL, 10);
					}
					line = line.substr(i);
				}
                break;
			}
		}
		fclose(pf);
	}
	return make_tuple(tbytes, rbytes);
}


void Measurement::analyze(string type)
{
    string filePath = getcwdStr();
    string fileName = filePath + "/" + m_protocolName + "*" + type;

    for (int idx = 1; idx< m_arguments.size(); idx++)
    {
        fileName += "*" + m_arguments[idx].second;

    }
    fileName += ".json";

    //party is the root of the json objects
    json party = json::array();

    for (int taskNameIdx = 0; taskNameIdx < m_names.size(); taskNameIdx++)
    {
        //Write for each task name all the iteration
        json task = json::object();
        task["name"] = m_names[taskNameIdx];

        for (int iterationIdx = 0; iterationIdx < m_numberOfIterations; iterationIdx++)
        {
            if(type.compare("cpu") == 0)
                task["iteration_" + to_string(iterationIdx)] = (*m_cpuEndTimes)[taskNameIdx][iterationIdx];
            else if (type.compare("commSent") == 0)
                task["iteration_" + to_string(iterationIdx)] = (*m_commSentEndTimes)[taskNameIdx][iterationIdx];
            else if (type.compare("commReceived") == 0)
                task["iteration_" + to_string(iterationIdx)] = (*m_commReceivedStartTimes)[taskNameIdx][iterationIdx];
            else if (type.compare("memory") == 0)
                task["iteration_" + to_string(iterationIdx)] = (*m_memoryUsage)[taskNameIdx][iterationIdx];
        }
        party.insert(party.begin(), task);
    }

    //send json object to create file
    createJsonFile(party, fileName);
}


void Measurement::analyzeCpuData()
{
    analyze("cpu");
}

void Measurement::analyzeCommSentData()
{
    analyze("commSent");
}

void Measurement::analyzeCommReceivedData()
{
    analyze("commReceived");
}

void Measurement::analyzeMemory()
{
    analyze("memory");
}

void Measurement::createJsonFile(json j, string fileName)
{
    try
    {
        ofstream myfile (fileName, ostream::out);
        myfile << j;
    }

    catch (exception& e)
    {
        cout << "Exception thrown : " << e.what() << endl;
    }
}


Measurement::~Measurement()
{
    analyzeCpuData();
    analyzeCommSentData();
    analyzeCommReceivedData();
    analyzeMemory();
    delete m_cpuStartTimes;
    delete m_commSentStartTimes;
    delete m_commReceivedStartTimes;
    delete m_memoryUsage;
    delete m_cpuEndTimes;
    delete m_commSentEndTimes;
    delete m_commReceivedEndTimes;
}

