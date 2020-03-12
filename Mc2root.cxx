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
	tree->Branch("protocolVersion", &protocolVersion);
	uint8_t numHeaderWords;
	tree->Branch("numHeaderWords", &numHeaderWords);
	std::vector<uint8_t> dataType;
	tree->Branch("dataType", &dataType);
	std::vector<uint32_t> dataFormat;
	tree->Branch("dataFormat", &dataFormat);
	ULong64_t timestamp;
	tree->Branch("timestamp", &timestamp);
	ULong64_t extras;
	tree->Branch("extras", &extras);
	ULong64_t energy;
	tree->Branch("energy", &energy);

	// open input file
	std::ifstream input(argv[1], std::ios::binary);

	// get size of input file
	input.seekg(0, input.end);
	size_t fileSize = input.tellg();
	input.seekg(0, input.beg);

	// create buffer and read file into it (might want to check how much memory we create here?)
	std::vector<uint32_t> buffer(fileSize/sizeof(uint32_t));
	input.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	// loop over buffer
	size_t pos;
	size_t entry = 0;
	uint64_t data = 0;
	uint32_t mask = 0;
	int byte = 0;
	for(pos = 0; pos < buffer.size();) {
		// header
		if(debug) std::cout<<pos<<": header - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
		protocolVersion = buffer[pos]&0xff;
		numHeaderWords = (buffer[pos]>>8)&0xff; // no increment needed here!
		if(debug) std::cout<<"=> protocol "<<static_cast<int>(protocolVersion)<<" - "<<static_cast<int>(numHeaderWords)<<" words in header"<<std::endl;
		if(numHeaderWords == 0) {
			std::cout<<"something went wrong, I got zero header words?"<<std::endl;
			++pos;
			continue;
		}
		// read remaining header words with data type and format
		dataType.clear();
		dataFormat.clear();
		for(int h = 1; h < numHeaderWords; ++h) {
			if(debug) std::cout<<pos+h<<": header word "<<h<<" - 0x"<<std::hex<<buffer[pos+h]<<std::dec<<std::endl;
			dataType.push_back(buffer[pos+h]&0xff);
			dataFormat.push_back((buffer[pos+h]>>8)&0xffffff);
		}
		// advance pos past the header
		pos += numHeaderWords;
		// loop over all data types and formats and read them
		for(size_t i = 0; i < dataType.size(); ++i) {
			switch(dataFormat[i]) {
				case 0:
				case 1:
					// read single byte
					data = (buffer[pos]>>(8*byte))&0xff;
					if(debug) std::cout<<pos<<": single byte "<<byte<<" from 0x"<<std::hex<<buffer[pos]<<" = 0x"<<data<<std::dec<<std::endl;
					byte += 1;
					break;
				case 2:
				case 3:
					// read two bytes
					if(byte < 3) {
						// can read both bytes from this word
						data = (buffer[pos]>>(8*byte))&0xffff;
						if(debug) std::cout<<pos<<": two bytes "<<byte<<" from 0x"<<std::hex<<buffer[pos]<<" = 0x"<<data<<std::dec<<std::endl;
						byte += 2;
					} else {
						// read one byte from this word
						data = (buffer[pos++]>>(8*byte))&0xff;
						// go to next word and read one more byte
						data |= (buffer[pos]<<8)&0xff00;
						if(debug) std::cout<<pos<<": two bytes "<<byte<<" from 0x"<<std::hex<<buffer[pos-1]<<" and 0x"<<buffer[pos]<<" = 0x"<<data<<std::dec<<std::endl;
						byte = 1;
					}
					break;
				case 4:
				case 5:
					// read four bytes
					// use the 4-n bytes from this word and n bytes from the next word (also works for n = 0!)
					for(int n = 4-byte, mask = 0x00; n > 0; --n) {
						mask = mask<<8;
						mask |= 0xff;
					}
					data = (buffer[pos++]>>(8*byte))&mask;
					if(debug) std::cout<<pos-1<<": "<<4-byte<<" bytes from 0x"<<std::hex<<buffer[pos-1]<<" using mask 0x"<<mask<<" = 0x"<<data<<std::dec<<std::endl;
					for(int n = 4-byte, mask = 0x00; n > 0; --n) {
						mask = mask<<8;
						mask |= 0xff;
					}
					data |= (buffer[pos]&mask)<<(8*byte);
					if(debug) std::cout<<pos<<": plus "<<byte<<" bytes from 0x"<<std::hex<<buffer[pos]<<" using mask 0x"<<mask<<" = 0x"<<data<<std::dec<<std::endl;
					break;
				case 6:
				case 7:
					// read eight bytes
					// use the 4-n bytes from this word, the whole next word, and n bytes from the next-to-next word (also works for n = 0!)
					for(int n = 4-byte, mask = 0x00; n > 0; --n) {
						mask = mask<<8;
						mask |= 0xff;
					}
					data = (buffer[pos++]>>(8*byte))&mask;
					if(debug) std::cout<<pos-1<<": "<<4-byte<<" bytes from 0x"<<std::hex<<buffer[pos-1]<<" using mask 0x"<<mask<<" = 0x"<<data<<std::dec<<std::endl;
					data |= static_cast<uint64_t>(buffer[pos++])<<(8*byte);
					if(debug) std::cout<<pos-1<<": plus 0x"<<std::hex<<buffer[pos-1]<<" = 0x"<<data<<std::dec<<std::endl;
					for(int n = 4-byte, mask = 0x00; n > 0; --n) {
						mask = mask<<8;
						mask |= 0xff;
					}
					data |= static_cast<uint64_t>(buffer[pos]&mask)<<(8*byte+32);
					if(debug) std::cout<<pos<<": plus "<<byte<<" bytes from 0x"<<std::hex<<buffer[pos]<<" using mask 0x"<<mask<<" = 0x"<<data<<std::dec<<std::endl;
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
			if(byte > 3) {
				byte = byte%4;
				++pos;
			}
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
	} // end of event loop

	std::cout<<"100% done, "<<pos<<"/"<<buffer.size()<<" words read = "<<entry<<" entries"<<std::endl;

	tree->Write();
	output.Close();

	return 0;
}
