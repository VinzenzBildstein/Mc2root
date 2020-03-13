// g++ -o Mc2root Mc2root.cxx `root-config --cflags --libs`
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

#include "TFile.h"
#include "TTree.h"

int main(int argc, char** argv)
{
	if(argc != 3 && argc != 4) {
		std::cout<<"usage: "<<argv[0]<<" <input file> <output file> <option debug flag>"<<std::endl;
		return 1;
	}

	bool debug = false;
	if(argc == 4) debug = true;

	std::string inputFileName = argv[1];

	// open output root file
	TFile output(argv[2], "recreate");
	if(!output.IsOpen()) {
		std::cerr<<"Failed to open output root file "<<argv[2]<<std::endl;
	}

	// create tree
	TTree* tree = new TTree("dt5780","data from dt5780");

	// create and add leafs
	uint8_t protocolVersion;
	//tree->Branch("protocolVersion", &protocolVersion);
	uint8_t numHeaderWords;
	//tree->Branch("numHeaderWords", &numHeaderWords);
	std::vector<uint8_t> dataType;
	//tree->Branch("dataType", &dataType);
	std::vector<uint32_t> dataFormat;
	//tree->Branch("dataFormat", &dataFormat);
	ULong64_t timestamp;
	ULong64_t extras;
	ULong64_t energy;

	// open input file
	std::ifstream input(argv[1], std::ios::binary);

	// get size of input file
	input.seekg(0, input.end);
	size_t fileSize = input.tellg();
	input.seekg(0, input.beg);

	// create buffer and read file into it (might want to check how much memory we create here?)
	std::vector<uint8_t> buffer(fileSize);
	input.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	// loop over buffer
	size_t pos = 0;
	size_t entry = 0;
	uint64_t data = 0;
	// header
	if(debug) std::cout<<pos<<": protocol version - 0x"<<std::hex<<(static_cast<uint64_t>(buffer[pos])&0xff)<<std::dec<<std::endl;
	protocolVersion = buffer[pos++];
	if(debug) std::cout<<pos<<": number of header words - 0x"<<std::hex<<(static_cast<uint64_t>(buffer[pos])&0xff)<<std::dec<<std::endl;
	numHeaderWords = buffer[pos++];
	if(debug) std::cout<<"skipping two reserved bytes "<<(static_cast<uint64_t>(buffer[pos])&0xff)<<" - "<<(static_cast<uint64_t>(buffer[pos+1])&0xff)<<std::endl;
	pos += 2;
	if(numHeaderWords == 0) {
		std::cout<<"something went wrong, I got zero header words?"<<std::endl;
		return 1;
	}
	// read remaining header words with data type and format
	dataType.clear();
	dataFormat.clear();
	for(int h = 1; h < numHeaderWords; ++h) {
		if(debug) std::cout<<pos<<": data type - 0x"<<std::hex<<static_cast<uint64_t>(buffer[pos])<<std::dec<<std::endl;
		dataType.push_back(buffer[pos++]);
		dataFormat.push_back((static_cast<uint64_t>(buffer[pos+2])<<16)|(static_cast<uint64_t>(buffer[pos+1])<<8)|(buffer[pos]));
		if(debug) std::cout<<pos<<": data format - 0x"<<std::hex<<static_cast<uint64_t>(buffer[pos])<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+1])<<8)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+2])<<16)<<" = 0x"<<dataFormat.back()<<std::dec<<std::endl;
		pos += 3;
		switch(dataType.back()) {
				case 0:
					tree->Branch("timestamp", &timestamp);
					break;
				case 1:
					tree->Branch("energy", &energy);
					break;
				case 2:
					tree->Branch("extras", &extras);
					break;
				case 4:
					if((dataFormat.back()&0xff) != 0x80) {
						std::cerr<<"Unknown DPP code format "<<std::hex<<dataFormat.back()<<std::dec<<", was expecting 0x80!"<<std::endl;
						return 1;
					}
				default:
					break;
		};
	}
	while(pos < buffer.size()) {
		// loop over all data types and formats and read them
		for(size_t i = 0; i < dataType.size(); ++i) {
			switch(dataFormat[i]) {
				case 0:
				case 1:
					// read single byte
					data = buffer[pos];
					if(debug) std::cout<<pos<<": single byte from 0x"<<std::hex<<static_cast<uint64_t>(buffer[pos])<<" = 0x"<<data<<std::dec<<std::endl;
					pos += 1;
					break;
				case 2:
				case 3:
					// read two bytes
					data = (static_cast<uint64_t>(buffer[pos+1])<<8)|(buffer[pos]);
					if(debug) std::cout<<pos<<": two bytes from 0x"<<std::hex<<static_cast<uint64_t>(buffer[pos])<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+1])<<8)<<" = 0x"<<data<<std::dec<<std::endl;
					pos += 2;
					break;
				case 4:
				case 5:
					// read four bytes
					data = (static_cast<uint64_t>(buffer[pos+3])<<24)|(static_cast<uint64_t>(buffer[pos+2])<<16)|(static_cast<uint64_t>(buffer[pos+1])<<8)|(buffer[pos]);
					if(debug) std::cout<<pos<<": four bytes from 0x"<<std::hex<<static_cast<uint64_t>(buffer[pos])<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+1])<<8)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+2])<<16)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+3])<<24)<<" = 0x"<<data<<std::dec<<std::endl;
					pos += 4;
					break;
				case 6:
				case 7:
					// read eight bytes
					data = (static_cast<uint64_t>(buffer[pos+7])<<56)|(static_cast<uint64_t>(buffer[pos+6])<<48)|(static_cast<uint64_t>(buffer[pos+5])<<40)|(static_cast<uint64_t>(buffer[pos+4])<<32)|(static_cast<uint64_t>(buffer[pos+3])<<24)|(static_cast<uint64_t>(buffer[pos+2])<<16)|(static_cast<uint64_t>(buffer[pos+1])<<8)|(buffer[pos]);
					if(debug) std::cout<<pos<<": eight bytes from 0x"<<std::hex<<static_cast<uint64_t>(buffer[pos])<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+1])<<8)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+2])<<16)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+3])<<24)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+4])<<32)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+5])<<40)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+6])<<48)<<" + 0x"<<(static_cast<uint64_t>(buffer[pos+7])<<56)<<" = 0x"<<data<<std::dec<<std::endl;
					pos += 8;
					break;
				case 8:
					std::cout<<"no idea how to handle strings!"<<std::endl;
					break;
				case 9:
					std::cout<<"no idea how to handle longs!"<<std::endl;
					break;
				case 10:
					std::cout<<"no idea how to handle doubles!"<<std::endl;
					break;
				case 11:
					std::cout<<"no idea how to handle chars!"<<std::endl;
					break;
				default:
					break;
			};
			switch(dataType[i]) {
				case 0:
					timestamp = data;
					break;
				case 1:
					energy = data;
					break;
				case 2:
					extras = data;
					break;
				default:
					break;
			};
		} // end of data loop
		tree->Fill();
		++entry;
		if(entry%1000 == 0) {
			std::cout<<(100*pos)/buffer.size()<<"% done, "<<pos<<"/"<<buffer.size()<<" words read\r"<<std::flush;
		}
	} // end of buffer loop

	std::cout<<"100% done, "<<pos<<"/"<<buffer.size()<<" words read = "<<entry<<" entries"<<std::endl;

	tree->Write();
	output.Close();

	return 0;
}
