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
	if(argc != 3) {
		std::cout<<"usage: "<<argv[0]<<" <input file> <output file>"<<std::endl;
		return 1;
	}

	std::string inputFileName = argv[1];

	// open output root file
	TFile output(argv[2], "recreate");
	if(!output.IsOpen()) {
		std::cerr<<"Failed to open output root file "<<argv[2]<<std::endl;
	}

	// create tree
	TTree* tree = new TTree("dt5780","data from dt5780");

	// create and add leafs
	int aggregateSize;
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
	uint64_t timestamp;
	tree->Branch("timestamp", &timestamp);
	std::vector<uint16_t> waveform;
	tree->Branch("waveform", &waveform);
	uint8_t extras;
	tree->Branch("extras", &extras);
	bool pileUp;
	tree->Branch("pileUp", &pileUp);
	uint16_t energy;
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
	for(pos = 0; pos < buffer.size();) {
		// header - 31 = format word following, [20:0] = aggregate size
		aggregateSize = buffer[pos] & 0x1fffff;
		if(((buffer[pos++]>>31)&0x1) == 0x1) {
			// format word - 31 = dual trace, 30 = waveform, 29 = energy, 28 = time, [23:22] = analog 1, [21:20] = analog 2, [19:16] = digital, [15:0] = numSamples/2
			dualTraceEnabled = (((buffer[pos]>>31)&0x1) == 0x1);
			waveformEnabled =  (((buffer[pos]>>30)&0x1) == 0x1);
			energyEnabled =    (((buffer[pos]>>29)&0x1) == 0x1);
			timeEnabled =      (((buffer[pos]>>28)&0x1) == 0x1);
			analogProbe1 =     (buffer[pos]>>22)&0x3;
			analogProbe2 =     (buffer[pos]>>20)&0x3;
			digitalProbe =     (buffer[pos]>>16)&0x3;
			numSamples =       buffer[pos]&0xffff;
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
		while(pos < buffer.size()) {
			// time word - 31 = timestamp roll over, [29:0] = timestamp
			if(((buffer[pos]>>31)&0x1) == 0x1) { ++timestampRollover; }
			timestamp = static_cast<uint64_t>(buffer[pos++]&0x3fffffff) + (static_cast<uint64_t>(timestampRollover)<<30);
			// waveform words - 31 = trigger sample, 30 = digital probe, [29:16] = analog probe, 15 = trigger sample, 14 = digital probe, [13:0] = analog probe
			waveform.clear();
			for(int w = 0; w < numSamples; ++w) {
				waveform.push_back(buffer[pos]&0xffff);
				waveform.push_back((buffer[pos++]>>16)&0xffff);
			}
			// energy word - [23:16] = extras, 15 - pile up, [14:0] = energy
			// extras - 7 = double satutration, 6 = total triggers, 5 = lost triggers, 4 = input saturation, 3 = fake event, 2 = timestamp reset, 1 = timestamp rollover, 0 = dead time
			extras = (buffer[pos]>>16)&0xff;
			pileUp = (((buffer[pos]>>15)&0x1) == 0x1);
			energy = buffer[pos++]&0x7fff;
			tree->Fill();
		}
		std::cout<<(100*pos)/buffer.size()<<"% done, "<<pos<<"/"<<buffer.size()<<" words read"<<std::endl;
	}

	std::cout<<"100% done, "<<pos<<"/"<<buffer.size()<<" words read"<<std::endl;

	tree->Write();
	output.Close();

	return 0;
}
