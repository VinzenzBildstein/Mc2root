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
	uint32_t boardAggregateSize;
	tree->Branch("boardAggregateSize", &boardAggregateSize);
	uint8_t boardId;
	tree->Branch("boardId", &boardId);
	uint8_t channelMask;
	tree->Branch("channelMask", &channelMask);
	uint32_t boardAggregateCounter;
	tree->Branch("boardAggregateCounter", &boardAggregateCounter);
	uint32_t boardTimestamp;
	tree->Branch("boardTimestamp", &boardTimestamp);
	uint32_t aggregateSize;
	tree->Branch("aggregateSize", &aggregateSize);
	bool dualTraceEnabled;
	tree->Branch("dualTraceEnabled", &dualTraceEnabled);
	bool waveformEnabled;
	tree->Branch("waveformEnabled", &waveformEnabled);
	bool energyEnabled;
	tree->Branch("energyEnabled", &energyEnabled);
	bool timeEnabled;
	tree->Branch("timeEnabled", &timeEnabled);
	uint8_t analogProbe1;
	tree->Branch("analogProbe1", &analogProbe1);
	uint8_t analogProbe2;
	tree->Branch("analogProbe2", &analogProbe2);
	uint8_t digitalProbe;
	tree->Branch("digitalProbe", &digitalProbe);
	int numSamples;
	int timestampRollover = 0;
	tree->Branch("timestampRollover", &timestampRollover);
	ULong64_t timestamp;
	tree->Branch("timestamp", &timestamp);
	std::vector<uint16_t> waveform;
	tree->Branch("waveform", &waveform);
	uint8_t extras;
	tree->Branch("extras", &extras);
	bool pileUp;
	tree->Branch("pileUp", &pileUp);
	uint16_t energy;
	tree->Branch("energy", &energy);
	int channel;
	tree->Branch("channel", &channel);

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
	for(pos = 0; pos < buffer.size();) {
		// board header
		if(((buffer[pos]>>28)&0xf) != 0xa) {
			std::cout<<pos<<": Error, expecting board header starting with 0xa, not 0x"<<std::dec<<buffer[pos]<<std::dec<<std::endl;
			++pos;
			continue;
		}
		if(debug) std::cout<<pos<<": board header - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
		boardAggregateSize = buffer[pos++]&0xfffffff;
		if(debug) std::cout<<"=> "<<boardAggregateSize<<" words in aggregate"<<std::endl;
		// second board header word
		if(debug) std::cout<<pos<<": 2nd board header - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
		boardId = (buffer[pos]>>27)&0x1f;
		if(((buffer[pos]>>26)&0x1) == 0x1) {
			std::cout<<pos<<": board fail 0x"<<std::dec<<buffer[pos]<<std::dec<<std::endl;
		}
		channelMask = buffer[pos++]&0xff;
		// third board header word - board aggregate counter
		if(debug) std::cout<<pos<<": board counter - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
		boardAggregateCounter = buffer[pos++]&0x7fffff;
		// fourth board header word - board timestamp
		if(debug) std::cout<<pos<<": board timestamp - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
		boardTimestamp = buffer[pos++];
		// read each channel that is in the channel mask
		for(channel = 0; channel < 8; ++channel) {
			if(((channelMask>>channel)&0x1) == 0x0) continue;
			// channel header - 31 = format word following, [20:0] = aggregate size
			if(debug) std::cout<<pos<<": header - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
			aggregateSize = buffer[pos] & 0x1fffff;
			if(debug) std::cout<<"=> "<<aggregateSize<<" words in channel"<<std::endl;
			if(((buffer[pos++]>>31)&0x1) == 0x1) {
				// format word - 31 = dual trace, 30 = waveform, 29 = energy, 28 = time, [23:22] = analog 1, [21:20] = analog 2, [19:16] = digital, [15:0] = numSamples/2
				if(debug) std::cout<<pos<<": format - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
				dualTraceEnabled = (((buffer[pos]>>31)&0x1) == 0x1);
				waveformEnabled =  (((buffer[pos]>>30)&0x1) == 0x1);
				energyEnabled =    (((buffer[pos]>>29)&0x1) == 0x1);
				timeEnabled =      (((buffer[pos]>>28)&0x1) == 0x1);
				analogProbe1 =     (buffer[pos]>>22)&0x3;
				analogProbe2 =     (buffer[pos]>>20)&0x3;
				digitalProbe =     (buffer[pos]>>16)&0x3;
				numSamples =       buffer[pos]&0xffff;
				++pos;
			} else {
				dualTraceEnabled = false;
				waveformEnabled =  false;
				energyEnabled =    false;
				timeEnabled =      false;
				analogProbe1 =     0;
				analogProbe2 =     0;
				digitalProbe =     0;
				numSamples =       0;
			}
			while(pos < aggregateSize) {
				// time word - 31 = timestamp roll over, [29:0] = timestamp
				if(debug) std::cout<<pos<<": timestamp - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
				if(((buffer[pos]>>31)&0x1) == 0x1) { ++timestampRollover; }
				timestamp = static_cast<uint64_t>(buffer[pos++]&0x3fffffff) + (static_cast<uint64_t>(timestampRollover)<<30);
				// waveform words - 31 = trigger sample, 30 = digital probe, [29:16] = analog probe, 15 = trigger sample, 14 = digital probe, [13:0] = analog probe
				waveform.clear();
				for(int w = 0; w < numSamples; ++w) {
					if(debug) std::cout<<pos<<": waveform - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
					waveform.push_back(buffer[pos]&0xffff);
					waveform.push_back((buffer[pos++]>>16)&0xffff);
				}
				// energy word - [23:16] = extras, 15 - pile up, [14:0] = energy
				// extras - 7 = double satutration, 6 = total triggers, 5 = lost triggers, 4 = input saturation, 3 = fake event, 2 = timestamp reset, 1 = timestamp rollover, 0 = dead time
				if(debug) std::cout<<pos<<": energy - 0x"<<std::hex<<buffer[pos]<<std::dec<<std::endl;
				extras = (buffer[pos]>>16)&0xff;
				pileUp = (((buffer[pos]>>15)&0x1) == 0x1);
				energy = buffer[pos++]&0x7fff;
				tree->Fill();
				++entry;
				if(entry%1000 == 0) std::cout<<(100*pos)/buffer.size()<<"% done, "<<pos<<"/"<<buffer.size()<<" words read\r"<<std::flush;
			}
		} // end of channel loop
		if(debug && pos > 1000) break;
	} // end of data loop

	std::cout<<"100% done, "<<pos<<"/"<<buffer.size()<<" words read = "<<entry<<" entries"<<std::endl;

	tree->Write();
	output.Close();

	return 0;
}
