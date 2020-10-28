#pragma once

#include <fstream>
#include <sstream>
#include <memory>
#include <cstdint>


struct Waveform {
	Waveform(unsigned numSamples = 0, unsigned rate = 9600, unsigned chan = 1,unsigned bitSample=16)
		: sampleRate(rate), sampleCount(numSamples),chanNo(chan),bitsPerSample(bitSample) {}
	Waveform(const Waveform&) = delete;
	Waveform(Waveform&& other)
		: sampleRate(other.sampleRate),
		sampleCount(other.sampleCount),
		floats_(std::move(other.floats_)),
		shorts_(std::move(other.shorts_)) {
		other.sampleCount = 0;
	}

	Waveform& operator=(Waveform&& other) {
		sampleCount = other.sampleCount;
		sampleRate = other.sampleRate;
		chanNo = other.chanNo;
		bitsPerSample = other.bitsPerSample;
		floats_ = std::move(other.floats_);
		shorts_ = std::move(other.shorts_);
		other.sampleCount = 0;
		return *this;
	}

	unsigned sampleRate;
	unsigned sampleCount;
	unsigned chanNo;
	unsigned bitsPerSample;
	float* floatData() {
		if (!sampleCount || floats_) {
			return floats_.get();
		}
		floats_.reset(new float[sampleCount]);
		if (shorts_) {
			for (unsigned cnt = 0; cnt < sampleCount; ++cnt)
				floats_[cnt] = shorts_[cnt] / 32767.0f;
		}
		return floats_.get();
	}

	int16_t* shortData() {
		if (!sampleCount || shorts_) {
			return shorts_.get();
		}
		shorts_.reset(new int16_t[sampleCount]);
		if (floats_) {
			for (unsigned cnt = 0; cnt < sampleCount; ++cnt)
				shorts_[cnt] = int16_t(floats_[cnt] * 32767);
		}
		return shorts_.get();
	}

private:
	std::unique_ptr<float[]> floats_;
	std::unique_ptr<int16_t[]> shorts_;
};

#pragma pack(push, 1)
struct wavChunkHeader {
	char chunkID[4];
	uint32_t chunkSize;
};

struct chunkList {
	char chunkID[4];
	char padding[30];
	char chunkDataID[4];
	uint32_t chunkSize;
};
struct wavFormatChunk {
	wavChunkHeader chunkHeader;
	uint16_t format;
	uint16_t chanNo;
	uint32_t samplesPerSec;
	uint32_t bytesPerSec;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};

struct wavHeader {
	wavChunkHeader masterChunk;
	char WAVEmagic[4];
	wavFormatChunk format;
	char padding[64];
};
#pragma pack(pop)

bool isChunkIDEqual(const char* ID1, const char* ID2) {
	return *reinterpret_cast<const uint32_t*>(ID1) == *reinterpret_cast<const uint32_t*>(ID2);
}

Waveform loadWAV(const std::string& filename) {
	std::ifstream ifs(filename, std::ifstream::binary);

	if (!ifs.is_open()) {
		throw std::runtime_error("Can't open " + filename);
	}
	wavHeader hdr;
	//printf("size:%d\n", sizeof(hdr));
	if (!ifs.read(reinterpret_cast<char*>(&hdr), sizeof(hdr)))
		throw std::runtime_error("Can not read WAV header");
	// Check master chunk IDs
	if (!isChunkIDEqual(hdr.masterChunk.chunkID, "RIFF") || !isChunkIDEqual(hdr.WAVEmagic, "WAVE"))
		throw std::runtime_error("Corrupted RIFF header");
	// Validate format chunk
	if (!isChunkIDEqual(hdr.format.chunkHeader.chunkID, "fmt ") || hdr.format.chunkHeader.chunkSize < 16)
		throw std::runtime_error("Corrupted runtime header");
	if (hdr.format.format != 1)
		throw std::runtime_error("Only PCM format is supported");
	//std::cout << hdr.format.bitsPerSample << std::endl;
	//printf("%d,%d", hdr.format.bitsPerSample, hdr.format.chanNo);
	if (hdr.format.bitsPerSample != 16 )
		//printf("Only 16 bit mono PCMs are supported");
		throw std::runtime_error("Only 16 bit mono/steoro PCMs are supported");

	// Seek data chunk at the end of previous chunk header + chunkSize
	uint32_t dataSize = 0;
	auto* dataHeader = reinterpret_cast<wavChunkHeader*>(reinterpret_cast<char*>(&hdr.format.format) + (hdr.format.chunkHeader.chunkSize));
	//auto* dataHeaders = reinterpret_cast<wavChunkHeader*>(reinterpret_cast<char*>(&hdr.format.format) + (hdr.format.chunkHeader.chunkSize));
	// Validate data chunk id
	//printf("%d", dataHeader->chunkID);
	if (isChunkIDEqual(dataHeader->chunkID, "data"))
	{
		dataSize = dataHeader->chunkSize;
		ifs.seekg(std::distance(reinterpret_cast<char*>(&hdr), reinterpret_cast<char*>(dataHeader + 1)), ifs.beg);
		//printf("seek : %d\n", std::distance(reinterpret_cast<char*>(&hdr), reinterpret_cast<char*>(dataHeader + 1)));

	}
	else {
		auto* listDataHeader = reinterpret_cast<chunkList*>(reinterpret_cast<char*>(&hdr.format.format) + (hdr.format.chunkHeader.chunkSize));
		dataSize = listDataHeader->chunkSize;
		ifs.seekg(std::distance(reinterpret_cast<char*>(&hdr), reinterpret_cast<char*>(listDataHeader + 1)), ifs.beg);
		//printf("seek : %d\n", std::distance(reinterpret_cast<char*>(&hdr), reinterpret_cast<char*>(listDataHeader + 1)));
	}


	Waveform rc(dataSize / sizeof(short), hdr.format.samplesPerSec,hdr.format.chanNo,hdr.format.bitsPerSample);
	//printf("dataSize : %d\n", (dataSize / sizeof(short)) / hdr.format.samplesPerSec);
	auto* shorts = rc.shortData();
	if (!ifs.read(reinterpret_cast<char*>(shorts), dataSize))
		throw std::runtime_error("Premature end of file");
	ifs.close();
	return rc;
}

